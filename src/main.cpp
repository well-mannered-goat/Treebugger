#include <sys/ptrace.h>
#include "debugger.hpp"

using namespace std;

void run_target(const char* programname) {
    procmsg("target started. will run '%s'", programname);

    if (ptrace(PTRACE_TRACEME, 0, nullptr, nullptr) < 0) {
        perror("ptrace");
        return;
    }

    execl(programname, programname, nullptr);
}

int main(int argc, char** argv) {
    if (argc < 2) {
        cerr << "Error: Expected a program name" << endl;
        return -1;
    }

    pid_t child_pid = fork();
    if (child_pid == 0) {
        run_target(argv[1]);
    }
    else if (child_pid > 0) {
        Debugger *dbgr = new Debugger(child_pid);
        dbgr->run_debugger();
    }
    else {
        perror("fork");
        return -1;
    }

    return 0;
}