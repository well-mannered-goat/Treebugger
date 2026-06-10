#pragma once
#include <sys/types.h>
#include <vector>
#include <map>
#include "debuggee.hpp"

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
        std::vector<RegisterDetails> get_register_map(struct user_regs_struct& regs);

    public:
    Debugger();
    ~Debugger();
    Debugger(pid_t target_pid);

    Debuggee *debuggee;

    using CommandHandler = void (Debugger::*)(const std::vector<std::string>& args);
    map<std::string, CommandHandler> command_map;
    void initialize_commands();
    void handle_user_input();

    // Command Handlers
    void handle_step(const vector<string> &args);
    void handle_continue(const vector<string> &args);
    void handle_read_registers(const std::vector<std::string>& args);
    void handle_write_registers(const std::vector<std::string>& args);
    void run_debugger();



};