#include <debugger.hpp>
#include <iomanip>
#include <iostream>
#include <sys/user.h>
using namespace std;

void Debugger::handle_step(const vector<string> &args) {
  (void)args;
  procmsg("Stepping target forward by 1 instruction...");
  int err = this->trdbg_step_instruction();
  if (err == -1) {
    cerr << "[ERROR] Internal ptrace error during single-step." << endl;
  } else if (err == 0) {
    print_disassembly(5);
  } else if (err == 1) {
    procmsg("The target process has finished execution and exited.");
  } else if (err == 2) {
    cerr << "[WARNING] Process stopped unexpectedly or crashed during stepping."
         << endl;
  }
}

void Debugger::handle_continue(const vector<string> &args) {
  (void)args;
  procmsg("Resuming execution path...");
  int err = this->trdbg_continue();
  if (err == -1) {
    cerr << "[ERROR] Internal ptrace error during continue." << endl;
  } else if (err == 0) {
    print_disassembly(5);
  } else if (err == 1) {
    procmsg("The target process has finished execution and exited.");
  } else if (err == 2) {
    cerr
        << "[WARNING] Process stopped unexpectedly or crashed during execution."
        << endl;
  }
}

void Debugger::handle_read_registers(const std::vector<std::string> &args) {
  struct user_regs_struct regs;
  if (!trdbg_read_registers(regs)) {
    cerr << "[ERROR] Could not read target registers." << endl;
    return;
  }

  auto reg_map = get_register_map(regs);

  if (!args.empty()) {
    std::string target_reg = args[0];
    for (const auto &reg : reg_map) {
      if (reg.name == target_reg) {
        procmsg("%s: 0x%016llx", reg.name.c_str(), *reg.value_ptr);
        return;
      }
    }
    cerr << "[ERROR] Unknown register: " << target_reg << endl;
    return;
  }

  procmsg("--- Current Registers ---");

  int count = 0;
  for (const auto &reg : reg_map) {
    std::cout << "  " << std::left << std::setfill(' ') << std::setw(7)
              << reg.name << ": 0x" << std::right << std::setfill('0')
              << std::setw(16) << std::hex << *reg.value_ptr;

    if (++count % 3 == 0) {
      std::cout << "\n";
    } else {
      std::cout << "    ";
    }
  }

  if (count % 3 != 0) {
    std::cout << "\n";
  }

  std::cout << std::dec << std::setfill(' ') << std::left;
  std::cout << std::dec << std::endl;
}

void Debugger::handle_write_registers(const std::vector<std::string> &args) {
  if (args.size() < 2) {
    cerr << "[ERROR] Usage: register_write <register_name> <value>" << endl;
    cerr << "Example: register_write rax 0x123" << endl;
    return;
  }

  std::string target_reg = args[0];
  std::string value_str = args[1];

  // Read existing registers first so we don't clobber others
  struct user_regs_struct regs;
  if (!trdbg_read_registers(regs)) {
    cerr << "[ERROR] Could not read target registers before update." << endl;
    return;
  }

  auto reg_map = get_register_map(regs);
  bool found = false;

  for (auto &reg : reg_map) {
    if (reg.name == target_reg) {
      // Parse string value (handles both base 10 and base 16 prefixed with 0x)
      try {
        *reg.value_ptr = std::stoull(value_str, nullptr, 0);
        found = true;
      } catch (const std::exception &e) {
        cerr << "[ERROR] Invalid numeric value: " << value_str << endl;
        return;
      }
      break;
    }
  }

  if (!found) {
    cerr << "[ERROR] Unknown register: " << target_reg << endl;
    return;
  }

  // Write modified structural layout back to process
  if (trdbg_write_registers(regs)) {
    procmsg("Successfully updated %s to %s", target_reg.c_str(),
            value_str.c_str());
  } else {
    cerr << "[ERROR] Failed saving new register state via ptrace." << endl;
  }
}

void Debugger::handle_disasm(const std::vector<std::string> &args) {
  size_t count = 5;

  if (!args.empty()) {
    try {
      long val = std::stol(args[0]);
      if (val > 0) {
        count = static_cast<size_t>(val);
      } else {
        std::cerr << "[ERROR] Count must be greater than 0." << std::endl;
        return;
      }
    } catch (const std::exception &e) {
      std::cerr << "[ERROR] Invalid instruction count: " << args[0]
                << std::endl;
      return;
    }
  }

  print_disassembly(count);
}

void Debugger::handle_quit(const std::vector<std::string> &args) {
  (void)args;
  procmsg("Exiting debugger.");
  std::exit(0);
}

void Debugger::handle_break(const std::vector<std::string> &args) {
  uint64_t addr = 0;

  if (args.empty()) {
    struct user_regs_struct regs;
    if (!trdbg_read_registers(regs)) {
      std::cerr << "[ERROR] Could not read registers to fetch current RIP."
                << std::endl;
      return;
    }
    addr = regs.rip;
  } else {
    try {
      addr = std::stoull(args[0], nullptr, 0);
    } catch (const std::exception &e) {
      std::cerr << "[ERROR] Invalid address format: " << args[0] << std::endl;
      return;
    }
  }

  if (trdbg_add_breakpoint(addr)) {
    procmsg("Breakpoint set successfully at address 0x%016llx", addr);
  } else {
    std::cerr << "[WARNING] Breakpoint already exists or failed to set at 0x"
              << std::hex << addr << std::dec << std::endl;
  }
}

void Debugger::handle_delete_break(const std::vector<std::string> &args) {
  uint64_t addr = 0;

  if (args.empty()) {
    struct user_regs_struct regs;
    if (!trdbg_read_registers(regs)) {
      std::cerr << "[ERROR] Could not read registers to fetch current RIP."
                << std::endl;
      return;
    }
    addr = regs.rip;
  } else {
    try {
      addr = std::stoull(args[0], nullptr, 0);
    } catch (const std::exception &e) {
      std::cerr << "[ERROR] Invalid address format: " << args[0] << std::endl;
      return;
    }
  }

  if (trdbg_remove_breakpoint(addr)) {
    procmsg("Breakpoint removed successfully from address 0x%016llx", addr);
  } else {
    std::cerr << "[ERROR] No active breakpoint found at address 0x" << std::hex
              << addr << std::dec << std::endl;
  }
}

void Debugger::handle_add_checkpoint(const std::vector<std::string> &args){
  int pid = trdbg_fork_child();
  if(pid > 0){
      procmsg("Checkpoint added");
  }
  else{
    procmsg("Unable to add checkpoint");
  }
}