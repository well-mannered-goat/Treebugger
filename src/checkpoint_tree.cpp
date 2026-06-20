#include <checkpoint_tree.hpp>

Checkpoint_Tree::Checkpoint_Tree(int root_pid) {
  next_checkpoint_id = 0;

  root = new Debuggee_Node(root_pid, next_checkpoint_id++);

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
  if (node != nullptr) {
    current = node;
  }
}

void Checkpoint_Tree::print_tree() {
  if (!root) {
    return;
  }

  cout << "#" << root->checkpoint_id << endl;

  for (size_t i = 0; i < root->children.size(); i++) {
    bool is_last = (i == root->children.size() - 1);
    print_tree(root->children[i], "", is_last);
  }
}

void Checkpoint_Tree::print_tree(Debuggee_Node *node, const string &prefix, bool is_last) {
  if (!node) {
    return;
  }

  cout << prefix;

  if (is_last) {
    cout << "└── ";
  } else {
    cout << "├── ";
  }

  cout << "#" << node->checkpoint_id;

  if (node == current) {
    cout << " [CURRENT]";
  }

  if (node->pinned) {
    cout << " [PINNED]";
  }

  cout << endl;

  string child_prefix = prefix;

  if (is_last) {
    child_prefix += "    ";
  } else {
    child_prefix += "│   ";
  }

  for (size_t i = 0; i < node->children.size(); i++) {
    bool child_is_last = (i == node->children.size() - 1);

    print_tree(node->children[i], child_prefix, child_is_last);
  }
}

static Debuggee_Node *traverse_find(Debuggee_Node *node, int checkpoint_id) {
  if (!node) {
    return nullptr;
  }

  if (node->checkpoint_id == checkpoint_id) {
    return node;
  }

  for (Debuggee_Node *child : node->children) {
    Debuggee_Node *ret = traverse_find(child, checkpoint_id);
    if (ret) {
      return ret;
    }
  }
  return nullptr;
}

Debuggee_Node *Checkpoint_Tree::find_checkpoint(int checkpoint_id) {
  return traverse_find(root, checkpoint_id);
}