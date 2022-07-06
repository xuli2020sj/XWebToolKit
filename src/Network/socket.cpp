//
// Created by x on 22-7-4.
//

#include "socket.h"

void Socket::setOnRead(const onReadCB &cb) {
    lock_guard<mutex> lk(mtx_read_);
    read_cb_ = cb;
}

void Socket::setOnErr(const onErrorCB &cb) {
    lock_guard<mutex> lk(mtx_error_);
    error_cb_ = cb;
}

void Socket::setOnAccept(const onAcceptCB &cb) {
    lock_guard<mutex> lk(mtx_accept_);
    accept_cb_ = cb;
}

void Socket::setOnFlush(const onFlushCB &cb) {
    lock_guard<mutex> lk(mtx_flush_);
    flush_cb_ = cb;
}

string Socket::get_local_ip() {
    SockFD::Ptr sock;

    return std::string();
}

uint16_t Socket::get_local_port() {
    return 0;
}

string Socket::get_peer_ip() {
    return std::string();
}

uint16_t Socket::get_peer_port() {
    return 0;
}

void Socket::connect(const string &url, uint16_t port, Socket::onErrorCB &&connectCB, int time_out_sec) {

}

int Socket::send(const string* buf, struct sockaddr* peer_addr, int flags) {
    if (buf->empty()) return 0;
    SockFD::Ptr sock;
    {
        lock_guard<mutex> lk(mtx_sock_fd_);
        sock = sock_fd_;
    }
    if (!sock) return -1;


}

void Socket::closeSock() {
    lock_guard<mutex> lk(mtx_sock_fd_);
    sock_fd_.reset();
}

bool Socket::setPeerSock(int sock, struct sockaddr *addr) {
    closeSock();

}
