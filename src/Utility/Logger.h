//
// Created by x on 22-7-7.
//

#ifndef XLTOOLKIT_LOGGER_H
#define XLTOOLKIT_LOGGER_H

#include <string>
#include <memory>
#include <map>
#include <utility>
#include <iostream>
#include <chrono>
#include "ctime"
#include <sstream>
#include <memory>
#include <thread>
#include <deque>

class Logger;
class LogWriter;
class LogChannel;
class LogInfo;
class LogInfoMaker;

using LogInfoPtr = std::shared_ptr<LogInfo>;
using LogChannelPtr = std::shared_ptr<LogChannel>;
using LogWriterPtr = std::shared_ptr<LogWriter>;

class Noncopyable {
public:
    Noncopyable(const Noncopyable&) = delete;
    Noncopyable operator=(const Noncopyable&) = delete;
protected:
    Noncopyable() = default;
    ~Noncopyable() = default;
};

#define CLEAR_COLOR "\033[0m"
static const char *COLOR[6][2] = {{ "\033[44;37m", "\033[34m"},
                                  {"\033[42;37m", "\033[32m"},
                                  { "\033[46;37m", "\033[36m"},
                                  {"\033[43;37m", "\033[33m"},
                                  { "\033[45;37m", "\033[35m"},
                                  {"\033[41;37m", "\033[31m"}};

static const char *LogLevelStr[] = { "trace", "debug", "info", "warn", "error",
                                     "fatal" };

enum LogLevel {
    Trace = 0,
    Debug,
    Info,
    Warn,
    Error,
    Fatal,
};

class LogInfo {
public:
    friend class LogInfoMaker;
    void format(std::ostream& ost, bool enableColor = true) {
        using std::chrono::system_clock;
        //TODO
        static std::string appName;
//        ost << appName << " "<< file_ << " " << line_ << "\r\n ";
        if (enableColor) {
            ost << COLOR[level_][1];
        }
        tm = system_clock::to_time_t(system_clock::now());
        ost << ctime(&tm);
        ost << " [" << LogLevelStr[level_] << "] ";
        ost << function_ << " ";
        ost << message_.str();
        if (enableColor) {
            ost << CLEAR_COLOR;
        }
        ost.flush();
    }

    LogLevel getLevel() const {
        return level_;
    }
private:
    time_t tm{};
    LogLevel level_;
    size_t line_;
    std::string function_;
    std::string file_;
    std::ostringstream message_;
    LogInfo(LogLevel level, const char* file, const char* function,
            int line) : level_(level), file_(file), function_(function), line_(line) {}
};


class LogChannel {
protected:
    std::string name_;
    LogLevel level_;
public:
    LogChannel(std::string name, LogLevel level) : name_(std::move(name)), level_(level) {}
    virtual ~LogChannel() = default;
    virtual void write(const LogInfoPtr & stream)=0;
    const std::string &getName() const {
        return name_;
    }

    LogLevel getLevel() const {
        return level_;
    }

    void setLevel(LogLevel level) {
        level_ = level;
    }
};

class LogWriter {
public:
    LogWriter() = default;
    virtual ~LogWriter() = default;
    virtual void write(const LogInfoPtr &stream) =0;
};

class Logger : public Noncopyable {
private:
    LogWriterPtr writer_;
    std::map<std::string, LogChannelPtr> channels_;
public:
    [[nodiscard]] const std::map<std::string, LogChannelPtr> &getChannels() const {
        return channels_;
    }
public:
    static Logger& getLogger() {
        static auto* instance = new Logger();
        return *instance;
    }

    static void Destroy() {
        delete &Logger::getLogger();
    }

    void addChannel(const LogChannelPtr&& channel) {
        channels_[channel->getName()] = channel;
    }

    void deleteChannel(const std::string& name) {
        auto iter = channels_.find(name);
        if (iter != channels_.end()) {
            channels_.erase(iter);
        }
    }

    void setWriter(const LogWriterPtr &&writer) {
        if (writer) {
            writer_ = writer;
        }
    }

    void write(const LogInfoPtr &stream) {
        if (writer_) {
            writer_->write(stream);
            return;
        }
        for (auto &chn : channels_) {
            chn.second->write(stream);
        }
    }

    void setLoggerLevel(LogLevel logLevel) {
        for (const auto& chn : channels_) {
            chn.second->setLevel(logLevel);
        }
    }
};

class ConsoleChannel: public LogChannel {
public:
    ConsoleChannel(const std::string& name, LogLevel level) :
            LogChannel(name, level) {
    }
    ~ConsoleChannel() override = default;
    void write(const LogInfoPtr &logInfo) override {
        if (getLevel() > logInfo->getLevel()) {
            return;
        }
        logInfo->format(std::cout, true);
    }
};

class LogInfoMaker {
private:
    LogInfoPtr log_info;
public:
    LogInfoMaker(LogLevel level, const char* file, const char* function,
                 int line) :
            log_info(new LogInfo(level, file, function, line)) {
    }

    LogInfoMaker(LogInfoMaker &&other)  noexcept {
        this->log_info = other.log_info;
        other.log_info.reset();
    }

    LogInfoMaker(const LogInfoMaker &other) {
        this->log_info = other.log_info;
        (const_cast<LogInfoMaker &>(other)).log_info.reset();
    }
    ~LogInfoMaker() {
        *this << std::endl;
    }

    template<typename T>
    LogInfoMaker& operator <<(const T& data) {
        if (!log_info) {
            return *this;
        }
        log_info->message_ << data;
        return *this;
    }

    LogInfoMaker& operator <<(const char *data) {
        if (!log_info) {
            return *this;
        }
        if(data){
            log_info->message_ << data;
        }
        return *this;
    }

    LogInfoMaker& operator <<(std::ostream&(*f)(std::ostream&)) {
        if (!log_info) {
            return *this;
        }
        log_info->message_ << f;
        Logger::getLogger().write(log_info);
        log_info.reset();
        return *this;
    }
    void clear() {
        log_info.reset();
    }
};

class AsyncLogWriter: public LogWriter {
public:
    AsyncLogWriter() : exit_flag(false) {
        _thread.reset(new std::thread([this]() {this->run();}));
    }

    ~AsyncLogWriter() override {
        exit_flag = true;
        sem.release();
        _thread->join();
        while (!_pending.empty()) {
            auto &next = _pending.front();
            realWrite(next);
            _pending.pop_front();
        }
    }

    void write(const LogInfoPtr &stream) override {
        {
            std::lock_guard<std::mutex> lock(_mutex);
            _pending.push_back(stream);
        }
        sem.release();
    }
protected:
    void run() {
        while (!exit_flag) {
            sem.acquire();
            {
                std::lock_guard<std::mutex> lock(_mutex);
                if (!_pending.empty()) {
                    auto &next = _pending.front();
                    realWrite(next);
                    _pending.pop_front();
                }
            }
        }
    }
    static inline void realWrite(const LogInfoPtr &stream) {
        for (auto &chn : Logger::getLogger().getChannels()) {
            chn.second->write(stream);
        }
    }
    bool exit_flag;
    std::shared_ptr<std::thread> _thread;
    std::deque<LogInfoPtr> _pending;
    std::counting_semaphore<99999> sem = std::counting_semaphore<99999>(0);
    std::mutex _mutex;
};

#define TRACE LogInfoMaker(Trace, __FILE__,__FUNCTION__, __LINE__)
#define DEBUG LogInfoMaker(Debug, __FILE__,__FUNCTION__, __LINE__)
#define INFO LogInfoMaker(Info, __FILE__,__FUNCTION__, __LINE__)
#define WARN LogInfoMaker(Warn,__FILE__, __FUNCTION__, __LINE__)
#define ERROR LogInfoMaker(Error,__FILE__, __FUNCTION__, __LINE__)
#define FATAL LogInfoMaker(Fatal,__FILE__, __FUNCTION__, __LINE__)

#endif //XLTOOLKIT_LOGGER_H
