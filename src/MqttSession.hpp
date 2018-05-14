//
//  Session.hpp
//  MQTTd
//
//  Created by Khan on 16/3/25.
//  Copyright © 2016年 Khan. All rights reserved.
//

#ifndef __SESSION_HPP__
#define __SESSION_HPP__

#include <string>
#include <memory>
#include <set>
#include <queue>
#include <thread>

#include <boost/asio.hpp>

#include "Protocol.hpp"
#include "MqttException.hpp"
#include "MqttClient.hpp"

using std::unique_ptr;
using std::make_unique;
using std::chrono::duration;
using std::chrono::time_point;
using std::chrono::system_clock;
using std::chrono::duration_cast;
using boost::asio::async_read;
using boost::asio::async_write;


typedef struct MqttContext{
    Uint8 head[5]; //读header 和 不超过4字节的长度 size
    
    unique_ptr< vector<Uint8> > pdata;
    std::function< void (const error_code& ec, unique_ptr<MqttMessage> pmsg)> func;
    
} *pMqttContext;


class MqttSession : public std::enable_shared_from_this<MqttSession> {
    
    public:
        MqttSession( PSocket p_socket )
            : _client{ std::make_shared<MqttClient>(p_socket) }, _timer{ p_socket->get_io_context() },
            _strand{p_socket->get_io_context()},
            _atime{system_clock::now()}
        {
        }
    
        virtual ~MqttSession(){
            this->_thread.join();
        }
    
        PMqttClient& client()
        {
            return this->_client;
        }
    
        void start() {  //每生成一个新的chat_session都会调用
            auto self( this->shared_from_this() );
            
            this->_client->func_OnLogin = [self](){ //登录成功时
                self->_atime = system_clock::now();
                self->do_active(ACTIVE_TIME);
            };
            
            this->_client->func_OnWrite = [self](){ //写入消息时
                self->_atime = system_clock::now();
            };
            
            this->_client->func_OnUnLogin = [self](){ //退出登录 或 出错 强制断开时
                self->do_close();
            };
            
            this->_strand.dispatch( [self](){
                self->do_read_message(); //异步读客户端发来的消息
            } );


            _thread = std::thread([self](){ //启动时发现未发送消息, 则统一另启线程推送
                while(! self->_client->Socket()->get_io_context().stopped()){
                    self->_client->sendRunLoop();
                }
            });
        }

    
    private:

        /**
         * @brief 返回length字段的size
         * 
         * @param data 缓冲区
         * @param len 缓冲区大小
         * @return Int8 size, -1表示出错; 0表示未结尾; >0 表示位置, 位置从1开始
         */
        Int8 length_ending(Uint8 *data, size_t len){
            if (nullptr == data || len < 1) return -1;

            for (Uint8 i = 0; i < len; i++) {
                if ( *( data + i ) < 0x80 ) {
                    return i+1;
                }
            }
            return 0;
        }

        /**
         * @brief 处理异常出错
         * 
         */
        void do_error(){
            if (this->_client->func_OnUnLogin != nullptr) {
                this->_client->func_OnUnLogin();
            }
            this->do_close();
        }
    
        void do_read_message() {
            auto self(shared_from_this());
            std::shared_ptr<MqttContext> context = std::make_shared<MqttContext>();
            context->func = [self](const error_code& ec, std::shared_ptr<MqttMessage> pmsg) { //读取到消息后的回调函数
                if (!ec) {
                    self->_atime = system_clock::now();
                    self->_strand.dispatch([self, &pmsg](){
                        self->_client->deliver(*pmsg); ///?
                    });
                    
                    self->_strand.dispatch( [self](){
                        self->do_read_message(); //异步读客户端发来的消息
                    } );
                    
                } else {
                    //出错或未读到数据
                    self->do_error();
                }
            };
            
            this->_client->Socket()->async_read_some(buffer(context->head, sizeof(context->head)), [self, context](const error_code& ec, size_t bytes_transferred ){
                if (!ec && bytes_transferred > 0) {
                    MQTTHeader mh =  *(reinterpret_cast<MQTTHeader *>( &context->head[0] ));  //header
                    
                    if (bytes_transferred > 1 ) { //已读出全部或部分长度数据
                        Uint8 * pbuf =  (context->head + 1);
                        Int8 ending = self->length_ending(pbuf, bytes_transferred );
                        if( ending > 0) { //已读出全部长度数据
                            vector<Uint8> tmp;
                            std::copy(pbuf, context->head + ending, std::back_inserter(tmp));
                            size_t len = decLen(tmp);
                            
                            context->pdata = make_unique<vector<Uint8>>( bytes_transferred + len  );
                            std::copy(context->head, context->head + bytes_transferred, std::begin(*context->pdata) );
                            
                            self->do_read_remainder(context, bytes_transferred, mh, len );
                            
                        } else if (ending == 0) { //长度未到结尾
                            self->do_read_len(context->head + sizeof(MQTTHeader), sizeof(context->head)-1 , bytes_transferred-1, [self, context, mh](const error_code& ec, Uint len, Uint len_size, Uint buffer_size){
                                if (!ec) {
                                    context->pdata = make_unique<vector<Uint8>>(  sizeof(MQTTHeader) + len_size + len  );
                                    std::copy(context->head, context->head + buffer_size, std::begin(*context->pdata));
                                    
                                    self->do_read_remainder(context, 1 + buffer_size, mh, len);
                                } else {
                                    self->do_error();
                                }
                                
                            });
                        } else { //出错
                            self->do_error();
                        }
                        
                    } else { //未读到 length 字段
                        self->do_read_len(context->head + 1, sizeof(context->head)-1 , bytes_transferred-1, [self, context, mh](const error_code& ec, Uint len, Uint len_size, Uint buffer_size){
                            if (!ec) {
                                context->pdata = make_unique<vector<Uint8>>(  sizeof(MQTTHeader) + len_size + len  );
                                std::copy(context->head, context->head + buffer_size, std::begin(*context->pdata));
                                
                                self->do_read_remainder(context, 1 + buffer_size, mh, len);
                            } else {
                                self->do_error();
                            }
                        });
                    }
                    
                } else {
                    self->do_error();
                }
            });
        }
    
    
        /**
         * data 缓冲区指针
         * len 缓冲区大小
         * readed_len 已读字节数
         * fun 回调函数 void(错误代码, len字段, len字段长度, 已读的长度)
         */
        void do_read_len(Uint8 *data, size_t len, size_t readed_len, std::function<void(const error_code& ec, Uint len, Uint len_size, Uint buffer_size)> &&func){
            if(len == readed_len) {
                return;
            }
            
            auto self(shared_from_this());
            this->_client->Socket()->async_read_some( buffer( data + readed_len, (len-readed_len) ), [self, data, len, readed_len, func=std::move(func)](const error_code& ec, size_t bytes_transferred) mutable {
                if (!ec) {

                    Int8 ending = self->length_ending((data + readed_len),  bytes_transferred);
                    if( ending > 0) {  //是否读完长度字段
                        Uint8 len_size = readed_len + ending;  //长度所占字节数
                        vector<Uint8> tmp;
                        std::copy(data, data + len_size , std::back_inserter(tmp));
                        size_t len = decLen(tmp);
                        error_code error(boost::system::errc::success, boost::system::system_category());
                        func(error, (Uint)len, len_size, (Uint)(readed_len + bytes_transferred) );
                        
                    } else {
                        self->do_read_len( data, len , readed_len+bytes_transferred, std::move(func) );
                    }
                    
                } else {
                    func(ec, 0, 0, 0);
                }
            });
        }
    
    
    
        /** 超长内容 分多次递归接收
         * context
         * readded_n 已读字节数
         * header
         * size payload长度
         */
        void do_read_remainder(const std::shared_ptr<MqttContext> context, size_t readed_n,  const MQTTHeader header, size_t size) {
            
            auto self(shared_from_this());
            
            if ( size > 0 ) { //如果 body size > 0, 则有 可变头 或 payload
                
                size_t buf_size = 0;
                if (size <= SOCKET_RECV_BUFFER_SIZE ) { //小于系统缓冲区长度, 一次读完
                    buf_size = size;
                } else {
                    buf_size = SOCKET_RECV_BUFFER_SIZE + readed_n >= size ? SOCKET_SEND_BUFFER_SIZE : size - readed_n;
                }
                
                async_read(*(this->_client->Socket()),
                           buffer( context->pdata->data() + readed_n, size ), [context, self, readed_n, size, header](const error_code& ec, size_t bytes_transferred)
                {
                    if(!ec) {
                        if (readed_n + bytes_transferred < size) { //如果没读完, 递归继续读
                            self->do_read_remainder( context, readed_n + bytes_transferred, header, size );
                        } else {
                            //解码
                            unique_ptr<MqttMessage> pmsg = self->parseMqtt(header, *context->pdata);
                            if (pmsg != nullptr) {
                                error_code error;
                                context->func(error, std::move(pmsg) );
                            } else {
                                error_code error(3, mqtt_category());
                                context->func(error, std::move(pmsg) );
                            }
                        }
                    } else {
                        context->func(ec, nullptr);
                    }
                });
            } else { //类似 ping pong 这样没有payload与可变头的空包
                unique_ptr<MqttMessage> pmsg = this->parseMqtt(header, *context->pdata);
                if (pmsg != nullptr) {
                    error_code error;
                    context->func(error, std::move(pmsg) );
                } else {
                    error_code error(3, mqtt_category());
                    context->func(error, std::move(pmsg) );
                }
            }
        }
    
    
    
        int write(MqttMessage &msg, std::function<void(const error_code& ec)> &&func){
            std::shared_ptr<vector<Uint8>> pbuf = make_shared<vector<Uint8>>();
            Int ret = msg.encode(*pbuf);
            if (ret < 0) {
                return ret;
            }
            
            this->do_write(pbuf, 0, [func](const error_code &ec){
                func(ec);
            });
            return 0;
        }
    
        /** 超长内容 分多次递归发送
         * buf 发送缓冲区
         * writed_len 已发送字节数
         * func 发送完成后回调
         */
        void do_write(const std::shared_ptr<vector<Uint8>> pbuf, size_t writed_len, std::function<void(const error_code& ec)> &&func) {
            
            auto self(shared_from_this());
            if (writed_len >= pbuf->size()) {
                return;
            }
            
            size_t buf_size = 0;
            
            if (pbuf->size() < SOCKET_SEND_BUFFER_SIZE) {
                buf_size = pbuf->size() ;
            } else {
                buf_size = SOCKET_SEND_BUFFER_SIZE + writed_len >= pbuf->size() ? SOCKET_SEND_BUFFER_SIZE : pbuf->size() - writed_len;
            }
            async_write(*(this->_client->Socket()),
                        buffer(pbuf->data()+writed_len, buf_size),
                        [self, pbuf, writed_len, func = std::move(func)](const error_code& ec, size_t bytes_transferred) mutable
                        {
                            if(!ec) {
                                if (writed_len + bytes_transferred < pbuf->size()) {
                                    self->do_write( pbuf, writed_len + bytes_transferred, std::move(func) );
                                }
                            } else {
                                func(ec);
                            }
                        });
        }
    
    
    
        unique_ptr<MqttMessage> parseMqtt(MQTTHeader header, const vector<Uint8> &data){
            switch (header.Command) {
                case CONNECT: {  //客户端到服务端, 客户端请求连接服务端
                    unique_ptr<MqttConnect> pmsg = make_unique<MqttConnect>();
                    pmsg->decode(data);
                    return unique_ptr<MqttMessage> (std::move(pmsg)) ;
                }
                case CONNACK: {            //服务端到客户端, 连接报文确认
                    unique_ptr<MqttConnectAck> pmsg = make_unique<MqttConnectAck>();
                    pmsg->decode(data);
                    return (std::move(pmsg)) ;
                }
                case PUBLISH: {             //两个方向都允许, 发布消息
                    unique_ptr<MqttPublish> pmsg = make_unique<MqttPublish>();
                    pmsg->decode(data);
                    return (std::move(pmsg)) ;
                }
                case PUBACK: {             //两个方向都允许, QoS 1 消息发布收到确认
                    unique_ptr<MqttPuback> pmsg = make_unique<MqttPuback>();
                    pmsg->decode(data);
                    return (std::move(pmsg)) ;
                }
                case PUBREC:  {             //两个方向都允许, 发布收到(保证交付第一步)
                    unique_ptr<MqttPubrec> pmsg = make_unique<MqttPubrec>();
                    pmsg->decode(data);
                    return (std::move(pmsg)) ;
                }
                case PUBREL:  {             //两个方向都允许, 发布释放(保证交付第二步)
                    unique_ptr<MqttPubrel> pmsg = make_unique<MqttPubrel>();
                    pmsg->decode(data);
                    return (std::move(pmsg)) ;
                }
                case PUBCOMP: {             //两个方向都允许, QoS 2 消息发布完成(保证交互第三步)
                    unique_ptr<MqttPubcomp> pmsg = make_unique<MqttPubcomp>();
                    pmsg->decode(data);
                    return (std::move(pmsg)) ;
                }
                case SUBSCRIBE: {           //客户端到服务端, 客户端订阅请求
                    unique_ptr<MqttSubscribe> pmsg = make_unique<MqttSubscribe>();
                    pmsg->decode(data);
                    return (std::move(pmsg)) ;
                }
                case SUBACK: {             //服务端到客户端, 订阅请求报文确认
                    unique_ptr<MqttSuback> pmsg = make_unique<MqttSuback>();
                    pmsg->decode(data);
                    return (std::move(pmsg)) ;
                }
                case UNSUBSCRIBE: {         //客户端到服务端, 客户端取消订阅请求
                    unique_ptr<MqttUnsubscribe> pmsg = make_unique<MqttUnsubscribe>();
                    pmsg->decode(data);
                    return (std::move(pmsg)) ;
                }
                case UNSUBACK: {           //服务端到客户端, 取消订阅报文确认
                    unique_ptr<MqttUnsuback> pmsg = make_unique<MqttUnsuback>();
                    pmsg->decode(data);
                    return (std::move(pmsg)) ;
                }
                case PINGREQ: {             //客户端到服务端, 心跳请求
                    unique_ptr<MqttPing> pmsg = make_unique<MqttPing>();
                    pmsg->decode(data);
                    return (std::move(pmsg)) ;
                }
                case PINGRESP: {            //服务端到客户端, 心跳响应
                    unique_ptr<MqttPong> pmsg = make_unique<MqttPong>();
                    pmsg->decode(data);
                    return (std::move(pmsg)) ;
                }
                case DISCONNECT: {          //客户端到服务端, 客户端断开连接
                    unique_ptr<MqttDisconnect> pmsg = make_unique<MqttDisconnect>();
                    pmsg->decode(data);
                    return (std::move(pmsg)) ;
                }
                default:
                    return nullptr;
            }
        }
    
    
        /** later秒之后检查心跳
         */
        void do_active(int later){
            auto self(shared_from_this());
            _timer.expires_at( _timer.expires_at() + boost::posix_time::seconds( later ) );
            
            _timer.async_wait( [self](const error_code ec){
                if (!ec) { //
                    
                    typedef duration<int> seconds_type;
                    //    typedef std::chrono::duration<int, std::milli> milliseconds_type;
                    //    typedef std::chrono::duration<int, std::ratio<60*60>> hours_type;
                    //    hours_type h_oneday (24);                  // 24h
                    if ( duration_cast< seconds_type >(system_clock::now() - self->_atime).count() >  (ACTIVE_TIME)) {
                        //心跳异常, 超时处理
                        self->do_close();
                        
                    } else {
                        self->do_active( duration_cast< seconds_type >(system_clock::now() - self->_atime).count() );
                    }
                    
                } else { //对于任何时间未到的 timer，只要对该 timer 做了 cancel 或者 expires_xxx 操作，该 timer 原先登记的 handler 都会被调用，并且 err 为 true
                    
                }
                
                
            });
        }
    
        void do_close() {
            error_code ec;
            
            _timer.cancel(ec);
            this->_client->close();
        }
    
    
    private:
        PMqttClient _client;

        boost::asio::deadline_timer _timer;
        boost::asio::io_context::strand _strand;
        time_point<system_clock> _atime;    //最后活动时间

        std::thread _thread;
    };



#endif /* __SESSION_HPP__ */
