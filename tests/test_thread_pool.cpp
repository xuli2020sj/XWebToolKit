#include "spdlog/spdlog.h"
#include "Thread/thread_pool.h"
#include <csignal>
#include <iostream>
#include <sstream>
#include "chrono"
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/async.h"

using namespace std;

string getThreadIDStr(thread::id id) {
    stringstream s;
    s << id;
    return s.str();
}

int main() {
    auto file_logger = spdlog::rotating_logger_mt<spdlog::async_factory>("file_logger", "test_logs",
                                                              1024 * 1024 * 5, 100);
    file_logger->info("main thread id: {0}", getThreadIDStr(this_thread::get_id()));
    file_logger->info("start async task");
    auto tp = ThreadPool();
    for (int i = 0; i < 100; i++) {
        tp.async([i, &file_logger](){
            sleep(1);
            file_logger->info("async finished thread id: {0}, task {1}",
                              getThreadIDStr(this_thread::get_id()), i);
        });
    }

    file_logger->info("start sync task");
    tp.sync([&file_logger](){
        sleep(1);
        file_logger->info("async thread id: {0}", getThreadIDStr(this_thread::get_id()));
        file_logger->info("sync task finished");
    });

    tp.waitAndExit();
    return 0;
}