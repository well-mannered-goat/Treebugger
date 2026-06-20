#include "debuggee_node.hpp"

Debuggee_Node::Debuggee_Node(int pid, int chkpt_id){
        dbgee = new Debuggee(pid);
        checkpoint_id = chkpt_id;
}

Debuggee_Node::Debuggee_Node(int pid, int chkpt_id, Debuggee_Node *node_parent){
        dbgee = new Debuggee(pid);
        checkpoint_id = chkpt_id;
        parent = node_parent;
}

Debuggee_Node::~Debuggee_Node(){
    for(Debuggee_Node* child: children){
        delete child->dbgee;
        delete child;
    }
}