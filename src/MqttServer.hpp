//
//  MqttServer.hpp
//  MQTTd
//
//  Created by Khan on 18/5/11.
//  Copyright © 2018年 Khan. All rights reserved.
//

#ifndef __MqttServer_HPP__
#define __MqttServer_HPP__

#include <string>
#include <iostream>
#include <vector>
// #include <chrono>
#include <functional>
// #include <future>

#include <boost/asio.hpp>

#include "MqttClient.hpp"
#include "MqttSession.hpp"

// using std::cout;
// using std::endl;
using std::string;
using std::vector;

using boost::asio::ip::tcp;
using boost::asio::ip::address;

typedef size_t(boost::asio::io_context::* PMF)();  //成员函数指针

class MQTTServer : private boost::noncopyable
{
    public:

        /**
         * @brief Construct a new MQTTServer object
         * 
         * @param strIP 绑定IP
         * @param strPort 绑定端口号
         * @param nThreads 线程数
         */
        MQTTServer(string const &strIP, string const &strPort, std::size_t nThreads) : _acceptor{_ioContent}, _nThreads{nThreads}
        {
            tcp::resolver resolver(_ioContent);
            tcp::resolver::query query(strIP, strPort);
            tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
            boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve(query);
            _acceptor.open(endpoint.protocol());
            _acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
            _acceptor.bind(endpoint);
            _acceptor.listen();

            StartAccept();
        }

    public:

        void Stop()
        {
            _ioContent.stop();
            for (std::vector<std::shared_ptr<std::thread>>::const_iterator it = _listThread.cbegin();
                it != _listThread.cend(); ++ it)
            {
                (*it)->join();
            }
        }

        void Start() {
            for (int i = 0; i != _nThreads; ++i) {
                // std::shared_ptr<std::thread> pTh( new std::thread([this](){
                //     this->_ioContent.run();
                // }));
                
                //boost::asio::io_context::* 成员函数指针
                std::shared_ptr<std::thread> pTh(new std::thread(std::bind(static_cast<PMF>(&boost::asio::io_context::run), &_ioContent)));  
                _listThread.push_back(pTh);
            }
        }




    private:

        void StartAccept()
        {
            _acceptor.async_accept( [this](const boost::system::error_code& ec, ASocket new_socket){

                if (!ec) {
                    //@TODO 此处可以于ip黑名单或其他基于ip过滤的功能
                    if ( this->_blackList.end() == std::find(std::begin(this->_blackList), std::end(this->_blackList), new_socket.remote_endpoint().address() ) ) {
                        PSocket p_socket = std::make_shared<ASocket>( std::move(new_socket) );

                        //构造了一个shared_ptr指针, 等同于 shared_ptr p(_csocket);
                        std::shared_ptr<MqttSession> p_session = std::make_shared<MqttSession>( std::move(p_socket) ); 
                        p_session->start();
                    }
                }
                this->StartAccept();
            } );
        }


    private:
        boost::asio::io_context _ioContent;
        boost::asio::ip::tcp::acceptor _acceptor;
        std::vector<std::shared_ptr<std::thread>> _listThread;
        std::size_t _nThreads;

        vector<address> _blackList;  //黑名单
};



#endif //__MqttServer_HPP__
