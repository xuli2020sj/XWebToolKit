//
// Created by x on 2022/7/18.
//

#include <Utility/timer.h>
#include "Utility/Logger.h"

int main() {
    Logger::getLogger().addChannel(std::make_shared<ConsoleChannel>());
    Timer timer;
    sleep(1);
    InfoL << "time us: " <<timer.getTimerMicroSec() << "us";
    InfoL << "time ms: " <<timer.getTimerMilliSec() << "ms";
    InfoL << "time s: " <<timer.getTimerSec() << "s";

}