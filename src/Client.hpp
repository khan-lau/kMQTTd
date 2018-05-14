//
//  Client.hpp
//  MQTTd
//
//  Created by Khan on 16/3/29.
//  Copyright © 2016年 Khan. All rights reserved.
//


#ifndef __CLIENT_HPP__
#define __CLIENT_HPP__

#include <boost/asio.hpp>

#include "Protocol.hpp"
#include "Channel.hpp"

using boost::asio::ip::tcp;

class Client;

typedef std::shared_ptr<Client> client_ptr;


/** 参与者 */
class Client {
    public:
        explicit Client() {}
        explicit Client(std::shared_ptr<boost::asio::ip::tcp::socket> psocket) : _psocket{psocket} { }
        virtual ~Client() {}
        virtual void deliver(const MqttMessage& msg) = 0; //后面需要重载
    
    public:
        std::weak_ptr<boost::asio::ip::tcp::socket> _psocket;
};



#endif //__CLIENT_HPP__


