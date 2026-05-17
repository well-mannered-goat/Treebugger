#pragma once
#include <sys/types.h>
#include "debuggee.hpp"

void procmsg(const char* format, ...);

class Debugger{
    public:
    Debuggee *debuggee;
    void run_debugger();
    Debugger();
    Debugger(pid_t target_pid);
};