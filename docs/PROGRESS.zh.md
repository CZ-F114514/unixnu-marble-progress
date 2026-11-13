# 当前进度：真实用户代码已执行，基础服务尚未验收

[English](PROGRESS.md) | **简体中文** | [首页](../README.zh-CN.md)

日期：2026-11-14。下面“已验证”来自这条 UniXNU 链的实际观察；原始照片、录像和设备日志不公开。

| 阶段 | 状态与证据范围 |
|---|---|
| 原厂 ABL → UEFI → UniXNU | 菜单和 Tools 已看到，音量键/短按电源可操作；不是仅凭 fastboot OKAY |
| XNU | 已得到屏幕 panic/阶段输出；早期根设备失败已进入更后的 RAM 路线 |
| RAM APFS | 挂载成功，原版 expr/dyld/launchd 文件可读取 |
| 原版 expr 子进程 | 自有 PID1 监管，原版 expr 作为子进程执行并退出0，管道读回31字节 |
| 原版 launchd | 已进入原版代码；标准 IO 初始化的三个 dup2 有正常返回；不是完整服务启动 |
| 当前256档 | 主动阈值停点，pre265/ret256，lost1；不接受完整无丢失256 |
| 正常 SystemOS/Data、系统服务、GUI | 未验收；恢复根缺少选定的 SpringBoard/backboardd 资源 |
| 触控、GPU像素读回、持续交互、多核 | 本路线均未验收；当前候选使用 cpus=1 |

## 最近256档

`n1333 / pre265 / ret256 / limit256 / stop7 / dup3 / lost1 / errors64 / kept16 / over48`。
stop7 是观察器达到返回数量后的主动停止。只看到了前16条非零结果：2条 ENOMEM、13条 ENOENT、1条 ESRCH；剩余48条详情缺失。

新增 persona494 返回 ESRCH3，但没有具体 operation 和完整参数，不能直接认定致命。末条 close_nocancel399(fd4) 的原生 C 处理函数返回0；这不等于整个更上层流程完成。

FIRST 是原 libc 的 sysctlbyname 查询路径，采样 MIB `[1,149]`、长度128→0。149属于动态 OID 范围，实际名称和 ENOMEM 分支未锁定。路径字段只有20字符前缀，不能当完整路径。pre-ret差9不能当线程数量。

lost只说明票据或代际状态校验未配对。pending[8]覆盖是一种可能机制，没有失败序号/TID/原因，不能确认根因或宣称修好。

## 标准 IO 的进展

将原版程序直接当诊断 PID1 时出现过 exec 后 EBADF。生命周期探针表明，exec保留阶段的描述符仍存在，但后续用户路径发生关闭；不能归因于 CLOEXEC 或凭空消失。改为自有 PID1 监管原版 expr 子进程后，测试得到正常退出和实际输出。这个有限成功不能代替原版 launchd 完整服务验收。

## 载体边界

最近冻结镜像是 Android v4、ramdisk字段0、BootShim + 3MiB FD + UXRAM02 + 192MiB FAT 的 RAM 载体。其 SHA-256：
`9fde85ff4d2221cda0e7a0d36bff29e252aec10fcb01c5acea897c61f8aa1c1a`。
镜像本身不分发，哈希是旧实验身份，不是公开补丁生成任何新镜像的通过凭据。

物理 UFS/DMA/分区写入及物理日志后端禁用。当前实际 PANIC_TEXT=0，不能宣称 panic 文字自动落盘或 RAM 文字回填。HakiOS 的 GPU/触控/多核结果与本链分开。
