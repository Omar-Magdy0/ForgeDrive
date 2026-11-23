
#!/usr/bin/env bash
set -euo pipefail

# ============================================================
# System packages
# ============================================================

sudo apt update
sudo apt install -y cmake ninja-build build-essential git xz-utils socat pkg-config

# ============================================================
# ARM Embedded Toolchain
# ============================================================

sudo apt install -y gcc-arm-none-eabi binutils-arm-none-eabi libnewlib-arm-none-eabi gdb-multiarch

# ============================================================
# Host C++ dependencies
# ============================================================

sudo apt install -y libboost-system-dev libboost-thread-dev libboost-date-time-dev libasio-dev

# ============================================================
# GUI, Graphics, USB and Data
# ============================================================

sudo apt install -y libglfw3-dev libgl1-mesa-dev libhdf5-dev libusb-1.0-0-dev
sudo apt install libprotobuf-dev protobuf-compiler libprotoc-dev
# ============================================================
# Python environment
# ============================================================

sudo apt install -y python3 python3-venv python3-pip
python3 -m venv venv
source venv/bin/activate
python -m pip install --upgrade pip
python -m pip install Jinja2 nanopb grpcio-tools protobuf

# ============================================================
# Verification
# ============================================================

echo "========================================"
echo "Development environment installed"
echo "========================================"

cmake --version
gcc --version
arm-none-eabi-gcc --version
gdb-multiarch --version

python --version
python -m pip list