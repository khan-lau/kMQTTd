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


class Client;
typedef std::shared_ptr<Client> PClient;

typedef std::shared_ptr<ASocket> PSocket;
typedef std::weak_ptr<ASocket> WPSocket;



/** 参与者 */
class Client {
    public:
        explicit Client() {}
    
        explicit Client(PSocket psocket) : _psocket{psocket}
        {
        }
    
        virtual ~Client() {}
        virtual void deliver(const MqttMessage& msg) = 0; //后面需要重载
    
        virtual void close() = 0;
    
        PSocket& Socket(){return this->_psocket;}
    
    public:
        PSocket _psocket;
};



#endif //__CLIENT_HPP__


