import re
from pathlib import Path
from c_parse import parse_header
from generate import FDGEN_RE, DRIVER_ROOT

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
GPIO_MODE = {
    "AF": "LL_GPIO_MODE_ALTERNATE",
    "IN": "LL_GPIO_MODE_INPUT",
    "OUT": "LL_GPIO_MODE_OUTPUT",
    "AN": "LL_GPIO_MODE_ANALOG"
}
OUTPUT_TYPE = {
    None: "LL_GPIO_OUTPUT_PUSHPULL",
    "PP": "LL_GPIO_OUTPUT_PUSHPULL",
    "OD": "LL_GPIO_OUTPUT_OPENDRAIN"
}
PULL_TYPE = {
    None: "LL_GPIO_PULL_NO",
    "PU": "LL_GPIO_PULL_UP",
    "PD": "LL_GPIO_PULL_DOWN"
}
DMA_RE = {
    "stream": re.compile(
        r"DMA(?P<dma>\d+)_STREAM(?P<stream>\d+)_CH(?P<channel>\d+)"
    ),
    "channel": re.compile(
        r"DMA(?P<dma>\d+)_CH(?P<channel>\d+)"
    ),
}

def dma_parse(cfg, arch):
    name, priority = cfg
    match = DMA_RE[arch].fullmatch(name)

    if match is None:
        raise ValueError(f"Invalid {arch} DMA identifier: {name}")

    fields = match.groupdict()
    if(arch == "stream"):
        irq_name =  f"DMA{fields['dma']}_Stream{fields['stream']}"
    else:
        irq_name = f"DMA{fields['dma']}_Channel{fields['channel']}"
        
    return {
        "irq_name": irq_name,
        "dma": f"DMA{fields['dma']}",
        "irq_flag_num": fields.get("stream") if fields.get("stream") is not None else fields['channel'],
        "stream": f"LL_DMA_STREAM_{fields['stream']}" if fields.get("stream") is not None else None,
        "channel": f"LL_DMA_CHANNEL_{fields['channel']}",
        "prio": f"LL_DMA_PRIORITY_{priority.upper()}",
    }

def gpio_init(pin_config):
    port, pin, mode_expr,  pull, speed = pin_config
    # Resolve portable values into STM32 LL expressions.
    parts = mode_expr.split("_", mode_expr.count("_"))
    mode = parts[0]
    af = None
    if mode.startswith("AF"):
        af = int(mode[2:])
        mode = mode[:2]
    spec = parts[1:]
    type = None
    if len(spec) > 1:
        type = spec[0]

    port_expr = f"GPIO{port}"
    pin_expr = f"LL_GPIO_PIN_{pin}"
    speed_expr = f"LL_GPIO_SPEED_FREQ_{(speed or 'low').upper()}"
    mode_expr = GPIO_MODE[mode]
    output_expr = OUTPUT_TYPE[type]
    pull_expr = PULL_TYPE[pull]
    af_expr = f"LL_GPIO_AF_{af}"

    return f"""
    {{
        __FDGEN__CLK_EN({port_expr})
        LL_GPIO_InitTypeDef GPIO_InitStruct = {{0}};
        GPIO_InitStruct.Pin = {pin_expr};
        GPIO_InitStruct.Mode = {mode_expr};
        GPIO_InitStruct.Speed = {speed_expr};
        GPIO_InitStruct.OutputType = {output_expr};
        GPIO_InitStruct.Pull = {pull_expr};
        {f"GPIO_InitStruct.Alternate = {af_expr};" if af is not None else ""}
        LL_GPIO_Init({port_expr}, &GPIO_InitStruct);
    }}
    """

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
    irqs = ctx["device"]["irq"]

    # Prefer an exact match.
    if irq in irqs:
        return irq
    # Fall back to substring matching.
    matches = [name for name in irqs if irq in name]

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
    methods = {method for handlers in bindings.values() for method in handlers}

    for method in methods:
        code.append(f"\nextern void {method}(void);")

    # Emit one handler per resolved IRQ vector
    for irq, handlers in bindings.items():
        code.append(f"\n\nvoid {irq}_IRQHandler(void)\n{{\n")
        for method in handlers:
            code.append(f"    {method}();\n")
        code.append("}\n")

    with open(system_c, "a", encoding="utf-8") as f:
        f.write("".join(code))

    code = []
    for key,value in device.items():
        code.append(f"set(DEVICE_{key.upper()} \"{str(value).upper()}\")\n")
    output_path = ctx["temp_dir"]/"device.cmake"
    output_path.parent.mkdir(parents=True, exist_ok=True)
    with output_path.open("a", encoding="utf-8") as f:
        f.writelines(code)
    ctx["generated"].add(str(output_path))
    
        
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

def pre_macro(ctx):
    generated = [Path(p) for p in ctx["generated"]]
    system_c = next((p for p in generated if p.name == "System.c"), None)
    system_h = next((p for p in generated if p.name == "System.h"), None)
    if system_c is None or system_h is None:
        ctx.setdefault("error", []).append(
            "FDGEN: System.c or System.h not found in generated files"
        )
        return
    code = []
    device = ctx["device"]
    code.append("\nvoid SystemGPIO_Init(void)\n{\n")
    for name, data in device["gpio"].items():
        code.append(f"// GPIO {name}  Owner: {data['owner']}\n")
        code.append("   " + gpio_init(data['config']).strip() + "\n")
    code.append("}\n")
    with open(system_c, "a", encoding="utf-8") as f:
        f.write("".join(code))

def fdgen(ctx):
    pre_macro(ctx)
    fdgen_macro(ctx)
    emit_system(ctx)

def setup(ctx):
    #add the cmake file needed
    #get the product code, flash/ density and so on letters and variants and add them to device dictionary
    device = ctx["device"]
    #parse CMSIS headers for device information
    ctx["device_def"] = parse_header(ctx["cmsis_dir"]/f"Device/ST/STM32{device["family"].upper()}xx/Include/stm32{device["family"][0]}{device["product"]}x{device["density_code"]}.h") 
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
    
    #path and search additions
    ctx["gen_sDir"].append(f"{DRIVER_ROOT}/{ctx["device"]["platform"]}/stm32{(ctx["device"]["family"]).lower()}")
    ctx.setdefault("gen", {}).update({
        "CMakeLists.txt": None,
        "stm32f4xx_hal_conf.h": None,
        "System.h": None,
        "fd_adc.c": None,
        "fd_adc.h": None,
        "System.c": None,
        "post_build.cmake": None,
        "toolchain.cmake": None,
        "usbd/usbd_conf.c": "usbd/usbd_conf.c",
        "usbd/usbd_conf.h": "usbd/usbd_conf.h",
        "usbd/usbd_desc.c": "usbd/usbd_desc.c",
        "usbd/usbd_desc.h": "usbd/usbd_desc.h",
    })
    if "usb" in ctx["config"]:
        ctx["gen"].update({
            "usbd/usbd_conf.c": "usbd/usbd_conf.c",
            "usbd/usbd_conf.h": "usbd/usbd_conf.h",
            "usbd/usbd_desc.c": "usbd/usbd_desc.c",
            "usbd/usbd_desc.h": "usbd/usbd_desc.h",
        })

def env_macros(ctx):
    env = ctx["env"]
    env.globals["dma_parse"] = dma_parse
    pass