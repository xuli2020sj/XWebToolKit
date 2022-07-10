//
// Created by x on 22-7-4.
//

#include "EventPoller.h"

#define toEpoll(event)	(((event) & EVENT_READ) ? EPOLLIN : 0) \
						| (((event) & EVENT_WRITE) ? EPOLLOUT : 0) \
						| (((event) & EVENT_ERROR) ? (EPOLLHUP | EPOLLERR) : 0) \
						| (((event) & EVENT_LT) ?  0 : EPOLLET)

#define toPoller(epoll_event) (((epoll_event) & EPOLLIN) ? Event_Read : 0) \
							| (((epoll_event) & EPOLLOUT) ? Event_Write : 0) \
							| (((epoll_event) & EPOLLHUP) ? Event_Error : 0) \
							| (((epoll_event) & EPOLLERR) ? Event_Error : 0)

EventPoller::EventPoller(bool enable_self_run) {
    epoll_fd_ = epoll_create(1024);
    if (epoll_fd_ == -1) {
        throw runtime_error("创建epoll文件描述符失败:");
    }
}

EventPoller::~EventPoller() {

}

int EventPoller::addEvent(int fd, int event, PollEventCB &&cb) {
    if (!cb) {
        return -1;
    }
    struct epoll_event ee{};
    ee.data.fd = fd;
    ee.events = toEpoll(event);
    int ret = epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &ee);
    if (ret == 0) {
        lock_guard<mutex> lk(mtx_event_map_);
        event_map_.emplace(fd, cb);
    }
    return ret;
}


