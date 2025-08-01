#include "header.h"
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>

std::vector<NetworkInterface> getNetworkInfo() {
    std::vector<NetworkInterface> interfaces;
    
    // Read network statistics from /proc/net/dev
    std::ifstream net_file("/proc/net/dev");
    if (!net_file.is_open()) return interfaces;
    
    std::string line;
    // Skip header lines
    std::getline(net_file, line);
    std::getline(net_file, line);
    
    // Get IP addresses for interfaces
    std::map<std::string, std::string> ip_addresses;
    struct ifaddrs *ifaddr, *ifa;
    if (getifaddrs(&ifaddr) != -1) {
        for (ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
            if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET) {
                struct sockaddr_in* addr_in = (struct sockaddr_in*)ifa->ifa_addr;
                ip_addresses[ifa->ifa_name] = inet_ntoa(addr_in->sin_addr);
            }
        }
        freeifaddrs(ifaddr);
    }
    
    while (std::getline(net_file, line)) {
        std::istringstream iss(line);
        NetworkInterface iface;
        
        std::string name_with_colon;
        if (!(iss >> name_with_colon)) continue;
        
        // Remove colon from interface name
        iface.name = name_with_colon.substr(0, name_with_colon.find(':'));
        
        // Read RX statistics
        if (!(iss >> iface.rx_bytes >> iface.rx_packets >> iface.rx_errs >> iface.rx_drop
                  >> iface.rx_fifo >> iface.rx_frame >> iface.rx_compressed >> iface.rx_multicast)) {
            continue;
        }
        
        // Read TX statistics
        if (!(iss >> iface.tx_bytes >> iface.tx_packets >> iface.tx_errs >> iface.tx_drop
                  >> iface.tx_fifo >> iface.tx_colls >> iface.tx_carrier >> iface.tx_compressed)) {
            continue;
        }
        
        // Set IP address if available
        auto ip_it = ip_addresses.find(iface.name);
        if (ip_it != ip_addresses.end()) {
            iface.ipv4_address = ip_it->second;
        } else {
            iface.ipv4_address = "N/A";
        }
        
        interfaces.push_back(iface);
    }
    
    return interfaces;
}

std::string formatNetworkBytes(unsigned long bytes) {
    const double KB = 1024.0;
    const double MB = KB * 1024.0;
    const double GB = MB * 1024.0;
    
    if (bytes >= GB) {
        double gb = bytes / GB;
        if (gb >= 10.0) return ""; // Too big, don't display
        return std::to_string((int)(gb * 100) / 100.0) + " GB";
    } else if (bytes >= MB) {
        double mb = bytes / MB;
        if (mb >= 1000.0) return ""; // Too big
        return std::to_string((int)(mb * 100) / 100.0) + " MB";
    } else if (bytes >= KB) {
        double kb = bytes / KB;
        if (kb >= 1000000.0) return ""; // Too big
        if (kb < 0.01) return ""; // Too small
        return std::to_string((int)(kb * 100) / 100.0) + " KB";
    } else {
        if (bytes == 0) return "0 B";
        return std::to_string(bytes) + " B";
    }
}