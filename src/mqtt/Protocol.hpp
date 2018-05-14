//
//  Protocol.hpp
//  MQTTd
//
//  Created by Khan on 16/3/22.
//  Copyright © 2016年 Khan. All rights reserved.
//


#ifndef __PROTOCOL_HPP__
#define __PROTOCOL_HPP__

#include "VarTypes.h"

#include <string>

#include <boost/asio.hpp>

#define ACTIVE_TIME    60
#define SOCKET_SEND_BUFFER_SIZE 1024 * 8
#define SOCKET_RECV_BUFFER_SIZE 1024 * 8



using std::string;
using std::vector;
using std::shared_ptr;
using std::tuple;
using std::make_shared;


class MqttMessage;

typedef std::shared_ptr<MqttMessage> PMessage;





/** 编码长度字段
 * @param data 传入需要编码的长度, 编码后, 出参为编码完成的char[n<4]缓冲. Big-endian, 须htonl转换
 * @return 编码后的有效字节宽度
 */
shared_ptr<vector<Uint8>> encLen(Uint len) ;


/** 解码长度字段 Big-endian
 * @param data 4字节缓冲区, 用int暂存, 实际为char[4];
 * @return 实际长度
 */
size_t decLen(const vector<Uint8> &data) ;




enum COMMAND {
    NONE       = 0x00,   //--Reserved
    CONNECT    = 0x01,   //客户端到服务端, 客户端请求连接服务端
    CONNACK,              //服务端到客户端, 连接报文确认
    PUBLISH,              //两个方向都允许, 发布消息
    PUBACK,               //两个方向都允许, QoS 1 消息发布收到确认
    PUBREC,               //两个方向都允许, 发布收到(保证交付第一步)
    PUBREL,               //两个方向都允许, 发布释放(保证交付第二步)
    PUBCOMP,              //两个方向都允许, QoS 2 消息发布完成(保证交互第三步)
    SUBSCRIBE,            //客户端到服务端, 客户端订阅请求
    SUBACK,               //服务端到客户端, 订阅请求报文确认
    UNSUBSCRIBE,          //客户端到服务端, 客户端取消订阅请求
    UNSUBACK,             //服务端到客户端, 取消订阅报文确认
    PINGREQ,              //客户端到服务端, 心跳请求
    PINGRESP,             //服务端到客户端, 心跳响应
    DISCONNECT,           //客户端到服务端, 客户端断开连接
    RESERVED              //保留
};

enum MQTTQOS {
    QOS0,                //最多分发一次
    QOS1,                //至少分发一次
    QOS2                 //仅分发一次
};

enum  CONNECTSTATUS {
    FINISH,                  //连接完成
    NOSUPPORT_VER,           //不支持此版本协议
    CLIENTID_ERROR,          //Client ID不符合规范
    SERVICE_FORBIDDEN,       //服务不可用
    USER_OR_PASSWD_ERROR,    //用户名密码不符合规范
    AUTH_FAULT               //鉴权失败
};

#pragma pack (1)         //指定按1字节对齐
typedef struct MQTTHeader{
    
    bool Retain   : 1;   //服务器保留副本标志, bit 0, 仅出现于PUBLISH报文, 其他时候都为0
    Uint8 QoS     : 2;   //服务质量等级, bit 1--2, 仅出现于PUBLISH报文, 其他报文都为0
    bool Dup      : 1;   //重发标志, bit 3, 仅出现于PUBLISH报文, 其他报文都为0
    
    Uint8 Command : 4;   //Command, bit 4--7
    
} Header;
#pragma pack ()          /*取消指定对齐，恢复缺省对齐*/


#pragma pack (1)
typedef struct SMQTTConnectFlag {
    bool   reserved      : 1;  // 保留
    bool   clean_session : 1;  // 清理回话标志, 0: 回话状态可重用; 重连后未接收消息重发, 包括未完成确认的ack, 1: 反之
    bool   will_flag     : 1;  // 遗嘱标志, 1: 服务器存储并发布链接异常状态, 直至DISCONNECT报文时删除了这个遗嘱消息
    Uint8  will_qos      : 2;  // 遗嘱消息的QoS服务质量
    bool   will_retain   : 1;  // 遗嘱被服务器保留
    bool   passwd_flag   : 1;  // 如果用户名(User Name)标志被设置为 0,有效载荷中不能包含用户名字段 [MQTT-3.1.2-18]。 passwd和flag值必须一致
    bool   name_flag     : 1;  // 如果密码(Password)标志被设置为 1,有效载荷中必须包含密码字段 [MQTT-3.1.2-21]。
    
} ConnectFlag;
#pragma pack ()


class MqttMessage {
    
protected:
    Uint   size;
    
public:
    Header header;
    
    virtual int encode(vector<Uint8>& buf) { return 0; }// = 0;
    virtual int decode(const vector<Uint8>& buf) { return 0; } //= 0;
    
    
    MqttMessage(){};
    virtual ~MqttMessage(){};
};

class MqttConnect : public MqttMessage {
    
public:
    //可变头
    // Uint16      protocol_len;      // 协议名长度
    string      protocol;          // 协议名, 'MQTT'
    Uint8       level;
    ConnectFlag flag;
    Uint16      keep_time ;        // 心跳间隔
    
    //payload
    // Uint16      cid_len
    string      cid;               // Client Identifier, server必须允许1-23字节长度, 必须允许[a-zA-Z0-9]; 可以允许超过23字节; 可以允许0字节长度, 如果允许了0字节长度, clean_session必须为1; 如果不允许, 必须返回0x02, 表示非法clientid
    // Uint16      topic_len
    string      will_topic;        // 遗嘱主题
    // Uint16      msg_len;
    string      will_msg;          // 遗嘱消息
    // Uint16      user_len;
    string      user;              // 用户名
    // Uint16      passwd_len;
    string      passwd;            // 密码
    
    MqttConnect(){}
    
    explicit MqttConnect(Uint8 ver, Uint16 keep_time, string cid, string will_topic, string will_msg, string name, string passwd):
        protocol{"MQTT"},
        level{ver},
        flag {false, false, false, 0x01, false, true, true} ,
//        header {false, 0, false, CONNECT}, // 子类的初始化列表不能初始化父类或者祖先类的成员
        keep_time{keep_time},
        cid{cid},
        will_topic{will_topic},
        will_msg{will_msg},
        user{name},
        passwd{passwd}
    {
        memset(& this->header, 0, sizeof(Header));
        this->header.Command = CONNECT;
        this->size = 0;
        
    }
    
    virtual ~MqttConnect(){};
    
    
    virtual int encode(vector<Uint8>& buf) ;
    virtual int decode(const vector<Uint8>& buf) ;
};

class MqttConnectAck : public MqttMessage{
private:
    
    
public:
    Uint8 sp;                      // 当前会话标志, 见协议3.2.2.2
    CONNECTSTATUS result;          // 返回码
    
    MqttConnectAck(){}
    
    explicit MqttConnectAck(Uint8 status, CONNECTSTATUS ret):sp{status}, result{ret}
    {
        this->header = {false, QOS0, false, CONNACK};
        this->size = 2;
        
    }
    
    virtual ~MqttConnectAck(){};
    
    
    virtual int encode(vector<Uint8>& buf) ;
    virtual int decode(const vector<Uint8>& buf) ;
};





class MqttPublish : public MqttMessage {
public:
    //可变头
    // Int16  topic_len;
    string topic;          //主题名, 不得有通配符
    Int16  seq;            //可选, 只有当QoS为[1|2]时才必须, 否则不能有报文标示符
    
    //payload
    string payload;        //发送内容
    
    MqttPublish(){}
    
    explicit MqttPublish(bool dup, bool retain, string topic, string payload): topic{topic}, payload{payload}{
        this->header = {dup, QOS0, retain, PUBLISH};
        this->seq = 0;
    }
    
    explicit MqttPublish(bool dup, Uint8 qos, bool retain, Int16 seq, string topic, string payload): topic{topic}, seq{seq}, payload{payload}{
        this->header = {dup, qos, retain, PUBLISH};
    }
    
    virtual ~MqttPublish(){};
    
    
    virtual int encode(vector<Uint8>& buf) ;
    virtual int decode(const vector<Uint8>& buf) ;
    
};


/** 发布应答
 * QoS0, 无ack; QoS1响应Puback;
 */
class MqttPuback : public MqttMessage {
    
public:
    Int16  seq ;
    
    MqttPuback(){}
    
    explicit MqttPuback(Int16 seq) : seq{seq}{
        this->header = { 0, 0, 0, PUBACK } ;
        this->size = 2;
    }
    
    virtual ~MqttPuback(){};
    
    virtual int encode(vector<Uint8>& buf) ;
    virtual int decode(const vector<Uint8>& buf) ;
};


/** 发布收到
 * QoS0, 无ack; QoS2响应Pubrec;
 */
class MqttPubrec: public MqttMessage {
    
public:
    Int16  seq ;
    
    MqttPubrec(){}
    
    explicit MqttPubrec(Int16 seq) : seq{seq} {
        this->header = {0, 0, 0, PUBREC};
        this->size = 2;
    }
    
    virtual ~MqttPubrec(){};
    
    virtual int encode(vector<Uint8>& buf) ;
    virtual int decode(const vector<Uint8>& buf) ;
};


/** 发布释放
 * QoS0, 无ack; QoS2响应的第二步;
 */
class MqttPubrel: public MqttMessage {
    
public:
    Int16  seq ;
    
    MqttPubrel(){}
    
    explicit MqttPubrel(Int16 seq) : seq{seq} {
        this->header = {false, QOS1, false, PUBREL};
        this->size = 2;
    }
    
    virtual ~MqttPubrel(){};
    
    virtual int encode(vector<Uint8>& buf) ;
    virtual int decode(const vector<Uint8>& buf) ;
};


/** 发布完成
 * QoS0, 无ack; QoS2响应的第三步;
 */
class MqttPubcomp: public MqttMessage {
    
public:
    Int16  seq ;
    
    MqttPubcomp(){}
    
    explicit MqttPubcomp(Int16 seq) : seq{seq}{
        this->header = {false, QOS0, false, PUBCOMP};
        this->size = 2;
    }
    
    virtual ~MqttPubcomp(){};
    
    virtual int encode(vector<Uint8>& buf) ;
    virtual int decode(const vector<Uint8>& buf) ;
};



/** 订阅主题
 *
 */
class MqttSubscribe: public MqttMessage {
    
protected:
    //payload
    vector<tuple<string, Uint8>> topics;  //订阅的topic列表以及对应的最大qos
    
    
public:
    Int16  seq ;
   
    MqttSubscribe(){}
    
    //允许多filter , 批量订阅
    void addTopic(string topic, MQTTQOS qos){
        this->topics.push_back(make_tuple(topic, qos));
    }
    
    const vector<tuple<string, Uint8>> getTopics(void) const {
        return this->topics;
    }
    
    
    explicit MqttSubscribe(Int16 seq): seq{seq} {
        this->header = {false, QOS1, false, SUBSCRIBE};
    }
    
    
    virtual ~MqttSubscribe(){};
    
    virtual int encode(vector<Uint8>& buf) ;
    virtual int decode(const vector<Uint8>& buf) ;
};

/** 订阅确认
 * 允许的最高QoS 只允许使用0, 1, 2, 80(failt)
 */
class MqttSuback: public MqttMessage {
    
public:
    Int16  seq ;
    
    //payload
    vector<Uint8> results;
    
    MqttSuback(){}
    
    //允许多个返回结果, 结果的顺序与订阅消息一致
    
    explicit MqttSuback(Int16 seq, vector<Uint8> rets) : seq(seq), results{rets} {
        this->header = {false, QOS0, false, SUBACK};
    }
    
    
    virtual ~MqttSuback(){};
    
    virtual int encode(vector<Uint8>& buf) ;
    virtual int decode(const vector<Uint8>& buf) ;
};






/** 取消订阅
 *
 */
class MqttUnsubscribe: public MqttMessage {
    
protected:
    //payload
    vector<string> topics;  //订阅的topic列表以及对应的最大qos
    
public:
    Int16  seq ;
    
    //允许多filter , 批量取消
    void delTopic(string topic){
        this->topics.push_back(topic);
    }
    
    MqttUnsubscribe(){}
    explicit MqttUnsubscribe(Int16 seq) : seq{seq} {
        this->header = {false, QOS1, false, UNSUBSCRIBE};
    }
    
    
    virtual ~MqttUnsubscribe(){};
    
    virtual int encode(vector<Uint8>& buf) ;
    virtual int decode(const vector<Uint8>& buf) ;
};



/** 取消订阅应答
 *
 */
class MqttUnsuback :public MqttMessage {
    
public:
    Int16  seq ;
    MqttUnsuback(){}
    explicit MqttUnsuback(Int16 seq) : seq{seq} {
        this->header = {false, QOS0, false, UNSUBACK};
    }
    
    virtual ~MqttUnsuback(){};
    
    virtual int encode(vector<Uint8>& buf) ;
    virtual int decode(const vector<Uint8>& buf) ;
};






/** ping
 *
 */
class MqttPing :public MqttMessage {
    
public:
    
    explicit MqttPing(){
        this->header = {false, QOS0, false, PINGREQ};
    }
    
    virtual ~MqttPing(){};
    
    virtual int encode(vector<Uint8>& buf) ;
    virtual int decode(const vector<Uint8>& buf) ;
};


/** pong
 *
 */
class MqttPong :public MqttMessage {
    
public:
    
    explicit MqttPong(){
        this->header = {false, QOS0, false, PINGRESP};
    }
    
    virtual ~MqttPong(){};
    
    virtual int encode(vector<Uint8>& buf) ;
    virtual int decode(const vector<Uint8>& buf) ;
};



/** disconnect
 *
 */
class MqttDisconnect :public MqttMessage {
    
public:
    
    explicit MqttDisconnect(){
        this->header = {false, QOS0, false, DISCONNECT};
    }

    virtual ~MqttDisconnect(){};
    
    virtual int encode(vector<Uint8>& buf) ;
    virtual int decode(const vector<Uint8>& buf) ;
};

//////////////////////////////////////






#endif //__PROTOCOL_HPP__




/*
 typedef struct MQTTBody{
	Uint Len;            //可变长字节,最短1字节, 最长4字节， 剩余长度 不包括用于编码剩余长度字段本身的字节数。
 
	Uint16 Identifier;   //报文标识符, PUBLISH(QoS>0 时), PUBACK,PUBREC,PUBREL,PUBCOMP,SUBSCRIBE, SUBACK,UNSUBSCIBE, UNSUBACK 需要, QoS 设置为 0 的 PUBLISH 报文不能包含报文标识符 [MQTT-2.3.1-5]。
	string Payload;
 } Body; 
 
 
 typedef struct SMQTTConnectPayload {
	Uint16 cid_len
	string cid;               // Client Identifier, server必须允许1-23字节长度, 必须允许[a-zA-Z0-9]; 可以允许超过23字节; 可以允许0字节长度, 如果允许了0字节长度, clean_session必须为1; 如果不允许, 必须返回0x02, 表示非法clientid
 
 Uint16 topic_len
	string will_topic;        // 遗嘱主题
 
	Uint16 msg_len;
	string will_msg;          // 遗嘱消息
 
	Uint16 user_len;
	string user;              // 用户名
 
	Uint16 passwd_len;
	string passwd;            // 密码
 
 } ConnectPayload;
 */


