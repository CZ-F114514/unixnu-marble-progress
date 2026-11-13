# UniXNU marble：进度与复现记录

Redmi Note 12 Turbo / POCO F5，marble / SM7475 / Adreno 725。
截至 **2026-11-14**，已在原厂 Fastboot 临时启动链上看到 UniXNU 菜单、XNU 屏幕输出、RAM APFS 根和原版用户程序执行。原版 launchd 的 bootstrap 仍在诊断；**完整 iOS 服务、SpringBoard、触控和 GPU 未通过本路线验收**。

这个小仓库用于同步进度、评审改动和复现实验。上游项目通过 [固定版本引用](dependencies.json) 获取，不内置完整依赖源码。这里只保留自己的增量补丁、当前观察器的主机测试、离线二维码解码器和文档；不提供 Apple 系统资源或预编译启动包。

## 从这里开始

- [当前进度、已验证和未验证](docs/PROGRESS.zh.md)
- [依赖获取、补丁应用与离线构建步骤](docs/REPRODUCE.zh.md)
- [最小临时启动与分阶段验收](docs/DEVICE-TEST.zh.md)
- [下一步问题与证据要求](docs/NEXT.zh.md)
- [脱敏范围与发布说明](docs/PUBLICATION.zh.md)

无需 Apple 资源或手机即可运行：

```sh
bash tools/run-host-checks.sh
python3 tools/verify_manifest.py
```

主机观察器回归和补丁重放已独立验证。完整 v4 RAM 载体还包含定制固件 RAM 驱动、APRIORI 调整、可视内核和 CRC 绑定，**尚未从本公开副本完成干净环境冷构建验证**。指南明确区分可独立复现的检查、已有构建记录和剩余集成工作；不能把上游通用 bootimg 的输出当成本轮冻结载体。

系统目标固定为 iPadOS 17.7.11 / 21H461。iOS/iPadOS 27 的 PAC/4KiB 研究与这条启动链分开；HakiOS / 20F66 的成果不能算作 UniXNU 验收。

## 目录

```text
docs/           进度、依赖、操作步骤和验收边界
patches/        相对固定上游版本的两个增量补丁及文件哈希
examples/       实际参数示例、禁用物理存储的配置、补丁顺序
host-tests/     当前 bootstrap 观察器和纯合成二维码测试
tools/          离线测试、二维码解码及文件完整性检查
LICENSES/       增量补丁涉及的上游版权说明
```

版权及原有许可证保持不变，见 [NOTICE.md](NOTICE.md)。依赖仍由各自上游维护。
