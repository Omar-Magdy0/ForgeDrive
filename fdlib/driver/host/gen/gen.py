import re
from pathlib import Path

platform_dir = Path(__file__).resolve().parent.parent

def fdgen(ctx):
    device = ctx["device"]
    code = []
    for key,value in device.items():
        code.append(f"set(DEVICE_{key.upper()} \"{str(value).upper()}\")\n")
    output_path = ctx["temp_dir"]/"device.cmake"
    output_path.parent.mkdir(parents=True, exist_ok=True)
    with output_path.open("w", encoding="utf-8") as f:
        f.writelines(code)
    ctx["generated"].add(str(output_path))

def setup(ctx):
    #add the cmake file needed
    #get the product code, flash/ density and so on letters and variants and add them to device dictionary
    device = ctx["device"]
    ctx.setdefault("gen", {}).update({
        "CMakeLists.txt": None,
        "toolchain.cmake": None,
        "System.h": None,
        "System.cpp": None,
        "fd_SILServer.h": None,
        "fd_TCPClient.h": None,
    })

def env_macros(ctx):
    pass