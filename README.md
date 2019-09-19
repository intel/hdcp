# Intel(R) unified HDCP


## Introduction

The Intel(R) unified HDCP (High-bandwidth Digital Content Protection) is a user space implementation to prevent copying of digital audio & video content across digital display interfaces. It provides Linux user space implementation to enable the HDCP1.4 and HDCP2.2 protection for external digital display interfaces (HDMI/DP).


## License

The Intel(R) unified HDCP is distributed under the MIT license with portions covered under the BSD 3-clause "New" or "Revised" License. You may obtain a copy of the License at:
https://opensource.org/licenses/MIT


## Prerequisites

For Ubuntu 16.04 and above
\# sudo apt-get install cmake autoconf libdrm-dev libssl-dev
Equivalents for other distributions should work.


## Dependencies

For libdrm 2.4.89 and above - https://dri.freedesktop.org/libdrm/

For openssl 1.0.2 and above - https://www.openssl.org/source/

The Intel(R) unified HDCP is dependent on kernel space HDCP implementation to provide a complete stack to enable the HDCP1.4 and HDCP2.2 protection. You need upgrade your Linux kernel to include dependent kernel space HDCP implementations with below steps:
1.  Obtain Linux kernel from https://github.com/freedesktop/drm-tip
2.  Follow below steps to build and upgrade kernel:
```
$ make ARCH=x86_64 defconfig
$ make ARCH=x86_64 menuconfig
    # include device driver->Network device driver->USB Network Adapters->the corresponding adapter if you use usb adapter for network hub.
    # include drivers->misc drivers->Intel Management Engine Interface & ME Enabled Intel Chipsets & Intel Trusted Execution Enviroment with ME Interface to enable MEI_HDCP for HDCP 2.2
$ make -j
$ make modules
$ sudo make modules_install
$ sudo make install
```

## Building

1.  Build and install unified HDCP master
2.  Get unified HDCP repo and format the workspace folder as below:
```
<workspace>
    |- daemon
    |- sdk
    |- common
    |- CMakeLists.txt
    ...
```
3. Create build folder, and generate targets in the directory
```
$ cd <workspace>
$ mkdir build; cd build
$ cmake ../
```
4.  Then run
```
$ make -j8
```

## Install

```
$ sudo make install
```
This will install the following files (e.g. on Ubuntu):
```
-- Installing: /usr/local/bin/hdcpd
-- Installing: /usr/local/include/hdcpapi.h
-- Installing: /usr/local/lib/libhdcpsdk.so
-- Installing: /usr/local/lib/pkgconfig/libhdcpsdk.pc
-- Installing: /lib/systemd/system/hdcpd.service
-- ...
```


## Supported Platforms

APL (Apollolake)    for HDCP1.4

KBL (Kabylake)      for HDCP1.4 and HDCP2.2

GLK (Geminilake)    for HDCP1.4 and HDCP2.2


## Known Issues and Limitations

1.  APIs currently supported by drm-tip kernel (https://github.com/freedesktop/drm-tip) :
```
        HDCPCreate
        HDCPDestroy
        HDCPEnumerateDisplay
        HDCPSetProtectionLevel for HDCP_LEVEL0/HDCP_LEVEL1
        HDCPGetStatus
        HDCPSetProtectionLevel for HDCP_LEVEL2
```
    APIs tested internally but not supported by drm-tip kernel :
```
        HDCPGetKsvList
        HDCPSendSRMData
        HDCPGetSRMVersion
        HDCPConfig
```



## References - HDCP SDK programming guide

HDCP SDK APIs are defined in https://github.com/intel/hdcp/sdk/hdcpapi.h

You may follow below basic steps for HDCP enabling:
1.  App calls HDCPCreate. HDCP SDK initializes a session with the HDCP daemon.

2.  App calls HDCPEnumerateDisplay. The HDCP daemon will populate a client supplied buffer with a list of connections and authentication status for each attached display. This list includes available HDCP ports with associated port identifiers. Port identifiers are only valid within the scope of this software stack.

3.  If there is a revoked list of HDCP Bksv values, the App can call HDCPSendSRMData to send the SRM data to the daemon. This is not required as part of the standard HDCP sequence. Those data will be checked during HDCP enabling.

4.  If the App desires HDCP authentication for a connected port, then the App calls HDCPSetProtectionLevel with the corresponding port identifier and HDCP_LEVEL1/HDCP_LEVEL2. The HDCP daemon will initiate HDCP authentication step 1, and if the selected downstream device is a repeater, the daemon will also perform authentication step 2. At startup, the HDCP daemon spawns a work thread to check the link status. This thread is configured to check link status at a minimum frequency of 200ms in accordance with the HDCP specification. If this thread finds that the link is lost (re-authentication failed), it will notify App by PORT_EVENT_LINK_LOST.

5.  App starts playing protected content.

6.  The HDCP daemon will continue to monitor for hotplug events, notify the App by PORT_EVNET_PLUG_OUT if a hotplug-out event is detected. App will call HDCPSetProtectionLevel with HDCP_LEVEL0 to disable link in callback function.

7.  App finishes playing protected content.

8.  App is finished and calls HDCPDestroy. HDCP SDK cleans up and destroys its session with the daemon. The daemon will remove the App from a list of active applications using the port.
