# Fprime Zephyr Integration

This package integrates F Prime and Zephyr RTOS allowing F Prime applications to run on the Zephyr RTOS! This document will walk through how to construct an F Prime deployment that works with this package.

## Basic Setup

Add this module as a submodule to your project (typically in `lib/fprime-zephyr`) and update the `library_locations` key in `settings.ini` to point to it.

```ini
[fprime]
...
library_locations: ./lib/fprime-zephyr:...
```

Finally, users are encouraged to install the `west` Python package. This is used by Zephyr to fetch support modules, and much more!

```bash
pip install west
```

> [!TIP]
> Remember to source your project's virtual environment.

## Recommended Project Structure and Required Files

In order to ease developent and support of F Prime + Zephyr, we recommend projects follow this recommended directory structure. 

```bash
project-folder/
│
├── proj.conf                # REQUIRED: Zephyr Kconfig options for this app
├── west.yml                 # REQUIRED: West manifest (declares external modules)
├── CMakeLists.txt           # Top-level CMake entry for the project (created by fprime-bootstrap)
│
├── .west/                   # West workspace metadata
│   └── config               # REQUIRED: west configuration for this structure
│
├── Project/                 # Your F Prime project 
│
├── lib/
│   ├── fprime/              # Core F´ framework (created by fprime-bootstrap)
│   ├── fprime-zephyr/       # Zephyr platform support for F Prime (added as submodule)
│   └── zephyr-workspace/    # Zephyr + modules workspace (created by west update)
│
└── boards/                  # OPTIONAL: custom board definitions and overlays
```

The initial `fprime-bootstrap` and `git submodule add` steps to include this directory in a new project will create most of this structure. Users must create the following files. `fprime-zephyr` provides initial samples for each of these files

1. [`proj.conf`](./sample-config/proj.conf): contains the Zephyr KConfig options for the project
2. [`west.yml`](./sample-config/west.yml): configuration for the extra modules used by Zephyr
3. [`.west/config`](./sample-config/.west/config): `west` tool configuration setup to work with the above recommended structure
4. (Optional)[`boards/<overlay>.overlay](./sample-config/boards/rpi_pico2_rp2350a_m33.overlay): Board specific overlay files for configuring the Zephyr Device Tree
