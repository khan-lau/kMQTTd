//
//  MqttException.hpp
//  MQTTd
//
//  Created by Khan on 16/3/29.
//  Copyright © 2016年 Khan. All rights reserved.
//

#ifndef __MqttException_HPP__
#define __MqttException_HPP__


#include <boost/system/error_code.hpp>
#include <iostream>
#include <string>

static const std::string error_messages[] = {
    "MQTT succes!"                       //0
    "MQTT length error!",                //1
    "MQTT protocol error!",              //2
    "MQTT decode error!",                //3
    "Socket is destroyed!"               //4
};


class Mqtt_ErrorCategory : public boost::system::error_category {
    
public:
    const char *name() const noexcept{
        return "MQTT Exception ";
    }
    
    
    std::string message(int ev) const {
        if (ev >=0 && ev < sizeof(error_messages) ) {
            return error_messages[ev];
        } else {
            return "MQTT undefine error!";
        }
    }
};



inline const Mqtt_ErrorCategory &  mqtt_category() BOOST_SYSTEM_NOEXCEPT {
    static Mqtt_ErrorCategory cat;

    return cat;
}



#endif /* __MqttException_HPP__ */
