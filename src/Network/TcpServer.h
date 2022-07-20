//
// Created by x on 2022/7/19.
//

#ifndef XLTOOLKIT_TCPSERVER_H
#define XLTOOLKIT_TCPSERVER_H


#include <memory>
#include <functional>
#include <unordered_map>

#include "Poller/EventPoller.h"



class TcpServer {
public:
    using Ptr = std::shared_ptr<TcpServer>;
private:
    EventPoller::Ptr poller_;
    std::function<SessionHelper::Ptr(const TcpServer::Ptr &server, const Socket::Ptr &)> _session_alloc;
public:
    explicit TcpServer(const EventPoller::Ptr &poller = nullptr);
    ~TcpServer();

    template<typename SessionType>
    void start(uint16_t port, const std::string &host = "::", uint32_t backlog = 1024) {
        //TcpSession创建器，通过它创建不同类型的服务器
        _session_alloc = [](const TcpServer::Ptr &server, const Socket::Ptr &sock) {
            auto session = std::make_shared<SessionType>(sock);
            session->setOnCreateSocket(server->_on_create_socket);
            return std::make_shared<SessionHelper>(server, session);
        };
        start_l(port, host, backlog);
    }

};


#endif //XLTOOLKIT_TCPSERVER_H
