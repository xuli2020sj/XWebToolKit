//
// Created by x on 22-7-4.
//

#include "EventPoller.h"
#include "errno.h"

#define toEpoll(event)	(((event) & EVENT_READ) ? EPOLLIN : 0) \
						| (((event) & EVENT_WRITE) ? EPOLLOUT : 0) \
						| (((event) & EVENT_ERROR) ? (EPOLLHUP | EPOLLERR) : 0) \
						| (((event) & EVENT_LT) ?  0 : EPOLLET)

#define toPoller(epoll_event) (((epoll_event) & EPOLLIN) ? EPOLLIN : 0) \
							| (((epoll_event) & EPOLLOUT) ? EPOLLOUT : 0) \
							| (((epoll_event) & EPOLLHUP) ? EPOLLERR : 0) \
							| (((epoll_event) & EPOLLERR) ? EPOLLERR : 0)

EventPoller::EventPoller(bool enable_self_run) {
    epoll_fd_ = epoll_create(1024);
    if (epoll_fd_ == -1) {
        throw runtime_error("创建epoll文件描述"
                            "符失败:");
    }
    if (enable_self_run) {
        loop_thread_ = new thread(&EventPoller::runLoop, this);
        loop_thread_id_ = loop_thread_->get_id();
    }
}

EventPoller::~EventPoller() {
    exit_flag_ = true;
    if (loop_thread_) {
        loop_thread_->join();
        delete loop_thread_;
    }
    if (epoll_fd_ != -1) {
        close(epoll_fd_);
        epoll_fd_ = -1;
    }
}

void EventPoller::runLoop() {
    loop_thread_id_ = this_thread::get_id();

    struct epoll_event events[1024];
    int epoll_ret = 0;

    while (!exit_flag_) {
        epoll_ret = epoll_wait(epoll_fd_, events, 1024, -1);
        if (epoll_ret < 0) continue;

        for (int i = 0; i < epoll_ret; i++) {
            struct epoll_event& ee = events[i];
            int fd = ee.data.fd;
            auto iter = event_map_.find(fd);
            if (iter == event_map_.end()) {
                epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr);
                continue;
            }
            auto cb = iter->second;
            try {
                (*cb)(toPoller(ee.events));
            } catch (std::exception &ex){
                ErrorL << ex.what();
            }
        }
    }
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

int EventPoller::delEvent(int fd, PollDelCB &&cb) {
    if (!cb) {
        return -1;
    }
    int ret0 = epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr);
    int ret1 = 0;
    {
        lock_guard<mutex> lk(mtx_event_map_);
        ret1 = event_map_.erase(fd);
    }
    bool success = ret1 == 0 && ret0;
    cb(success);
    return success;
}

int EventPoller::modifyEvent(int fd, int event) {
    struct epoll_event ee = {0 };
    ee.events = toEpoll(event);
    ee.data.fd = fd;
    return epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &ee);
}

void EventPoller::init() {
    if (addEvent(pipe_.getReadFD(), EVENT_READ | EVENT_ERROR, [](int event){}) == -1) {
        std::runtime_error("add pipe failed");
    }
}

void EventPoller::async(PollAsyncCB &&cb) {
    if (!cb) {
        return;
    }
    if (loop_thread_id_ == this_thread::get_id()) {
        cb();
        return;
    }

}

void EventPoller::sync(PollAsyncCB &&cb) {
    if (!cb) return;
    binary_semaphore sem(0);
    async([&](){
        cb();
        sem.release();
    });
    sem.acquire();
}

EventPoller &EventPoller::getEventPoller(bool enable_self_run) {
    static auto* event_poller = new EventPoller(enable_self_run);
    return *event_poller;
}

void EventPoller::destroy() {
    delete &EventPoller::getEventPoller();
}

void EventPoller::onPipeEvent() {
    char buf[1024];
    int err = 0;
    do {
        if (pipe_.read(buf, sizeof(buf)) > 0) {
            continue;
        }
        err =
    }
}


