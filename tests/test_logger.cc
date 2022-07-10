#include "Utility/Logger.h"

using namespace std;


int main() {
//    Logger::getLogger().addChannel(std::make_shared<ConsoleChannel> ("stdout", Trace));
    Logger::getLogger().addChannel(std::make_shared<ConsoleChannel>("s", Trace));
    Logger::getLogger().setWriter(std::make_shared<AsyncLogWriter>());

    TRACE << "int"<< (int)1  ;
    DEBUG << "short:"<< (short)2  << endl;
    INFO << "float:" << (float)3.1415926  << endl;
    WARN << "double:" << (double)4.12345678901234567  << endl;
    ERROR << "void *:" << (void *)0x12345678 << endl;
    FATAL << "without endl!";
    sleep(3);
//    Logger::Destroy();
    return 0;
}