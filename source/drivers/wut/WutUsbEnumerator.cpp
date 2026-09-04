/****************************************************************************
 * libgui - drivers/wut
 * Daryl Borth 2026
 * WutUsbEnumerator.cpp
 ***************************************************************************/
#include "WutUsbEnumerator.h"
#include <sys/statvfs.h>
#include <malloc.h>
#include <cstring>
#include <algorithm>

#define UHS_IOCTL_GET_DEVICE_LIST 0x01
#define UHS_IOCTL_GET_DEVICE_DESC 0x02
#define UHS_IOCTL_GET_STRING_DESC 0x05

namespace {
constexpr UsbPortLocation MapInterfaceToPhysicalPort(int interface_index) {
    switch (interface_index) {
        case 0:  return UsbPortLocation::FrontUpper;
        case 1:  return UsbPortLocation::FrontLower;
        case 2:  return UsbPortLocation::RearUpper;
        case 3:  return UsbPortLocation::RearLower;
        default: return UsbPortLocation::Unknown;
    }
}
}

WutUsbEnumerator::WutUsbEnumerator() : ios_fd(-1), fsa_fd(0) {}

WutUsbEnumerator::~WutUsbEnumerator() {
    if (fsa_fd > 0) {
        FSADelClient(fsa_fd);
        FSAShutdown();
    }
    if (ios_fd >= 0) {
        IOS_Close(ios_fd);
    }
}

bool WutUsbEnumerator::InitializeSubsystems() {
    if (ios_fd < 0) {
        ios_fd = IOS_Open("/dev/uhs/0", IOS_OPEN_READWRITE);
    }
    if (fsa_fd <= 0) {
        FSAInit();
        fsa_fd = FSAAddClient(nullptr);
    }
    return (ios_fd >= 0 && fsa_fd > 0);
}

std::vector<UsbStorageDevice> WutUsbEnumerator::EnumerateDevices() {
    if (!InitializeSubsystems()) {
        return DevoptabFallbackPolling();
    }

    std::vector<UsbStorageDevice> physical_devices;

    uint32_t* list_buffer = static_cast<uint32_t*>(memalign(0x40, 0x100));
    if (list_buffer) {
        memset(list_buffer, 0, 0x100);
        int num_devices = IOS_Ioctl(ios_fd, UHS_IOCTL_GET_DEVICE_LIST, nullptr, 0, list_buffer, 0x100);
        
        if (num_devices > 0 && num_devices <= 8) {
            for (int i = 0; i < num_devices; ++i) {
                UsbStorageDevice device = {};
                device.port_location = MapInterfaceToPhysicalPort(list_buffer[i]);
                ExtractHardwareMetadata(device, list_buffer[i]);
                physical_devices.push_back(device);
            }
        }
        free(list_buffer);
    }

    std::vector<std::string> valid_mounts;
    for (int i = 0; i < 4; ++i) {
        std::string target = "usb" + std::to_string(i) + ":";
        struct statvfs vfs;
        if (statvfs((target + "/").c_str(), &vfs) == 0) {
            valid_mounts.push_back(target);
        }
    }

    if (physical_devices.empty() || valid_mounts.empty()) {
        return DevoptabFallbackPolling();
    }

    std::vector<UsbStorageDevice> current_devices;
    size_t pair_count = std::min(physical_devices.size(), valid_mounts.size());
    
    for (size_t i = 0; i < pair_count; ++i) {
        UsbStorageDevice device = physical_devices[i]; 
        device.mount_path = valid_mounts[i];           
        device.is_connected = true;
        PopulateTelemetry(device);                     
        current_devices.push_back(device);
    }

    AnalyzePowerTopology(current_devices);
    
    std::lock_guard<std::mutex> lock(enum_mutex);
    for (auto& dev : current_devices) {
        std::string sig = dev.GetUniqueSignature();
        if (!device_cache.count(sig)) {
            device_cache[sig] = dev;
        } else {
            bool notified = device_cache[sig].state_notified;
            device_cache[sig] = dev;
            device_cache[sig].state_notified = notified;
        }
    }

    for (auto it = device_cache.begin(); it != device_cache.end(); ) {
        bool still_connected = false;
        for (const auto& current : current_devices) {
            if (current.GetUniqueSignature() == it->first) {
                still_connected = true;
                break;
            }
        }
        
        if (!still_connected) {
            it->second.is_connected = false;
            it->second.state_notified = false; 
        }
        ++it;
    }

    return current_devices;
}

std::vector<UsbStorageDevice> WutUsbEnumerator::GetHotplugChanges() {
    std::lock_guard<std::mutex> lock(enum_mutex);
    std::vector<UsbStorageDevice> changes;
    
    for (auto it = device_cache.begin(); it != device_cache.end(); ) {
        if (!it->second.state_notified) {
            changes.push_back(it->second);
            it->second.state_notified = true;
            
            if (!it->second.is_connected) {
                it = device_cache.erase(it);
                continue;
            }
        }
        ++it;
    }
    return changes;
}

void WutUsbEnumerator::AnalyzePowerTopology(std::vector<UsbStorageDevice>& devices) {
    bool interface_active[4] = { false, false, false, false };
    for (const auto& dev : devices) {
        if (dev.port_location == UsbPortLocation::FrontUpper) interface_active[0] = true;
        if (dev.port_location == UsbPortLocation::FrontLower) interface_active[1] = true;
        if (dev.port_location == UsbPortLocation::RearUpper)  interface_active[2] = true;
        if (dev.port_location == UsbPortLocation::RearLower)  interface_active[3] = true;
    }

    for (auto& dev : devices) {
        int idx = -1;
        if (dev.port_location == UsbPortLocation::FrontUpper) idx = 0;
        else if (dev.port_location == UsbPortLocation::FrontLower) idx = 1;
        else if (dev.port_location == UsbPortLocation::RearUpper)  idx = 2;
        else if (dev.port_location == UsbPortLocation::RearLower)  idx = 3;

        if (idx != -1) {
            int paired_idx = idx ^ 1; 
            dev.is_y_cable_pair_detected = !interface_active[paired_idx];
            dev.has_insufficient_power_risk = interface_active[paired_idx];
        }
    }
}

void WutUsbEnumerator::ExtractHardwareMetadata(UsbStorageDevice& device, int interface_index) {
    void* io_buffer = memalign(0x40, 0x1000);
    uint32_t* req_index = static_cast<uint32_t*>(memalign(0x40, 0x40));
    
    if (!io_buffer || !req_index) {
        free(io_buffer);
        free(req_index);
        return;
    }
    
    memset(io_buffer, 0, 0x1000);
    *req_index = static_cast<uint32_t>(interface_index);

    int res = IOS_Ioctl(ios_fd, UHS_IOCTL_GET_DEVICE_DESC, req_index, 0x40, io_buffer, 0x1000);
    if (res >= 0) {
        uint8_t* desc = static_cast<uint8_t*>(io_buffer);
        device.vid = (desc[9] << 8) | desc[8];
        device.pid = (desc[11] << 8) | desc[10];
        
        device.manufacturer = ExtractUhsString(interface_index, desc[14]);
        device.product_name = ExtractUhsString(interface_index, desc[15]);
        device.serial_number = ExtractUhsString(interface_index, desc[16]);
    }

    free(req_index);
    free(io_buffer);
}

std::string WutUsbEnumerator::ExtractUhsString(int interface_index, uint8_t string_index) {
    if (string_index == 0) return "";
    
    void* io_buffer = memalign(0x40, 0x100);
    uint32_t* request = static_cast<uint32_t*>(memalign(0x40, 0x40));
    
    if (!io_buffer || !request) {
        free(io_buffer);
        free(request);
        return "";
    }
    
    memset(io_buffer, 0, 0x100);
    request[0] = static_cast<uint32_t>(interface_index);
    request[1] = static_cast<uint32_t>(string_index);
    
    std::string result = "";
    int res = IOS_Ioctl(ios_fd, UHS_IOCTL_GET_STRING_DESC, request, 0x40, io_buffer, 0x100);
    
    if (res > 2) {
        uint8_t* desc = static_cast<uint8_t*>(io_buffer);
        int len = desc[0]; 
        
        if (len >= 2 && len <= 0x100) { 
            len -= 2;
            for (int i = 0; i < len; i += 2) {
                if ((i + 2) < 0x100 && desc[2 + i] != 0) {
                    result += static_cast<char>(desc[2 + i]);
                }
            }
        }
    }
    
    free(request);
    free(io_buffer);
    return result;
}

void WutUsbEnumerator::PopulateTelemetry(UsbStorageDevice& device) {
    struct statvfs vfs;
    if (statvfs((device.mount_path + "/").c_str(), &vfs) == 0) {
        device.total_bytes = static_cast<uint64_t>(vfs.f_blocks) * vfs.f_frsize; 
        device.free_bytes  = static_cast<uint64_t>(vfs.f_bavail) * vfs.f_frsize;
        device.is_read_only = (vfs.f_flag & ST_RDONLY) != 0;
    }

    if (fsa_fd > 0) {
        FSADirectoryEntry* entry = static_cast<FSADirectoryEntry*>(memalign(0x40, sizeof(FSADirectoryEntry)));
        if (entry) {
            FSADirectoryHandle dir_handle;
            if (FSAOpenDir(fsa_fd, (device.mount_path + "/").c_str(), &dir_handle) == FS_ERROR_OK) {
                if (FSAReadDir(fsa_fd, dir_handle, entry) == FS_ERROR_OK) {
                    device.is_dirty_mount = false; // Note: FSA_STAT_FLAG_DIRTY maps similarly if supported natively in WUT, default to false
                }
                FSACloseDir(fsa_fd, dir_handle);
            }
            free(entry);
        }
    }
    
    if (device.product_name.empty()) {
        device.volume_fallback = ExtractVolumeLabel(device.mount_path); 
    }
}

std::vector<UsbStorageDevice> WutUsbEnumerator::DevoptabFallbackPolling() {
    std::vector<UsbStorageDevice> devices;
    const int max_usb_mounts = 4;
    
    for (int i = 0; i < max_usb_mounts; ++i) {
        std::string path = "usb" + std::to_string(i) + ":";
        struct statvfs vfs;
        
        if (statvfs((path + "/").c_str(), &vfs) == 0) {
            UsbStorageDevice dev = {};
            dev.mount_path = path;
            dev.port_location = MapInterfaceToPhysicalPort(i);
            dev.is_connected = true;
            dev.volume_fallback = ExtractVolumeLabel(path);
            dev.total_bytes = static_cast<uint64_t>(vfs.f_blocks) * vfs.f_frsize;
            dev.free_bytes = static_cast<uint64_t>(vfs.f_bavail) * vfs.f_frsize;
            dev.is_read_only = (vfs.f_flag & ST_RDONLY) != 0;
            dev.is_dirty_mount = false;
            
            devices.push_back(dev);
        }
    }
    AnalyzePowerTopology(devices);
    return devices;
}

std::string WutUsbEnumerator::ExtractVolumeLabel(const std::string& mount_path) {
    if (fsa_fd <= 0) return "Generic USB Volume";
    
    FSAVolumeInfo* vol_info = static_cast<FSAVolumeInfo*>(memalign(0x40, sizeof(FSAVolumeInfo)));
    std::string label = "Generic USB Volume";
    
    if (vol_info) {
        if (FSAGetVolumeInfo(fsa_fd, (mount_path + "/").c_str(), vol_info) == FS_ERROR_OK) {
            char vol_name[128] = {0};
            strncpy(vol_name, vol_info->volumeLabel, sizeof(vol_name) - 1);
            label = std::string(vol_name);
        }
        free(vol_info);
    }
    return label;
}
