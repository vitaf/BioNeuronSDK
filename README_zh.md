# BioNeuronSDK — 插件开发者工具包

## 用途

BioNeuronSDK 是一套用于为计算引擎 **BioNeuronCore** 创建自定义修饰器（插件）的工具。SDK 使您无需重新编译内核即可开发离子通道、神经递质受体、泵和突触前调节器。

## SDK 提供的内容

- **plugin-devkit.h** — 包含所有类型、常量和辅助函数的单一头文件。只需包含一个文件，即可获得插件开发所需的一切。
- **biocore-gen-dispatch.py** — 从 `.desc` 描述符生成 GPU/SYCL 分发代码的生成器。自动将插件接入 GPU 路径。
- **CMake 模块** — `biocore_add_plugin()` 和 `biocore_gen_dispatch()` 函数，用于集成到构建系统。
- **模板和示例** — 5 分钟创建插件的现成基础。

## 快速开始

### 1. 从模板创建插件

```bash
cp BioNeuronSDK/templates/plugin_template.cpp my_plugin.cpp
cp BioNeuronSDK/templates/plugin_template.desc my_plugin.desc
```

### 2. 编辑模板

在 `my_plugin.cpp` 中：
- 将 `M_PLUGIN_ID` 替换为您的类型的 ID（124–127）
- 实现电流函数 `MyModCurrent(Modifier* m, Segment* s, float dt)`
- 如需要，添加突触前调节器 `MyModRelease`
- 在 `BIOCORE_DESC_FULL` 中填写元数据

在 `my_plugin.desc` 中：
- 指定 `id`、`name`、`category`、`description`、`release`

### 3. 构建 .so

```bash
g++ -std=c++17 -fPIC -shared -O2 -Wall \
    -I<BioNeuronSDK>/include -I<BioNeuronCore>/include \
    -o my_plugin.so my_plugin.cpp
```

重要：**仅限 GCC**，不要使用 Intel dpcpp。插件通过 `dlopen` 从 Studio（GCC 构建）加载。

### 4. 安装插件

将 `.so` 和 `.desc` 复制到 BioNeuronCore 的插件目录：

```bash
cp my_plugin.so my_plugin.desc <BioNeuronCore>/plugins/
```

### 5. CPU 路径（无需重新构建）

插件将在仿真启动时由内核自动加载。电流函数通过 `modifier_current_table[]` 调用 — 无需重新构建 Core。

### 6. GPU 路径（可选）

要为自定义修饰器启用 GPU 加速：

```bash
python3 BioNeuronSDK/src/biocore-gen-dispatch.py <plugins_dir>/ \
    --header -o dispatch_generated.h
```

将 `dispatch_generated.h` 复制到 `BioNeuronCore/src/`，在单独的头文件中提供该函数的 `static inline` 版本，然后使用 dpcpp 重新构建 Core。

## SDK 组成

| 组件 | 文件 | 用途 |
|-----------|------|-----------|
| DevKit | `include/plugin-devkit.h` | 类型（Modifier、Segment、Vec3）、常量（ION_*、NT_*）、宏（BIOCORE_PLUGIN、BIOCORE_MOD_CURRENT、BIOCORE_MOD_RELEASE）、辅助函数（nernst、area、clamp） |
| 生成器 | `src/biocore-gen-dispatch.py` | 扫描 `.desc`，生成 `dispatch_generated.cpp`（CPU extern）或 `dispatch_generated.h`（GPU inline） |
| CMake | `cmake/biocore-plugin.cmake` | `biocore_add_plugin()`、`biocore_gen_dispatch()`、`biocore_find_sdk()` |
| 模板 | `templates/` | `plugin_template.cpp` + `plugin_template.desc` — 新插件的基础 |
| 示例 | `examples/` | `example_ca_channel`（离子型）和 `example_d2_receptor`（代谢型 + release） |

## API：关键宏

```cpp
#include "plugin-devkit.h"

// 入口点 — 向 Core 注册函数
BIOCORE_PLUGIN("名称", "1.0.0", "作者") {
    BIOCORE_MOD_CURRENT(124, my_current_func);     // 电流函数
    BIOCORE_MOD_RELEASE(124, my_release_func);     // 突触前调节器（可选）
    return 2;                                      // 已注册函数的数量
}

// 插件 JSON 描述符
BIOCORE_DESC_FULL(
    "{"
    "\"name\":\"我的 Ca 通道\","
    "\"version\":\"1.0.0\","
    "\"author\":\"作者\","
    "\"modifiers\":[{"
        "\"id\":124,"
        "\"name\":\"Ca 通道\","
        "\"category\":\"离子型\","
        "\"description\":\"电压门控 Ca2+ 通道\","
        "\"release\":false"
    "}]"
    "}"
);
```

## plugin-devkit.h 中的辅助函数

| 函数 | 用途 |
|---------|-----------|
| `biocore_nernst(intra, extra, valence)` | 能斯特平衡电位（mV） |
| `biocore_segment_area(seg)` | 片段表面积（μm²） |
| `biocore_clamp_ions(arr, count)` | 防止浓度漂移到零 |

## 常量

- **离子：** `ION_NA` (0)、`ION_K` (1)、`ION_CA` (2)、`ION_CL` (3)、`ION_MG` (4)
- **神经递质：** `NT_GLUTAMATE` (0) ... `NT_MELATONIN` (11)、`NT_MAGNESIUM` (12)、`NT_NICOTINE` (13)，共 14 种
- **物理常量：** `FARADAY_CONST` (96485.0)、`GAS_CONST` (8.314)、`CELSIUS_37` (310.15K)、`PI`

## 类型 ID

| 范围 | 用途 |
|----------|-----------|
| 0–123 | 内建类型（位于 Core 的 `kernel_dispatch.h` 中） |
| **124–127** | **自定义类型**（您的插件） |

## CPU 与 GPU

| 路径 | 机制 | 重新构建 Core |
|------|----------|-----------------|
| **CPU** | `.so` → `dlopen` → `modifier_current_table[128]` | 否 |
| **GPU** | 带 `static inline` 的 `dispatch_generated.h` → dpcpp | 是 |

未生成 dispatch 的自定义类型在 GPU 上返回 0.0f（无电流）。要为自定义插件启用 GPU 加速，请使用生成器的 `--header` 模式。

## .desc 格式

```json
{
  "name": "插件名称",
  "version": "1.0.0",
  "author": "作者",
  "type_id": 124,
  "modifiers": [
    {
      "id": 93,
      "name": "修饰器",
      "category": "类别",
      "description": "描述",
      "release": false
    }
  ]
}
```

字段：`id`（124–127）、`name`、`category`、`description`、`release`（若存在突触前调节器则为 true）。

## 测试

**总计：4 个测试，100% PASS。** Valgrind：无泄漏。

| 测试 | 检查内容 | 检查项数 |
|------|--------------|----------|
| `test_plugin_devkit` | plugin-devkit.h：biocore_nernst、biocore_segment_area、biocore_clamp_ions、常量、POD 尺寸 | 13 |
| `test_example_plugins` | 示例插件的 dlopen/dlsym、biocore_plugin_init/desc 符号、.desc 文件 | 4 |
| `test_cmake_module` | biocore-plugin.cmake：.so 构建、.desc 复制、gen_dispatch、find_sdk、无 DESC | 6 |
| `test_dispatch_gen` | biocore-gen-dispatch.py 的 Python 测试 | 生成 |

运行：
```bash
cd BioNeuronSDK/build
cmake .. -DBUILD_SDK_TESTS=ON && ctest --output-on-failure
```

---

## 权利与知识产权

© 2024–2026 **Фешин Виталий Александрович** / Vitaly Alexandrovich Feshin. All rights reserved.
E-mail: feshin2004@mail.ru · vfeshin@mindcopy.ru · 电话：+7(918)345-75-18. Краснодар, Россия / Krasnodar, Russia.

计算机程序国家注册证书（Rospatent）**编号 727894_683451**。
未经作者书面许可，禁止商业使用、分发和再许可。
全文请参阅平台根目录下的 `LICENSE` 和 `DISCLAIMER.md`。
