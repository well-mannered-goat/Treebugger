#include "debuggee_node.hpp"

class Checkpoint_Tree {
private:
  Debuggee_Node *root;
  Debuggee_Node *current;

  int next_checkpoint_id;

public:
  Checkpoint_Tree(int root_pid);
  ~Checkpoint_Tree();

  Debuggee_Node *get_root();
  Debuggee_Node *get_current();

  int get_next_checkpoint_id();

  Debuggee_Node *create_child(int pid);
  void switch_current(Debuggee_Node *node);

  Debuggee_Node *find_checkpoint(int checkpoint_id);

  void print_tree();
  void print_tree(Debuggee_Node *node, const string &prefix, bool is_last);
};