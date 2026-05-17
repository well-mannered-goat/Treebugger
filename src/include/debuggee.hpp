#include <unistd.h>
#include <iostream>
using namespace std;

class Process{
    private:
        pid_t pid;
    public:
    Process();
    Process(int p);

    int get_pid();
    void sys_exec(const char* programname);
};

class Debuggee : public Process{
    using Process::Process;
};