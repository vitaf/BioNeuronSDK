# BioNeuronSDK — plugin developer toolkit

## Purpose

BioNeuronSDK is a set of tools for creating custom modifiers (plugins) for the **BioNeuronCore** compute engine. The SDK lets you develop ion channels, neurotransmitter receptors, pumps, and presynaptic modulators without recompiling the core.

## What the SDK provides

- **plugin-devkit.h** — a single header with all types, constants, and helpers. Include one file and you get everything you need for plugin development.
- **biocore-gen-dispatch.py** — a GPU/SYCL dispatch-code generator from `.desc` descriptors. Automates connecting a plugin to the GPU path.
- **CMake module** — the `biocore_add_plugin()` and `biocore_gen_dispatch()` functions for integration into the build system.
- **Templates and examples** — a ready-made foundation for creating a plugin in 5 minutes.

## Quick start

### 1. Create a plugin from the template

```bash
cp BioNeuronSDK/templates/plugin_template.cpp my_plugin.cpp
cp BioNeuronSDK/templates/plugin_template.desc my_plugin.desc
```

### 2. Edit the template

In `my_plugin.cpp`:
- Replace `M_PLUGIN_ID` with the ID of your type (124–127)
- Implement the current function `MyModCurrent(Modifier* m, Segment* s, float dt)`
- Add the presynaptic modulator `MyModRelease` if needed
- Fill in the metadata in `BIOCORE_DESC_FULL`

In `my_plugin.desc`:
- Specify `id`, `name`, `category`, `description`, `release`

### 3. Build the .so

```bash
g++ -std=c++17 -fPIC -shared -O2 -Wall \
    -I<BioNeuronSDK>/include -I<BioNeuronCore>/include \
    -o my_plugin.so my_plugin.cpp
```

Important: **GCC only**, not Intel dpcpp. Plugins are loaded via `dlopen` from Studio (a GCC build).

### 4. Install the plugin

Copy the `.so` and `.desc` into BioNeuronCore's plugin directory:

```bash
cp my_plugin.so my_plugin.desc <BioNeuronCore>/plugins/
```

### 5. CPU path (without rebuild)

The plugin is loaded automatically by the core when the simulation starts. The current function is called through `modifier_current_table[]` — no Core rebuild is required.

### 6. GPU path (optional)

To GPU-accelerate a custom modifier:

```bash
python3 BioNeuronSDK/src/biocore-gen-dispatch.py <plugins_dir>/ \
    --header -o dispatch_generated.h
```

Copy `dispatch_generated.h` into `BioNeuronCore/src/`, provide a `static inline` version of the function in a separate header, and rebuild the Core with dpcpp.

## SDK contents

| Component | File | Purpose |
|-----------|------|-----------|
| DevKit | `include/plugin-devkit.h` | Types (Modifier, Segment, Vec3), constants (ION_*, NT_*), macros (BIOCORE_PLUGIN, BIOCORE_MOD_CURRENT, BIOCORE_MOD_RELEASE), helpers (nernst, area, clamp) |
| Generator | `src/biocore-gen-dispatch.py` | Scans `.desc`, generates `dispatch_generated.cpp` (CPU extern) or `dispatch_generated.h` (GPU inline) |
| CMake | `cmake/biocore-plugin.cmake` | `biocore_add_plugin()`, `biocore_gen_dispatch()`, `biocore_find_sdk()` |
| Templates | `templates/` | `plugin_template.cpp` + `plugin_template.desc` — the basis for a new plugin |
| Examples | `examples/` | `example_ca_channel` (ionotropic) and `example_d2_receptor` (metabotropic + release) |

## API: key macros

```cpp
#include "plugin-devkit.h"

// Entry point — registers functions with the Core
BIOCORE_PLUGIN("Name", "1.0.0", "Author") {
    BIOCORE_MOD_CURRENT(124, my_current_func);     // current function
    BIOCORE_MOD_RELEASE(124, my_release_func);     // presynaptic modulator (optional)
    return 2;                                      // number of registered functions
}

// Plugin JSON descriptor
BIOCORE_DESC_FULL(
    "{"
    "\"name\":\"My Ca-channel\","
    "\"version\":\"1.0.0\","
    "\"author\":\"Author\","
    "\"modifiers\":[{"
        "\"id\":124,"
        "\"name\":\"Ca-channel\","
        "\"category\":\"Ionotropic\","
        "\"description\":\"Voltage-gated Ca2+ channel\","
        "\"release\":false"
    "}]"
    "}"
);
```

## Helpers in plugin-devkit.h

| Function | Purpose |
|---------|-----------|
| `biocore_nernst(intra, extra, valence)` | Nernst equilibrium potential (mV) |
| `biocore_segment_area(seg)` | Segment surface area (μm²) |
| `biocore_clamp_ions(arr, count)` | Prevents concentrations from drifting to zero |

## Constants

- **Ions:** `ION_NA` (0), `ION_K` (1), `ION_CA` (2), `ION_CL` (3), `ION_MG` (4)
- **Neurotransmitters:** `NT_GLUTAMATE` (0) ... `NT_MELATONIN` (11), `NT_MAGNESIUM` (12), `NT_NICOTINE` (13), 14 in total
- **Physics:** `FARADAY_CONST` (96485.0), `GAS_CONST` (8.314), `CELSIUS_37` (310.15K), `PI`

## Type IDs

| Range | Purpose |
|----------|-----------|
| 0–123 | Built-in types (in Core's `kernel_dispatch.h`) |
| **124–127** | **Custom types** (your plugins) |

## CPU vs GPU

| Path | Mechanism | Core rebuild |
|------|----------|-----------------|
| **CPU** | `.so` → `dlopen` → `modifier_current_table[128]` | No |
| **GPU** | `dispatch_generated.h` with `static inline` → dpcpp | Yes |

Custom types on the GPU without a generated dispatch return 0.0f (no current). To GPU-accelerate a custom plugin, use the generator's `--header` mode.

## .desc format

```json
{
  "name": "Plugin name",
  "version": "1.0.0",
  "author": "Author",
  "type_id": 124,
  "modifiers": [
    {
      "id": 93,
      "name": "Modifier",
      "category": "Category",
      "description": "Description",
      "release": false
    }
  ]
}
```

Fields: `id` (124–127), `name`, `category`, `description`, `release` (true if there is a presynaptic modulator).

## Testing

**Total: 4 tests, 100% PASS.** Valgrind: no leaks.

| Test | What it checks | Checks |
|------|--------------|----------|
| `test_plugin_devkit` | plugin-devkit.h: biocore_nernst, biocore_segment_area, biocore_clamp_ions, constants, POD sizes | 13 |
| `test_example_plugins` | dlopen/dlsym of example plugins, biocore_plugin_init/desc symbols, .desc files | 4 |
| `test_cmake_module` | biocore-plugin.cmake: .so build, .desc copy, gen_dispatch, find_sdk, no-DESC | 6 |
| `test_dispatch_gen` | Python tests of biocore-gen-dispatch.py | generation |

Run:
```bash
cd BioNeuronSDK/build
cmake .. -DBUILD_SDK_TESTS=ON && ctest --output-on-failure
```

---

## Rights and intellectual property

© 2024–2026 **Фешин Виталий Александрович** / Vitaly Alexandrovich Feshin. All rights reserved.
E-mail: feshin2004@mail.ru · vfeshin@mindcopy.ru · Tel.: +7(918)345-75-18. Краснодар, Россия / Krasnodar, Russia.

Certificate of state registration of the computer program (Rospatent) **No. 727894_683451**.
Commercial use, distribution, and sublicensing without the author's written permission are prohibited.
For the full text, see `LICENSE` and `DISCLAIMER.md` in the platform root.
