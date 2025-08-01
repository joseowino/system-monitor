#ifndef HEADER_H
#define HEADER_H

#include <string>
#include <vector>
#include <map>
#include <deque>
#include <chrono>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <numeric>
#include <iomanip>
#include <unistd.h>
#include <sys/sysinfo.h>
#include <sys/statvfs.h>
#include <pwd.h>
#include <dirent.h>

// ImGui includes
#include "imgui/lib/imgui.h"
#include "imgui/lib/backend/imgui_impl_sdl.h"
#include "imgui/lib/backend/imgui_impl_opengl3.h"
#include <SDL.h>
#include <GL/gl3w.h>

// Data structures for system information
struct SystemInfo {
    std::string os_type;
    std::string username;
    std::string hostname;
    std::string cpu_type;
    int total_processes = 0;
    int running_processes = 0;
    int sleeping_processes = 0;
    int zombie_processes = 0;
    int stopped_processes = 0;
};

struct ProcessInfo {
    int pid;
    std::string name;
    std::string state;
    float cpu_usage;
    float memory_usage;
    unsigned long memory_kb;
};

struct MemoryInfo {
    unsigned long total_ram;
    unsigned long used_ram;
    unsigned long free_ram;
    unsigned long total_swap;
    unsigned long used_swap;
    unsigned long free_swap;
    unsigned long total_disk;
    unsigned long used_disk;
    unsigned long free_disk;
};

struct NetworkInterface {
    std::string name;
    unsigned long rx_bytes, rx_packets, rx_errs, rx_drop, rx_fifo, rx_frame, rx_compressed, rx_multicast;
    unsigned long tx_bytes, tx_packets, tx_errs, tx_drop, tx_fifo, tx_colls, tx_carrier, tx_compressed;
    std::string ipv4_address;
};

struct CPUInfo {
    float usage_percent;
    std::deque<float> usage_history;
    long user, nice, system, idle, iowait, irq, softirq;
};

struct ThermalInfo {
    float temperature;
    std::deque<float> temp_history;
};

struct FanInfo {
    bool active;
    int speed;
    int level;
    std::deque<float> speed_history;
};

// Graph settings
struct GraphSettings {
    bool animate = true;
    float fps = 30.0f;
    float y_scale = 100.0f;
    int max_points = 200;
};

// Function declarations
// System functions
SystemInfo getSystemInfo();
std::vector<ProcessInfo> getProcesses();
MemoryInfo getMemoryInfo();
std::vector<NetworkInterface> getNetworkInfo();
CPUInfo getCPUInfo();
ThermalInfo getThermalInfo();
FanInfo getFanInfo();

// Utility functions
std::string formatBytes(unsigned long bytes);
std::string trim(const std::string& str);
float calculateCPUUsage();
void updateGraphData(std::deque<float>& data, float value, int max_points);

// GUI functions
void renderSystemMonitor();
void renderMemoryAndProcessMonitor();
void renderNetworkMonitor();
void renderGraph(const std::deque<float>& data, const char* label, float overlay_value, 
                ImVec2 size, GraphSettings& settings, const char* overlay_format = "%.1f%%");

// Global variables
extern GraphSettings cpu_graph_settings;
extern GraphSettings fan_graph_settings;
extern GraphSettings thermal_graph_settings;
extern std::string process_filter;
extern std::vector<int> selected_processes;


std::string formatNetworkBytes(unsigned long bytes);

#endif // HEADER_H
