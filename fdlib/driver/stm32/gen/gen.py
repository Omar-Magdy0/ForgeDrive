import re
from pathlib import Path

platform_dir = Path(__file__).resolve().parent.parent

def setup(ctx):
    #add the cmake file needed
    #get the product code, flash/ density and so on letters and variants and add them to device dictionary
    device = ctx["device"]
    match = re.fullmatch(r"STM32([A-Z])(\d)(\d{2})([A-Z])([A-Z0-9]+)", device["name"], re.I)
    if not match:
        ctx.setdefault("error", []).append(f"Unsupported STM32 device name: {device['name']}")
        return

    letter, series, product, pin_code, density = match.groups()
    print(platform_dir)
    
    device.update(family=f"{letter}{series}", product=f"{series}{product}", pin_code=pin_code, density_code=density)
    print(device)
    ctx.setdefault("gen", {}).update({
        "CMakeLists.txt": None,
        "stm32f4xx_hal_conf.h": None,
        "System.h": None,
        "toolchain.cmake": None,
        "usbd/usbd_conf.c": "usbd/usbd_conf.c",
        "usbd/usbd_conf.h": "usbd/usbd_conf.h",
        "usbd/usbd_desc.c": "usbd/usbd_desc.c",
        "usbd/usbd_desc.h": "usbd/usbd_desc.h",
    })
