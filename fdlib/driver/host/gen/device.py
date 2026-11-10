def device_meta(ctx):
    import platform
    import struct
    import sys
    import os

    device = ctx["device"]
    machine = platform.machine().lower()
    system = platform.system().upper()

    # Normalize common host architecture names.
    cpu_core_map = {
        "x86_64": "X86_64",
        "amd64": "X86_64",
        "i386": "X86",
        "i686": "X86",
        "aarch64": "AARCH64",
        "arm64": "AARCH64",
        "armv7l": "ARMV7",
        "riscv64": "RISCV64",
        "riscv32": "RISCV32",
        "ppc64le": "PPC64LE",
    }

    os_map = {
        "LINUX": "LINUX",
        "WINDOWS": "WINDOWS",
        "DARWIN": "MACOS",
        "FREEBSD": "FREEBSD",
        "OPENBSD": "OPENBSD",
        "NETBSD": "NETBSD",
    }

    device.update({
        # Identity
        "vendor": None,
        "family": "HOST",
        "part": platform.node(),

        # CPU / architecture
        "cpu_core": cpu_core_map.get(machine, machine.upper()),
        "cpu_arch": machine.upper(),
        "cpu_word_bits": struct.calcsize("P") * 8,
        "pointer_bits": struct.calcsize("P") * 8,
        "endianness": sys.byteorder.upper(),

        # CPU capabilities: don't guess hardware features.
        "fpu": None,
        "has_fpu": None,
        "has_dsp": None,
        "has_mpu": None,

        # Host OS
        "os": os_map.get(system, system),
        "os_name": platform.system(),
        "os_release": platform.release(),
        "os_version": platform.version(),
        "os_machine": platform.machine(),
        "hostname": platform.node(),

        # Runtime environment
        "python_version": platform.python_version(),
        "python_implementation": platform.python_implementation(),
        "python_compiler": platform.python_compiler(),
        "python_bits": struct.calcsize("P") * 8,
        "libc": platform.libc_ver(),
        "executable": sys.executable,

        # Useful build-environment information
        "cwd": os.getcwd(),
        "cpu_count": os.cpu_count(),
    })
    # Additional Linux-specific information.
    if system == "LINUX":
        try:
            info = platform.freedesktop_os_release()
            device["distro"] = info.get("ID")
            device["distro_version"] = info.get("VERSION_ID")
            device["distro_name"] = info.get("PRETTY_NAME")
        except (AttributeError, OSError):
            device["distro"] = None
            device["distro_version"] = None
            device["distro_name"] = None