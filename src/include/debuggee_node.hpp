#include <debuggee.hpp>

class Debuggee_Node{

    public:
    int pid;
    Debuggee *dbgee;
    Debuggee_Node *parent;
    Debuggee_Node *child;
    Debuggee_Node(int pid);
    Debuggee_Node(int pid, Debuggee_Node *node_parent ,Debuggee_Node *node_child);
    ~Debuggee_Node();
};