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
#include <sstream>

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

class SockInfo {
public:
    SockInfo() = default;
    virtual ~SockInfo() = default;

    //获取本机ip
    virtual std::string get_local_ip() = 0;
    //获取本机端口号
    virtual uint16_t get_local_port() = 0;
    //获取对方ip
    virtual std::string get_peer_ip() = 0;
    //获取对方端口号
    virtual uint16_t get_peer_port() = 0;
    //获取标识符
    virtual std::string getIdentifier() const { return ""; }
};

class SockSender {
public:
    SockSender() = default;
    virtual ~SockSender() = default;
    virtual ssize_t send(Buffer::Ptr buf) = 0;
    virtual void shutdown(const SockException &ex = SockException(Err_shutdown, "self shutdown")) = 0;

    //发送char *
    SockSender &operator << (const char *buf);
    //发送字符串
    SockSender &operator << (std::string buf);
    //发送Buffer对象
    SockSender &operator << (Buffer::Ptr buf);

    //发送其他类型数据
    template<typename T>
    SockSender &operator << (T &&buf) {
        std::ostringstream ss;
        ss << std::forward<T>(buf);
        send(ss.str());
        return *this;
    }

    ssize_t send(std::string buf);
    ssize_t send(const char *buf, size_t size = 0);
};

//Socket对象的包装类
class SocketHelper : public SockSender, public SockInfo, public TaskExecutorInterface {
public:
    SocketHelper(const Socket::Ptr &sock);
    ~SocketHelper() override;

    ///////////////////// Socket util std::functions /////////////////////
    /**
     * 获取poller线程
     */
    const EventPoller::Ptr& getPoller() const;

    /**
     * 设置批量发送标记,用于提升性能
     * @param try_flush 批量发送标记
     */
    void setSendFlushFlag(bool try_flush);

    /**
     * 设置socket发送flags
     * @param flags socket发送flags
     */
    void setSendFlags(int flags);

    /**
     * 套接字是否忙，如果套接字写缓存已满则返回true
     */
    bool isSocketBusy() const;

    /**
     * 设置Socket创建器，自定义Socket创建方式
     * @param cb 创建器
     */
    void setOnCreateSocket(Socket::onCreateSocket cb);

    /**
     * 创建socket对象
     */
    Socket::Ptr createSocket();

    ///////////////////// SockInfo override /////////////////////
    std::string get_local_ip() override;
    uint16_t get_local_port() override;
    std::string get_peer_ip() override;
    uint16_t get_peer_port() override;

    ///////////////////// TaskExecutorInterface override /////////////////////
    /**
     * 任务切换到所属poller线程执行
     * @param task 任务
     * @param may_sync 是否运行同步执行任务
     */
    Task::Ptr async(TaskIn task, bool may_sync = true) override;
    Task::Ptr async_first(TaskIn task, bool may_sync = true) override;

    ///////////////////// SockSender override /////////////////////
    /**
     * 统一发送数据的出口
     */
    ssize_t send(Buffer::Ptr buf) override;

    /**
     * 触发onErr事件
     */
    void shutdown(const SockException &ex = SockException(Err_shutdown, "self shutdown")) override;

protected:
    void setPoller(const EventPoller::Ptr &poller);
    void setSock(const Socket::Ptr &sock);
    const Socket::Ptr& getSock() const;

private:
    bool _try_flush = true;
    uint16_t _peer_port = 0;
    uint16_t _local_port = 0;
    std::string _peer_ip;
    std::string _local_ip;
    Socket::Ptr _sock;
    EventPoller::Ptr _poller;
    Socket::onCreateSocket _on_create_socket;
};

#endif //XLTOOLKIT_SOCKET_H
