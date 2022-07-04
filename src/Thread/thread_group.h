//
// Created by x on 22-7-3.
//

#ifndef XLTOOLKIT_THREAD_GROUP_H
#define XLTOOLKIT_THREAD_GROUP_H

#include <thread>
#include <memory>

using namespace std;

class ThreadGroup {
private:
    unordered_map<thread::id, shared_ptr<thread>> threads_map_;
public:
    ThreadGroup() = default;
    ~ThreadGroup() = default;

    template<typename F>
    shared_ptr<thread> createThread(F thread_func) {
        auto new_thread = make_shared<thread>(thread(thread_func));
        threads_map_[new_thread->get_id()] = new_thread;
        return new_thread;
    };

    void joinAllThreads() {
        for (const auto& th : threads_map_) {
            if (th.second->joinable()) {
                th.second->join();
            }
        }
    }

    bool isThisThreadIn() {
        return threads_map_.count(this_thread::get_id());
    }

    bool isThreadIn(thread* thread) {
        return threads_map_.count(thread->get_id());
    }

    size_t size() const {
        return threads_map_.size();
    }

};

#endif //XLTOOLKIT_THREAD_GROUP_H
