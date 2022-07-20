//
// Created by x on 22-7-7.
//

#include <iomanip>
#include "Logger.h"

Logger *g_defaultLogger = nullptr;

Logger &getLoggerg() {
    if (!g_defaultLogger) {
        g_defaultLogger = &Logger::getLogger();
    }
    return *g_defaultLogger;
}

void setLogger(Logger *logger) {
    g_defaultLogger = logger;
}

static inline std::string exePath() {
    char buffer[PATH_MAX * 2 + 1] = {0};
    int n = -1;
    n = readlink("/proc/self/exe", buffer, sizeof(buffer));
    std::string filePath;
    if (n <= 0) {
        filePath = "./";
    } else {
        filePath = buffer;
    }
    return filePath;
}
static inline std::string exeDir() {
    auto path = exePath();
    return path.substr(0, path.rfind('/') + 1);
}
static inline std::string exeName() {
    auto path = exePath();
    return path.substr(path.rfind('/') + 1);
}
static inline const char *getFileName(const char *file) {
    auto pos = strrchr(file, '/');
    return pos ? pos + 1 : file;
}
static inline std::string getThreadName() {
    std::string ret;
    ret.resize(32);
    auto tid = pthread_self();
    pthread_getname_np(tid, (char *) ret.data(), ret.size());
    if (ret[0]) {
        ret.resize(strlen(ret.data()));
        return ret;
    }
    return std::to_string((uint64_t) tid);
}

static std::string s_module_name = exeName();


/////////////////// Logger Channel ///////////////////
std::string LogChannel::printTime() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    auto time_since_epoch = now.time_since_epoch();
    std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(time_since_epoch) % 1000;
    std::chrono::microseconds us = std::chrono::duration_cast<std::chrono::microseconds>(time_since_epoch) % 1000;
    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %X") << ":"<< ms.count() << ":"<< us.count();
    return ss.str();
}

const std::string &LogChannel::getName() const {
    return name_;
}

LogLevel LogChannel::getLevel() const {
    return level_;
}

void LogChannel::setLevel(LogLevel level) {
    level_ = level;
}

void LogChannel::format(const Logger &logger, std::ostream &ost, const LogContextPtr &ctx, bool enable_color,
                        bool enable_detail) {
    if (!enable_detail && ctx->str().empty()) {
        return;
    }
    if (enable_color) {
        ost << LOG_CONST_TABLE[ctx->getLevel()][1];
    }
    ost << printTime() << " " << LOG_CONST_TABLE[ctx->level_][2] << " ";
    if (enable_detail) {
        ost << (!ctx->flag_.empty()? ctx->flag_: logger.getName()) << "[" << getpid() << "-" << ctx->thread_name_ << "] "
        << ctx->file_name_ << ":" << ctx->line_ << " " << ctx->function_name_ << " | " << ctx->str();
    }
    if (enable_color) {
        ost << CLEAR_COLOR;
    }
    ost << std::endl;
}

void ConsoleChannel::write(const Logger &logger, const LogContextPtr &ctx) {
    if (getLevel() > ctx->getLevel()) {
        return;
    }
    format(logger, std::cout, ctx);
}

ConsoleChannel::ConsoleChannel(const std::string &name, LogLevel level) :
        LogChannel(name, level) {
}

FileChannel::FileChannel(const std::string &name, const std::string &path, LogLevel level) :
        LogChannel(name, level) , path_(name + exeName() + ".log"){
}

FileChannel::~FileChannel() {
    ofstream_.close();
}

void FileChannel::write(const Logger &logger, const LogContextPtr &ctx) {
    if (level_ > ctx->getLevel()) {
        return;
    }
    format(logger, ofstream_, ctx, false);

}

bool FileChannel::createFile() {
    ofstream_.close();
    ofstream_.open(path_.data(), std::ios::out | std::ios::app);
    if (!ofstream_.is_open()) {
        return false;
    }
    return true;
}

const std::string &FileChannel::getPath() const {
    return path_;
}

void FileChannel::setPath(const std::string &path) {
    path_ = path;
}

/////////////////// Logger Writer ///////////////////
void AsyncLogWriter::write(const LogContextPtr &log_context, Logger* logger) {
    {
        std::lock_guard<std::mutex> lock(mtx_);
        pending_queue_.emplace_back(std::make_pair(log_context, logger));
    }
    sem.release();
}

void AsyncLogWriter::flush() {
    decltype(pending_queue_) temp;
    {
        std::lock_guard<std::mutex> lk(mtx_);
        temp.swap(pending_queue_);
    }
    std::for_each(temp.begin(), temp.end(), [](std::pair<LogContextPtr, Logger*>& pair){
        pair.second->write(pair.first);
    });
}

void AsyncLogWriter::run() {
    while (!exit_flag) {
        sem.acquire();
        flush();
    }
}

AsyncLogWriter::AsyncLogWriter() : exit_flag(false) {
    thread_ = std::make_shared<std::thread>([this](){ this->run();});
}

AsyncLogWriter::~AsyncLogWriter() {
    exit_flag = true;
    sem.release();
    thread_->join();
    flush();
}

//////////////// LogContext /////////////////////
LogContext::LogContext(LogLevel level, const char *file, const char *function, int line, const char *module_name,
                       const char *flag)
        : level_(level), line_(line), file_name_(getFileName(file)), function_name_(function),
          module_name_(module_name), flag_(flag) {
    thread_name_ = getThreadName();
}

LogLevel LogContext::getLevel() const {
    return level_;
}

const std::string &LogContext::str() {
    if (got_content_) {
        return content_;
    }
    content_ = std::ostringstream::str();
    got_content_ = true;
    return content_;
}

LogContextCapture::LogContextCapture(Logger &logger, LogLevel level, const char *file, const char *function, int line,
                                     const char *flag) :
        log_context_(new LogContext(level, file, function, line, s_module_name.c_str(), flag)), logger_(logger) {
}

LogContextCapture::~LogContextCapture() {
    *this << std::endl;
}

LogContextCapture &LogContextCapture::operator<<(std::ostream &(*f)(std::ostream &)) {
    if (!log_context_) {
        return *this;
    }
    logger_.write(log_context_);
    log_context_.reset();
    return *this;
}



/////////////////// Logger ///////////////////
Logger::Logger(const std::string &loggerName) {
    logger_name_ = loggerName;
}

Logger::~Logger() {
    writer_.reset();
    {
        LogContextCapture(*this, LInfo, __FILE__, __FUNCTION__, __LINE__, "");
    }
}

Logger &Logger::getLogger() {
    static std::shared_ptr<Logger> s_instance(new Logger(exeName()));
    static Logger &s_instance_ref = *s_instance;
    return s_instance_ref;
}

void Logger::Destroy() {
    delete &Logger::getLogger();
}

void Logger::addChannel(const LogChannelPtr &&channel) {
    channels_[channel->getName()] = channel;
}

void Logger::setWriter(const LogWriterPtr &&writer) {
    if (writer) {
        writer_ = writer;
    }
}

void Logger::write(const LogContextPtr &ctx) {
    for (auto &chn : channels_) {
        chn.second->write(*this, ctx);
    }
}

void Logger::setLoggerLevel(LogLevel logLevel) {
    for (const auto& chn : channels_) {
        chn.second->setLevel(logLevel);
    }
}

const std::string &Logger::getName() const {
    return logger_name_;
}

void Logger::deleteChannel(const std::string &name) {
    auto iter = channels_.find(name);
    if (iter != channels_.end()) {
        channels_.erase(iter);
    }
}

const std::map<std::string, LogChannelPtr> &Logger::getChannels() const {
    return channels_;
}


