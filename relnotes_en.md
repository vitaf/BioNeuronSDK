# Release Notes — BioNeuronSDK

## v0.6.2 (2026-07-01) — Agent Skills + dispatch fix
- **Agent Skills**: DDD methodology + debugging triage (SDK specifics: dispatch, dlopen, GCC-only) + planning workflow in CLAUDE.md
- **biocore-gen-dispatch fix**: generates weak symbols instead of extern declarations — prevents link-time conflicts
- **GPU compatibility**: GPU backend compatibility documented in install.md
- **Compatibility with Core v0.19.5** (CUDA two-pass diffusion fix)

## v0.6.1 (2026-06-25) — Compatibility with Core v0.19.4
- Compatibility with the unified log format unixtime [LEVEL] msg
- Explicit GCC-only plugin builds (no Intel dependencies)

## v0.6.0 — BION v6 + final shift of custom IDs

**Date:** 16 June 2026

- **MOD_TYPE_COUNT=121** (Core v0.16.7). Core added sensory modifiers (115-120).
- **Custom ID range: 121–127** (was 93–127).
- **Full BION v6 compatibility**: `Nucleus` 332 bytes, `NetworkHeader` 84 bytes, `Segment` 988 bytes.
- All struct static_asserts match Core v0.16.7.
- Examples and templates updated: ID 121 (Ca-channel), ID 122 (D2-receptor).
- **Tests (18.06.2026):** +2 tests — `test_sdk_security` (.desc JSON validation, 8 checks), `test_sdk_integration` (full plugin cycle, 3 checks). SDK: 6/6 tests.

## v0.5.0-v0.4.0 — CMake module tests + shift to MOD_TYPE_COUNT=107

**Date:** 10 June 2026

**v0.5.0:**
- test_cmake_module.cpp: 6 tests of biocore-plugin.cmake (.so build, dispatch, find_sdk)
- test_example_plugins.cpp: 4 dlopen/dlsym plugin tests
- Total: 4/4 tests, 100% PASS
- Valgrind: no leaks

**v0.4.0:**
- **MOD_TYPE_COUNT=107** (was 98). Core added 9 new types (98-106).
- **Custom ID range: 107–127** (was 98–127). Shift due to new built-in homeostasis and channel types.
- New built-in types (98-106): PMCA, NCX, KCC2, Ca_Leak, Ca_Buffer, SK, BK, NaP, Kir
- Compatible with Core v0.13.0 and Studio v0.20.0

**Date:** 10 June 2026

- test_cmake_module.cpp: 6 tests of biocore-plugin.cmake (.so build, dispatch, find_sdk)
- test_example_plugins.cpp: 4 dlopen/dlsym plugin tests
- Total: 4/4 tests, 100% PASS
- Valgrind: no leaks

## v0.4.0 — Compatibility with Core v0.13.0

**Date:** June 2026

- **MOD_TYPE_COUNT=107** (was 98). Core added 9 new types (98-106).
- **Custom ID range: 107–127** (was 98–127). Shift due to new built-in homeostasis and channel types.
- New built-in types (98-106): PMCA, NCX, KCC2, Ca_Leak, Ca_Buffer, SK, BK, NaP, Kir
- All new types: GHK + direct ion movement (no I=g*(V-E_rev))
- Compatible with Core v0.13.0 and Studio v0.20.0

## v0.3.0 — Compatibility with Core v0.11.0

**Date:** June 2026

- Custom ID range: 93–127 (Core MOD_TYPE_COUNT=98)
- **Segment fix**: added `intra_nt[NT_COUNT]` (56 bytes) — 988-byte binary compatibility with Core
- `MAX_INTRACONNECTIONS_PER_SEGMENT` (16) instead of a magic number
- `static_assert(sizeof(Segment) == 988)` and `sizeof(Modifier) == 44)` to guard against desynchronization
- Examples: ID 93 (Ca-channel), ID 94 (D2-receptor)

## v0.2.0 — Compatibility with Core v0.10.0

**Date:** June 2026

- Custom ID range: 93–127 (was 64–127)
- Built-in types extended to 0–92 (added consolidation modifiers 87-92)
- Compatible with BION v5 and Nucleus structures

## v0.1.0 — First SDK release

**Date:** May 2026

### General
- `VERSION` — file with the SDK version (0.1.0), read by Studio for display

### plugin-devkit.h
- Single header file for plugin development
- All POD types: `Modifier`, `Segment`, `Vec3` with `#pragma pack(1)`
- Constants: 5 ions (`ION_*`), 14 neurotransmitters (`NT_*`), physical (`FARADAY_CONST`, `GAS_CONST`, `CELSIUS_37`, `PI`)
- `extern "C"` registration API: `register_modifier_current()`, `register_modulator_release()`
- Macros: `BIOCORE_PLUGIN`, `BIOCORE_MOD_CURRENT`, `BIOCORE_MOD_RELEASE`, `BIOCORE_DESC_FULL`
- Helpers: `biocore_nernst()`, `biocore_segment_area()`, `biocore_clamp_ions()`

### biocore-gen-dispatch.py
- Generator of `dispatch_generated.cpp` from `.desc` files
- Two modes: `.cpp` (CPU, extern) and `.h` (GPU, static inline)
- `--stub` mode for generating a stub without plugins
- Plugin directory support via `--plugins-dir`
- Filtering of custom types (64–127); built-in ones (0–62) are skipped
- Automatic output detection via `--core-path`

### biocore-plugin.cmake
- `biocore_add_plugin()` — builds a .so plugin via GCC (not Intel dpcpp)
- `biocore_gen_dispatch()` — runs the dispatch generator
- `biocore_find_sdk()` — auto-detects SDK paths
- Support for `DESC` and `OUTPUT_DIR` parameters
- Aggregate target `user_plugins`

### Templates
- `plugin_template.cpp` — plugin base with SDK macros
- `plugin_template.desc` — JSON descriptor

### Examples
- `example_ca_channel` — ionotropic Ca²⁺ channel (type 64, no release)
- `example_d2_receptor` — metabotropic D2-like receptor (type 65, with release modulator)
- CMakeLists.txt for building examples

### Documentation
- `read_me.md` — SDK description and quickstart
- `install.md` — installation and integration instructions
- `CLAUDE.md` — technical documentation for the AI assistant

### Integration
- Compatible with BioNeuronCore v0.6.0
- Support in BioNeuronStudio (path configuration, dispatch rebuild button, plugin display)
- ID range: 64–127 for custom types
- CPU path via .so plugins — no Core rebuild
- GPU path via dispatch_generated.h — planned for future versions


## Rights and intellectual property

© 2024–2026 **Фешин Виталий Александрович** / Vitaly Alexandrovich Feshin. All rights reserved.
E-mail: feshin2004@mail.ru · vfeshin@mindcopy.ru · Tel.: +7(918)345-75-18. Краснодар, Россия / Krasnodar, Russia.

Certificate of state registration of the computer program (Rospatent) **No. 727894_683451**.
Commercial use, distribution, and sublicensing without the author's written permission are prohibited.
For the full text, see `LICENSE` and `DISCLAIMER.md` in the platform root.
