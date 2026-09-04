/****************************************************************************
 * libgui - drivers/wut
 * Daryl Borth 2026
 * WutUsbEnumerator.h
 ***************************************************************************/
#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <mutex>
#include <map>
#include <coreinit/ios.h>
#include <coreinit/filesystem_fsa.h>

enum class UsbPortLocation {
    Unknown,
    FrontUpper,
    FrontLower,
    RearUpper,
    RearLower
};

struct UsbStorageDevice {
    std::string mount_path;         
    UsbPortLocation port_location = UsbPortLocation::Unknown;
    
    uint16_t vid = 0;                   
    uint16_t pid = 0;                   
    std::string manufacturer;       
    std::string product_name;       
    std::string serial_number;      
    std::string volume_fallback;    
    
    uint64_t total_bytes = 0;
    uint64_t free_bytes = 0;
    
    bool is_read_only = false;
    bool is_dirty_mount = false;
    bool is_connected = false;
    bool state_notified = false;
    bool is_y_cable_pair_detected = false;
    bool has_insufficient_power_risk = false;
    
    std::string GetUniqueSignature() const {
        return mount_path + "_" + std::to_string(vid) + "_" + std::to_string(pid) + "_" + serial_number;
    }
};

class WutUsbEnumerator {
public:
    static WutUsbEnumerator& GetInstance() {
        static WutUsbEnumerator instance;
        return instance;
    }

    WutUsbEnumerator(const WutUsbEnumerator&) = delete;
    WutUsbEnumerator& operator=(const WutUsbEnumerator&) = delete;

    std::vector<UsbStorageDevice> EnumerateDevices();
    std::vector<UsbStorageDevice> GetHotplugChanges();

private:
    WutUsbEnumerator();
    ~WutUsbEnumerator();

    std::mutex enum_mutex;
    IOSHandle ios_fd;
    FSAClientHandle fsa_fd;
    std::map<std::string, UsbStorageDevice> device_cache;

    bool InitializeSubsystems();
    void ExtractHardwareMetadata(UsbStorageDevice& device, int interface_index);
    std::string ExtractUhsString(int interface_index, uint8_t string_index);
    void PopulateTelemetry(UsbStorageDevice& device);
    void AnalyzePowerTopology(std::vector<UsbStorageDevice>& devices);
    
    std::vector<UsbStorageDevice> DevoptabFallbackPolling();
    std::string ExtractVolumeLabel(const std::string& mount_path);
};
