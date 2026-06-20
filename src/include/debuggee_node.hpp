#include <debuggee.hpp>
#include <vector>

class Debuggee_Node{
    public:
    int pid;
    int checkpoint_id;
    bool pinned = false;
    Debuggee *dbgee;
    Debuggee_Node *parent;
    vector<Debuggee_Node *> children;
    Debuggee_Node(int pid,int chkpt_id);
    Debuggee_Node(int pid, int chkpt_id,  Debuggee_Node *node_parent);
    ~Debuggee_Node();
};