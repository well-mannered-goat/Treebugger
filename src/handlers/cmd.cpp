#include <debugger.hpp>
#include <iostream>
using namespace std;

void Debugger::handle_step(const vector<string>& args) {
    (void)args;
    
    procmsg("Stepping target forward by 1 instruction...");
    
    int err = this->trdbg_step_instruction();
    if (err == -1) {
        cerr << "[ERROR] Internal ptrace error during single-step." << endl;
    }
    else if (err == 0) {
        procmsg("Step successful.");
    }
    else if (err == 1) {
        procmsg("The target process has finished execution and exited.");
    }
    else if (err == 2) {
        cerr << "[WARNING] Process stopped unexpectedly or crashed during stepping." << endl;
    }
}

void Debugger::handle_continue(const vector<string>& args) {
    (void)args;
    
    procmsg("Resuming execution path...");
    
    int err = this->trdbg_continue();
    if (err == -1) {
        cerr << "[ERROR] Internal ptrace error during continue." << endl;
    }
    else if (err == 0) {
        procmsg("Target hit a breakpoint or hardware event.");
    }
    else if (err == 1) {
        procmsg("The target process has finished execution and exited.");
    }
    else if (err == 2) {
        cerr << "[WARNING] Process stopped unexpectedly or crashed during execution." << endl;
    }
}