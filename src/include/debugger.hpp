#pragma once
#include <sys/types.h>
#include <vector>
#include <map>
#include <unordered_map>
#include <cstdint>
#include "debuggee_node.hpp"

void procmsg(const char* format, ...);

struct RegisterDetails {
    std::string name;
    unsigned long long* value_ptr;
};

class Debugger{
    private:
        bool trdbg_read_registers(struct user_regs_struct& regs);
        bool trdbg_write_registers(const struct user_regs_struct& regs);
        int trdbg_step_instruction();
        int trdbg_continue();
        void print_disassembly(size_t instruction_count);
        bool trdbg_add_breakpoint(uint64_t addr);
        bool trdbg_remove_breakpoint(uint64_t addr);
        int trdbg_fork_child();
        void get_syscall_libc_addr();
        bool trdbg_attach_new_child(int pid);
        std::vector<RegisterDetails> get_register_map(struct user_regs_struct& regs);
        std::unordered_map<uint64_t, uint8_t> breakpoints;
        uint64_t syscall_addr;

    public:
    Debugger();
    ~Debugger();
    Debugger(pid_t target_pid);

    Debuggee *debuggee;
    Debuggee_Node *root;
    Debuggee_Node *curr;

    using CommandHandler = void (Debugger::*)(const std::vector<std::string>& args);
    map<std::string, CommandHandler> command_map;
    void initialize_commands();
    void handle_user_input();

    // Command Handlers
    void handle_step(const vector<string> &args);
    void handle_continue(const vector<string> &args);
    void handle_read_registers(const std::vector<std::string>& args);
    void handle_write_registers(const std::vector<std::string>& args);
    void handle_disasm(const std::vector<std::string>& args);
    void handle_quit(const std::vector<std::string>& args);
    void run_debugger();
    void handle_break(const std::vector<std::string>& args);
    void handle_delete_break(const std::vector<std::string>& args);
    void handle_add_checkpoint(const std::vector<std::string> &args);


};