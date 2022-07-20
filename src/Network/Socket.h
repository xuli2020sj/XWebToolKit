//
// Created by x on 2022/7/19.
//

#ifndef XLTOOLKIT_SOCKET_H
#define XLTOOLKIT_SOCKET_H

#include "memory"
#include "functional"
#include "Poller/EventPoller.h"

typedef enum {
    Err_success = 0, //成功
    Err_eof, //eof
    Err_timeout, //超时
    Err_refused,//连接被拒绝
    Err_dns,//dns解析失败
    Err_shutdown,//主动关闭
    Err_other = 0xFF,//其他错误
} ErrCode;

class SockException : public std::exception {
public:
    SockException(ErrCode code = Err_success, const std::string &msg = "", int custom_code = 0) {
        _msg = msg;
        _code = code;
        _custom_code = custom_code;
    }

    //重置错误
    void reset(ErrCode code, const std::string &msg, int custom_code = 0) {
        _msg = msg;
        _code = code;
        _custom_code = custom_code;
    }

    //错误提示
    const char *what() const noexcept override {
        return _msg.c_str();
    }

    //错误代码
    ErrCode getErrCode() const {
        return _code;
    }

    //用户自定义错误代码
    int getCustomCode() const {
        return _custom_code;
    }

    //判断是否真的有错
    operator bool() const {
        return _code != Err_success;
    }

private:
    ErrCode _code;
    int _custom_code = 0;
    std::string _msg;
};

class Socket {
public:
    using Ptr = std::shared_ptr<Socket>;
    using onReadCB = std::function<void(const Buffer::Ptr &buf, struct sockaddr *addr, int addr_len)>;
    //发生错误回调
    using onErrCB = std::function<void(const SockException &err)>;
    //tcp监听接收到连接请求
    using onAcceptCB = std::function<void(Socket::Ptr &sock, std::shared_ptr<void> &complete)>;
    //socket发送缓存清空事件，返回true代表下次继续监听该事件，否则停止
    using onFlush = std::function<bool()>;
    //在接收到连接请求前，拦截Socket默认生成方式
    using onCreateSocket = std::function<Ptr(const EventPoller::Ptr &poller)>;
    //发送buffer成功与否回调
    using onSendResult = BufferList::SendResult;


    std::string get_local_ip();
    uint16_t get_local_port();
    std::string get_peer_ip();
    uint16_t get_peer_port();
    std::string getIdentifier();
};


#endif //XLTOOLKIT_SOCKET_H
