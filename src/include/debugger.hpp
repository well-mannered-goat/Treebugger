#pragma once
#include <sys/types.h>
#include <vector>
#include <map>
#include "debuggee.hpp"

void procmsg(const char* format, ...);

class Debugger{
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
    void run_debugger();

    //Debugger functions
    int trdbg_step_instruction();
    int trdbg_continue();

};