# 最小实机步骤：只适用于另行复核过的候选

此文档不自动操作设备。先读 REPRODUCE 的RAM载体集成缺口，不从本仓库直接生成或下载现成镜像。每轮由操作者物理回到小米原厂 FASTBOOT 图形界面；临时U-Boot的Fastboot端点不能替代。

## 启动前

1. 独占设备，不与其他Agent或项目同时访问。
2. 确认型号marble、原厂ABL Fastboot、非userspace，并记录当前槽位；槽位可能因设备而异，不写死或切换。
3. 在电脑保存候选完整SHA-256、来源提交、编译器、config、参数及包检查结果；本次日志使用新目录。

仅查询这些基本信息，然后只对经过复核的候选执行一次临时启动：

```sh
fastboot getvar product
fastboot getvar current-slot
fastboot getvar is-userspace
# 操作者确认屏幕确为原厂FASTBOOT，且上面的身份与自己的记录相符后：
fastboot boot /path/to/reviewed-candidate.img
```

不调用su/adb root；不刷写、擦除、切槽或改变安全状态；不读写手机分区。不执行上游文档中的固定LBA/UFS根分区或持久化引导步骤。保留原厂Fastboot物理恢复能力，不以EDL为前提。

## 分阶段验收

| 阶段 | 必须取得的实际证据 |
|---|---|
| 菜单 | 真正显示菜单，音量键选择/短按电源确认，Tools可进入 |
| XNU | 可归属该镜像的内核屏幕输出或可定位panic；不能只看loader hand-off |
| RAM根 | APFS挂载及目标文件读取成功 |
| 用户程序 | 真实执行、系统调用/退出/输出证据；不能只看文件存在 |
| launchd | bootstrap、子服务/Mach/XPC与持续运行分别验收 |
| GUI/触控 | SpringBoard持续响应，真实打开并滚动设置 |
| GPU | 实际命令执行及正确像素读回；之后另验桌面长期交互 |
| 多核 | 实际并行工作证据；不以cpus参数或报告CPU数量替代 |

当前 `examples/unixnu.toml` 是原实验参数参考，引用的内核、RAM根、EFI工具均不随仓库分发。先菜单/Tools，再64档；64通过后才另起一轮256。`stop7`是主动返回预算停点，不等于错误根因。至少记录lost、errors/kept/over、FIRST和copy状态。

画面稳定时先记录，不因一次OKAY就宣布执行成功。原始日志保留在操作者本机；公开问题单仅提供自行脱敏的最小摘要，不提交序列号、GPT、系统资源或原始录像。
