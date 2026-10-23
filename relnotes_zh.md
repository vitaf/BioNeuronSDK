# 发布说明 — BioNeuronSDK

## v0.6.2 (2026-07-01) — Agent Skills + dispatch 修复
- **Agent Skills**：DDD 方法论 + 调试排查（SDK 专属：dispatch、dlopen、仅限 GCC）+ CLAUDE.md 中的规划工作流
- **biocore-gen-dispatch 修复**：生成弱符号（weak symbols）而非 extern 声明 — 防止链接冲突
- **GPU 兼容性**：已在 install.md 中记录 GPU 后端兼容性
- **与 Core v0.19.5 兼容**（CUDA 两遍扩散修复）

## v0.6.1 (2026-06-25) — 与 Core v0.19.4 兼容
- 与统一日志格式 unixtime [LEVEL] msg 兼容
- 明确指定插件仅用 GCC 构建（无 Intel 依赖）

## v0.6.0 — BION v6 + 用户 ID 的最终移位

**日期：** 2026 年 6 月 16 日

- **MOD_TYPE_COUNT=121**（Core v0.16.7）。Core 新增了感觉修饰器（115-120）。
- **用户 ID 范围：121–127**（原为 93–127）。
- **完整 BION v6 兼容**：`Nucleus` 332 字节，`NetworkHeader` 84 字节，`Segment` 988 字节。
- 所有 struct static_assert 与 Core v0.16.7 一致。
- 示例和模板已更新：ID 121（Ca 通道）、ID 122（D2 受体）。
- **测试（2026-06-18）：** +2 个测试 — `test_sdk_security`（.desc JSON 校验，8 项检查）、`test_sdk_integration`（完整插件周期，3 项检查）。SDK：6/6 个测试。

## v0.5.0-v0.4.0 — CMake 模块测试 + 移位至 MOD_TYPE_COUNT=107

**日期：** 2026 年 6 月 10 日

**v0.5.0:**
- test_cmake_module.cpp：biocore-plugin.cmake 的 6 个测试（.so 构建、dispatch、find_sdk）
- test_example_plugins.cpp：4 个 dlopen/dlsym 插件测试
- 总计：4/4 个测试，100% PASS
- Valgrind：无泄漏

**v0.4.0:**
- **MOD_TYPE_COUNT=107**（原为 98）。Core 新增了 9 个新类型（98-106）。
- **用户 ID 范围：107–127**（原为 98–127）。因新增内建稳态和通道类型而移位。
- 新增内建类型（98-106）：PMCA、NCX、KCC2、Ca_Leak、Ca_Buffer、SK、BK、NaP、Kir
- 兼容 Core v0.13.0 和 Studio v0.20.0

**日期：** 2026 年 6 月 10 日

- test_cmake_module.cpp：biocore-plugin.cmake 的 6 个测试（.so 构建、dispatch、find_sdk）
- test_example_plugins.cpp：4 个 dlopen/dlsym 插件测试
- 总计：4/4 个测试，100% PASS
- Valgrind：无泄漏

## v0.4.0 — 与 Core v0.13.0 兼容

**日期：** 2026 年 6 月

- **MOD_TYPE_COUNT=107**（原为 98）。Core 新增了 9 个新类型（98-106）。
- **用户 ID 范围：107–127**（原为 98–127）。因新增内建稳态和通道类型而移位。
- 新增内建类型（98-106）：PMCA、NCX、KCC2、Ca_Leak、Ca_Buffer、SK、BK、NaP、Kir
- 所有新类型：GHK + 直接离子移动（无 I=g*(V-E_rev)）
- 兼容 Core v0.13.0 和 Studio v0.20.0

## v0.3.0 — 与 Core v0.11.0 兼容

**日期：** 2026 年 6 月

- 用户 ID 范围：93–127（Core MOD_TYPE_COUNT=98）
- **Segment 修复**：新增 `intra_nt[NT_COUNT]`（56 字节）— 与 Core 的 988 字节二进制兼容
- `MAX_INTRACONNECTIONS_PER_SEGMENT`（16）取代魔数
- `static_assert(sizeof(Segment) == 988)` 和 `sizeof(Modifier) == 44)` 用于防止失同步
- 示例：ID 93（Ca 通道）、ID 94（D2 受体）

## v0.2.0 — 与 Core v0.10.0 兼容

**日期：** 2026 年 6 月

- 用户 ID 范围：93–127（原为 64–127）
- 内建类型扩展至 0–92（新增巩固修饰器 87-92）
- 兼容 BION v5 和 Nucleus 结构

## v0.1.0 — SDK 首个版本

**日期：** 2026 年 5 月

### 概述
- `VERSION` — 包含 SDK 版本（0.1.0）的文件，由 Studio 读取用于显示

### plugin-devkit.h
- 用于插件开发的单一头文件
- 所有 POD 类型：`Modifier`、`Segment`、`Vec3`，使用 `#pragma pack(1)`
- 常量：5 种离子（`ION_*`）、14 种神经递质（`NT_*`）、物理常量（`FARADAY_CONST`、`GAS_CONST`、`CELSIUS_37`、`PI`）
- `extern "C"` 注册 API：`register_modifier_current()`、`register_modulator_release()`
- 宏：`BIOCORE_PLUGIN`、`BIOCORE_MOD_CURRENT`、`BIOCORE_MOD_RELEASE`、`BIOCORE_DESC_FULL`
- 辅助函数：`biocore_nernst()`、`biocore_segment_area()`、`biocore_clamp_ions()`

### biocore-gen-dispatch.py
- 从 `.desc` 文件生成 `dispatch_generated.cpp`
- 两种模式：`.cpp`（CPU，extern）和 `.h`（GPU，static inline）
- `--stub` 模式用于生成无插件的占位文件
- 通过 `--plugins-dir` 支持插件目录
- 过滤用户类型（64–127），跳过内建类型（0–62）
- 通过 `--core-path` 自动确定输出

### biocore-plugin.cmake
- `biocore_add_plugin()` — 通过 GCC（非 Intel dpcpp）构建 .so 插件
- `biocore_gen_dispatch()` — 运行 dispatch 生成器
- `biocore_find_sdk()` — 自动确定 SDK 路径
- 支持 `DESC` 和 `OUTPUT_DIR` 参数
- 聚合目标 `user_plugins`

### 模板
- `plugin_template.cpp` — 带 SDK 宏的插件基础
- `plugin_template.desc` — JSON 描述符

### 示例
- `example_ca_channel` — 离子型 Ca²⁺ 通道（类型 64，无 release）
- `example_d2_receptor` — 代谢型 D2 样受体（类型 65，带 release 调节器）
- 用于构建示例的 CMakeLists.txt

### 文档
- `read_me.md` — SDK 说明和快速开始
- `install.md` — 安装与集成说明
- `CLAUDE.md` — 面向 AI 助手的技术文档

### 集成
- 兼容 BioNeuronCore v0.6.0
- 在 BioNeuronStudio 中受支持（路径配置、dispatch 重建按钮、插件显示）
- 用户类型的 ID 范围：64–127
- 通过 .so 插件的 CPU 路径 — 无需重建 Core
- 通过 dispatch_generated.h 的 GPU 路径 — 计划在未来版本中实现


## 权利与知识产权

© 2024–2026 **Фешин Виталий Александрович** / Vitaly Alexandrovich Feshin. All rights reserved.
E-mail: feshin2004@mail.ru · vfeshin@mindcopy.ru · 电话：+7(918)345-75-18. Краснодар, Россия / Krasnodar, Russia.

计算机程序国家注册证书（Rospatent）**编号 727894_683451**。
未经作者书面许可，禁止商业使用、分发和再许可。
全文请参阅平台根目录下的 `LICENSE` 和 `DISCLAIMER.md`。
