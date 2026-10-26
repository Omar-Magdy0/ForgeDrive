import re
from pathlib import Path
from c_parse import parse_header
from generate import FDGEN_RE

platform_dir = Path(__file__).resolve().parent.parent
BUS_ROOTS = {
    "APB1PERIPH_BASE": "APB1",
    "APB2PERIPH_BASE": "APB2",
    "AHB1PERIPH_BASE": "AHB1",
    "AHB2PERIPH_BASE": "AHB2",
    "AHB3PERIPH_BASE": "AHB3",
}
BUS_CLK_GETTER = {
    "APB1": "HAL_RCC_GetPCLK1Freq",
    "APB2": "HAL_RCC_GetPCLK2Freq",
    "AHB1": "HAL_RCC_GetHCLKFreq",
    "AHB2": "HAL_RCC_GetHCLKFreq",
    "AHB3": "HAL_RCC_GetHCLKFreq",
}
BUS_PSC_GETTER = {
    "APB1": ("LL_RCC_GetAPB1Prescaler", "LL_RCC_APB1_DIV_1"),
    "APB2": ("LL_RCC_GetAPB2Prescaler", "LL_RCC_APB2_DIV_1"),
}
PERIPH_EXCLUDE = {"FLASH", "FLASH_R"}
def resolve_periph_clock(name, macros, seen=None):
    seen = set() if seen is None else seen
    if name in seen or name not in macros:
        return None
    if name in BUS_ROOTS:
        return BUS_ROOTS[name]

    seen.add(name)
    for ref in re.findall(r"\b[A-Za-z_]\w*\b", macros[name]):
        bus = resolve_periph_clock(ref, macros, seen.copy())
        if bus is not None:
            return bus
    return None


def irq_match(ctx, irq, loc="<unknown>"):
    matches = [name for name in ctx["device"]["irq"] if irq in name]

    if not matches:
        ctx.setdefault("error", []).append(
            f"{loc}: unknown IRQ '{irq}'"
        )
        return None

    if len(matches) > 1:
        ctx.setdefault("error", []).append(
            f"{loc}: ambiguous IRQ '{irq}', matches: {', '.join(matches)}"
        )
        return None

    return matches[0]

def clk_en(ctx, loc, periph):
    clocks = ctx["device"].setdefault("clk_en", [])
    if periph not in clocks:
        clocks.append(periph)

def irq_bind(ctx, loc, irq_name, handler):
    irq = irq_match(ctx, irq_name, loc)
    bindings = ctx["device"].setdefault("irq_bind", {})
    bindings.setdefault(irq, []).append(handler)

def irq_en(ctx, loc, irq_name, prio):
    try:
        prio = int(prio)
        irq = irq_match(ctx, irq_name, loc)
    except ValueError:
        ctx.setdefault("error", []).append(
            f"{loc}: IRQ '{irq}': invalid priority '{prio}', expected an integer"
        )
        return
    priorities = ctx["device"].setdefault("irq_en", {})
    priorities[irq] = min(priorities.get(irq, prio), prio)

def emit_system(ctx):
    generated = [Path(p) for p in ctx["generated"]]
    system_c = next((p for p in generated if p.name == "System.c"), None)
    system_h = next((p for p in generated if p.name == "System.h"), None)
    if system_c is None or system_h is None:
        ctx.setdefault("error", []).append(
            "FDGEN: System.c or System.h not found in generated files"
        )
        return
    
    device = ctx["device"]
    code = []

    #CLOCK CODE GENERATED
    code.append("\nvoid SystemPeriphClock_Init(void)\n{\n")
    for periph in device.get("clk_en", []):
        code.append(f"    __HAL_RCC_{periph}_CLK_ENABLE();\n")
    code.append("}\n\n")

    code.append("void SystemNVIC_Init(void)\n{\n")
    for irq, prio in device.get("irq_en", {}).items():
        code.append(f"    NVIC_SetPriority({irq}_IRQn, {prio});\n")
        code.append(f"    NVIC_EnableIRQ({irq}_IRQn);\n")
    code.append("}\n")

    ##Now lets spit out IRQ handlers
        # IRQ HANDLERS
    bindings = device.get("irq_bind", {})
    irq_handlers = {}

    for irq, methods in bindings.items():
        resolved = irq_match(ctx, irq)
        if resolved is None:
            continue
        irq_handlers.setdefault(resolved, []).extend(methods)

    # Declare all attached methods before defining IRQ handlers
    methods = dict.fromkeys(
        method
        for handlers in irq_handlers.values()
        for method in handlers
    )
    for method in methods:
        code.append(f"\nextern void {method}(void);")

    # Emit one handler per resolved IRQ vector
    for irq, handlers in irq_handlers.items():
        code.append(f"\n\nvoid {irq}_IRQHandler(void)\n{{\n")
        for method in handlers:
            code.append(f"    {method}();\n")
        code.append("}\n")

    with open(system_c, "a", encoding="utf-8") as f:
        f.write("".join(code))
    pass

def fdgen_macro(ctx):
    handlers = {
        "CLK_EN": clk_en,
        "IRQ_BIND": irq_bind,
        "IRQ_EN": irq_en,
    }

    for file in ctx["generated"]:
        path = Path(file)
        source = path.read_text()

        for match in FDGEN_RE.finditer(source):
            name, raw_args = match.groups()
            loc = f"{path}:{source.count('\n', 0, match.start()) + 1}"

            handler = handlers.get(name)
            if handler is None:
                ctx.setdefault("error", []).append(f"{loc}: unknown FDGEN directive '{name}'")
                continue
            
            args = [arg.strip() for arg in raw_args.split(",")]
            handler(ctx, loc, *args)


def fdgen(ctx):
    fdgen_macro(ctx)
    emit_system(ctx)

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
    #parse CMSIS headers for device information
    ctx["device_def"] = parse_header(ctx["cmsis_dir"]/f"Device/ST/STM32F4xx/Include/stm32f{series}{product}x{density}.h") 
    #now we have device definitions... Awesome, we can get most information such as IRQ handler names, peripheral instances, clock buses and much more
    ctx["device"]["irq"] = [
        name.removesuffix("_IRQn")
        for name in ctx["device_def"]["enums"].get("IRQn_Type", [])
    ]
    ctx["device"]["periph"] = [
        {"name": name, "bus": bus}
        for macro in ctx["device_def"]["macros"]
        if macro.endswith("_BASE")
        if (name := macro.removesuffix("_BASE")) not in {b.removesuffix("_BASE") for b in BUS_ROOTS}
        if name not in PERIPH_EXCLUDE
        if not re.fullmatch(r"DMA\d+_Stream\d+", name)
        if (bus := resolve_periph_clock(macro, ctx["device_def"]["macros"])) is not None
    ]
    ctx["device"]["bus_clk_getter"] = BUS_CLK_GETTER
    ctx["device"]["bus_psc_getter"] = BUS_PSC_GETTER
    device.update(family=f"{letter}{series}", product=f"{series}{product}", pin_code=pin_code, density_code=density)
    ctx.setdefault("gen", {}).update({
        "CMakeLists.txt": None,
        "stm32f4xx_hal_conf.h": None,
        "System.h": None,
        "fd_adc.c": None,
        "fd_adc.h": None,
        "System.c": None,
        "toolchain.cmake": None,
        "usbd/usbd_conf.c": "usbd/usbd_conf.c",
        "usbd/usbd_conf.h": "usbd/usbd_conf.h",
        "usbd/usbd_desc.c": "usbd/usbd_desc.c",
        "usbd/usbd_desc.h": "usbd/usbd_desc.h",
    })
