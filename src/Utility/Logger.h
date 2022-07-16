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
#include <ctime>
#include <sstream>
#include <memory>
#include <thread>
#include <deque>
#include <cstring>
#include <list>
#include <fstream>
#include "chrono"

class Noncopyable {
public:
    Noncopyable(const Noncopyable&) = delete;
    Noncopyable(const Noncopyable&&) = delete;
    Noncopyable operator=(const Noncopyable&) = delete;
    Noncopyable operator=(const Noncopyable&&) = delete;
protected:
    Noncopyable() = default;
    ~Noncopyable() = default;
};

#define CLEAR_COLOR "\033[0m"
static const char *LOG_CONST_TABLE[][3] = {
        {"\033[44;37m", "\033[34m", "T"},
        {"\033[42;37m", "\033[32m", "D"},
        {"\033[46;37m", "\033[36m", "I"},
        {"\033[43;37m", "\033[33m", "W"},
        {"\033[41;37m", "\033[31m", "E"}};

typedef enum {
    LTrace = 0, LDebug, LInfo, LWarn, LError
} LogLevel;

class Logger;
class LogWriter;
class LogContext;
class LogContextCapture;
class LogChannel;

using LogContextPtr = std::shared_ptr<LogContext>;
using LogChannelPtr = std::shared_ptr<LogChannel>;
using LogWriterPtr = std::shared_ptr<LogWriter>;
using LoggerPtr = std::shared_ptr<Logger>;

//////////////// Logger /////////////////////
class Logger : public Noncopyable {
private:
    std::string logger_name_;
    LogWriterPtr writer_;
    std::map<std::string, LogChannelPtr> channels_;
public:
    explicit Logger(const std::string &loggerName);
    static void Destroy();
    ~Logger();
    static Logger& getLogger();
    const std::string &getName() const;
    const std::map<std::string, LogChannelPtr> &getChannels() const;
    void addChannel(const LogChannelPtr&& channel);
    void deleteChannel(const std::string& name);
    void setWriter(const LogWriterPtr &&writer);
    void write(const LogContextPtr &ctx);
    void setLoggerLevel(LogLevel logLevel);
};

//////////////// LogContext /////////////////////
class LogContext : public std::ostringstream {
    friend class LogChannel;
private:
    LogLevel level_;
    std::string file_name_;
    std::string function_name_;
    std::string thread_name_;
    std::string module_name_;
    std::string content_;
    std::string flag_;
    int line_;
    bool got_content_ = false;
public:
    LogContext(LogLevel level, const char *file, const char *function, int line, const char *module_name, const char *flag);
    const std::string& str();
    LogLevel getLevel() const;
};

class LogContextCapture {
private:
    LogContextPtr log_context_;
    Logger& logger_;
public:
    LogContextCapture(Logger &logger, LogLevel level, const char *file, const char *function, int line, const char *flag = "");
    ~LogContextCapture();
    LogContextCapture &operator<<(std::ostream &(*f)(std::ostream &));
    template<typename T>
    LogContextCapture &operator<<(T &&data) {
        if (!log_context_) {
            return *this;
        }
        (*log_context_) << std::forward<T>(data);
        return *this;
    }

};

//////////////// LogChannel /////////////////////
class LogChannel : public Noncopyable {
protected:
    std::string name_;
    LogLevel level_;
    static std::string printTime();
public:
    LogChannel(std::string name, LogLevel level) : name_(std::move(name)), level_(level) {}
    virtual ~LogChannel() = default;
    virtual void write(const Logger &logger, const LogContextPtr &ctx) = 0;
    const std::string &getName() const;
    LogLevel getLevel() const;
    void setLevel(LogLevel level);
    void format(const Logger &logger, std::ostream &ost, const LogContextPtr &ctx,
                        bool enable_color = true, bool enable_detail = true);
};

class ConsoleChannel: public LogChannel {
public:
    explicit ConsoleChannel(const std::string& name = "ConsoleChannel", LogLevel level = LTrace);
    ~ConsoleChannel() override = default;
    void write(const Logger &logger, const LogContextPtr &ctx) override;
};

class FileChannel : public LogChannel {
private:
    std::string path_;
public:
    const std::string &getPath() const;

    void setPath(const std::string &path);

private:
    std::ofstream ofstream_;
public:
    FileChannel(const std::string &path, const std::string &name = "FileChannel", LogLevel level = LTrace);
    ~FileChannel() override;

    bool createFile();
    void write(const Logger &logger, const LogContextPtr &ctx) override;
};



//////////////// LogWriter /////////////////////
class LogWriter {
public:
    LogWriter() = default;
    virtual ~LogWriter() = default;
    virtual void write(const LogContextPtr &log_context, Logger* logger) = 0;
};

class AsyncLogWriter: public LogWriter {
public:
    AsyncLogWriter();
    ~AsyncLogWriter() override;
    void write(const LogContextPtr &log_context, Logger* logger) override;
    void flush();

private:
    void run();
    bool exit_flag;
    std::shared_ptr<std::thread> thread_;
    std::list<std::pair<LogContextPtr, Logger*>> pending_queue_;
    std::counting_semaphore<1> sem = std::counting_semaphore<1>(0);
    std::mutex mtx_;
};


//用法: DebugL << 1 << "+" << 2 << '=' << 3;
#define WriteL(level) LogContextCapture(Logger::getLogger(), level, __FILE__, __FUNCTION__, __LINE__)
#define TraceL WriteL(LTrace)
#define DebugL WriteL(LDebug)
#define InfoL WriteL(LInfo)
#define WarnL WriteL(LWarn)
#define ErrorL WriteL(LError)


#endif //XLTOOLKIT_LOGGER_H
