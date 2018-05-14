//
//  Channel.hpp
//  MQTTd
//
//  Created by Khan on 16/4/12.
//  Copyright © 2016年 Khan. All rights reserved.
//

#ifndef __CHANNEL_HPP__
#define __CHANNEL_HPP__

#include <set>
#include <list>
#include "Protocol.hpp"

class Channel {
public:
    void join(std::string &client_id) {
        _clients.insert(client_id);
    }
    
    void leave(std::string &client_id) {
        _clients.erase(client_id);
    }
    
private:
    std::set<std::string> _clients;  //用set来保存用户信息
    std::list<MqttMessage> msgs;     //用来保存从某个客户端接收到的信息
};

#endif /* __CHANNEL_HPP__ */
