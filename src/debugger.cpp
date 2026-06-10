#include "debugger.hpp"
#include <sys/ptrace.h>
#include<wait.h>
#include <cstdio>
#include <cstdarg>
#include <sstream>
#include <sys/user.h>
#include <iomanip>
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
    command_map["register_read"] = &Debugger::handle_read_registers;
    command_map["register_write"] = &Debugger::handle_write_registers;  
}


void Debugger::run_debugger() {
    int wait_status;
    procmsg("Debugger started");

    waitpid(debuggee->get_pid(), &wait_status, 0);
    
    if (WIFSTOPPED(wait_status)) {
        procmsg("Child stopped at startup. Ready for commands.");
    }

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

bool Debugger::trdbg_read_registers(struct user_regs_struct& regs) {
    if (ptrace(PTRACE_GETREGS, debuggee->get_pid(), nullptr, &regs) < 0) {
        perror("ptrace GETREGS failed");
        return false;
    }
    return true;
}

bool Debugger::trdbg_write_registers(const struct user_regs_struct& regs) {
    if (ptrace(PTRACE_SETREGS, debuggee->get_pid(), nullptr, &regs) < 0) {
        perror("ptrace SETREGS failed");
        return false;
    }
    return true;
}

// Helper function to map string names dynamically to the struct fields
std::vector<RegisterDetails> Debugger::get_register_map(struct user_regs_struct& regs) {
    return {
        {"rax", &regs.rax}, {"rbx", &regs.rbx}, {"rcx", &regs.rcx}, {"rdx", &regs.rdx},
        {"rsi", &regs.rsi}, {"rdi", &regs.rdi}, {"rbp", &regs.rbp}, {"rsp", &regs.rsp},
        {"r8",  &regs.r8},  {"r9",  &regs.r9},  {"r10", &regs.r10}, {"r11", &regs.r11},
        {"r12", &regs.r12}, {"r13", &regs.r13}, {"r14", &regs.r14}, {"r15", &regs.r15},
        {"rip", &regs.rip}, {"eflags", &regs.eflags}
    };
}