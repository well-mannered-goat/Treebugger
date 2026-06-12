#include "debugger.hpp"
#include <capstone/capstone.h>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <format>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <wait.h>
using namespace std;

void procmsg(const char *format, ...) {
  va_list args;
  va_start(args, format);
  fprintf(stdout, "[TRDBG] ");
  vfprintf(stdout, format, args);
  fprintf(stdout, "\n");
  va_end(args);
}

Debugger::Debugger(pid_t target_pid) {
  root = new Debuggee_Node(target_pid);
  curr = root;
  debuggee = root->dbgee;
  initialize_commands();
}

Debugger::~Debugger() { 
  delete root;
}

void Debugger::initialize_commands() {
  command_map["step"] = &Debugger::handle_step;
  command_map["continue"] = &Debugger::handle_continue;
  command_map["register_read"] = &Debugger::handle_read_registers;
  command_map["register_write"] = &Debugger::handle_write_registers;
  command_map["disasm"] = &Debugger::handle_disasm;
  command_map["q"] = &Debugger::handle_quit;
  command_map["quit"] = &Debugger::handle_quit;
  command_map["exit"] = &Debugger::handle_quit;
  command_map["break"] = &Debugger::handle_break;
  command_map["b"] = &Debugger::handle_break;
  command_map["delete"] = &Debugger::handle_delete_break;
  command_map["d"] = &Debugger::handle_delete_break;
  command_map["checkpoint"] = &Debugger::handle_add_checkpoint;
}

void Debugger::run_debugger() {
  int wait_status;
  procmsg("Debugger started");
  procmsg("PID is: %d", root->dbgee->get_pid());

  waitpid(root->dbgee->get_pid(), &wait_status, 0);

  if (WIFSTOPPED(wait_status)) {
    get_syscall_libc_addr();
    procmsg("Child stopped at startup. Ready for commands.");
  }

  while (true) {
    handle_user_input();
  }
}

int Debugger::trdbg_step_instruction() {
  if (ptrace(PTRACE_SINGLESTEP, root->dbgee->get_pid(), nullptr, nullptr) < 0) {
    perror("ptrace singlestep failed");
    return -1;
  }

  int wait_status;
  if (waitpid(debuggee->get_pid(), &wait_status, 0) < 0) {
    perror("waitpid failed");
    return -1;
  }

  if (WIFSTOPPED(wait_status)) {
    int stop_signal = WSTOPSIG(wait_status);
    if (stop_signal == SIGTRAP) {
      return 0;
    }
    return 2;
  } else if (WIFEXITED(wait_status)) {
    return 1;
  }

  return 2;
}

int Debugger::trdbg_continue() {
  if (ptrace(PTRACE_CONT, debuggee->get_pid(), nullptr, nullptr) < 0) {
    perror("ptrace continue failed");
    return -1;
  }

  int wait_status;
  if (waitpid(debuggee->get_pid(), &wait_status, 0) < 0) {
    perror("waitpid failed");
    return -1;
  }

  if (WIFSTOPPED(wait_status)) {
    int stop_signal = WSTOPSIG(wait_status);
    if (stop_signal == SIGTRAP) {
      return 0;
    }
    return 2;
  } else if (WIFEXITED(wait_status)) {
    return 1;
  }

  return 2;
}

bool Debugger::trdbg_read_registers(struct user_regs_struct &regs) {
  if (ptrace(PTRACE_GETREGS, debuggee->get_pid(), nullptr, &regs) < 0) {
    perror("ptrace GETREGS failed");
    return false;
  }
  return true;
}

bool Debugger::trdbg_write_registers(const struct user_regs_struct &regs) {
  if (ptrace(PTRACE_SETREGS, debuggee->get_pid(), nullptr, &regs) < 0) {
    perror("ptrace SETREGS failed");
    return false;
  }
  return true;
}

// Helper function to map string names dynamically to the struct fields
std::vector<RegisterDetails>
Debugger::get_register_map(struct user_regs_struct &regs) {
  return {{"rax", &regs.rax}, {"rbx", &regs.rbx}, {"rcx", &regs.rcx},
          {"rdx", &regs.rdx}, {"rsi", &regs.rsi}, {"rdi", &regs.rdi},
          {"rbp", &regs.rbp}, {"rsp", &regs.rsp}, {"r8", &regs.r8},
          {"r9", &regs.r9},   {"r10", &regs.r10}, {"r11", &regs.r11},
          {"r12", &regs.r12}, {"r13", &regs.r13}, {"r14", &regs.r14},
          {"r15", &regs.r15}, {"rip", &regs.rip}, {"eflags", &regs.eflags}};
}

void Debugger::print_disassembly(size_t instruction_count) {
  struct user_regs_struct regs;
  if (ptrace(PTRACE_GETREGS, debuggee->get_pid(), nullptr, &regs) < 0) {
    perror("ptrace getregs failed");
    return;
  }
  uint64_t current_rip = regs.rip;

  const size_t buffer_size = 64;
  uint8_t code_buffer[buffer_size];

  for (size_t i = 0; i < buffer_size; i += sizeof(long)) {
    long word =
        ptrace(PTRACE_PEEKTEXT, debuggee->get_pid(), current_rip + i, nullptr);
    if (word == -1 && errno != 0) {
      perror("ptrace peektext failed");
      return;
    }
    std::memcpy(code_buffer + i, &word, sizeof(long));
  }

  csh handle;
  if (cs_open(CS_ARCH_X86, CS_MODE_64, &handle) != CS_ERR_OK) {
    std::cerr << "Capstone init failed" << std::endl;
    return;
  }

  cs_insn *insn;
  size_t count = cs_disasm(handle, code_buffer, buffer_size, current_rip,
                           instruction_count, &insn);

  if (count > 0) {
    procmsg("--- Disassembly ---");
    for (size_t i = 0; i < count; i++) {
      std::string marker = (insn[i].address == current_rip) ? " => " : "    ";

      std::cout << marker << "0x" << std::right << std::setfill('0')
                << std::setw(16) << std::hex << insn[i].address << ":  "
                << std::left << std::setfill(' ') << std::setw(10)
                << insn[i].mnemonic << "  " << insn[i].op_str << std::endl;
    }
    cs_free(insn, count);
  } else {
    std::cerr << "Disassembly failed" << std::endl;
  }

  std::cout << std::dec << std::setfill(' ') << std::left;
  cs_close(&handle);
}

bool Debugger::trdbg_add_breakpoint(uint64_t addr) {
  if (breakpoints.find(addr) != breakpoints.end()) {
    return false;
  }

  errno = 0;
  long word = ptrace(PTRACE_PEEKTEXT, debuggee->get_pid(), addr, nullptr);
  if (word == -1 && errno != 0) {
    perror("ptrace peektext failed during breakpoint addition");
    return false;
  }

  uint8_t original_byte = static_cast<uint8_t>(word & 0xFF);
  breakpoints[addr] = original_byte;

  long modified_word = (word & ~0xFF) | 0xCC;
  if (ptrace(PTRACE_POKETEXT, debuggee->get_pid(), addr, modified_word) < 0) {
    perror("ptrace poketext failed during breakpoint addition");
    breakpoints.erase(addr);
    return false;
  }

  return true;
}

bool Debugger::trdbg_remove_breakpoint(uint64_t addr) {
  auto it = breakpoints.find(addr);
  if (it == breakpoints.end()) {
    return false;
  }

  errno = 0;
  long word = ptrace(PTRACE_PEEKTEXT, debuggee->get_pid(), addr, nullptr);
  if (word == -1 && errno != 0) {
    perror("ptrace peektext failed during breakpoint removal");
    return false;
  }

  long modified_word = (word & ~0xFF) | it->second;
  if (ptrace(PTRACE_POKETEXT, debuggee->get_pid(), addr, modified_word) < 0) {
    perror("ptrace poketext failed during breakpoint removal");
    return false;
  }

  breakpoints.erase(it);
  return true;
}

static uint64_t get_syscall_addr(uint64_t start_addr, uint64_t end_addr,
                                 int pid) {
  for (uint64_t addr = start_addr; addr < end_addr; addr = addr + 8) {
    uint64_t word = ptrace(PTRACE_PEEKTEXT, pid, addr, nullptr);
    for (int i = 0; i < 7; i++) {
      uint16_t op = (word >> (i * 8)) & 0xffff;
      if (op == 0x050f) {
        return addr + i;
      }
    }
  }
  return 0;
}

void Debugger::get_syscall_libc_addr() {

  string maps_path = "/proc/" + to_string(debuggee->get_pid()) + "/maps";
  ifstream f(maps_path);
  if (!f) {
    cerr << "Error opening maps file\n";
    return;
  }

  string line;
  while (getline(f, line)) {
    stringstream ss(line);

    string addr_range;
    string perms;
    string offset;
    string device;
    uint64_t inode;
    string object;

    ss >> addr_range >> perms >> offset >> device >> inode >> object;

    if (object != "[vdso]") {
      continue;
    }

    size_t dash = addr_range.find('-');

    uint64_t start = stoull(addr_range.substr(0, dash), nullptr, 16);
    uint64_t end = stoull(addr_range.substr(dash + 1), nullptr, 16);

    uint64_t size = end - start;

    uint64_t syscall_addr = get_syscall_addr(start, end, debuggee->get_pid());
    if (syscall_addr == 0) {
      cout << "Cannot find address of syscall" << endl;
    }
    this->syscall_addr = syscall_addr;
    break;
  }
}

int Debugger::trdbg_fork_child() {
  int pid = debuggee->get_pid();

  struct user_regs_struct saved_regs;
  struct user_regs_struct regs;

  trdbg_read_registers(saved_regs);
  regs = saved_regs;

  regs.rax = 57; // SYS_fork
  regs.rip = syscall_addr;

  trdbg_write_registers(regs);
  cout << hex << regs.rip << endl;
  if (ptrace(PTRACE_SINGLESTEP, pid, nullptr, nullptr) < 0) {
    perror("PTRACE_SINGLESTEP");
    trdbg_write_registers(saved_regs);
    return -1;
  }

  int status;
  waitpid(pid, &status, 0);

  trdbg_read_registers(regs);
  cout << hex << regs.rip << endl;
  long ret = regs.rax;

  cout << "[TRDBG] fork syscall returned: " << ret << endl;

  if (ret > 0) {
    cout << "[TRDBG] fork succeeded, child pid = " << dec << ret << endl;
    int wait_status;

    kill(ret, SIGSTOP);
    if (WIFSTOPPED(wait_status)) {
      procmsg("Child stopped at startup. Ready for commands.");
    }
  } else if (ret == 0) {
    cout << "[TRDBG] currently executing in child" << endl;
  } else {
    cout << "[TRDBG] fork failed, errno = " << dec << -ret << endl;
  }

  trdbg_write_registers(saved_regs);

  return ret;
}

bool Debugger::trdbg_attach_new_child(int pid){
  int ret = ptrace(PTRACE_ATTACH, pid, nullptr, nullptr);
  if(ret == -1){
    cout<<"Error number is: "<<errno<<endl;
    cout<<"Error attaching to process: "<<pid<<endl;
    return false;
  }

  Debuggee_Node *child = new Debuggee_Node(pid, curr, nullptr);
  curr = child;
  debuggee = curr->dbgee;
  cout<<"New child created, debuggee is process: "<<debuggee->get_pid()<<endl;
  return true;
}