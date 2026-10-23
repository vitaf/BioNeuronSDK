# Installing and configuring BioNeuronSDK

## System requirements

- Linux x86_64
- Python 3.6+
- GCC 9+ (for building plugins)
- BioNeuronCore (installed or built from source)
- CMake 3.20+ (optional, for the cmake module)

## Installation from a .deb package

```bash
sudo dpkg -i mindcopy-sdk_0.6.0_amd64.deb
```

Files are installed into `/opt/mindcopy/`:
- `include/plugin-devkit.h` — header for plugin development
- `bin/biocore-gen-dispatch.py` — dispatch-code generator
- `share/sdk/cmake/` — the `biocore-plugin.cmake` CMake module
- `share/sdk/templates/` — plugin templates
- `share/sdk/examples/` — plugin examples

## Loading plugins (locally / via daemon)

Plugins are compiled as `.so` files and loaded via `dlopen`:

- **Locally (STATIC mode):** Studio links `libbiocore_gcc.a` and loads plugins in its own process
- **Remotely (IPC mode):** the `biocore-daemon` daemon loads plugins in its own process. Studio connects to the daemon via a Unix socket or TCP (port 6789 by default)

The plugin API (`extern "C" register_modifier_current / register_modulator_release`) is identical for both modes — plugins do not need to know which process they are loaded into.

## Environment variables

- `BIOCORE_ROOT` — path to BioNeuronCore (optional)
- `BIOCORE_SDK_DIR` — path to the SDK (optional)

## GPU compatibility of plugins

Plugins are compiled with GCC and loaded via dlopen — the GPU backend does not affect plugins.
To choose a GPU backend for the Core, see [Core install.md](../BioNeuronCore/install.md#выбор-gpu-бекенда).

## Verification

```bash
python3 /opt/mindcopy/bin/biocore-gen-dispatch.py --stub
```


## Rights and intellectual property

© 2024–2026 **Фешин Виталий Александрович** / Vitaly Alexandrovich Feshin. All rights reserved.
E-mail: feshin2004@mail.ru · vfeshin@mindcopy.ru · Tel.: +7(918)345-75-18. Краснодар, Россия / Krasnodar, Russia.

Certificate of state registration of the computer program (Rospatent) **No. 727894_683451**.
Commercial use, distribution, and sublicensing without the author's written permission are prohibited.
For the full text, see `LICENSE` and `DISCLAIMER.md` in the platform root.
