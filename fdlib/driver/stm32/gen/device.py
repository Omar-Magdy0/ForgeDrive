STM32_FAMILIES = {
    # Original mainstream families
    "STM32F0": {
        "cpu_core": "CORTEX_M0",
        "cpu_arch": "ARMV6_M",
        "fpu": None,
        "has_dsp": False,
        "has_mpu": False,
        "dma_arch": "channel",
    },
    "STM32F1": {
        "cpu_core": "CORTEX_M3",
        "cpu_arch": "ARMV7_M",
        "fpu": None,
        "has_dsp": False,
        "has_mpu": True,
        "dma_arch": "channel",
    },
    "STM32F2": {
        "cpu_core": "CORTEX_M3",
        "cpu_arch": "ARMV7_M",
        "fpu": None,
        "has_dsp": False,
        "has_mpu": True,
        "dma_arch": "stream",
    },
    "STM32F3": {
        "cpu_core": "CORTEX_M4",
        "cpu_arch": "ARMV7E_M",
        "fpu": "FPV4_SP_D16",
        "has_dsp": True,
        "has_mpu": True,
        "dma_arch": "channel",
    },
    "STM32F4": {
        "cpu_core": "CORTEX_M4",
        "cpu_arch": "ARMV7E_M",
        "fpu": "FPV4_SP_D16",
        "has_dsp": True,
        "has_mpu": True,
        "dma_arch": "stream",
    },
    "STM32F7": {
        "cpu_core": "CORTEX_M7",
        "cpu_arch": "ARMV7E_M",
        "fpu": "FPV5_SP_D16",
        "has_dsp": True,
        "has_mpu": True,
        "dma_arch": "stream",
    },

    # Low-power families
    "STM32L0": {
        "cpu_core": "CORTEX_M0_PLUS",
        "cpu_arch": "ARMV6_M",
        "fpu": None,
        "has_dsp": False,
        "has_mpu": False,
    },
    "STM32L1": {
        "cpu_core": "CORTEX_M3",
        "cpu_arch": "ARMV7_M",
        "fpu": None,
        "has_dsp": False,
        "has_mpu": True,
    },
    "STM32L4": {
        "cpu_core": "CORTEX_M4",
        "cpu_arch": "ARMV7E_M",
        "fpu": "FPV4_SP_D16",
        "has_dsp": True,
        "has_mpu": True,
    },
    "STM32L4_PLUS": {
        "cpu_core": "CORTEX_M4",
        "cpu_arch": "ARMV7E_M",
        "fpu": "FPV4_SP_D16",
        "has_dsp": True,
        "has_mpu": True,
    },

    # Modern mainstream families
    "STM32C0": {
        "cpu_core": "CORTEX_M0_PLUS",
        "cpu_arch": "ARMV6_M",
        "fpu": None,
        "has_dsp": False,
        "has_mpu": False,
    },
    "STM32G0": {
        "cpu_core": "CORTEX_M0_PLUS",
        "cpu_arch": "ARMV6_M",
        "fpu": None,
        "has_dsp": False,
        "has_mpu": False,
    },
    "STM32G4": {
        "cpu_core": "CORTEX_M4",
        "cpu_arch": "ARMV7E_M",
        "fpu": "FPV4_SP_D16",
        "has_dsp": True,
        "has_mpu": True,
    },

    # Modern security-capable families
    "STM32L5": {
        "cpu_core": "CORTEX_M33",
        "cpu_arch": "ARMV8_M_MAIN",
        "fpu": "FPV5_SP_D16",
        "has_dsp": True,
        "has_mpu": True,
        "has_trustzone": True,
    },
    "STM32U0": {
        "cpu_core": "CORTEX_M0_PLUS",
        "cpu_arch": "ARMV6_M",
        "fpu": None,
        "has_dsp": False,
        "has_mpu": False,
    },
    "STM32U3": {
        "cpu_core": "CORTEX_M33",
        "cpu_arch": "ARMV8_M_MAIN",
        "fpu": "FPV5_SP_D16",
        "has_dsp": True,
        "has_mpu": True,
        "has_trustzone": True,
    },
    "STM32U5": {
        "cpu_core": "CORTEX_M33",
        "cpu_arch": "ARMV8_M_MAIN",
        "fpu": "FPV5_SP_D16",
        "has_dsp": True,
        "has_mpu": True,
        "has_trustzone": True,
    },
    "STM32H5": {
        "cpu_core": "CORTEX_M33",
        "cpu_arch": "ARMV8_M_MAIN",
        "fpu": "FPV5_SP_D16",
        "has_dsp": True,
        "has_mpu": True,
        "has_trustzone": True,
    },
}
import re

def device_meta(ctx):
    device = ctx["device"]
    match = re.fullmatch(r"STM32([A-Z])(\d)(\d{2})([A-Z])([A-Z0-9]+)", device["name"], re.I)
    if not match:
        ctx.setdefault("error", []).append(f"Unsupported STM32 device name: {device['name']}")
        return
    letter, series, product, pin_code, density = match.groups()
    device.update(family=f"{letter}{series}", product=f"{series}{product}", pin_code=pin_code, density_code=density)
    family = ("STM32" + device["family"]).upper()
    part = device["product"]
    family_meta = STM32_FAMILIES.get(family)
    if family_meta is None:
        ctx["error"].append(
            f"Unsupported STM32 family: {family}"
        )
        return

    # Part-specific data takes precedence over family defaults.

    meta = {
        **family_meta,
    }
    device.update(meta)
    device["vendor"] = "STMicroelectronics"
    device["has_fpu"] = device.get("fpu") is not None
    device["cpu_word_bits"] = 32
