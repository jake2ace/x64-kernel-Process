# x64 Windows Kernel Process Study

**English** | [简体中文](README.zh-CN.md)

A learning milestone by **JAKE**, documenting experiments with process mechanisms in the x64 Windows kernel. This WDM driver brings together four process enumeration approaches and a process termination experiment, developed while following a Windows kernel course and practicing in a virtual machine.

## Experiments

| Topic | Function | What the code explores |
| --- | --- | --- |
| Process ID lookup | `EnumProcess` | Scans a fixed range of candidate PIDs and resolves process objects with `PsLookupProcessByProcessId`. |
| Active process list | `EnumActiveProcess` | Walks `EPROCESS.ActiveProcessLinks` using offsets observed in the lab system. |
| System process information | `EnumProcessbySystemInfo` | Queries `ZwQuerySystemInformation(SystemProcessInformation)` and walks the returned records. |
| System handle information | `EnumProcessByHandleInfo` | Queries `SystemHandleInformation`, deduplicates owner PIDs, and resolves the corresponding process objects. |
| Process termination | `KillProcess` | Looks up a target process, scans candidate thread IDs, checks thread ownership, and invokes an internal thread termination routine. |

The exercises cover process and thread objects, linked lists, variable-length query buffers, handle ownership, reference counting, and kernel debugging. They record hands-on experiments in a simulated VM environment; the driver performs real kernel operations.

## Tested Windows version and lab environment

- **Tested OS: Windows 10 Pro, version 1703 (RS2), x64, build 15063**, running in VMware.
- Recorded kernel build: `15063.0.amd64fre.rs2_release.170317-1834`; desktop build label: `15063.rs2_release.170317-1834`.
- Testing is limited to the environment above. Other Windows versions and architectures have not been validated for this project.
- Visual Studio 2019 solution format, with the `WindowsKernelModeDriver10.0` WDK toolset.
- WinDbg for kernel debugging and `[JAKE]` debug output.
- A Debug x64 build and a termination experiment were observed during the learning session. This publication copy has not been rebuilt or run separately.

## Project layout

```text
x64-kernel-process-study/
├── README.md
├── README.zh-CN.md
├── LICENSE
├── .gitignore
├── x64-kernel-process-study.sln
└── PROCESS/
    ├── main.c
    ├── PROCESS.vcxproj
    └── PROCESS.vcxproj.filters
```

Open `x64-kernel-process-study.sln` in Visual Studio with the required C++ and WDK components, then select **Debug | x64**. The standalone solution contains only the `PROCESS` project. The inherited project file also contains other platform configurations; this learning record covers x64 only.

## Current snapshot and limitations

The current `DriverEntry` selects the termination experiment; the four enumeration calls are commented out. It contains a fixed target PID and a hardcoded internal function address from the recorded session. Loading this snapshot starts that experiment immediately. Review the entry point before using it in a disposable test VM.

Structure offsets and the internal function signature/address are specific to the studied environment. The address is not portable across boots or Windows builds. Pattern-based function discovery is a future learning item and is **not implemented** here.

The enumeration methods expose different views. A fixed PID range is not exhaustive, and the handle-table approach finds processes represented as owners of returned handles. The legacy handle record used here has a 16-bit owner PID field. None of these examples establishes a guarantee of complete enumeration or universal termination success.

The source preserves the learning snapshot, including its experimental code and `[JAKE]` log prefix. Its Chinese comments have been converted from the original local encoding to UTF-8 with BOM for display on GitHub. Build outputs, signing certificates, local IDE state, and debugger configuration are excluded.

## License

[MIT](LICENSE) © 2026 JAKE.
