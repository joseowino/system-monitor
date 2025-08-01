#include "header.h"

SystemInfo getSystemInfo() {
    SystemInfo info;
    
    // Get OS type
    std::ifstream os_file("/etc/os-release");
    std::string line;
    while (std::getline(os_file, line)) {
        if (line.find("PRETTY_NAME=") == 0) {
            info.os_type = line.substr(12);
            // Remove quotes
            info.os_type.erase(std::remove(info.os_type.begin(), info.os_type.end(), '"'), info.os_type.end());
            break;
        }
    }
    if (info.os_type.empty()) {
        info.os_type = "Linux";
    }
    
    // Get username
    uid_t uid = getuid();
    struct passwd *pw = getpwuid(uid);
    if (pw) {
        info.username = pw->pw_name;
    } else {
        info.username = "unknown";
    }
    
    // Get hostname
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == 0) {
        info.hostname = hostname;
    } else {
        info.hostname = "unknown";
    }
    
    // Get CPU type
    std::ifstream cpu_file("/proc/cpuinfo");
    while (std::getline(cpu_file, line)) {
        if (line.find("model name") != std::string::npos) {
            size_t pos = line.find(':');
            if (pos != std::string::npos) {
                info.cpu_type = trim(line.substr(pos + 1));
                break;
            }
        }
    }
    
    // Get process counts
    info.total_processes = 0;
    info.running_processes = 0;
    info.sleeping_processes = 0;
    info.zombie_processes = 0;
    info.stopped_processes = 0;
    
    DIR* proc_dir = opendir("/proc");
    if (proc_dir) {
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
            
            if (is_pid && strlen(entry->d_name) > 0) {
                info.total_processes++;
                
                // Read process state
                std::string stat_path = "/proc/" + std::string(entry->d_name) + "/stat";
                std::ifstream stat_file(stat_path);
                if (stat_file.is_open()) {
                    std::string stat_line;
                    if (std::getline(stat_file, stat_line)) {
                        std::istringstream iss(stat_line);
                        std::string token;
                        // Skip PID and comm
                        iss >> token >> token;
                        // Get state
                        if (iss >> token) {
                            char state = token[0];
                            switch (state) {
                                case 'R': info.running_processes++; break;
                                case 'S': case 'D': info.sleeping_processes++; break;
                                case 'Z': info.zombie_processes++; break;
                                case 'T': case 't': info.stopped_processes++; break;
                            }
                        }
                    }
                }
            }
        }
        closedir(proc_dir);
    }
    
    return info;
}

CPUInfo getCPUInfo() {
    static CPUInfo cpu_info;
    static long prev_user = 0, prev_nice = 0, prev_system = 0, prev_idle = 0;
    static long prev_iowait = 0, prev_irq = 0, prev_softirq = 0;
    
    std::ifstream stat_file("/proc/stat");
    std::string line;
    if (std::getline(stat_file, line)) {
        std::istringstream iss(line);
        std::string cpu_label;
        iss >> cpu_label >> cpu_info.user >> cpu_info.nice >> cpu_info.system 
            >> cpu_info.idle >> cpu_info.iowait >> cpu_info.irq >> cpu_info.softirq;
        
        long total_prev = prev_user + prev_nice + prev_system + prev_idle + prev_iowait + prev_irq + prev_softirq;
        long total_curr = cpu_info.user + cpu_info.nice + cpu_info.system + cpu_info.idle + cpu_info.iowait + cpu_info.irq + cpu_info.softirq;
        
        long idle_prev = prev_idle + prev_iowait;
        long idle_curr = cpu_info.idle + cpu_info.iowait;
        
        long total_diff = total_curr - total_prev;
        long idle_diff = idle_curr - idle_prev;
        
        if (total_diff > 0) {
            cpu_info.usage_percent = 100.0f * (total_diff - idle_diff) / total_diff;
        } else {
            cpu_info.usage_percent = 0.0f;
        }
        
        prev_user = cpu_info.user;
        prev_nice = cpu_info.nice;
        prev_system = cpu_info.system;
        prev_idle = cpu_info.idle;
        prev_iowait = cpu_info.iowait;
        prev_irq = cpu_info.irq;
        prev_softirq = cpu_info.softirq;
    }
    
    return cpu_info;
}

ThermalInfo getThermalInfo() {
    static ThermalInfo thermal_info;
    
    // Try to read temperature from thermal zone
    std::ifstream temp_file("/sys/class/thermal/thermal_zone0/temp");
    if (temp_file.is_open()) {
        int temp_millidegrees;
        if (temp_file >> temp_millidegrees) {
            thermal_info.temperature = temp_millidegrees / 1000.0f;
        }
    } else {
        // Fallback to a simulated temperature
        thermal_info.temperature = 45.0f + (rand() % 20); // 45-65°C
    }
    
    return thermal_info;
}

FanInfo getFanInfo() {
    static FanInfo fan_info;
    
    // Try to read fan information from hwmon
    bool found_fan = false;
    for (int i = 0; i < 10; i++) {
        std::string fan_input_path = "/sys/class/hwmon/hwmon" + std::to_string(i) + "/fan1_input";
        std::ifstream fan_file(fan_input_path);
        if (fan_file.is_open()) {
            if (fan_file >> fan_info.speed) {
                fan_info.active = fan_info.speed > 0;
                fan_info.level = fan_info.speed / 1000; // Approximate level
                found_fan = true;
                break;
            }
        }
    }
    
    if (!found_fan) {
        // Simulate fan data
        fan_info.active = true;
        fan_info.speed = 2000 + (rand() % 1000); // 2000-3000 RPM
        fan_info.level = fan_info.speed / 1000;
    }
    
    return fan_info;
}

std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(' ');
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(' ');
    return str.substr(first, (last - first + 1));
}

void updateGraphData(std::deque<float>& data, float value, int max_points) {
    data.push_back(value);
    while (data.size() > float(max_points)) {
        data.pop_front();
    }
}