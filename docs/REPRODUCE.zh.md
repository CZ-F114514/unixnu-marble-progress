# 复现指南

[English](REPRODUCE.md) | **简体中文** | [首页](../README.zh-CN.md)

## 1. 不依赖手机的自包含检查

在 Linux/WSL 的本仓库根目录运行。需要 Bash、Python3、支持 ASan/UBSan 的 Clang（本次主机实际使用21）。不需要 Apple 文件、root 权限、手机或网络。

```sh
bash tools/run-host-checks.sh
python3 tools/verify_manifest.py
```

输出写入新的 `build/host.*`，不覆盖原记录。观察器测试使用合成进程、地址和字符串；覆盖错误行可见性、sysctl参数边界、复制异常、一次性票据、代际变化、容量溢出、64/256阈值和锁外复制。二维码4项测试也只使用合成帧。

主机通过不代表 ARM64 真机、内核整体栈或服务通过。目标编译器Clang24与主机测试Clang21分别记录。

## 2. 获取上游，只应用本项目的增量

依赖链接与完整提交固定在 [dependencies.json](../dependencies.json)。不要改成默认分支。以下操作只在新建的电脑目录中进行，不访问手机：

```sh
PUBLIC_REPO="$PWD"
WORK="$(mktemp -d "${TMPDIR:-/tmp}/unixnu-repro.XXXXXX")"
git clone https://github.com/UniXNU/UniXNU "$WORK/UniXNU"
git -C "$WORK/UniXNU" checkout --detach 04b4e558bcded5213c2d802984862dc062f28257
git clone https://github.com/MiLacy/UniXNU-marble "$WORK/UniXNU-marble"
git -C "$WORK/UniXNU-marble" checkout --detach 78f4e7bf00e88129f8b6ef5680d81770d386832d

git -C "$WORK/UniXNU" apply --check "$PUBLIC_REPO/patches/unixnu-host-menu-overlay.patch"
git -C "$WORK/UniXNU" apply "$PUBLIC_REPO/patches/unixnu-host-menu-overlay.patch"
git -C "$WORK/UniXNU-marble" apply --check "$PUBLIC_REPO/patches/board-runtime-overlay.patch"
git -C "$WORK/UniXNU-marble" apply "$PUBLIC_REPO/patches/board-runtime-overlay.patch"
python3 "$PUBLIC_REPO/tools/check_overlays.py" --unixnu "$WORK/UniXNU" --board "$WORK/UniXNU-marble"
```

`check_overlays.py`逐文件对照补丁输出SHA，不接受“patch命令成功”作为唯一证据。本次已用保留的固定版本来源独立重放27个文件变更；远端下载可用性未重验。

board补丁添加当前阶段的有序增量，包含RAM根、expr/stdio、原版launchd与错误观察器；不能只取最后一份错误补丁套到原版XNU。`examples/xnu-series`是设备级顺序，arch/soc/os的补丁仍由上游 resolver 按其各自顺序处理。禁止再次把这些嵌套补丁手工重复应用到UniXNU构建树。

## 3. 工具链与内核构建记录

按照固定 UniXNU 版本的 Linux 工具链说明准备本地工具链。上游的脚本、依赖源码和安装文档直接引用其仓库，不在这里复制。工具版本见 dependencies.json；不得用系统 Linux 头文件替换 Darwin 目标头文件。宿主BSD库/头文件与目标搜索路径分开。

在自有目录设置工具链，清理目标编译不应继承的宿主搜索路径：

```sh
export UNIXNU_TOOLCHAIN="/path/to/your/unixnu-toolchain"
export PATH="$UNIXNU_TOOLCHAIN/shims:$UNIXNU_TOOLCHAIN/bin:$PATH"
unset CPATH C_INCLUDE_PATH CPLUS_INCLUDE_PATH LIBRARY_PATH
```

自行取得匹配21H461的 kernelcache、信任数据和恢复资源；不从手机分区提取，不公开上传。设置真实的本地解码缓存路径后，在上一步 `WORK` 的同一shell运行：

```sh
KERNELCACHE="/path/to/private/21H461/kernelcache.decompressed"
test -f "$KERNELCACHE"
cd "$WORK/UniXNU"
./unixnu defconfig marble-ram-apfs_defconfig -D "$WORK/UniXNU-marble"
./unixnu status --json -D "$WORK/UniXNU-marble" > "$WORK/resolved-status.json"
./unixnu tree -D "$WORK/UniXNU-marble"
./unixnu build kernel -D "$WORK/UniXNU-marble" --kernelcache "$KERNELCACHE" -j 4
```

这是既有构建所用的驱动命令形式，**尚未在完全干净的公开依赖下载环境重跑**。依赖源树、archive/PMGR预链接阶段、工具链宿主库、恢复资源解码与APFS匹配均为前置条件。若发生缺少先前匹配prelink的错误，先保留第一遍内核，依据上游PMGR桥要求完成第二遍；不能用空桥替代后宣称通过。每一遍保存config、resolved-status、命令、日志、compiler版本和SHA。

配置里物理UFS/owner/virtio存储相关选项应保持n，检查实际最终 `.config`，不能只看样例或defconfig。DT中GIC stride/region须以当前板级DTS与硬件证据核对，不能套用别的SoC。

## 4. RAM载体集成仍待公开冷构建验证

上面的两个补丁不是完整固件补丁集，也不是现成启动包。定制固件RAM文件系统驱动、菜单前APRIORI顺序、BootShim和DT、内核入口/panic可视化、ADT、信任缓存及恢复RAM卷仍需要匹配集成。这里不复制完整Mu源码、Apple资源或既有FD/FAT。

应引用固定Mu-Silicium来源，另外准备并复核其RAM载体增量后，再使用上游固件构建入口。不能直接将未经复核的上游 `bootimg` 或旧v0输出拿去手机测试。特别是当前board中保留的旧v0头字段只是历史配置，**并非已验证v4打包配方**。

最近实际载体的包装约束：Android头v4/4096B头页、ramdisk=0；kernel字段为gzip(BootShim144B + FD0x300000 + UXRAM02头64B + FAT192MiB)后接DTB。UXRAM02含载荷长度、载荷CRC和头CRC；固件中的期望CRC必须同步。保留区域、装载地址、EL、MMIO映射、DT /chosen及framebuffer行宽分别检查；不能只核对文件扩展名或fastboot响应。

**验收门禁：** 至少完成解包逐字节比对、压缩流尾部DTB核验、实际FFS/APRIORI/RAM驱动及CRC一致性检查、物理存储provider排除和源版本复核，才能按 DEVICE-TEST 进行一次临时启动。这个公开仓库没有声称已提供一键完整镜像重建。
