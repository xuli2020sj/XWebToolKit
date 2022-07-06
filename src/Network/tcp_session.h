//
// Created by x on 22-7-4.
//

#ifndef XWEBTOOLKIT_TCPSESSION_H
#define XWEBTOOLKIT_TCPSESSION_H

#include "socket.h"
#include <string>
#include <memory>
#include "Thread/thread_pool.h"

using namespace std;

class TcpSession : public enable_shared_from_this<TcpSession> {
private:
    string local_ip_;
    string peer_ip_;
    uint16_t local_port_;
    uint16_t peer_port_;
    shared_ptr<ThreadPool> thread_pool_;
    Socket::Ptr sock_;
public:
    const string& getLocalIp() const {
        return local_ip_;
    }
    const string& getPeerIp() const {
        return peer_ip_;
    }
    uint16_t getLocalPort() const {
        return local_port_;
    }
    uint16_t getPeerPort() const {
        return peer_port_;
    }
protected:
    virtual int send(const string &buf) {
        return sock_->send(buf);
    }
    virtual int send(const char *buf, int size) {
        return sock_->send(buf, size);
    }


};


#endif //XWEBTOOLKIT_TCPSESSION_H
