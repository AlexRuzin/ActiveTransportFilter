# ActiveTransportFilter

### _This project is currently under development_

## Table of Contents (TOC)

- [Summary](#summary)
- [Features and Requirements](#features-and-requirements)
- [Directory summary](#directory-summary)
- [`src` Directory](#-src--directory)
- [ActiveTransportFilter Project UML diagram](#activetransportfilter-project-uml-diagram)
- [ActiveTransportFilter (ATF) project summary](#activetransportfilter--atf--project-summary)
  * [`DeviceConfigService`](#-deviceconfigservice-)
  * [`ActiveTransportFilter`](#-activetransportfilter-)
  * [`DriverController`](#-drivercontroller-)
  * [The IPv4 filtering algorithm](#the-ipv4-filtering-algorithm)
- [Interesting Fixes and Discoveries During Development](#interesting-fixes-and-discoveries-during-development)
  * [Implement correct IRQL](#implement-correct-irql)
  * [BSOD in flow after starting Opera](#bsod-in-flow-after-starting-opera)
  * [Example of Callout Handlers Working](#example-of-callout-handlers-working)

<small><i><a href='http://ecotrust-canada.github.io/markdown-toc/'>Table of contents generated with markdown-toc</a></i></small>


## Summary

This is a project that I have started for educational purposes, please feel free to use it how you wish.

`ActiveTransportFilter` is a device driver that makes use of the Windows Filtering Platform (WFP) for operating-system level **IP blacklisting**, **Domain level blacklisting**, and **packet inspection**. I have decided to first implement the **IP blacklisting** component, as this is simpler and will allow me to build the architecture necessary for implementing a more complex filter driver.

## Features and Requirements
* Uses `vcpkg` manifest, so `vcpkg` is required, along with VS2022
* Compiled with Windows SDK 10.0.22621.2428
* Driver is compiled with WinDDK 10.0.22621.2428
* User-mode code is compiled with C++20
* Statically linked libraries (for compatibility)

## Directory summary
* `.git` -> git directory
* `/bin` -> output binary directory. The installer will be `/bin/DriverController.exe` (which contains all other components in the resource)
* `/build` -> temporary build directory for objects and such
* `/config` -> runtime configurations
* `/docs` -> contains images and resources for `README.md`
* `/src` -> Main source directory

## `src` Directory

| Filename                  | Description                                                                                                                                                                                                                                                                                                                                                        |
|---------------------------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| ActiveTransportFilter/    | Driver project                                                                                                                                                                                                                                                                                                                                                     |
| common/                   | The 'common' directory, containing inline headers and shared headers between user mode and kernel mode                                                                                                                                                                                                                                                             |
| DeviceConfigService/      | Main Config service, configures and controls ActiveTransportFilter                                                                                                                                                                                                                                                                                                 |
| DriverController/         | Project that generates the unified installer                                                                                                                                                                                                                                                                                                                       |
| InterfaceConsole/         | A placeholder project for a usermode console that interfaces with DeviceConfigService                                                                                                                                                                                                                                                                              |
| ActiveTransportFilter.sln | ActiveTransportFilter solutions file                                                                                                                                                                                                                                                                                                                               |
| vcpkg.json                | Contains external dependencies (vcpkg)                                                                                                                                                                                                                                                                                                                             |
| vcpkg-configuration.json  | vcpkg config file 

## ActiveTransportFilter Project UML diagram
_TODO_

## ActiveTransportFilter (ATF) project summary

The driver will consist of three major components: the user-mode service (`DeviceConfigService`), the device driver (`ActiveTransportFilter`), and lastly the installer (`DriverController`).

### `DeviceConfigService`

The user-mode service will be used to configure the driver, to control the WFP callout layers, and to populate the IPv4 and IPv6 blocklists.

* Uses `IOCTL` to control the driver, the commands are defined in `ioctl_codes.h`, and the implementation on the driver side is in `ioctl.c`.
* The service will read an ini file, stored in `config/filter_config.ini`
* The service will read the ini file for a list of IP blocklists
* The service will provide a DNSBL list to the driver as well
* The service controls the driver and WFP callouts:

1) `DriverCommand.InitializeDriverComms()` is first called, which will attempt to communicate with the driver through a symbolic link (driver creates this on `DriverEntry()`)
2) `FilterConfig` is created, which parses the ini. Later, the ini will contain the domains of IP blocklists, where the blacklisted IPs will be obtained
3) `DriverCommand.CmdSendIniConfiguration()` is called, which send the "basic" configuration, i.e. the ini file, to the driver. At this point the driver is waiting for instructions
4) `DriverCommand.CmdPopulateIpv4List()` is called (to be implemented), which will send blacklisted IPs to the driver in chunks, being appended by the driver
5) `DriverCommand.CmdStartWfp()` is called which will instruct the driver to initialize WFP, register callouts and filters, and, lastly, begin intercepting traffic as it comes from NDIS/WFP
6) To update the config, the driver needs to disable filtering. This is done through `DriverCommand.CmdStopWfp()`. Then update or append the config and call `DriverCommand.CmdStartWfp()` again to start filtering

### `ActiveTransportFilter`

The device driver component that will use Windows Driver Framework (WDF) and Windows Filtering Platform (WFP) to intercept inbound/outbound TCP traffic.

* The driver must be signaled and configured by the service before it can begin operations
* The WFP callout layers are configurable, the default ones are:

```
ADD_WFP_LAYER(data->enableLayerIpv4TcpInbound, &FWPM_LAYER_INBOUND_TRANSPORT_V4);
ADD_WFP_LAYER(data->enableLayerIpv4TcpOutbound, &FWPM_LAYER_OUTBOUND_TRANSPORT_V4);
ADD_WFP_LAYER(data->enableLayerIpv6TcpInbound, &FWPM_LAYER_INBOUND_TRANSPORT_V6);
ADD_WFP_LAYER(data->enableLayerIpv6TcpOutbound, &FWPM_LAYER_OUTBOUND_TRANSPORT_V6);
ADD_WFP_LAYER(data->enableLayerIcmpv4, &FWPM_CONDITION_ORIGINAL_ICMP_TYPE);
```

* `ioctl.c` handles the IOCTL communication from the usermode service
* `wfp.c` initializes WFP
* `config.c` produces a `CONFIG_CTX` context structure, which will be used to shape traffic
* `filter.c` will use the `CONFIG_CTX` structure to perform filtering, all WFP callouts are sent over to `filter.c`

### `DriverController`

The `DriverController` is used to install the driver, service, and other components. Using VisualStudio, I have compiled all the necessary binaries and objects into the `DriverController.exe` file, which will unpack the files and create the necessary services.

The components are packed into `DriverController`'s resource segment, and automatically updated when doing a full rebuild in VisualStudio.

### The IPv4 filtering algorithm

This is still under design. I will most likely use a type of radix "trie" to create a tree which will have 255 children for each octet in the IP. I will experiment with various methods until I find one that is most suitable.

* Experimentation with a "patricia trie", an O(m) search (for each ipv4 octet), but in 64-bit space. 
* Each array contains a pointer to another array and so on for 4 octets. 
* The size of a trie is twice as large on x64 than x86 due to the pointer size.

## Interesting Fixes and Discoveries During Development

### Implement correct IRQL
Note that the function for initializing WFP is `FwpmEngineOpen()`, for this function to work, the kthread must be in `PASSIVE_LEVEL`. An issue I encountered was that I attempted to add a spinlock to synchronize IOCTLs. This raised the IRQL to `APC_LEVEL`, and `FwpmEngineOpen()` would fail.

This was resolved by using a less intensive lock, like `KMUTEX`, which operates at `PASSIVE_LEVEL`.

### BSOD in flow after starting Opera

Symptoms: BSOD when opera browser is opened, and hundreds of new connections come in. Crash is in ATF in `KMUTEX` lock

<img src="docs/images/interesting_crash_callout_sync.png">

Issue is probably that WFP doesn't like locking in the callout functions, which I used to sync up with the filter.c engine. I need to remove the `KMUTEX` entirely, and freeup the callout immediately.

As a result, I cannot "dynamically" change the config for the filter engine, I need to stop WFP entirely, refresh the config, and restart WFP.

**Update** removing the `KMUTEX` in the callout was the fix.

### Example of Callout Handlers Working

Below is Windbg in a remote kernel debugging instance (using hyper-v). Each packet is intercepted by the ATF driver and sent to the debugger using `trace.c`.

<img src="docs/images/callout_example.png">