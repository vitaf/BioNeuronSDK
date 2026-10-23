# BioNeuronSDK 安装与配置

## 系统要求

- Linux x86_64
- Python 3.6+
- GCC 9+（用于构建插件）
- BioNeuronCore（已安装或从源码构建）
- CMake 3.20+（可选，用于 cmake 模块）

## 从 .deb 包安装

```bash
sudo dpkg -i mindcopy-sdk_0.6.0_amd64.deb
```

文件将安装到 `/opt/mindcopy/`：
- `include/plugin-devkit.h` — 插件开发头文件
- `bin/biocore-gen-dispatch.py` — 分发代码生成器
- `share/sdk/cmake/` — CMake 模块 `biocore-plugin.cmake`
- `share/sdk/templates/` — 插件模板
- `share/sdk/examples/` — 插件示例

## 加载插件（本地 / 通过守护进程）

插件编译为 `.so` 文件，并通过 `dlopen` 加载：

- **本地（STATIC 模式）：** Studio 链接 `libbiocore_gcc.a`，并在自身进程中加载插件
- **远程（IPC 模式）：** 守护进程 `biocore-daemon` 在自身进程中加载插件。Studio 通过 Unix 套接字或 TCP（默认端口 6789）连接到守护进程

插件 API（`extern "C" register_modifier_current / register_modulator_release`）在两种模式下完全相同 — 插件无需知道自身被加载到哪个进程中。

## 环境变量

- `BIOCORE_ROOT` — BioNeuronCore 的路径（可选）
- `BIOCORE_SDK_DIR` — SDK 的路径（可选）

## 插件的 GPU 兼容性

插件使用 GCC 编译并通过 dlopen 加载 — GPU 后端不影响插件。
为 Core 选择 GPU 后端 — 请参阅 [Core install.md](../BioNeuronCore/install.md#выбор-gpu-бекенда)。

## 验证

```bash
python3 /opt/mindcopy/bin/biocore-gen-dispatch.py --stub
```


## 权利与知识产权

© 2024–2026 **Фешин Виталий Александрович** / Vitaly Alexandrovich Feshin. All rights reserved.
E-mail: feshin2004@mail.ru · vfeshin@mindcopy.ru · 电话：+7(918)345-75-18. Краснодар, Россия / Krasnodar, Russia.

计算机程序国家注册证书（Rospatent）**编号 727894_683451**。
未经作者书面许可，禁止商业使用、分发和再许可。
全文请参阅平台根目录下的 `LICENSE` 和 `DISCLAIMER.md`。
