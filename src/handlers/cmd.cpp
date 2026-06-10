#include <debugger.hpp>
#include <iostream>
#include <sys/user.h>
#include <iomanip>
using namespace std;

void Debugger::handle_step(const vector<string>& args) {
    (void)args;
    
    procmsg("Stepping target forward by 1 instruction...");
    
    int err = this->trdbg_step_instruction();
    if (err == -1) {
        cerr << "[ERROR] Internal ptrace error during single-step." << endl;
    }
    else if (err == 0) {
        procmsg("Step successful.");
    }
    else if (err == 1) {
        procmsg("The target process has finished execution and exited.");
    }
    else if (err == 2) {
        cerr << "[WARNING] Process stopped unexpectedly or crashed during stepping." << endl;
    }
}

void Debugger::handle_continue(const vector<string>& args) {
    (void)args;
    
    procmsg("Resuming execution path...");
    
    int err = this->trdbg_continue();
    if (err == -1) {
        cerr << "[ERROR] Internal ptrace error during continue." << endl;
    }
    else if (err == 0) {
        procmsg("Target hit a breakpoint or hardware event.");
    }
    else if (err == 1) {
        procmsg("The target process has finished execution and exited.");
    }
    else if (err == 2) {
        cerr << "[WARNING] Process stopped unexpectedly or crashed during execution." << endl;
    }
}

void Debugger::handle_read_registers(const std::vector<std::string>& args) {
    struct user_regs_struct regs;
    if (!trdbg_read_registers(regs)) {
        cerr << "[ERROR] Could not read target registers." << endl;
        return;
    }

    auto reg_map = get_register_map(regs);

    if (!args.empty()) {
        std::string target_reg = args[0];
        for (const auto& reg : reg_map) {
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
    for (const auto& reg : reg_map) {
        std::cout << "  " << std::left << std::setfill(' ') << std::setw(7) << reg.name << ": 0x" 
                  << std::right << std::setfill('0') << std::setw(16) << std::hex << *reg.value_ptr;
        
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

void Debugger::handle_write_registers(const std::vector<std::string>& args) {
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

    for (auto& reg : reg_map) {
        if (reg.name == target_reg) {
            // Parse string value (handles both base 10 and base 16 prefixed with 0x)
            try {
                *reg.value_ptr = std::stoull(value_str, nullptr, 0);
                found = true;
            } catch (const std::exception& e) {
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
        procmsg("Successfully updated %s to %s", target_reg.c_str(), value_str.c_str());
    } else {
        cerr << "[ERROR] Failed saving new register state via ptrace." << endl;
    }
}