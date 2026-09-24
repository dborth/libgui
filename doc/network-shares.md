# Network shares (SMB)

`FileSystemDriver::getSmb()` returns the platform's `SmbDriver`, which
mounts a single SMB share as the `DEVICE_SMB` device at `smb:/`. GameCube and
Wii use `OgcSmbDriver`; Wii U uses `WutSmbDriver`. Both are built on libsmb2
and register an `smb:/` devoptab, so once connected the share behaves like
any other mounted device.

```cpp
SmbShareInfo share = {};                 // host, share, user, password (empty user = guest)
snprintf(share.host, sizeof(share.host), "192.168.0.100");
snprintf(share.share, sizeof(share.share), "Files");

SmbDriver * smb = platform->getFileSystem()->getSmb();
SmbConnectResult result = smb->connect(share);   // brings the network up first if needed

if(result != SmbConnectResult::Success)
    showError(smb->connectResultMessage(result), smb->getLastError());
else
    FILE * f = fopen("smb:/roms/game.sfc", "rb");
```

`connect()` brings the console's network connection up if it isn't already
(GameCube/Wii via libogc's network stack; Wii U via the system's network
account service), and is a no-op if already connected to the same share.
Results are `Success`, `InvalidSettings` (no host or share), `NetworkUnavailable`,
or `ConnectFailed` (the network is up but the server, share, or credentials
were rejected); `getLastError()` adds libsmb2's own detail, such as an
access-denied or host-resolution message. `disconnect()` unmounts the share.
The demo's Network Share screen shows the whole flow.


[Back to the README](../README.md)
