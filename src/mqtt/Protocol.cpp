
#include "Protocol.hpp"

#if defined(_WIN32) || defined(_WIN64)
#include <winsock2.h>
#else
#include <arpa/inet.h>
#endif



#include <regex>

static std::regex expression("^[a-zA-Z0-9@._]{0,23}$");


/////////////////MqttConnect/////////////////

int MqttConnect::encode(vector<Uint8>& buf) {
    
    std::cmatch what;
    
    //协议名称必须为MQTT
    if (protocol != "MQTT") {
        return -1;
    }
    
    //client id 必须为1--23位 utf8编码的[a-zA-Z0-9@._]字符串
    if( ! regex_match(this->cid.c_str(), what, expression) ){
        return -2;
    }
    
    //如果 client id长度为0, 则clean_session必须为1, 否则服务器必须ack返回码0x02
    if (this->cid.length() == 0 && this->flag.clean_session != 1) {
        return -3;
    }
    
    if (this->flag.name_flag != this->flag.passwd_flag) {
        return -4;
    }
    
    
    if ((this->flag.name_flag == 1 && this->user.length() == 0) || (this->flag.name_flag == 0 && this->user.length() > 0) ){
        return -5;
    }
    
    if ((this->flag.passwd_flag == 1 && this->passwd.length() == 0) || (this->flag.passwd_flag == 0 && this->passwd.length() > 0) ){
        return -6;
    }
    
    //可变头大小
    Uint vheader_len = (Uint)sizeof(Uint16) //协议长度
    + (Uint)this->protocol.length()         //协议名
    + (Uint)sizeof(this->level)             //协议版本
    + (Uint)sizeof(this->flag)              //连接标志
    + (Uint)sizeof(this->keep_time);        //心跳间隔
    
    //payload大小
    Uint payload_len = (Uint)sizeof(Uint16) //client id 长度
    + (Uint)this->cid.length()              //client id
    + (Uint)sizeof(Uint16)
    + (Uint)this->will_topic.length()
    + (Uint)sizeof(Uint16)
    + (Uint)this->will_msg.length()
    + (Uint)sizeof(Uint16)
    + (Uint)this->user.length()
    + (Uint)sizeof(Uint16)
    + (Uint)this->passwd.length();
    
    this->size = vheader_len + payload_len ;
    Uint it = htonl( this->size );  //确保是little-enddian的网络字节序
    
    buf.push_back( *(Uint8*)((void*)&this->header) );
    auto len_buf = encLen(it);
    if (len_buf == nullptr) return -7;
   
    std::copy(std::begin(*len_buf), std::end(*len_buf), std::back_inserter(buf));
    
    //可变头
    Uint16 t = htons(this->protocol.length());
    t = htons(t);
    std::copy( (Uint8*)&t, (Uint8*)&t + sizeof(Uint16), std::back_inserter(buf) );
    std::copy( std::begin(this->protocol), std::end(this->protocol), std::back_inserter(buf));
    
    buf.push_back(this->level);
    buf.push_back(*(Uint8*)((void*)&this->flag));
    
    t = htons(this->keep_time);
    std::copy((Uint8*)&t, (Uint8*)&t + sizeof(Uint16), std::back_inserter(buf));
    

    //payload
    t = htons(this->cid.length());
    std::copy( (Uint8*)&t, (Uint8*)&t + sizeof(Uint16), std::back_inserter(buf));
    std::copy(std::begin(this->cid), std::end(this->cid), std::back_inserter(buf));
    
    t = htons(this->will_topic.length());
    std::copy( (Uint8*)&t, (Uint8*)&t + sizeof(Uint16), std::back_inserter(buf));
    std::copy(std::begin(this->will_topic), std::end(this->will_topic), std::back_inserter(buf));

    t = htons(this->will_msg.length());
    std::copy( (Uint8*)&t, (Uint8*)&t + sizeof(Uint16), std::back_inserter(buf));
    std::copy(std::begin(this->will_msg), std::end(this->will_msg), std::back_inserter(buf));

    
    t = htons(this->user.length());
    std::copy( (Uint8*)&t, (Uint8*)&t + sizeof(Uint16), std::back_inserter(buf));
    std::copy(std::begin(this->user), std::end(this->user), std::back_inserter(buf));

    
    
    t = htons(this->passwd.length());
    std::copy( (Uint8*)&t, (Uint8*)&t + sizeof(Uint16), std::back_inserter(buf));
    std::copy(std::begin(this->passwd), std::end(this->passwd), std::back_inserter(buf));

//    memcpy(data, &(this -> header), sizeof(Header));
//    data[0] = *static_cast<Uint8 *>( static_cast<void *>(&this->header) );
//    data[0] = *reinterpret_cast<Uint8 *>( reinterpret_cast<void *>(&this->header) );
//    data[0] = *reinterpret_cast<Uint8 *>( static_cast<void *>(&this->header) );
    
    return (Int) buf.size();
}

int MqttConnect::decode(const vector<Uint8>& buf) {
    this->header = * ( (Header*) (&buf[0]) );
    Uint step = 1;
    
    //获取长度字段size
    size_t ll = 1;
    for ( int i = 1; i < 5; ++i ) {
        if ( buf[i] > 0x7F ) {
            ll++;
        } else break;
    }
    
    vector<Uint8> buf_len(ll);
    std::copy(std::begin(buf)+step , std::begin(buf)+ll, std::back_inserter(buf_len));
    size_t l = decLen(std::move(buf_len));
    this->size = ntohl(l);
    step += ll;
    
    //可变头
    Uint16 t = 0;
    std::copy( std::begin(buf)+step , std::begin(buf)+(Uint)sizeof(Uint16), (Uint8*)&t );
    step += (Uint)sizeof(Uint16);
    t = ntohs(t);
    
    std::copy( std::begin(buf)+step , std::begin(buf)+t, std::back_inserter(this->protocol) );
    step += t;
    
    this->level = buf[step++];
    this->flag = * ( (ConnectFlag*) (&buf[step++]) );
    
    std::copy( std::begin(buf)+step , std::begin(buf)+(Uint)sizeof(Uint16), (Uint8*)&t );
    step += (Uint)sizeof(Uint16);
    this->keep_time = ntohs(t);
    
    
    
    //payload
    std::copy( std::begin(buf)+step , std::begin(buf)+(Uint)sizeof(Uint16), (Uint8*)&t );
    step += (Uint)sizeof(Uint16);
    t = ntohs(t);
    
    std::copy( std::begin(buf)+step , std::begin(buf)+t, std::back_inserter(this->cid) );
    step += t;
    
    
    std::copy( std::begin(buf)+step , std::begin(buf)+(Uint)sizeof(Uint16), (Uint8*)&t );
    step += (Uint)sizeof(Uint16);
    t = ntohs(t);
    
    std::copy( std::begin(buf)+step , std::begin(buf)+t, std::back_inserter(this->will_topic) );
    step += t;
    
    
    std::copy( std::begin(buf)+step , std::begin(buf)+(Uint)sizeof(Uint16), (Uint8*)&t );
    step += (Uint)sizeof(Uint16);
    t = ntohs(t);
    
    std::copy( std::begin(buf)+step , std::begin(buf)+t, std::back_inserter(this->will_msg) );
    step += t;
    
    
    std::copy( std::begin(buf)+step , std::begin(buf)+(Uint)sizeof(Uint16), (Uint8*)&t );
    step += (Uint)sizeof(Uint16);
    t = ntohs(t);
    
    std::copy( std::begin(buf)+step , std::begin(buf)+t, std::back_inserter(this->user) );
    step += t;
    
    
    std::copy( std::begin(buf)+step , std::begin(buf)+(Uint)sizeof(Uint16), (Uint8*)&t );
    step += (Uint)sizeof(Uint16);
    t = ntohs(t);
    
    std::copy( std::begin(buf)+step , std::begin(buf)+t, std::back_inserter(this->passwd) );
    step += t;
    
    return (Int) buf.size();
}



///////////////////MqttConnectAck///////////////

int MqttConnectAck::encode(vector<Uint8> &buf){
    Uint it = htonl( this->size );  //确保是little-enddian的网络字节序
    
    buf.push_back( *(Uint8*)((void*)&this->header) );
    auto len_buf = encLen(it);
    if (len_buf == nullptr) return -7;
    
    std::copy(std::begin(*len_buf), std::end(*len_buf), std::back_inserter(buf));
    
    buf.push_back( this->sp );
    buf.push_back( (Uint8)this->result );
    
    return (Int) buf.size();
}

int MqttConnectAck::decode(const vector<Uint8> &buf){
    this->header = * ( (Header*) (&buf[0]) );
    Uint step = 1;
    
    //获取长度字段size
    size_t ll = 1;
    for ( int i = 1; i < 5; ++i ) {
        if ( buf[i] > 0x7F ) {
            ll++;
        } else break;
    }
    
    vector<Uint8> buf_len(ll);
    std::copy(std::begin(buf)+step , std::begin(buf)+ll, std::back_inserter(buf_len));
    size_t l = decLen(std::move(buf_len));
    this->size = ntohl(l);
    step += ll;
    
    this->sp = buf[step++];
    this->result = * ( (CONNECTSTATUS*) (&buf[step++]) );
    
    return (Int) buf.size();
}



/////////////////MqttPublish///////////////

int MqttPublish::encode(vector<Uint8> &buf){
    this->size = (Uint) ( sizeof(Uint16) + this->topic.length() + this->payload.length() );
    this->size += (this->header.QoS > 0) ? 2 : 0;
    
    Uint it = htonl( this->size );  //确保是little-enddian的网络字节序
    
    buf.push_back( *(Uint8*)((void*)&this->header) );
    auto len_buf = encLen(it);
    if (len_buf == nullptr) return -7;
    
    std::copy(std::begin(*len_buf), std::end(*len_buf), std::back_inserter(buf));
    
    //可变头
    Uint16 t = htons(this->topic.length());
    t = htons(t);
    std::copy( (Uint8*)&t, (Uint8*)&t + sizeof(Uint16), std::back_inserter(buf) );
    std::copy(std::begin(this->topic), std::end(this->topic), std::back_inserter(buf));
    
    if (this->header.QoS > 0 ) {//只有当 QoS 等级是 1 或 2 时,报文标识符(Packet Identifier)字段才能出现在 PUBLISH 报文中 见3.3.2.2
        t = htonl(this->seq);
        std::copy( (Uint8*)&t, (Uint8*)&t + sizeof(Uint16), std::back_inserter(buf) );
    }
    
    if (this->payload.length() > 0) {
        std::copy(std::begin(this->payload), std::end(this->payload), std::back_inserter(buf));
    }
    
    return (Int) buf.size();
}

int MqttPublish::decode(const vector<Uint8> &buf){
    this->header = * ( (Header*) (&buf[0]) );
    Uint step = 1;
    
    //获取长度字段size
    size_t ll = 1;
    for ( int i = 1; i < 5; ++i ) {
        if ( buf[i] > 0x7F ) {
            ll++;
        } else break;
    }
    
    vector<Uint8> buf_len(ll);
    std::copy(std::begin(buf)+step , std::begin(buf)+ll, std::back_inserter(buf_len));
    size_t l = decLen(std::move(buf_len));
    this->size = ntohl(l);
    step += ll;
    
    //可变头
    Uint16 t = 0;
    std::copy( std::begin(buf)+step , std::begin(buf)+(Uint)sizeof(Uint16), (Uint8*)&t ); //topic length
    step += (Uint)sizeof(Uint16);
    t = ntohs(t);
    
    std::copy( std::begin(buf)+step , std::begin(buf)+t, std::back_inserter(this->topic) );
    step += t;
    
    if (this->header.QoS > 0) {
        std::copy( std::begin(buf)+step , std::begin(buf)+(Uint)sizeof(Uint16), (Uint8*)&t );
        step += (Uint)sizeof(Uint16);
        t = ntohs(t);
        this->seq = t;
    }
    
    t = this->size - (step - sizeof(Header));
    std::copy( std::begin(buf)+step , std::begin(buf)+t, std::back_inserter(this->payload) );
    step += t;
    
    return (Int) buf.size();
}



/////////////////MqttPuback///////////////

int MqttPuback::encode(vector<Uint8> &buf){
    Uint it = htonl( this->size );  //确保是little-enddian的网络字节序
    
    buf.push_back( *(Uint8*)((void*)&this->header) );
    auto len_buf = encLen(it);
    if (len_buf == nullptr) return -7;
    
    std::copy(std::begin(*len_buf), std::end(*len_buf), std::back_inserter(buf));
    
    //可变头
    Uint16 t = htons(this->seq);
    std::copy( (Uint8*)&t, (Uint8*)&t + sizeof(Uint16), std::back_inserter(buf) );
    
    return (Int) buf.size();
}

int MqttPuback::decode(const vector<Uint8> &buf){
    this->header = * ( (Header*) (&buf[0]) );
    Uint step = 1;
    
    //获取长度字段size
    size_t ll = 1;
    for ( int i = 1; i < 5; ++i ) {
        if ( buf[i] > 0x7F ) {
            ll++;
        } else break;
    }
    
    vector<Uint8> buf_len(ll);
    std::copy(std::begin(buf)+step , std::begin(buf)+ll, std::back_inserter(buf_len));
    size_t l = decLen(std::move(buf_len));
    this->size = ntohl(l);
    step += ll;
    
    //可变头
    Uint16 t = 0;
    std::copy( std::begin(buf)+step , std::begin(buf)+(Uint)sizeof(Uint16), (Uint8*)&t ); // seq
    step += (Uint)sizeof(Uint16);
    this->seq = ntohs(t);
    
    return (Int) buf.size();
}



/////////////////MqttPubrec///////////////

int MqttPubrec::encode(vector<Uint8> &buf){
    Uint it = htonl( this->size );  //确保是little-enddian的网络字节序
    
    buf.push_back( *(Uint8*)((void*)&this->header) );
    auto len_buf = encLen(it);
    if (len_buf == nullptr) return -7;
    
    std::copy(std::begin(*len_buf), std::end(*len_buf), std::back_inserter(buf));
    
    //可变头
    Uint16 t = htons(this->seq);
    std::copy( (Uint8*)&t, (Uint8*)&t + sizeof(Uint16), std::back_inserter(buf) );
    
    return (Int) buf.size();
}

int MqttPubrec::decode(const vector<Uint8> &buf){
    this->header = * ( (Header*) (&buf[0]) );
    Uint step = 1;
    
    //获取长度字段size
    size_t ll = 1;
    for ( int i = 1; i < 5; ++i ) {
        if ( buf[i] > 0x7F ) {
            ll++;
        } else break;
    }
    
    vector<Uint8> buf_len(ll);
    std::copy(std::begin(buf)+step , std::begin(buf)+ll, std::back_inserter(buf_len));
    size_t l = decLen(std::move(buf_len));
    this->size = ntohl(l);
    step += ll;
    
    //可变头
    Uint16 t = 0;
    std::copy( std::begin(buf)+step , std::begin(buf)+(Uint)sizeof(Uint16), (Uint8*)&t ); //seq
    step += (Uint)sizeof(Uint16);
    this->seq = ntohs(t);
    
    return (Int) buf.size();
}



/////////////////MqttPubrel///////////////

int MqttPubrel::encode(vector<Uint8> &buf){
    Uint it = htonl( this->size );  //确保是little-enddian的网络字节序
    
    buf.push_back( *(Uint8*)((void*)&this->header) );
    auto len_buf = encLen(it);
    if (len_buf == nullptr) return -7;
    
    std::copy(std::begin(*len_buf), std::end(*len_buf), std::back_inserter(buf));
    
    //可变头
    Uint16 t = htons(this->seq);
    std::copy( (Uint8*)&t, (Uint8*)&t + sizeof(Uint16), std::back_inserter(buf) );
    
    return (Int) buf.size();
}

int MqttPubrel::decode(const vector<Uint8> &buf){
    this->header = * ( (Header*) (&buf[0]) );
    Uint step = 1;
    
    //获取长度字段size
    size_t ll = 1;
    for ( int i = 1; i < 5; ++i ) {
        if ( buf[i] > 0x7F ) {
            ll++;
        } else break;
    }
    
    vector<Uint8> buf_len(ll);
    std::copy(std::begin(buf)+step , std::begin(buf)+ll, std::back_inserter(buf_len));
    size_t l = decLen(std::move(buf_len));
    this->size = ntohl(l);
    step += ll;
    
    //可变头
    Uint16 t = 0;
    std::copy( std::begin(buf)+step , std::begin(buf)+(Uint)sizeof(Uint16), (Uint8*)&t ); // seq
    step += (Uint)sizeof(Uint16);
    this->seq = ntohs(t);
    
    return (Int) buf.size();
}



/////////////////MqttPubcomp///////////////

int MqttPubcomp::encode(vector<Uint8> &buf){
    Uint it = htonl( this->size );  //确保是little-enddian的网络字节序
    
    buf.push_back( *(Uint8*)((void*)&this->header) );
    auto len_buf = encLen(it);
    if (len_buf == nullptr) return -7;
    
    std::copy(std::begin(*len_buf), std::end(*len_buf), std::back_inserter(buf));
    
    //可变头
    Uint16 t = htons(this->seq);
    std::copy( (Uint8*)&t, (Uint8*)&t + sizeof(Uint16), std::back_inserter(buf) );
    
    return (Int) buf.size();
}

int MqttPubcomp::decode(const vector<Uint8> &buf){
    this->header = * ( (Header*) (&buf[0]) );
    Uint step = 1;
    
    //获取长度字段size
    size_t ll = 1;
    for ( int i = 1; i < 5; ++i ) {
        if ( buf[i] > 0x7F ) {
            ll++;
        } else break;
    }
    
    vector<Uint8> buf_len(ll);
    std::copy(std::begin(buf)+step , std::begin(buf)+ll, std::back_inserter(buf_len));
    size_t l = decLen(std::move(buf_len));
    this->size = ntohl(l);
    step += ll;
    
    //可变头
    Uint16 t = 0;
    std::copy( std::begin(buf)+step , std::begin(buf)+(Uint)sizeof(Uint16), (Uint8*)&t ); //seq
    step += (Uint)sizeof(Uint16);
    this->seq = ntohs(t);
    
    return (Int) buf.size();
}



/////////////////MqttSubscribe///////////////

int MqttSubscribe::encode(vector<Uint8> &buf){
    if( this->topics.size() < 1 ) return -1; //无topic
    
    //计算payload大小
    Uint payload_len = 0; //test 安全性待定
    
    for_each(topics.begin(), topics.end(), [&payload_len](tuple<string, Uint16> pack){
        string topic; Uint16 qos;
        std::tie(topic, qos) = pack;
        
        payload_len += (Uint)(topic.length() + sizeof(Uint16));
    });
    
    this->size = payload_len + 2;
    
    Uint it = htonl( this->size );  //确保是little-enddian的网络字节序
    
    buf.push_back( *(Uint8*)((void*)&this->header) );
    auto len_buf = encLen(it);
    if (len_buf == nullptr) return -7;
    
    std::copy(std::begin(*len_buf), std::end(*len_buf), std::back_inserter(buf));
    
    //可变头
    Uint16 t = htons(this->seq);
    t = htons(t);
    std::copy( (Uint8*)&t, (Uint8*)&t + sizeof(Uint16), std::back_inserter(buf) ); //seq encode
    
    for_each(topics.begin(), topics.end(), [&buf](tuple<string, Uint16> pack){
        string topic; Uint16 qos;
        std::tie(topic, qos) = pack;
    
        Uint16 t = htons(topic.length());
        std::copy( (Uint8*)&t, (Uint8*)&t + sizeof(Uint16), std::back_inserter(buf) );
        std::copy( std::begin(topic), std::end(topic), std::back_inserter(buf) );
        buf.push_back(qos);
    });
    
    return (Int) buf.size();
}

int MqttSubscribe::decode(const vector<Uint8> &buf){
    this->header = * ( (Header*) (&buf[0]) );
    Uint step = 1;
    
    //获取长度字段size
    size_t ll = 1;
    for ( int i = 1; i < 5; ++i ) {
        if ( buf[i] > 0x7F ) {
            ll++;
        } else break;
    }
    
    vector<Uint8> buf_len(ll);
    std::copy(std::begin(buf)+step , std::begin(buf)+ll, std::back_inserter(buf_len));
    size_t l = decLen(std::move(buf_len));
    this->size = ntohl(l);
    step += ll;
    
    //可变头
    Uint16 t = 0;
    std::copy( std::begin(buf)+step , std::begin(buf)+(Uint)sizeof(Uint16), (Uint8*)&t ); //seq
    step += (Uint)sizeof(Uint16);
    this->seq = ntohs(t);
    
    
    while (this->size + 1 > step ) {
        std::copy( std::begin(buf)+step , std::begin(buf)+(Uint)sizeof(Uint16), (Uint8*)&t ); //topic length
        step += (Uint)sizeof(Uint16);
        Uint topic_len = ntohs(t);
        
        string topic;
        std::copy( std::begin(buf)+step , std::begin(buf)+topic_len, std::back_inserter(topic) ); //topic
        step += (Uint)sizeof(Uint16);
        
        Uint8 qos = buf[step++];
        
        this->addTopic(topic, (MQTTQOS)qos);
    }
    
    return (Int) buf.size();
}



/////////////////MqttSuback///////////////

int MqttSuback::encode(vector<Uint8> &buf){
    Uint it = htonl( this->size );  //确保是little-enddian的网络字节序
    
    buf.push_back( *(Uint8*)((void*)&this->header) );
    auto len_buf = encLen(it);
    if (len_buf == nullptr) return -7;
    
    std::copy(std::begin(*len_buf), std::end(*len_buf), std::back_inserter(buf));
    
    //可变头
    Uint16 t = htons(this->seq);
    std::copy( (Uint8*)&t, (Uint8*)&t + sizeof(Uint16), std::back_inserter(buf) );
    
    std::copy( std::begin(this->results), std::end( this->results), std::back_inserter(buf) );
    
    return (Int) buf.size();
}

int MqttSuback::decode(const vector<Uint8> &buf){
    this->header = * ( (Header*) (&buf[0]) );
    Uint step = 1;
    
    //获取长度字段size
    size_t ll = 1; //最少都有一个字节长度
    for ( int i = 1; i < 5; ++i ) {
        if ( buf[i] > 0x7F ) {
            ll++;
        } else break;
    }
    
    vector<Uint8> buf_len(ll);
    std::copy(std::begin(buf)+step , std::begin(buf)+ll, std::back_inserter(buf_len));
    size_t l = decLen(std::move(buf_len));
    this->size = ntohl(l);
    step += ll;
    
    //可变头
    Uint16 t = 0;
    std::copy( std::begin(buf)+step , std::begin(buf)+(Uint)sizeof(Uint16), (Uint8*)&t ); //seq
    step += (Uint)sizeof(Uint16);
    this->seq = ntohs(t);
    
    Uint16 remainder = this->size + 1 - step;
    std::copy( std::begin(buf)+step , std::begin(buf) + remainder,  std::back_inserter(this->results) ); //seq
    step += remainder;
    
    return (Int) buf.size();
}



/////////////////MqttUnsubscribe///////////////

int MqttUnsubscribe::encode(vector<Uint8> &buf){
    if( this->topics.size() < 1 ) return -1; //无topic
    
    //计算payload大小
    Uint payload_len = 0; //test 安全性待定
    
    for_each(topics.begin(), topics.end(), [&payload_len](string topic){
        
        payload_len += (Uint)( topic.length() );
    });
    
    this->size = payload_len + 2;
    
    Uint it = htonl( this->size );  //确保是little-enddian的网络字节序
    
    buf.push_back( *(Uint8*)((void*)&this->header) );
    auto len_buf = encLen(it);
    if (len_buf == nullptr) return -7;
    
    std::copy(std::begin(*len_buf), std::end(*len_buf), std::back_inserter(buf));
    
    //可变头
    Uint16 t = htons(this->seq);
    t = htons(t);
    std::copy( (Uint8*)&t, (Uint8*)&t + sizeof(Uint16), std::back_inserter(buf) ); //seq encode
    
    for_each(topics.begin(), topics.end(), [&buf](string topic){
        Uint16 t = htons(topic.length());
        std::copy( (Uint8*)&t, (Uint8*)&t + sizeof(Uint16), std::back_inserter(buf) );
        std::copy( std::begin(topic), std::end(topic), std::back_inserter(buf) );
    });
    
    return (Int) buf.size();
}

int MqttUnsubscribe::decode(const vector<Uint8> &buf){
    this->header = * ( (Header*) (&buf[0]) );
    Uint step = 1;
    
    //获取长度字段size
    size_t ll = 1;
    for ( int i = 1; i < 5; ++i ) {
        if ( buf[i] > 0x7F ) {
            ll++;
        } else break;
    }
    
    vector<Uint8> buf_len(ll);
    std::copy(std::begin(buf)+step , std::begin(buf)+ll, std::back_inserter(buf_len));
    size_t l = decLen(std::move(buf_len));
    this->size = ntohl(l);
    step += ll;
    
    //可变头
    Uint16 t = 0;
    std::copy( std::begin(buf)+step , std::begin(buf)+(Uint)sizeof(Uint16), (Uint8*)&t ); //seq
    step += (Uint)sizeof(Uint16);
    this->seq = ntohs(t);
    
    while (this->size + 1 > step ) {
        std::copy( std::begin(buf)+step , std::begin(buf)+(Uint)sizeof(Uint16), (Uint8*)&t ); //topic length
        step += (Uint)sizeof(Uint16);
        Uint topic_len = ntohs(t);
        
        string topic;
        std::copy( std::begin(buf)+step , std::begin(buf)+topic_len, std::back_inserter(topic) ); //topic
        step += (Uint)sizeof(Uint16);
        
        this->delTopic(topic);
    }
    
    return (Int) buf.size();
}



/////////////////MqttUnsuback///////////////

int MqttUnsuback::encode(vector<Uint8> &buf){
    Uint it = htonl( this->size );  //确保是little-enddian的网络字节序
    
    buf.push_back( *(Uint8*)((void*)&this->header) );
    auto len_buf = encLen(it);
    if (len_buf == nullptr) return -7;
    
    std::copy(std::begin(*len_buf), std::end(*len_buf), std::back_inserter(buf));
    
    //可变头
    Uint16 t = htons(this->seq);
    std::copy( (Uint8*)&t, (Uint8*)&t + sizeof(Uint16), std::back_inserter(buf) );
    
    return (Int) buf.size();
}

int MqttUnsuback::decode(const vector<Uint8> &buf){
    this->header = * ( (Header*) (&buf[0]) );
    Uint step = 1;
    
    //获取长度字段size
    size_t ll = 1;
    for ( int i = 1; i < 5; ++i ) {
        if ( buf[i] > 0x7F ) {
            ll++;
        } else break;
    }
    
    vector<Uint8> buf_len(ll);
    std::copy(std::begin(buf)+step , std::begin(buf)+ll, std::back_inserter(buf_len));
    size_t l = decLen(std::move(buf_len));
    this->size = ntohl(l);
    step += ll;
    
    //可变头
    Uint16 t = 0;
    std::copy( std::begin(buf)+step , std::begin(buf)+(Uint)sizeof(Uint16), (Uint8*)&t ); //seq
    step += (Uint)sizeof(Uint16);
    this->seq = ntohs(t);
    
    return (Int) buf.size();
}



/////////////////MqttPing///////////////

int MqttPing::encode(vector<Uint8> &buf){
    Uint it = htonl( this->size );  //确保是little-enddian的网络字节序
    
    buf.push_back( *(Uint8*)((void*)&this->header) );
    auto len_buf = encLen(it);
    if (len_buf == nullptr) return -7;
    
    std::copy(std::begin(*len_buf), std::end(*len_buf), std::back_inserter(buf));
    
    return (Int) buf.size();
}

int MqttPing::decode(const vector<Uint8> &buf){
    this->header = * ( (Header*) (&buf[0]) );
    Uint step = 1;
    
    //获取长度字段size
    size_t ll = 1;
    for ( int i = 1; i < 5; ++i ) {
        if ( buf[i] > 0x7F ) {
            ll++;
        } else break;
    }
    
    vector<Uint8> buf_len(ll);
    std::copy(std::begin(buf)+step , std::begin(buf)+ll, std::back_inserter(buf_len));
    size_t l = decLen(std::move(buf_len));
    this->size = ntohl(l);
    step += ll;
    
    return (Int) buf.size();
}



/////////////////MqttPong///////////////

int MqttPong::encode(vector<Uint8> &buf){
    Uint it = htonl( this->size );  //确保是little-enddian的网络字节序
    
    buf.push_back( *(Uint8*)((void*)&this->header) );
    auto len_buf = encLen(it);
    if (len_buf == nullptr) return -7;
    
    std::copy(std::begin(*len_buf), std::end(*len_buf), std::back_inserter(buf));
    
    return (Int) buf.size();
}

int MqttPong::decode(const vector<Uint8> &buf){
    this->header = * ( (Header*) (&buf[0]) );
    Uint step = 1;
    
    //获取长度字段size
    size_t ll = 1;
    for ( int i = 1; i < 5; ++i ) {
        if ( buf[i] > 0x7F ) {
            ll++;
        } else break;
    }
    
    vector<Uint8> buf_len(ll);
    std::copy(std::begin(buf)+step , std::begin(buf)+ll, std::back_inserter(buf_len));
    size_t l = decLen(std::move(buf_len));
    this->size = ntohl(l);
    step += ll;
    
    return (Int) buf.size();
}



/////////////////MqttDisconnect///////////////

int MqttDisconnect::encode(vector<Uint8> &buf){
    Uint it = htonl( this->size );  //确保是little-enddian的网络字节序
    
    buf.push_back( *(Uint8*)((void*)&this->header) );
    auto len_buf = encLen(it);
    if (len_buf == nullptr) return -7;
    
    std::copy(std::begin(*len_buf), std::end(*len_buf), std::back_inserter(buf));
    
    return (Int) buf.size();
}

int MqttDisconnect::decode(const vector<Uint8> &buf){
    this->header = * ( (Header*) (&buf[0]) );
    Uint step = 1;
    
    //获取长度字段size
    size_t ll = 1;
    for ( int i = 1; i < 5; ++i ) {
        if ( buf[i] > 0x7F ) {
            ll++;
        } else break;
    }
    
    vector<Uint8> buf_len(ll);
    std::copy(std::begin(buf)+step , std::begin(buf)+ll, std::back_inserter(buf_len));
    size_t l = decLen(std::move(buf_len));
    this->size = ntohl(l);
    step += ll;
    
    return (Int) buf.size();
}




shared_ptr<vector<Uint8>> encLen(Uint len) {
    if (len > 0x0FFFFFFF) return nullptr;
    
    vector<Uint8> vec(1);
    Uint8 tmp[4] = {0};
    tmp[0] = (Uint8) (len & 0x0000007F);
    tmp[1] = (Uint8) ((len >> 7) & 0x0000007F);
    tmp[2] = (Uint8) ((len >> 14) & 0x0000007F);
    tmp[3] = (Uint8) ((len >> 21) & 0x0000007F);
    
    if (tmp[3] != 0) {
        vec.push_back(tmp[3]);
        tmp[2] = (Uint8) (tmp[2] | 0x80);
        vec.push_back(tmp[2]);
        tmp[1] = (Uint8) (tmp[1] | 0x80);
        vec.push_back(tmp[1]);
        tmp[0] = (Uint8) (tmp[0] | 0x80);
        vec.push_back(tmp[0]);
        
    } else if (tmp[2] != 0) {
        vec.push_back(tmp[2]);
        tmp[1] = (Uint8) (tmp[1] | 0x80);
        vec.push_back(tmp[1]);
        tmp[0] = (Uint8) (tmp[0] | 0x80);
        vec.push_back(tmp[0]);
    
    } else if (tmp[1] != 0) {
        vec.push_back(tmp[1]);
        tmp[0] = (Uint8) (tmp[0] | 0x80);
        vec.push_back(tmp[0]);
    } else {
        vec.push_back(tmp[0]);
    }
    
    return make_shared<vector<Uint8>>(vec);
}

size_t decLen( const vector<Uint8> &data){
    size_t len = 0;
    
    for (size_t i = 0; i < data.size(); ++i) {
        if (data[i] == 0) break;
        Uint8 t = (Uint8) (data[i] & 0x7F);
        len = len | (t << (i * 7));
        
        if (data[i] < 0x7F) break;
    }
    return (Uint)len;
}



