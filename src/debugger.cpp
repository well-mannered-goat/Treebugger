#include "debugger.hpp"
#include <sys/ptrace.h>
#include<wait.h>
#include <cstdio>
#include <cstdarg>
#include <sstream>
using namespace std;

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
    initialize_commands();
}

Debugger::~Debugger(){
    delete debuggee;
}

void Debugger::initialize_commands(){
    command_map["step"] = &Debugger::handle_step;
    command_map["continue"] = &Debugger::handle_continue;
}


void Debugger::run_debugger() {
    int wait_status;
    procmsg("Debugger started");

    // 1. Wait for the initial signal from the child process (the startup trap)
    waitpid(debuggee->get_pid(), &wait_status, 0);
    
    if (WIFSTOPPED(wait_status)) {
        procmsg("Child stopped at startup. Ready for commands.");
    }

    // 2. Loop indefinitely, handing control over to your input handler
    while (true) {
        handle_user_input();
    }
}

int Debugger::trdbg_step_instruction() {
    if (ptrace(PTRACE_SINGLESTEP, debuggee->get_pid(), nullptr, nullptr) < 0) {
        perror("ptrace singlestep failed");
        return -1; 
    }

    int wait_status;
    if (waitpid(debuggee->get_pid(), &wait_status, 0) < 0) {
        perror("waitpid failed");
        return -1;
    }

    if (WIFSTOPPED(wait_status)) {
        int stop_signal = WSTOPSIG(wait_status);
        if (stop_signal == SIGTRAP) {
            return 0;
        }
        return 2;
    }
    else if (WIFEXITED(wait_status)) {
        return 1;
    }

    return 2;
}

int Debugger::trdbg_continue() {
    // 1. Tell ptrace to resume running execution path freely
    if (ptrace(PTRACE_CONT, debuggee->get_pid(), nullptr, nullptr) < 0) {
        perror("ptrace continue failed");
        return -1;
    }

    int wait_status;
    if (waitpid(debuggee->get_pid(), &wait_status, 0) < 0) {
        perror("waitpid failed");
        return -1;
    }

    if (WIFSTOPPED(wait_status)) {
        int stop_signal = WSTOPSIG(wait_status);
        if (stop_signal == SIGTRAP) {
            return 0;
        }
        return 2;
    } 
    else if (WIFEXITED(wait_status)) {
        return 1;
    }

    return 2;
}