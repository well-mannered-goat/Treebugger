#include "debugger.hpp"
#include <sys/ptrace.h>
#include<wait.h>
#include <cstdio>
#include <cstdarg>

void procmsg(const char* format, ...) {
    va_list args;
    va_start(args, format);
    fprintf(stdout, "[TRDBG] ");
    vfprintf(stdout, format, args);
    fprintf(stdout, "\n");
    va_end(args);
}

Debugger::Debugger(pid_t target_pid){
    debuggee = new Debuggee(target_pid);
}

void Debugger::run_debugger() {
    int wait_status;
    unsigned icounter = 0;
    procmsg("Debugger started");

    wait(&wait_status);

    while (WIFSTOPPED(wait_status)) {
        icounter++;
        if (ptrace(PTRACE_SINGLESTEP, debuggee->get_pid(), nullptr, nullptr) < 0) {
            perror("ptrace error");
            return;
        }
        wait(&wait_status);
    }

    procmsg("the child executed %u instructions", icounter);
}