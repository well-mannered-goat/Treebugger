#include "debuggee_node.hpp"

Debuggee_Node::Debuggee_Node(int pid){
        dbgee = new Debuggee(pid);
}

Debuggee_Node::Debuggee_Node(int pid, Debuggee_Node *node_parent ,Debuggee_Node *node_child){
        dbgee = new Debuggee(pid);
        child = node_child;
        parent = node_parent;
}

Debuggee_Node::~Debuggee_Node(){
    delete parent;
    delete child;
}