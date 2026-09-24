# Storage

Every storage device is described by a `StorageDevice` (id, display name,
mount prefix such as `sd:/`, volume label, whether it is removable, whether
it is mounted automatically at startup, whether it is always shown in a
device list, and optional capacity/free-space/block-size/read-only
telemetry). The app enumerates devices, mounts them, and then builds paths
with `getMountPath()` / `getPath()`; after that, all file access is ordinary
POSIX I/O. `getValidLoadDevices()` and `getValidSaveDevices()` list which
devices make sense as a load or save location on the current platform, and
`FindFirstMountedPath()` picks the first mounted device out of a priority
list.

Each device type has exactly one mount. A mount attempt returns
`MountResult::Success`, `DeviceNotFound`, or `MountFailed` (something is
present but can't be mounted, for example an unsupported format), and
`mountResultMessage()` supplies a short string suitable for showing to the
user.

| Device | GameCube | Wii | Wii U |
|---|---|---|---|
| SD card | SD Gecko in slot A or B, SD2SP2 in serial port 2 (FAT) | SD slot (FAT) | SD slot, mounted natively by Cafe OS |
| USB storage | - | Up to 3 simultaneous drives (FAT), via IOS | Up to 3 simultaneous drives (FAT, exFAT, NTFS), via libmocha + libdvm |
| GC Loader | Mounted as a FAT device | - | - |
| DVD | Data DVD (ISO9660) | Data DVD (ISO9660) | - |
| Network share | SMB | SMB | SMB |

**Multiple USB drives.** Wii and Wii U each expose three USB storage slots
(`DEVICE_USB`, `DEVICE_USB2`, `DEVICE_USB3`). A newly attached drive takes
the first free slot in attach order; slots are not tied to a physical port.
On Wii this is handled by `WiiUsbMulti`, which opens each attached mass
storage device through IOS independently and presents each one as its own
disc interface. On Wii U, each slot is opened through libmocha's raw disc
interface and mounted through libdvm, which recognizes FAT, exFAT, and NTFS.
USB on Wii U requires the Mocha CFW component (available under Aroma);
without it the USB slots are simply absent while the SD card and network
share continue to work.

**Hot-plug.** Devices can be inserted and removed while the app runs.
`pollStorageDevices()` is meant to be called about once a second from a
background thread (the demo's device-checking thread does exactly this at
low priority), and reports which mounted devices disappeared and whether the
device list changed shape so the UI can refresh.

* *Wii* - SD, all three USB slots, and the DVD drive are checked each cycle.
* *GameCube* - SD Gecko slots A and B and SD2SP2 are checked each cycle with
  an EXI presence probe. The GC Loader and the DVD drive are only touched
  when the user mounts them.
* *Wii U* - USB attach/detach is detected by scanning the USB stack; 
  a mounted USB volume is verified each cycle with a real, uncached sector
  read, so a drive that was pulled is noticed even if the filesystem cache
  could still answer from memory. A slot holding a device that won't mount
  gets a few quick retries and is then probed on a slow backoff schedule,
  which resets immediately when the USB stack reports a hardware change.

`isDevicePresent()` is a cheap, cached check that never mounts anything, so
a UI can list only devices that are actually there (devices flagged
`alwaysListed`, such as the network share, are shown regardless). On a mount
failure or a read/write error, `invalidateStorageDevice()` marks a device to
be re-mounted fresh on its next use.


[Back to the README](../README.md)
