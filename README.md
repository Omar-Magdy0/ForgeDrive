# ForgeDrive

**An open, modular software foundation for electric drives and embedded control.**

ForgeDrive brings together motor-control firmware, portable driver generation, reusable embedded libraries, and desktop tools for working with ForgeDrive devices. The project aims to make drive development easier to understand, adapt, simulate, and extend across hardware platforms.

The repository is open to contributions, ideas, bug reports, and platform ports.

## Project areas

| Directory | What it contains |
| --- | --- |
| [`ForgeDriveMC/`](ForgeDriveMC/) | Motor-control firmware, including PMSM control, platform integration, and target configuration. Supported platforms include STM32 targets and a native **Host** build for Software-in-the-Loop (SIL) simulation. |
| [`ForgeTool/`](ForgeTool/) | Desktop application for communicating with and inspecting ForgeDrive devices. |
| [`fdlib/`](fdlib/) | ForgeDrive middleware, core utilities, math, data structures, and the portable driver interface. |
| [`test/`](test/) | Unit tests for selected `fdlib` components and middleware. |
| [`thirdparty/`](thirdparty/) | Third-party libraries and assets used by the firmware, desktop tool, and tests. Each component retains its own license and notices. |
| [`Doc/`](Doc/) | Project and platform documentation. |
| [`setup.sh`](setup.sh) | Ubuntu/Debian environment setup for the toolchains and dependencies used in this repository. |

## A portable, generated driver layer

The driver layer in `fdlib/driver` is designed to separate reusable driver interfaces from target-specific hardware implementation. It builds on vendor HALs and CMSIS where appropriate, and uses Python with Jinja2 templates to generate device-specific driver code from a configuration.

For the motor-control firmware, [`ForgeDriveMC/fd_driver_conf.py`](ForgeDriveMC/fd_driver_conf.py) describes configured peripherals and resources. During CMake configuration, the driver generator uses this configuration and the selected device to produce the driver files consumed by the build. This keeps hardware mappings explicit while allowing the same higher-level code to work with different targets and host simulation.

The current firmware build defaults to `STM32F401xC`. Supported device names and platform details are determined by the generator and platform files under `fdlib/driver`.

The **Host** platform builds the control application for the development computer and connects it to the SIL platform implementation. Select it with `-DDEVICE=host`:

```bash
cmake -S ForgeDriveMC -B ForgeDriveMC/build-host -DDEVICE=host
cmake --build ForgeDriveMC/build-host --parallel
```

Use the Host build to develop and exercise control logic in simulation without an STM32 target. It does not replace validation on the intended hardware.

## Getting started

### Install development dependencies

On Ubuntu or Debian, run:

```bash
./setup.sh
source venv/bin/activate
```

The script installs system packages, the ARM embedded toolchain, and Python packages used for code generation and protocol tools. It uses `sudo` for system package installation.

### Build the desktop tool

```bash
cmake -S ForgeTool -B ForgeTool/build
cmake --build ForgeTool/build --parallel
```

The executable is placed at `ForgeTool/build/ForgeTool`. The desktop build uses CMake, a C++23 compiler, OpenGL, GLFW, libusb, Protobuf, and the Asio headers.

### Build motor-control firmware

Activate the environment created by `setup.sh`, then configure and build:

```bash
source venv/bin/activate
cmake -S ForgeDriveMC -B ForgeDriveMC/build
cmake --build ForgeDriveMC/build --parallel
```

The default device is set in `ForgeDriveMC/CMakeLists.txt`. To select another supported target, pass it at configure time, for example:

```bash
cmake -S ForgeDriveMC -B ForgeDriveMC/build -DDEVICE=STM32F401xC
```

The build generates configured drivers and protocol sources as part of the CMake configure step. Review the target configuration and generated output before flashing firmware to hardware.

### Build and run unit tests

```bash
cmake -S test -B test/build
cmake --build test/build --parallel
ctest --test-dir test/build --output-on-failure
```

The test project uses Catch2 through CMake FetchContent, so its initial configure may need network access to retrieve Catch2.

## Contributing

Contributions are welcome. You can help by reporting issues, improving documentation, adding tests, refining the driver generator, or contributing support for additional platforms and devices.

When opening a change, please include enough context to explain the problem or use case, keep changes focused, and describe how you verified them. For hardware-dependent changes, note the target board and the checks performed on real hardware or in simulation. Please preserve third-party license and attribution notices when modifying vendored code.

## Acknowledgements

ForgeDrive builds on the work of many open-source projects. Special thanks to:

- [ETL — Embedded Template Library](https://github.com/ETLCPP/etl)
- [Protocol Buffers](https://github.com/protocolbuffers/protobuf) and [nanopb](https://github.com/nanopb/nanopb)
- [Dear ImGui](https://github.com/ocornut/imgui) and [ImPlot](https://github.com/epezent/implot)

The repository also includes and uses projects such as FreeRTOS, CMSIS, CMSIS-DSP, Catch2, and other libraries and assets. See the relevant directories under [`thirdparty/`](thirdparty/) for their source files, license texts, and attribution details.

## Contact

- **Project maintainer:** Omar Magdy
- **Email:** omar.magdy.om.om@gmail.com

## License

ForgeDrive's original project code is offered under a **dual licensing** model:

- Open-source projects may use, modify, and distribute the covered ForgeDrive code under **GNU GPLv3-only**, subject to its terms. See [`LICENSES/GPL-3.0-only.txt`](LICENSES/GPL-3.0-only.txt).
- Closed-source products and other uses that cannot comply with GPLv3 require a separate written commercial license. See [`docs/licensing.md`](docs/licensing.md) for the scope and contact details.

This project license does not replace or relicense third-party components. Code and assets under [`thirdparty/`](thirdparty/) retain their own licenses and notices; review them individually. Generated output may include or depend on third-party code, so check the relevant notices and license terms when distributing a build.
