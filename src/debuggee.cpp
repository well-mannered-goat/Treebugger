#include "debuggee.hpp"

Process::Process(){
    pid = 0;
}

Process::Process(int p){
    if(p < 0){
        cerr<<"Incorrect value of PID to set for the Process:"<<p<<endl;
        return;
    }
    pid = p;
}

int Process::get_pid(){
    return pid;
}

void Process::sys_exec(const char* programname){
    execl(programname, programname, nullptr);
}