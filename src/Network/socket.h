//
// Created by x on 22-7-4.
//

#ifndef XLTOOLKIT_SOCKET_H
#define XLTOOLKIT_SOCKET_H

#include <cstdint>
#include <functional>
#include <memory>
//#include <bits/socket.h>
#include <sys/socket.h>

using namespace std;

typedef enum {
    Err_success = 0,
    Err_eof,
    Err_timeout,
    Err_refused,
    Err_dns,
    Err_other,
} ErrCode;

class SockException: public std::exception {
public:
    SockException(ErrCode errCode = Err_success, const string &errMsg = "") {
        errMsg_ = errMsg;
        errCode_ = errCode;
    }

    void reset(ErrCode _errCode, const string &_errMsg) {
        errMsg_ = _errMsg;
        errCode_ = _errCode;
    }

    const char* what() const noexcept override {
        return errMsg_.c_str();
    }

    ErrCode getErrCode() const {
        return errCode_;
    }

    explicit operator bool() const{
        return errCode_ != Err_success;
    }
private:
    string errMsg_;
    ErrCode errCode_;
};


class Buffer {
private:
    char* data_;
    uint32_t size_;
public:
    using Ptr = shared_ptr<Buffer>;
    explicit Buffer(uint32_t size) : size_(size) {
        data_ = new char[size];
    }

    virtual ~Buffer() {
        delete[] data_;
    }

    const char* data() {
        return data_;
    }

    uint32_t size() const {
        return size_;
    }
};

class SockFD {
private:
    int sock_fd_;
public:
    using Ptr = shared_ptr<SockFD>;
    explicit SockFD(int sock_fd) : sock_fd_(sock_fd) {};

    virtual ~SockFD() {
        shutdown(sock_fd_, SHUT_RDWR);

    }
};

class Socket {
public:
    using Ptr = shared_ptr<Socket>;
    using onReadCB = function<void(const Buffer::Ptr, struct sockaddr *addr)>;
    using onErrorCB = function<void(const SockException& err)>;
    using onAcceptCB = function<void(Socket::Ptr& sock)>;
    using onFlushCB = function<bool()>;
private:
    onReadCB read_cb_;
    onErrorCB error_cb_;
    onAcceptCB accept_cb_;
    onFlushCB flush_cb_;
    mutex mtx_read_, mtx_error_, mtx_accept_, mtx_flush_;
    SockFD::Ptr sock_fd_;
    mutex mtx_sock_fd_;
    struct sockaddr peer_addr_;

public:
    void setOnRead(const onReadCB &cb);
    void setOnErr(const onErrorCB &cb);
    void setOnAccept(const onAcceptCB &cb);
    void setOnFlush(const onFlushCB &cb);

    bool setPeerSock(int sock, struct sockaddr* addr);
    string get_local_ip();
    uint16_t get_local_port();
    string get_peer_ip();
    uint16_t get_peer_port();
    void connect(const string &url, uint16_t port, onErrorCB &&connectCB, int time_out_sec = 5);
    int send(const string* buf, struct sockaddr* peer_addr, int flags);

    void closeSock();
public:

    Socket() = default;



};


#endif //XLTOOLKIT_SOCKET_H
