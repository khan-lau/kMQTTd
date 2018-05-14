//
//  MQTTd.cpp
//  MQTTd
//
//  Created by Khan on 18/5/11.
//  Copyright © 2018年 Khan. All rights reserved.
//

#include <iostream>

#include <boost/lexical_cast.hpp>


#include "MqttServer.hpp"



//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[])
{
    try {
        if (argc != 4) {
            std::cerr << argv[0] << " <IP> <port> <threads>\n";
            return 1;
        }
        
        int nThreads = boost::lexical_cast<int>(argv[3]);
        MQTTServer mySer(argv[1], argv[2], nThreads);
        mySer.Start();
        getchar();
        mySer.Stop();
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }
    return 0;
}

