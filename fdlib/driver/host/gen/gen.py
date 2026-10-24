import re
from pathlib import Path

platform_dir = Path(__file__).resolve().parent.parent

def setup(ctx):
    #add the cmake file needed
    #get the product code, flash/ density and so on letters and variants and add them to device dictionary
    device = ctx["device"]
    ctx.setdefault("gen", {}).update({
        "CMakeLists.txt": None,
        "toolchain.cmake": None,
    })
