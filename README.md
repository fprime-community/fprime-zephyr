# Fprime Zephyr Support Package

This package integrates F Prime and Zephyr RTOS allowing F Prime applications to run on the Zephyr RTOS! This document will walk through how to construct an F Prime deployment that works with this package.

## Basic Setup

Add this module as a submodule to your project (typically in `lib/fprime-zephyr`) and update the `library_locations` key in `settings.ini` to point to it.

```ini
[fprime]
...
library_locations: ./lib/fprime-zephyr: ...
```

Finally, you should install the `west` Python package in your virtual environment. This is used by Zephyr to fetch support modules, and much more!

```bash
pip install west
```

> [!TIP]
> Remember to source your project's virtual environment before running the above command.

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

The initial `fprime-bootstrap` and `git submodule add` steps to include this directory in a new project will create most of this structure. Users must create the following files.

1. [`proj.conf`](./sample-config/proj.conf): contains the Zephyr KConfig options for the project
2. [`west.yml`](./sample-config/west.yml): configuration for the extra modules used by Zephyr
3. [`.west/config`](./sample-config/_dot_west/config): `west` tool configuration setup to work with the above recommended structure

> [!TIP]
> `fprime-zephyr` provides initial samples for each of these files as linked above.

> [!CAUTION]
> Make sure to put the sample in a hidden `.west` directory, as `west` requires this for its configuration.

4. (Optional)[`boards/<overlay>.overlay`](./sample-config/boards/rpi_pico2_rp2350a_m33.overlay): Board specific overlay files for configuring the Zephyr Device Tree

> [!CAUTION]
> The overlay provided is specific to the RPI Pico 2's M33 processor. Projects should configure their overlay files specifically to their board and usecase. See: [Zephyr Device Trees](https://docs.zephyrproject.org/latest/build/dts/index.html) for the whole story.

## Installing Zephyr

This step will install Zephyr, its modules, and the necessary Python tooling using your `west.yml` as the intial configuration. If you followed the recommended structure, installing Zephyr should be as easy as:

```
cd project-folder
west update
west packages pip --install
west sdk install
```

> [!TIP]
> These commands will take quite some time to run.

## Integrating Zephyr's CMake with F Prime

In order to integrate Zephyr and F Prime CMake systems, you should add the following lines to your top-level `CMakeLists.txt` file, before the `project()` call:

```cmake
# Enable Zephyr when not performing a unit testing build
if (NOT BUILD_TESTING)
    find_package(Zephyr HINTS "${CMAKE_CURRENT_LIST_DIR}/lib/zephyr-workspace")
endif()
project(...)
...
```

## Adding Zephyr Deployments

In order to turn an F Prime deployment into a Zephyr deployment, you need to add the following calls to your deployment's `CMakeLists.txt` file:

```cmake
register_fprime_zephyr_deployment(
        ${FPRIME_CURRENT_MODULE}
    SOURCES
        "${CMAKE_CURRENT_LIST_DIR}/Main.cpp"
    DEPENDS
        ${FPRIME_CURRENT_MODULE}_Top
)
```

> [!TIP]
> This will add `Main.cpp` to the Zephyr `app` target source list and sets up the topology module as a dependency.

## Build

In order to build with F Prime Zephyr, users must specify the `BOARD` environment variable (used by Zephyr) and the `zephyr` toolchain used by F Prime.

```bash
fprime-util generate -DBOARD=rpi_pico2/rp2350a/m33 zephyr
fprime-util build zephyr
```
