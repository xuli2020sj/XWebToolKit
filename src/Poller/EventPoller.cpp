//


#include <sys/ioctl.h>
#include "EventPoller.h"
#include "errno.h"
#include "thread"

#define toEpoll(event)	(((event) & EVENT_READ) ? EPOLLIN : 0) \
						| (((event) & EVENT_WRITE) ? EPOLLOUT : 0) \
						| (((event) & EVENT_ERROR) ? (EPOLLHUP | EPOLLERR) : 0) \
						| (((event) & EVENT_LT) ?  0 : EPOLLET)

#define toPoller(epoll_event) (((epoll_event) & EPOLLIN) ? EPOLLIN : 0) \
							| (((epoll_event) & EPOLLOUT) ? EPOLLOUT : 0) \
							| (((epoll_event) & EPOLLHUP) ? EPOLLERR : 0) \
							| (((epoll_event) & EPOLLERR) ? EPOLLERR : 0)

EventPoller::EventPoller(ThreadPool::Priority priority) : priority_(priority){
    int ul = 1;
    int ret1 = ioctl(pipe_.getReadFD(), FIONBIO, &ul);
    int ret2 = ioctl(pipe_.getWriteFD(), FIONBIO, &ul);
    logger_ = Logger::getLogger().shared_from_this();
    if (ret1 == -1 || ret2 == 1) {
        TraceL << "设置非阻塞失败!";
    }
    epoll_fd_ = epoll_create(1024);
    if (epoll_fd_ == -1) {
        throw runtime_error("创建epoll文件描述符失败:");
    }
    loop_thread_id_ = this_thread::get_id();
    if (addEvent(pipe_.getReadFD(), EVENT_READ, [this](int event) { onPipeEvent(); }) == -1) {
        throw std::runtime_error("epoll添加管道失败");
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
    onPipeEvent();
}


static thread_local weak_ptr<EventPoller> current_poller;

void EventPoller::runLoop(bool blocked, bool ref_self) {
    if (blocked) {
        ThreadPool::setPriority(priority_);
        lock_guard<mutex> lck(mtx_running_);
        loop_thread_id_ = this_thread::get_id();
        if (ref_self) {
            current_poller = shared_from_this();
        }
        sem_loop_.release();

        exit_flag_ = false;
        uint64_t minDelay;
        struct epoll_event events[1024];
        int epoll_ret = 0;

        while (!exit_flag_) {
            minDelay = getMinDelay();
            sleep();
            epoll_ret = epoll_wait(epoll_fd_, events, 1024, minDelay ? minDelay : -1);
            wake();
            if (epoll_ret <= 0) continue;

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

    } else {
        loop_thread_ = new thread(&EventPoller::runLoop, this, true, ref_self);
        sem_loop_.acquire();
    }
}

int EventPoller::addEvent(int fd, int event, PollEventCB &&cb) {
    if (!cb) {
        WarnL << "cb function is null";
        return -1;
    }
    struct epoll_event ee{};
    ee.data.fd = fd;
    ee.events = toEpoll(event);
    int ret = epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &ee);
    if (ret == 0) {
        lock_guard<mutex> lk(mtx_event_map_);
        event_map_.emplace(fd, make_shared<PollEventCB>(move(cb)));
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
        throw std::runtime_error("add pipe failed");
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
    std::binary_semaphore sem(0);
    async([&](){
        cb();
        sem.release();
    });
    sem.acquire();
}

EventPoller &EventPoller::getEventPoller(bool enable_self_run) {
    return *(EventPollerPool::Instance().getFirstPoller());
}

void EventPoller::destroy() {
    delete &EventPoller::getEventPoller();
}

void EventPoller::onPipeEvent() {
    char buf[1024];
    errno = 0;
    do {
        if (pipe_.read(buf, sizeof(buf)) > 0) {
            continue;
        }
    } while (errno != EAGAIN);
    decltype(_list_task) list_temp;
    {
        lock_guard<mutex> lck(mtx_task_);
        list_temp.swap(_list_task);
    }
    for_each(list_temp.begin(), list_temp.end(), [&](const Task::Ptr &task){
        try {
//            WarnL << "execute async task:" << list_temp.size();
            (*task)();
        } catch (std::exception &ex) {
            ErrorL << "EventPoller执行异步任务捕获到异常:" << ex.what();
        }
    });


}

bool EventPoller::isCurrentThread() const {
    return this_thread::get_id() == loop_thread_id_;
}

EventPoller::Ptr EventPoller::getCurrentPoller() {
    return current_poller.lock();
}

uint64_t EventPoller::getMinDelay() {
    auto iter = delay_task_map_.begin();
    if (iter == delay_task_map_.end()) {
        return 0;
    }
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    if (iter->first > now) {
        return iter->first - now;
    }
    return flushDelayTask(now);
}

uint64_t EventPoller::flushDelayTask(uint64_t now) {
    decltype(delay_task_map_) delay_task_copy;
    delay_task_copy.swap(delay_task_map_);

    for (auto it = delay_task_copy.begin(); it != delay_task_copy.end() && it->first <= now; it = delay_task_copy.erase(it)) {
        //已到期的任务
        try {
            auto next_delay = (*(it->second))();
            if (next_delay) {
                //可重复任务,更新时间截止线
                delay_task_map_.emplace(next_delay + now, std::move(it->second));
            }
        } catch (std::exception &ex) {
            ErrorL << "EventPoller执行延时任务捕获到异常:" << ex.what();
        }
    }

    delay_task_copy.insert(delay_task_map_.begin(), delay_task_map_.end());
    delay_task_copy.swap(delay_task_map_);

    auto iter = delay_task_map_.begin();
    if (iter == delay_task_map_.end()) {
        return 0;
    }
    return iter->first - now;
}

DelayTask::Ptr EventPoller::doDelayTask(uint64_t delay_ms, function<uint64_t()> task) {
    DelayTask::Ptr ret = std::make_shared<DelayTask>(std::move(task));
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    auto time_line = now_ms + delay_ms;
    asyncFirst([time_line, ret, this]() {
        //异步执行的目的是刷新select或epoll的休眠时间
        delay_task_map_.emplace(time_line, ret);
    }, false);
    return ret;
}

Task::Ptr EventPoller::async(TaskIn task, bool may_sync) {
    return async_(std::move(task), may_sync, false);
}

Task::Ptr EventPoller::asyncFirst(TaskIn task, bool may_sync) {
    return async_(std::move(task), may_sync, true);
}

Task::Ptr EventPoller::async_(TaskIn task, bool may_sync, bool is_first) {
    if (may_sync && isCurrentThread()) {
        task();
        return nullptr;
    }

    auto ret = std::make_shared<Task>(std::move(task));
    {
        lock_guard<mutex> lck(mtx_task_);
        if (is_first) {
            _list_task.emplace_front(ret);
        } else {
            _list_task.emplace_back(ret);
        }
    }
    //写数据到管道,唤醒主线程
    pipe_.write("", 1);
    return ret;
}


/////////////////// EventPollerPool //////////////////////////
static size_t s_pool_size = 0;
EventPollerPool &EventPollerPool::Instance() {
    static std::shared_ptr<EventPollerPool> s_instance(new EventPollerPool());
    static EventPollerPool &s_instance_ref = *s_instance;
    return s_instance_ref;
}

EventPoller::Ptr EventPollerPool::getFirstPoller() {
    return dynamic_pointer_cast<EventPoller>(threads_.front());
}

EventPoller::Ptr EventPollerPool::getPoller(bool prefer_current_thread) {
    auto poller = EventPoller::getCurrentPoller();
    if (prefer_current_thread && prefer_current_thread_ && poller) {
        return poller;
    }
    return dynamic_pointer_cast<EventPoller>(getExecutor());
}

void EventPollerPool::preferCurrentThread(bool flag) {
    prefer_current_thread_ = flag;
}

EventPollerPool::EventPollerPool() {
    auto size = addPoller("event poller", s_pool_size, ThreadPool::PRIORITY_HIGHEST, true);
    InfoL << "创建EventPoller个数:" << size;
}

void EventPollerPool::setPoolSize(size_t size) {
    s_pool_size = size;
}


