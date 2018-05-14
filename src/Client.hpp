//
//  Client.hpp
//  MQTTd
//
//  Created by Khan on 16/3/29.
//  Copyright © 2016年 Khan. All rights reserved.
//


#ifndef __CLIENT_HPP__
#define __CLIENT_HPP__

#include <memory>

#include <boost/asio.hpp>

#include "Protocol.hpp"
#include "Channel.hpp"


using ASocket = boost::asio::ip::tcp::socket;

using boost::asio::ip::tcp;

class Client;

typedef std::shared_ptr<ASocket> PSocket;
typedef std::weak_ptr<ASocket> WPSocket;
typedef std::shared_ptr<Client> PClient;


/** 参与者 */
class Client {
    public:
        explicit Client() {}
        explicit Client(PSocket psocket) : _psocket{psocket} { }
        virtual ~Client() {
            _psocket->shutdown(ASocket::shutdown_both, ec); //彻底关闭该socket上所有通信
            _psocket->close(ec);                                    //fd引用计数-1
        }
        virtual void deliver(const MqttMessage& msg) = 0; //后面需要重载
    
    public:
        WPSocket _psocket;
};



#endif //__CLIENT_HPP__


