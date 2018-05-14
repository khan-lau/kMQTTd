//
//  MqttClient.hpp
//  MQTTd
//
//  Created by Khan on 16/3/29.
//  Copyright © 2016年 Khan. All rights reserved.
//

#ifndef __MqttClient_HPP__
#define __MqttClient_HPP__

#include <map>
#include <vector>
#include <tuple>

#include <boost/asio.hpp>

#include "Client.hpp"
#include "VarTypes.h"
#include "MqttException.hpp"

using boost::asio::async_read;
using boost::asio::async_write;
using boost::system::error_code;
using boost::asio::buffer;
using boost::asio::ip::tcp;

class Package {
    
    public:
        explicit Package (const MqttMessage msg) :_status(0), _date_stamp(0){
            this->_pmsg = make_shared<MqttMessage>(msg);
        }
        
        explicit Package (const MqttMessage msg, Uint8 status) :_status(status), _date_stamp(0){
            this->_pmsg = make_shared<MqttMessage>(msg);
        }
        
        virtual ~Package(){}
        
        MqttMessage& getMessage() {
            return *( this->_pmsg );
        }
        
    public:
        Uint8 _status;
        Uint _date_stamp;
        
    private:
        PMessage _pmsg;
};


class MqttClient : public Client,  public std::enable_shared_from_this<MqttClient> {
    
    public:

        explicit MqttClient(PSocket psocket) : _logined{false} {
            this->_psocket = psocket;
        }

        virtual ~MqttClient() {
            // _psocket->shutdown(ASocket::shutdown_both, ec); //彻底关闭该socket上所有通信
            // _psocket->close(ec);                                    //fd引用计数-1
        }
        
        virtual void sendRunLoop(ASocket &socket) {
        
        }
        
        virtual void deliver(const MqttMessage& msg) {
            switch (msg.header.Command) {
                case CONNECT: {              //客户端到服务端, 客户端请求连接服务端
                    const MqttConnect& connect = static_cast<const MqttConnect&>(msg) ;

                    this->onAuth(connect );
                }
                    break;
                    
                case CONNACK: {              //服务端到客户端, 连接报文确认
                    // server do nothing
                    if (this->func_OnUnLogin != nullptr) { //收到不该收到的消息, 退出
                        this->func_OnUnLogin();
                    }
                }
                    break;
                    
                case PUBLISH: {              //两个方向都允许, 发布消息
                    const MqttPublish& publish = static_cast<const MqttPublish&>(msg) ;
                    this->onPublish( publish);
                }
                    break;
                    
                case PUBACK: {              //两个方向都允许, QoS 1 消息发布收到确认
                    const MqttPuback& puback = static_cast<const MqttPuback&>(msg) ;
                    this->onPubAck( puback);
                }
                    break;
                    
                case PUBREC: {               //两个方向都允许, 发布收到(保证交付第一步)
                    const MqttPubrec& pubrec = static_cast<const MqttPubrec&>(msg) ;
                    this->onPubRec( pubrec);
                }
                    break;
                    
                case PUBREL: {               //两个方向都允许, 发布释放(保证交付第二步)
                    const MqttPubrel& pubrel = static_cast<const MqttPubrel&>(msg) ;
                    this->onPubRel(  pubrel);
                }
                    break;
                    
                case PUBCOMP: {              //两个方向都允许, QoS 2 消息发布完成(保证交互第三步)
                    const MqttPubcomp& pubcomp = static_cast<const MqttPubcomp&>(msg) ;
                    this->onPubComp( pubcomp);
                }
                    break;
                    
                case SUBSCRIBE: {            //客户端到服务端, 客户端订阅请求
                    const MqttSubscribe& subscribe = static_cast<const MqttSubscribe&>(msg) ;
                    this->onSubscribe( subscribe);
                }
                    break;
                    
                case SUBACK: {              //服务端到客户端, 订阅请求报文确认
                    // server do nothing
                    if (this->func_OnUnLogin != nullptr) { //收到不该收到的消息, 退出
                        this->func_OnUnLogin();
                    }
                }
                    break;
                    
                case UNSUBSCRIBE: {          //客户端到服务端, 客户端取消订阅请求
                    const MqttUnsubscribe& unsub = static_cast<const MqttUnsubscribe&>(msg) ;
                    this->onUnSubscribe(unsub);
                }
                    break;
                    
                case UNSUBACK: {            //服务端到客户端, 取消订阅报文确认
                    // server do nothing
                    if (this->func_OnUnLogin != nullptr) { //收到不该收到的消息, 退出
                        this->func_OnUnLogin();
                    }
                }
                    break;
                    
                case PINGREQ: {              //客户端到服务端, 心跳请求
                    const MqttPing& ping = static_cast<const MqttPing&>(msg) ;
                    this->onPing( ping);
                }
                    break;
                    
                case PINGRESP: {             //服务端到客户端, 心跳响应
                    // server do nothing
                    if (this->func_OnUnLogin != nullptr) { //收到不该收到的消息, 退出
                        this->func_OnUnLogin();
                    }
                }
                    break;
                    
                case DISCONNECT: {           //客户端到服务端, 客户端断开连接
                    if (this->func_OnUnLogin != nullptr) { //收到`断开`消息, 退出
                        this->func_OnUnLogin();
                    }
                }
                    break;
                    
                default:
                    break;
            }
        }
    
    
    
    private:
        void onAuth(const MqttConnect& msg){
            std::shared_ptr<MqttConnect> p_msg = make_shared<MqttConnect>(msg);
            
        }
        
        void onPublish(const MqttPublish& msg){
            auto self( shared_from_this() );
            
            std::shared_ptr<MqttPublish> p_msg = make_shared<MqttPublish>(msg);
            if (p_msg->header.QoS == QOS1) {
                
                if( p_msg->header.Dup == true) { //对方重发的消息, 但是不表示前一条消息server以收到
                    //publish ack
                    MqttPuback puback( p_msg->seq );
                    
                    //TODO 去历史记录中查找有无相同的记录, 如没有. 加入. 如有, 改标记.
                    
                    this->write( puback, [p_msg](const error_code& ec){
                        //TODO 找到对应channel 放入
                    });
                
                } else { //对方首次发送的消息
                    //publish ack
                    MqttPuback puback( p_msg->seq );
                    this->write( puback, [p_msg](const error_code& ec){
                        
                    });
                }
                
            } else if(p_msg->header.QoS == QOS2) {
                if( p_msg->header.Dup == true) { //对方重发的消息
                    //publish rec
                    MqttPubrec pubrec( p_msg->seq  );
                    
                    //TODO 去历史记录中查找有无相同的记录, 如没有. 加入. 如有, 改标记.
                    
                    this->_msg_rqueue.insert( std::make_pair(p_msg->seq, Package(*p_msg, PUBLISH)) );
                    this->write( pubrec, [self, p_msg](const error_code& ec){
                        self->_msg_rqueue.at(p_msg->seq)._status = PUBREC;
                    });
                    
                } else {
                    //publish rec
                    MqttPubrec pubrec( msg.seq  );
                    
                    this->_msg_rqueue.insert( std::make_pair(p_msg->seq, Package(*p_msg, PUBLISH)) );
                    this->write( pubrec, [self, p_msg](const error_code& ec){
                        self->_msg_rqueue.at(p_msg->seq)._status = PUBREC;
                    });
                }
                
            } else {
                // do nothing
            }


        }
        
        void onPubAck(const MqttPuback& msg){
            // 从滑动窗口队列中删除publish记录
            this->_msg_squeue.erase(msg.seq);
        }
        
        void onPubRec(const MqttPubrec& msg){
            auto self( shared_from_this() );
            
            std::shared_ptr<MqttPubrec> p_msg = make_shared<MqttPubrec>(msg);
            MqttPubrel pubrel(p_msg->seq);
            this->write( pubrel, [self, p_msg](const error_code& ec){
                if(!ec){
                    self->_msg_rqueue.at(p_msg->seq)._status = PUBREL;
                } else {
                    
                    
                }
            });
            
        }
        
        void onPubRel( const MqttPubrel& msg){
            auto self( shared_from_this() );
            
            std::shared_ptr<MqttPubrel> p_msg = make_shared<MqttPubrel>(msg);
            MqttPubcomp pubcomp(p_msg->seq);
            this->write( pubcomp, [self, p_msg](const error_code& ec){
                if(!ec){
                    self->_msg_rqueue.at(p_msg->seq)._status = PUBCOMP;
                    self->_msg_rqueue.erase(p_msg->seq);
                } else {
                    
                }
            });
        }
        
        void onPubComp( const MqttPubcomp& msg){
            this->_msg_rqueue.erase(msg.seq);
        }

        
        /**
         * @brief  暂不支持通配符订阅
         * 
         * @param msg 
         */
        void onSubscribe( const MqttSubscribe& msg){
            //TODO channel 资源权限校验.
            const std::vector<std::tuple<string, Uint8>> topics = msg.getTopics();
            for ( auto item : topics) {
                string topic; Uint16 qos;
                std::tie(topic, qos) = item;
            }

            //TODO 加入channel的订阅用户
        }
        
        void onUnSubscribe( const MqttUnsubscribe& msg){
            
        }
        
        void onPing( const MqttPing& msg){
            auto self( shared_from_this() );
            MqttPong pong;
            this->write( pong, [self](const error_code& ec){
                if(!ec){
                    if (self->func_OnWrite != nullptr) {
                        self->func_OnWrite();
                    }
                } else {
                    if (ec.value() != 4) {
                        if (self->func_OnUnLogin != nullptr) { //发送心跳包出错, 退出
                            self->func_OnUnLogin();
                        }
                    }
                }
            });
        }
        
        
        int write( MqttMessage &msg, std::function<void(const error_code& ec)> &&func){
            std::shared_ptr<vector<Uint8>> pbuf = make_shared<vector<Uint8>>();
            Int ret = msg.encode(*pbuf);
            if (ret < 0) {
                return ret;
            }
            
            this->do_write( pbuf, 0, [func](const error_code &ec){
                func(ec);
            });
            return 0;
        }
        
        /** 超长内容 分多次递归发送
         * buf 发送缓冲区
         * writed_len 已发送字节数
         * func 发送完成后回调
         */
        void do_write( const std::shared_ptr<vector<Uint8>> pbuf, size_t writed_len, std::function<void(const error_code& ec)> &&func) {
            
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
            if ( !this->_psocket.expired() ) { //如果指针指向的shared_ptr还有效

                std::shared_ptr<ASocket> sp = this->_psocket.lock();
                if (sp != nullptr) {
                    sp->async_send(
                                buffer(pbuf->data()+writed_len, buf_size),
                                [self, pbuf, writed_len, func = std::move(func)](const error_code& ec, size_t bytes_transferred) mutable
                                {
                                    if(!ec){
                                        if (writed_len + bytes_transferred < pbuf->size()) {
                                            self->do_write( pbuf, writed_len + bytes_transferred, std::move(func) );
                                        }
                                    } else {
                                        func(ec);
                                    }
                                });
                } else {
                    error_code error(4, mqtt_category());
                    func(error);
                }
            } else {
                error_code error(4, mqtt_category());
                func(error);
            }
        }
        
    public:
        
        std::string _client_id;  //客户端id
        bool _logined;          //登录标记
        std::set<std::string> _channel_ids; //客户端订阅的频道
        
        std::map<Int16, Package> _msg_squeue; //发送窗口
        std::map<Int16, Package> _msg_rqueue; //接收窗口
        
        
        std::function< void() > func_OnLogin;
        std::function< void() > func_OnUnLogin;
        std::function< void() > func_OnWrite;
};



#endif /* __MqttClient_HPP__ */
