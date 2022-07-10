//
// Created by x on 22-7-10.
//

#include <functional>
#include <memory>
#include "chrono"
#include <iostream>
#include "queue"

using namespace std;

typedef struct TaskInfo{
    uint64_t task_id_;
    uint64_t execute_time_stamp_;
    uint64_t execute_time_interval_;
    std::function<bool()> task_;
    bool operator<(TaskInfo& other) const {
        return this->execute_time_stamp_ < other.execute_time_stamp_;
    }
    TaskInfo() {}
    TaskInfo(size_t task_id, size_t execute_time_point,  size_t execute_time_interval,
             std::function<bool()> task) : task_id_(task_id),
             execute_time_stamp_(execute_time_point), execute_time_interval_(execute_time_interval), task_(std::move(task)){}
} TaskInfo;

int main() {
    auto cmp = [](shared_ptr<TaskInfo>& s1, shared_ptr<TaskInfo>& s2){
        return s1->execute_time_stamp_ > s2->execute_time_stamp_;
    };
    priority_queue<shared_ptr<TaskInfo>, vector<shared_ptr<TaskInfo>>, less<>> pq;
    auto t1 = make_shared<TaskInfo>(1,
                                    chrono::system_clock::now().time_since_epoch().count() + chrono::milliseconds(1000).count(),
                                    chrono::milliseconds(1000).count(),
                                    [](){
                                        cout << "t1" << endl;
                                        return true;
                                    });
//    t1->task_id_ = 1;
//    t1->execute_time_interval_ = chrono::milliseconds(1000).count();
//    t1->execute_time_stamp_ = chrono::system_clock::now().time_since_epoch().count() + t1->execute_time_interval_;
//    t1->task_ = [](){
//        cout << "t1" << endl;
//        return true;
//    };
    auto t2 = make_shared<TaskInfo>();
    t2->task_id_ = 2;
    t2->execute_time_interval_ = chrono::milliseconds(1000).count();
    t2->execute_time_stamp_ = chrono::system_clock::now().time_since_epoch().count() + t1->execute_time_interval_;
    t2->task_ = [](){
        cout << "t1" << endl;
        return true;
    };
    pq.emplace(t2);
    pq.push(t1);
    cout << "finished" << endl;
    pq.pop();
}

