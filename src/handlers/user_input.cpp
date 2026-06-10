#include <cstdarg>
#include <cstdio>
#include <debugger.hpp>
#include <sstream>
using namespace std;

void Debugger::handle_user_input() {
  cout << "TRDBG>";
  string input;
  if (!getline(cin, input))
    return;

  stringstream ss(input);
  string command_name;
  ss >> command_name;

  vector<string> args;
  string arg;

  while (ss >> arg) {
    args.push_back(arg);
  }

  if (command_name.empty())
    return;

  auto it = command_map.find(command_name);
  if (it != command_map.end()) {
    CommandHandler handler = it->second;
    (this->*handler)(args);
  } else {
    cerr << "Invalid command entered" << command_name << endl;
    cout << "Try \"h\" to know about valid commands" << endl;
  }
}