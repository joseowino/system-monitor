#include "header.h"
#include <iostream>

// Global variables
GraphSettings cpu_graph_settings;
GraphSettings fan_graph_settings;
GraphSettings thermal_graph_settings;
std::string process_filter;
std::vector<int> selected_processes;

// Static data for graphs
static CPUInfo cpu_data;
static ThermalInfo thermal_data;
static FanInfo fan_data;
static auto last_update = std::chrono::steady_clock::now();

void renderGraph(const std::deque<float>& data, const char* label, float overlay_value, 
                ImVec2 size, GraphSettings& settings, const char* overlay_format) {
    
    ImGui::Text("%s", label);
    
    // Graph controls
    ImGui::Checkbox(("Animate##" + std::string(label)).c_str(), &settings.animate);
    ImGui::SameLine();
    ImGui::SliderFloat(("FPS##" + std::string(label)).c_str(), &settings.fps, 1.0f, 60.0f);
    ImGui::SliderFloat(("Y Scale##" + std::string(label)).c_str(), &settings.y_scale, 10.0f, 200.0f);
    
    // Convert deque to vector for ImGui
    std::vector<float> plot_data(data.begin(), data.end());
    
    // Create overlay text
    char overlay_text[64];
    snprintf(overlay_text, sizeof(overlay_text), overlay_format, overlay_value);
    
    ImGui::PlotLines(("##" + std::string(label)).c_str(), 
                     plot_data.data(), plot_data.size(), 0, overlay_text, 
                     0.0f, settings.y_scale, size);
}

void renderSystemMonitor() {
    static SystemInfo sys_info;
    static auto last_sys_update = std::chrono::steady_clock::now();
    
    auto now = std::chrono::steady_clock::now();
    auto sys_elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_sys_update);
    
    // Update system info every 5 seconds
    if (sys_elapsed.count() >= 5) {
        sys_info = getSystemInfo();
        last_sys_update = now;
    }
    
    ImGui::Text("System Information");
    ImGui::Separator();
    
    ImGui::Text("OS: %s", sys_info.os_type.c_str());
    ImGui::Text("User: %s", sys_info.username.c_str());
    ImGui::Text("Hostname: %s", sys_info.hostname.c_str());
    ImGui::Text("CPU: %s", sys_info.cpu_type.c_str());
    
    ImGui::Spacing();
    ImGui::Text("Process Summary:");
    ImGui::Text("Total: %d", sys_info.total_processes);
    ImGui::Text("Running: %d", sys_info.running_processes);
    ImGui::Text("Sleeping: %d", sys_info.sleeping_processes);
    ImGui::Text("Zombie: %d", sys_info.zombie_processes);
    ImGui::Text("Stopped: %d", sys_info.stopped_processes);
    
    ImGui::Spacing();
    ImGui::Separator();
    
    // Performance graphs section
    if (ImGui::BeginTabBar("PerformanceTabBar")) {
        
        // CPU Tab
        if (ImGui::BeginTabItem("CPU")) {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_update);
            float target_interval = 1000.0f / cpu_graph_settings.fps;
            
            if (cpu_graph_settings.animate && elapsed.count() >= target_interval) {
                cpu_data = getCPUInfo();
                updateGraphData(cpu_data.usage_history, cpu_data.usage_percent, cpu_graph_settings.max_points);
            }
            
            renderGraph(cpu_data.usage_history, "CPU Usage", cpu_data.usage_percent, 
                       ImVec2(0, 200), cpu_graph_settings, "%.1f%%");
            
            ImGui::EndTabItem();
        }
        
        // Fan Tab
        if (ImGui::BeginTabItem("Fan")) {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_update);
            float target_interval = 1000.0f / fan_graph_settings.fps;
            
            if (fan_graph_settings.animate && elapsed.count() >= target_interval) {
                fan_data = getFanInfo();
                updateGraphData(fan_data.speed_history, fan_data.speed, fan_graph_settings.max_points);
            }
            
            ImGui::Text("Status: %s", fan_data.active ? "Active" : "Inactive");
            ImGui::Text("Speed: %d RPM", fan_data.speed);
            ImGui::Text("Level: %d", fan_data.level);
            
            renderGraph(fan_data.speed_history, "Fan Speed", fan_data.speed, 
                       ImVec2(0, 200), fan_graph_settings, "%.0f RPM");
            
            ImGui::EndTabItem();
        }
        
        // Thermal Tab
        if (ImGui::BeginTabItem("Thermal")) {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_update);
            float target_interval = 1000.0f / thermal_graph_settings.fps;
            
            if (thermal_graph_settings.animate && elapsed.count() >= target_interval) {
                thermal_data = getThermalInfo();
                updateGraphData(thermal_data.temp_history, thermal_data.temperature, thermal_graph_settings.max_points);
            }
            
            renderGraph(thermal_data.temp_history, "Temperature", thermal_data.temperature, 
                       ImVec2(0, 200), thermal_graph_settings, "%.1f°C");
            
            ImGui::EndTabItem();
        }
        
        ImGui::EndTabBar();
    }
    
    // Update last_update time
    if (cpu_graph_settings.animate || fan_graph_settings.animate || thermal_graph_settings.animate) {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_update);
        float min_interval = std::min({1000.0f / cpu_graph_settings.fps, 
                                      1000.0f / fan_graph_settings.fps, 
                                      1000.0f / thermal_graph_settings.fps});
        if (elapsed.count() >= min_interval) {
            last_update = now;
        }
    }
}

void renderMemoryAndProcessMonitor() {
    static MemoryInfo mem_info;
    static std::vector<ProcessInfo> processes;
    static auto last_mem_update = std::chrono::steady_clock::now();
    
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_mem_update);
    
    // Update memory and process info every 2 seconds
    if (elapsed.count() >= 2) {
        mem_info = getMemoryInfo();
        processes = getProcesses();
        last_mem_update = now;
    }
    
    ImGui::Text("Memory Usage");
    ImGui::Separator();
    
    // RAM Usage
    ImGui::Text("Physical Memory (RAM):");
    float ram_percent = (float)mem_info.used_ram / mem_info.total_ram;
    ImGui::ProgressBar(ram_percent, ImVec2(0.0f, 0.0f), 
                      (formatBytes(mem_info.used_ram * 1024) + " / " + formatBytes(mem_info.total_ram * 1024)).c_str());
    
    // SWAP Usage
    ImGui::Text("Virtual Memory (SWAP):");
    float swap_percent = mem_info.total_swap > 0 ? (float)mem_info.used_swap / mem_info.total_swap : 0.0f;
    ImGui::ProgressBar(swap_percent, ImVec2(0.0f, 0.0f), 
                      (formatBytes(mem_info.used_swap * 1024) + " / " + formatBytes(mem_info.total_swap * 1024)).c_str());
    
    // Disk Usage
    ImGui::Text("Disk Usage:");
    float disk_percent = (float)mem_info.used_disk / mem_info.total_disk;
    ImGui::ProgressBar(disk_percent, ImVec2(0.0f, 0.0f), 
                      (formatBytes(mem_info.used_disk * 1024) + " / " + formatBytes(mem_info.total_disk * 1024)).c_str());
    
    ImGui::Spacing();
    ImGui::Separator();
    
    // Process table
    ImGui::Text("Processes");
    
    // Filter input
    char filter_buffer[256];
    strncpy(filter_buffer, process_filter.c_str(), sizeof(filter_buffer) - 1);
    filter_buffer[sizeof(filter_buffer) - 1] = '\0';
    
    if (ImGui::InputText("Filter", filter_buffer, sizeof(filter_buffer))) {
        process_filter = filter_buffer;
    }
    
    // Process table
    if (ImGui::BeginTable("ProcessTable", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | 
                         ImGuiTableFlags_Sortable | ImGuiTableFlags_ScrollY, ImVec2(0, 300))) {
        
        ImGui::TableSetupColumn("PID", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("State", ImGuiTableColumnFlags_WidthFixed, 60.0f);
        ImGui::TableSetupColumn("CPU %", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableSetupColumn("Memory %", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableHeadersRow();
        
        for (size_t i = 0; i < processes.size(); i++) {
            const ProcessInfo& proc = processes[i];
            
            // Apply filter
            if (!process_filter.empty()) {
                std::string proc_name_lower = proc.name;
                std::string filter_lower = process_filter;
                std::transform(proc_name_lower.begin(), proc_name_lower.end(), proc_name_lower.begin(), ::tolower);
                std::transform(filter_lower.begin(), filter_lower.end(), filter_lower.begin(), ::tolower);
                
                if (proc_name_lower.find(filter_lower) == std::string::npos) {
                    continue;
                }
            }
            
            ImGui::TableNextRow();
            
            // Check if row is selected
            bool is_selected = std::find(selected_processes.begin(), selected_processes.end(), proc.pid) != selected_processes.end();
            
            ImGui::TableSetColumnIndex(0);
            if (ImGui::Selectable(std::to_string(proc.pid).c_str(), is_selected, ImGuiSelectableFlags_SpanAllColumns)) {
                if (ImGui::GetIO().KeyCtrl) {
                    // Multi-select with Ctrl
                    if (is_selected) {
                        selected_processes.erase(std::remove(selected_processes.begin(), selected_processes.end(), proc.pid), 
                                               selected_processes.end());
                    } else {
                        selected_processes.push_back(proc.pid);
                    }
                } else {
                    // Single select
                    selected_processes.clear();
                    selected_processes.push_back(proc.pid);
                }
            }
            
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%s", proc.name.c_str());
            
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%s", proc.state.c_str());
            
            ImGui::TableSetColumnIndex(3);
            ImGui::Text("%.1f", proc.cpu_usage);
            
            ImGui::TableSetColumnIndex(4);
            ImGui::Text("%.1f", proc.memory_usage);
        }
        
        ImGui::EndTable();
    }
}

void renderNetworkMonitor() {
    static std::vector<NetworkInterface> interfaces;
    static auto last_net_update = std::chrono::steady_clock::now();
    
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_net_update);
    
    // Update network info every 2 seconds
    if (elapsed.count() >= 2) {
        interfaces = getNetworkInfo();
        last_net_update = now;
    }
    
    ImGui::Text("Network Information");
    ImGui::Separator();
    
    // Display network interfaces and their IPv4 addresses
    ImGui::Text("Network Interfaces:");
    for (const auto& iface : interfaces) {
        ImGui::Text("%s: %s", iface.name.c_str(), iface.ipv4_address.c_str());
    }
    
    ImGui::Spacing();
    ImGui::Separator();
    
    // Network statistics tables
    if (ImGui::BeginTabBar("NetworkTabBar")) {
        
        // RX Table
        if (ImGui::BeginTabItem("RX (Receive)")) {
            if (ImGui::BeginTable("RXTable", 9, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollX)) {
                ImGui::TableSetupColumn("Interface");
                ImGui::TableSetupColumn("Bytes");
                ImGui::TableSetupColumn("Packets");
                ImGui::TableSetupColumn("Errors");
                ImGui::TableSetupColumn("Drop");
                ImGui::TableSetupColumn("FIFO");
                ImGui::TableSetupColumn("Frame");
                ImGui::TableSetupColumn("Compressed");
                ImGui::TableSetupColumn("Multicast");
                ImGui::TableHeadersRow();
                
                for (const auto& iface : interfaces) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0); ImGui::Text("%s", iface.name.c_str());
                    ImGui::TableSetColumnIndex(1); ImGui::Text("%lu", iface.rx_bytes);
                    ImGui::TableSetColumnIndex(2); ImGui::Text("%lu", iface.rx_packets);
                    ImGui::TableSetColumnIndex(3); ImGui::Text("%lu", iface.rx_errs);
                    ImGui::TableSetColumnIndex(4); ImGui::Text("%lu", iface.rx_drop);
                    ImGui::TableSetColumnIndex(5); ImGui::Text("%lu", iface.rx_fifo);
                    ImGui::TableSetColumnIndex(6); ImGui::Text("%lu", iface.rx_frame);
                    ImGui::TableSetColumnIndex(7); ImGui::Text("%lu", iface.rx_compressed);
                    ImGui::TableSetColumnIndex(8); ImGui::Text("%lu", iface.rx_multicast);
                }
                ImGui::EndTable();
            }
            ImGui::EndTabItem();
        }
        
        // TX Table
        if (ImGui::BeginTabItem("TX (Transmit)")) {
            if (ImGui::BeginTable("TXTable", 9, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollX)) {
                ImGui::TableSetupColumn("Interface");
                ImGui::TableSetupColumn("Bytes");
                ImGui::TableSetupColumn("Packets");
                ImGui::TableSetupColumn("Errors");
                ImGui::TableSetupColumn("Drop");
                ImGui::TableSetupColumn("FIFO");
                ImGui::TableSetupColumn("Colls");
                ImGui::TableSetupColumn("Carrier");
                ImGui::TableSetupColumn("Compressed");
                ImGui::TableHeadersRow();
                
                for (const auto& iface : interfaces) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0); ImGui::Text("%s", iface.name.c_str());
                    ImGui::TableSetColumnIndex(1); ImGui::Text("%lu", iface.tx_bytes);
                    ImGui::TableSetColumnIndex(2); ImGui::Text("%lu", iface.tx_packets);
                    ImGui::TableSetColumnIndex(3); ImGui::Text("%lu", iface.tx_errs);
                    ImGui::TableSetColumnIndex(4); ImGui::Text("%lu", iface.tx_drop);
                    ImGui::TableSetColumnIndex(5); ImGui::Text("%lu", iface.tx_fifo);
                    ImGui::TableSetColumnIndex(6); ImGui::Text("%lu", iface.tx_colls);
                    ImGui::TableSetColumnIndex(7); ImGui::Text("%lu", iface.tx_carrier);
                    ImGui::TableSetColumnIndex(8); ImGui::Text("%lu", iface.tx_compressed);
                }
                ImGui::EndTable();
            }
            ImGui::EndTabItem();
        }
        
        // RX Visual Usage
        if (ImGui::BeginTabItem("RX Usage")) {
            ImGui::Text("Network RX Usage (0GB - 2GB scale):");
            for (const auto& iface : interfaces) {
                double gb_value = iface.rx_bytes / (1024.0 * 1024.0 * 1024.0);
                float progress = std::min(1.0f, (float)(gb_value / 2.0)); // Scale to 2GB max
                
                std::string formatted = formatNetworkBytes(iface.rx_bytes);
                if (!formatted.empty()) {
                    ImGui::Text("%s:", iface.name.c_str());
                    ImGui::ProgressBar(progress, ImVec2(0.0f, 0.0f), formatted.c_str());
                }
            }
            ImGui::EndTabItem();
        }
        
        // TX Visual Usage
        if (ImGui::BeginTabItem("TX Usage")) {
            ImGui::Text("Network TX Usage (0GB - 2GB scale):");
            for (const auto& iface : interfaces) {
                double gb_value = iface.tx_bytes / (1024.0 * 1024.0 * 1024.0);
                float progress = std::min(1.0f, (float)(gb_value / 2.0)); // Scale to 2GB max
                
                std::string formatted = formatNetworkBytes(iface.tx_bytes);
                if (!formatted.empty()) {
                    ImGui::Text("%s:", iface.name.c_str());
                    ImGui::ProgressBar(progress, ImVec2(0.0f, 0.0f), formatted.c_str());
                }
            }
            ImGui::EndTabItem();
        }
        
        ImGui::EndTabBar();
    }
}

int main(int argc, char* argv[]) {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0) {
        std::cerr << "Error: " << SDL_GetError() << std::endl;
        return -1;
    }

    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

    // Create window with graphics context
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Window* window = SDL_CreateWindow("System Monitor", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync

    // Initialize OpenGL loader
    if (gl3wInit() != 0) {
        std::cerr << "Failed to initialize OpenGL loader!" << std::endl;
        return 1;
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Initialize graph settings
    cpu_graph_settings = {true, 30.0f, 100.0f, 200};
    fan_graph_settings = {true, 30.0f, 4000.0f, 200};
    thermal_graph_settings = {true, 30.0f, 100.0f, 200};

    // Main loop
    bool done = false;
    while (!done) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                done = true;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
                done = true;
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame(window);
        ImGui::NewFrame();

        // Create main window
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(io.DisplaySize);
        if (ImGui::Begin("System Monitor", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse)) {
            
            if (ImGui::BeginTabBar("MainTabBar")) {
                
                if (ImGui::BeginTabItem("System")) {
                    renderSystemMonitor();
                    ImGui::EndTabItem();
                }
                
                if (ImGui::BeginTabItem("Memory & Processes")) {
                    renderMemoryAndProcessMonitor();
                    ImGui::EndTabItem();
                }
                
                if (ImGui::BeginTabItem("Network")) {
                    renderNetworkMonitor();
                    ImGui::EndTabItem();
                }
                
                ImGui::EndTabBar();
            }
        }
        ImGui::End();

        // Rendering
        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
