#include <checkpoint_tree.hpp>

Checkpoint_Tree::Checkpoint_Tree(int root_pid) {
    next_checkpoint_id = 0;

    root = new Debuggee_Node(
        root_pid,
        next_checkpoint_id++
    );

    current = root;
}

Debuggee_Node *Checkpoint_Tree::get_root() {
    return root;
}

Checkpoint_Tree::~Checkpoint_Tree() {
  delete root;
}

Debuggee_Node *Checkpoint_Tree::get_current() {
    return current;
}

int Checkpoint_Tree::get_next_checkpoint_id() {
    return next_checkpoint_id++;
}

Debuggee_Node *Checkpoint_Tree::create_child(int pid) {

    Debuggee_Node *parent = current;

    Debuggee_Node *child = new Debuggee_Node(pid, next_checkpoint_id++, parent);

    parent->children.push_back(child);

    current = child;

    return child;
}

void Checkpoint_Tree::switch_current(Debuggee_Node *node) {
    if(node != nullptr) {
        current = node;
    }
}