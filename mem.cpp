#include "header.h"

MemoryInfo getMemoryInfo() {
    MemoryInfo info = {};
    
    // Get RAM information from /proc/meminfo
    std::ifstream meminfo("/proc/meminfo");
    std::string line;
    std::map<std::string, unsigned long> mem_values;
    
    while (std::getline(meminfo, line)) {
        std::istringstream iss(line);
        std::string key;
        unsigned long value;
        std::string unit;
        
        if (iss >> key >> value >> unit) {
            // Remove the colon from key
            if (!key.empty() && key.back() == ':') {
                key.pop_back();
            }
            mem_values[key] = value; // Values are in kB
        }
    }
    
    info.total_ram = mem_values["MemTotal"];
    info.free_ram = mem_values["MemFree"] + mem_values["Buffers"] + mem_values["Cached"];
    info.used_ram = info.total_ram - info.free_ram;
    
    info.total_swap = mem_values["SwapTotal"];
    info.free_swap = mem_values["SwapFree"];
    info.used_swap = info.total_swap - info.free_swap;
    
    // Get disk information
    struct statvfs disk_stat;
    if (statvfs("/", &disk_stat) == 0) {
        info.total_disk = (disk_stat.f_blocks * disk_stat.f_frsize) / 1024; // Convert to kB
        info.free_disk = (disk_stat.f_bavail * disk_stat.f_frsize) / 1024;
        info.used_disk = info.total_disk - info.free_disk;
    }
    
    return info;
}

std::vector<ProcessInfo> getProcesses() {
    std::vector<ProcessInfo> processes;
    
    DIR* proc_dir = opendir("/proc");
    if (!proc_dir) return processes;
    
    // Get total system memory for percentage calculations
    MemoryInfo mem_info = getMemoryInfo();
    
    struct dirent* entry;
    while ((entry = readdir(proc_dir)) != nullptr) {
        // Check if directory name is a number (PID)
        bool is_pid = true;
        for (char* p = entry->d_name; *p; p++) {
            if (*p < '0' || *p > '9') {
                is_pid = false;
                break;
            }
        }
        
        if (!is_pid || strlen(entry->d_name) == 0) continue;
        
        ProcessInfo proc;
        proc.pid = std::stoi(entry->d_name);
        
        // Read process name and state from /proc/PID/stat
        std::string stat_path = "/proc/" + std::string(entry->d_name) + "/stat";
        std::ifstream stat_file(stat_path);
        if (!stat_file.is_open()) continue;
        
        std::string stat_line;
        if (!std::getline(stat_file, stat_line)) continue;
        
        std::istringstream iss(stat_line);
        std::string token;
        std::vector<std::string> stat_fields;
        
        // Parse stat line
        while (iss >> token) {
            stat_fields.push_back(token);
        }
        
        if (stat_fields.size() < 24) continue;
        
        // Extract process name (remove parentheses)
        proc.name = stat_fields[1];
        if (proc.name.size() >= 2 && proc.name[0] == '(' && proc.name.back() == ')') {
            proc.name = proc.name.substr(1, proc.name.size() - 2);
        }
        
        // Extract state
        proc.state = stat_fields[2];
        
        // Calculate CPU usage (simplified)
        unsigned long utime = std::stoul(stat_fields[13]);
        unsigned long stime = std::stoul(stat_fields[14]);
        unsigned long total_time = utime + stime;
        proc.cpu_usage = (total_time / 100.0f) * 0.01f; // Simplified calculation
        
        // Get memory usage from /proc/PID/status
        proc.memory_kb = 0;
        proc.memory_usage = 0.0f;
        
        std::string status_path = "/proc/" + std::string(entry->d_name) + "/status";
        std::ifstream status_file(status_path);
        if (status_file.is_open()) {
            std::string status_line;
            while (std::getline(status_file, status_line)) {
                if (status_line.find("VmRSS:") == 0) {
                    std::istringstream status_iss(status_line);
                    std::string key;
                    unsigned long value;
                    std::string unit;
                    if (status_iss >> key >> value >> unit) {
                        proc.memory_kb = value;
                        if (mem_info.total_ram > 0) {
                            proc.memory_usage = (value * 100.0f) / mem_info.total_ram;
                        }
                    }
                    break;
                }
            }
        }
        
        processes.push_back(proc);
    }
    
    closedir(proc_dir);
    
    // Sort processes by CPU usage (descending)
    std::sort(processes.begin(), processes.end(), 
              [](const ProcessInfo& a, const ProcessInfo& b) {
                  return a.cpu_usage > b.cpu_usage;
              });
    
    return processes;
}

std::string formatBytes(unsigned long bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unit_index = 0;
    double size = bytes;
    
    while (size >= 1024.0 && unit_index < 4) {
        size /= 1024.0;
        unit_index++;
    }
    
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << size << " " << units[unit_index];
    return oss.str();
}