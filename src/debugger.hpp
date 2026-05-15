#pragma once
#include <sys/types.h>

void run_debugger(pid_t child_pid);

void procmsg(const char* format, ...);