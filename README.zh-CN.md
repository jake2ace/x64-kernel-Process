# x64 Windows 内核进程机制学习

[English](README.md) | **简体中文**

这是 **JAKE** 学习 x64 Windows 内核进程机制的阶段性成果，记录跟随内核课程、在虚拟机中完成的实验与模拟验证。项目使用 WDM 内核驱动，集中整理了**四种进程遍历方式和进程强杀实验**。

## 实验内容

| 内容 | 对应函数 | 学习要点 |
| --- | --- | --- |
| 通过进程 ID 查找 | `EnumProcess` | 扫描固定范围内的候选 PID，通过 `PsLookupProcessByProcessId` 获取进程对象。 |
| 遍历活动进程链表 | `EnumActiveProcess` | 根据实验系统中的结构偏移，遍历 `EPROCESS.ActiveProcessLinks`。 |
| 查询系统进程信息 | `EnumProcessbySystemInfo` | 使用 `ZwQuerySystemInformation(SystemProcessInformation)`，逐条解析返回的进程记录。 |
| 从系统句柄信息提取进程 | `EnumProcessByHandleInfo` | 查询 `SystemHandleInformation`，对句柄所属 PID 去重，再查找对应进程对象。 |
| 进程强杀 | `KillProcess` | 查找目标进程、扫描候选线程 ID、判断线程所属进程，并调用内部线程终止例程。 |

这些练习涵盖进程与线程对象、链表遍历、变长查询缓冲区、句柄归属、对象引用计数和内核调试。这里的“模拟”指虚拟机实验环境；代码会执行真实的内核操作。

## 测试系统版本与实验环境

- **测试系统：Windows 10 专业版，1703（RS2），x64，Build 15063**，运行于 VMware 虚拟机。
- 调试记录中的内核构建标识：`15063.0.amd64fre.rs2_release.170317-1834`；桌面显示的构建标识：`15063.rs2_release.170317-1834`。
- 本项目的测试范围仅限上述环境，其他 Windows 版本与架构尚未验证。
- Visual Studio 2019 解决方案格式，WDK 工具集为 `WindowsKernelModeDriver10.0`。
- 使用 WinDbg 进行内核调试，通过 `[JAKE]` 前缀查看调试输出。
- 学习过程中已观察到 Debug x64 构建及进程终止实验结果；本次整理的发布副本没有重新编译或运行。

## 项目结构

```text
x64-kernel-process-study/
├── README.md                      英文介绍，默认显示
├── README.zh-CN.md                中文介绍
├── LICENSE                       MIT 许可证
├── .gitignore                    本机文件与构建产物排除规则
├── x64-kernel-process-study.sln   独立解决方案
└── PROCESS/
    ├── main.c                    四种遍历方式及进程强杀实验
    ├── PROCESS.vcxproj
    └── PROCESS.vcxproj.filters
```

在安装了相应 C++ 和 WDK 组件的 Visual Studio 中打开 `x64-kernel-process-study.sln`，选择 **Debug | x64**。独立解决方案仅包含 `PROCESS` 项目。原项目文件仍保留其他平台配置，本学习记录只覆盖 x64 环境。

## 当前代码状态与实验边界

当前 `DriverEntry` 选择的是进程强杀实验，四种遍历函数的调用处于注释状态。代码保留了实验时的固定 PID 和内部函数地址，加载驱动就会触发该实验。在可还原的测试虚拟机中使用前，需要先检查入口函数。

结构体偏移、内部函数签名和地址依赖所研究的系统环境。写死的地址不能跨系统重启或 Windows 版本直接复用。**后续计划学习通过特征码定位函数，目前尚未实现。**

四种遍历方式观察到的进程集合并不一定相同：固定 PID 范围可能遗漏对象；句柄信息法只能提取返回记录中的句柄所属进程，且此处采用的旧式句柄记录中，所属 PID 字段只有 16 位。这些实验不代表能够完整枚举所有进程，也不构成对任意进程终止成功率的保证。

源码保留了学习阶段的实验逻辑和 `[JAKE]` 日志前缀，仅将中文注释所在文件从原本的本地编码转换为 UTF-8 BOM，便于 GitHub 展示。发布目录不包含编译产物、签名证书、本机 IDE 状态或调试器配置。

## 许可证

[MIT](LICENSE)，Copyright © 2026 JAKE。
