#include "debugger.hpp"
#include <sys/ptrace.h>
#include<wait.h>
#include <cstdio>
#include <cstdarg>
#include <sstream>
#include <cstdint>
#include <cstring>
#include <sys/user.h>
#include <iomanip>
#include <capstone/capstone.h>
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
    command_map["disasm"] = &Debugger::handle_disasm;
    command_map["q"] = &Debugger::handle_quit;
    command_map["quit"] = &Debugger::handle_quit;
    command_map["exit"] = &Debugger::handle_quit;
    command_map["break"] = &Debugger::handle_break;
    command_map["b"] = &Debugger::handle_break;
    command_map["delete"] = &Debugger::handle_delete_break;
    command_map["d"] = &Debugger::handle_delete_break;
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

void Debugger::print_disassembly(size_t instruction_count) {
    struct user_regs_struct regs;
    if (ptrace(PTRACE_GETREGS, debuggee->get_pid(), nullptr, &regs) < 0) {
        perror("ptrace getregs failed");
        return;
    }
    uint64_t current_rip = regs.rip;

    const size_t buffer_size = 64;
    uint8_t code_buffer[buffer_size];
    
    for (size_t i = 0; i < buffer_size; i += sizeof(long)) {
        long word = ptrace(PTRACE_PEEKTEXT, debuggee->get_pid(), current_rip + i, nullptr);
        if (word == -1 && errno != 0) {
            perror("ptrace peektext failed");
            return;
        }
        std::memcpy(code_buffer + i, &word, sizeof(long));
    }

    csh handle;
    if (cs_open(CS_ARCH_X86, CS_MODE_64, &handle) != CS_ERR_OK) {
        std::cerr << "Capstone init failed" << std::endl;
        return;
    }

    cs_insn *insn;
    size_t count = cs_disasm(handle, code_buffer, buffer_size, current_rip, instruction_count, &insn);
    
    if (count > 0) {
        procmsg("--- Disassembly ---");
        for (size_t i = 0; i < count; i++) {
            std::string marker = (insn[i].address == current_rip) ? " => " : "    ";
            
            std::cout << marker 
                      << "0x" 
                      << std::right << std::setfill('0') << std::setw(16) << std::hex << insn[i].address 
                      << ":  "
                      << std::left << std::setfill(' ') << std::setw(10) << insn[i].mnemonic << "  "
                      << insn[i].op_str 
                      << std::endl;
        }
        cs_free(insn, count);
    } else {
        std::cerr << "Disassembly failed" << std::endl;
    }

    std::cout << std::dec << std::setfill(' ') << std::left;
    cs_close(&handle);
}

bool Debugger::trdbg_add_breakpoint(uint64_t addr) {
    if (breakpoints.find(addr) != breakpoints.end()) {
        return false;
    }

    errno = 0;
    long word = ptrace(PTRACE_PEEKTEXT, debuggee->get_pid(), addr, nullptr);
    if (word == -1 && errno != 0) {
        perror("ptrace peektext failed during breakpoint addition");
        return false;
    }

    uint8_t original_byte = static_cast<uint8_t>(word & 0xFF);
    breakpoints[addr] = original_byte;

    long modified_word = (word & ~0xFF) | 0xCC;
    if (ptrace(PTRACE_POKETEXT, debuggee->get_pid(), addr, modified_word) < 0) {
        perror("ptrace poketext failed during breakpoint addition");
        breakpoints.erase(addr);
        return false;
    }

    return true;
}

bool Debugger::trdbg_remove_breakpoint(uint64_t addr) {
    auto it = breakpoints.find(addr);
    if (it == breakpoints.end()) {
        return false;
    }

    errno = 0;
    long word = ptrace(PTRACE_PEEKTEXT, debuggee->get_pid(), addr, nullptr);
    if (word == -1 && errno != 0) {
        perror("ptrace peektext failed during breakpoint removal");
        return false;
    }

    long modified_word = (word & ~0xFF) | it->second;
    if (ptrace(PTRACE_POKETEXT, debuggee->get_pid(), addr, modified_word) < 0) {
        perror("ptrace poketext failed during breakpoint removal");
        return false;
    }

    breakpoints.erase(it);
    return true;
}