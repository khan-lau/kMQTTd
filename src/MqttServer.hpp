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
#include <chrono>
#include <functional>
// #include <future>

#include <boost/asio.hpp>

#include "MqttClient.hpp"
#include "Session.hpp"

// using namespace std::chrono;
using std::cout;
using std::endl;
using std::string;
using std::vector;

using boost::asio::ip::tcp;
using boost::asio::ip::address;


class MQTTServer : private boost::noncopyable
{
    public:


        MQTTServer(string const &strIP, string const &strPort, int nThreads) : _acceptor(_ioService) ,_nThreads(nThreads){
            tcp::resolver resolver(_ioService);
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
            _ioService.stop();
            for (std::vector<std::shared_ptr<std::thread>>::const_iterator it = _listThread.cbegin();
                it != _listThread.cend(); ++ it)
            {
                (*it)->join();
            }
        }

        void Start() {
            for (int i = 0; i != _nThreads; ++i) {
                std::shared_ptr<std::thread> pTh( new std::thread([this](){
                    this->_ioService.run();
                }));
                _listThread.push_back(pTh);
            }
        }

    private:
        void StartAccept()
        {
            Session::CPSessionTcpCon newSession = Session::CreateNew(_acceptor.get_io_service());
            _acceptor.async_accept( newSession->socket(), 
                    [this, &newSession]( const boost::system::error_code& ec )
                    {
                        if (!ec) {
                            // //@TODO 此处可以于ip黑名单或其他基于ip过滤的功能
                            // if ( this->_blackList.end() == std::find(std::begin(this->_blackList), std::end(this->_blackList), newSession->socket()->remote_endpoint().address() ) ) {
                            //     std::make_shared<Session>( this->_ioService )->start();
                            //     // newSession->start();
                        }
                    }
                    StartAccept();
            });
        }



    private:
        boost::asio::io_service _ioService;
        boost::asio::ip::tcp::acceptor _acceptor;
        std::vector<std::shared_ptr<std::thread>> _listThread;
        std::size_t _nThreads;

        vector<address> _blackList;  //黑名单
};



#endif //__MqttServer_HPP__
