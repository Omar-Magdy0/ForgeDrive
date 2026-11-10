

def mc3p(ctx):
    conf = ctx["config"]["mc3p"]
    conf["trap_sectors"] = [
        ["COMP", "L_ON", "FLOAT"],  # TRAP1
        ["COMP", "FLOAT", "L_ON"],  # TRAP2
        ["FLOAT", "COMP", "L_ON"],  # TRAP3
        ["L_ON", "COMP", "FLOAT"],  # TRAP4
        ["L_ON", "FLOAT", "COMP"],  # TRAP5
        ["FLOAT", "L_ON", "COMP"],  # TRAP6
    ]

    conf["trap_table"] = ",\n".join(
        f"  [FD_MC3P_SECTOR_TRAP{i+1} - 1] = "
        f"{{ {{ {', '.join(f'FD_MC3P_PHASE_{p}' for p in phases)} }} }}"
        for i, phases in enumerate(conf["trap_sectors"])
    )

    phase_names = ["u", "v", "w"]

    ## validation , adding errors/warnings
    for inst in conf["instances"]:
        #gpio linkage
        pwm_gpio = []
        for name, val in inst["pwm"].items():
            if(name in ("tim")): continue
            gpio = inst["gpio"].get(name)
            if gpio is not None:
                pwm_gpio.append(inst["gpio"][name])
            else:
                ctx["error"].append(f"[MC3P] gpio for pwm signal {name} not found")
        inst["pwm_gpio"] = pwm_gpio

        analog_gpio = []
        for name, val in inst["analog"].items():
            if(name in ("adc")): continue
            gpio = inst["gpio"].get(name)
            if gpio is not None:
                analog_gpio.append(inst["gpio"][name])
        inst["analog_gpio"] = analog_gpio

        if conf["current_sensing"]["topology"] in ["two_shunt","two_inline"]:
            inst["cs_found"] = {
                phase[2]
                for phase in inst["analog"]["inj"]
                if phase.startswith("cs") and len(phase) == 3
            }
            inst["cs_missing"] = set(phase_names) - inst["cs_found"]
            if len(inst["cs_missing"]) == 1:
                inst["cs_missing"] =  sorted(inst["cs_missing"])
                inst["cs_found"] = sorted(inst["cs_found"])
            else:
                ctx["error"].append(
                    f"[MC3P] Invalid current-sensing configuration: "
                    f"expected exactly one missing phase, found "
                    f"{len(inst['cs_missing'])}. "
                    f"Configured phases: {sorted(inst['cs_found'])}."
                )
    
    for inst in conf["instances"]:
        #generate voltage sense vector for trapezoidal adc config
        inst["trapVs_vector"] = [
            {
                "phase": phase_names[phases.index("FLOAT")].upper(),
                "ch": inst["analog"]["inj"]["vs" + phase_names[phases.index("FLOAT")]][0],
                "sign": 1
            }
            for phases in conf["trap_sectors"]
        ]
        #generate current sense vector for trapezoidal commutation current sense
        if conf["current_sensing"]["topology"] == "three_shunt":
            inst["trapCs_vector"] = [
                {
                    "phase": phase_names[phases.index("L_ON")].upper(),
                    "ch": inst["analog"]["inj"]["cs" + phase_names[phases.index("L_ON")]][0],
                    "sign": -1
                }
                for phases in conf["trap_sectors"]
            ]

        elif conf["current_sensing"]["topology"] in ["two_shunt","two_inline"]:
            inst["trapCs_vector"] = []
            for phases in conf["trap_sectors"]:
                selected = None

                for state, sign in [("L_ON", -1), ("COMP", 1)]:
                    phase = phase_names[phases.index(state)]

                    if phase in inst["cs_found"]:
                        selected = {
                            "phase": phase.upper(),
                            "ch": inst["analog"]["inj"]["cs" + phase][0],
                            "sign": sign
                        }
                        break
                if selected is None:
                    conf["error"].append(
                        f"[MC3P] Trap current sensing: no measurable phase "
                        f"found for sector {len(inst['trapCs_vector']) + 1}. "
                        f"Neither the L_ON nor COMP phase has a current sensor."
                    )
                inst["trapCs_vector"].append(selected)
        elif conf["current_sensing"]["topology"] == "inline":
            pass

from pprint import pprint    
def gpio(ctx):
    conf = ctx["config"]
    device = ctx["device"]
    gpios = {}

    def collect(node, path=()):
        if not isinstance(node, dict):
            return

        for key, value in node.items():
            if key == "gpio" and isinstance(value, dict):
                owner = ".".join(path)
                for sig, config in value.items():
                    name = str(config[0])+str(config[1])
                    owner_sig = f"{owner}.{sig}"
                    if name not in gpios:
                        gpios[name] = {}
                    else:
                        ctx["error"].append(
                            f"[GPIO] : multiple drivers sharing GPIO {name}: "
                            f"{owner_sig} conflicts with {gpios[name]['owner']}"
                        )
                        continue
                    gpios[name]["config"] = config
                    gpios[name]["owner"] = owner_sig
            elif isinstance(value, dict):
                collect(value, path + (key,))
            elif isinstance(value, list):
                for i, item in enumerate(value):
                    collect(item, path + (f"{key}[{i}]",))
    collect(conf)
    device["gpio"] = gpios


def dma(ctx):
    conf = ctx["config"]
    device = ctx["device"]
    dma = {}

    def collect(node, path=()):
        if not isinstance(node, dict):
            return

        for key, value in node.items():
            if key == "dma" and isinstance(value, dict):
                owner = ".".join(path)
                for sig, config in value.items():
                    name = str(config[0])
                    owner_sig = f"{owner}.{sig}"
                    if name not in dma:
                        dma[name] = {}
                    else:
                        ctx["error"].append(
                            f"[DMA] : multiple drivers sharing DMA {name}: "
                            f"{owner_sig} conflicts with {dma[name]['owner']}"
                        )
                        continue
                    dma[name]["config"] = config[1]
                    dma[name]["owner"] = owner_sig
            elif isinstance(value, dict):
                collect(value, path + (key,))
            elif isinstance(value, list):
                for i, item in enumerate(value):
                    collect(item, path + (f"{key}[{i}]",))
    collect(conf)
    device["dma"] = dma

def drv(ctx):
    gpio(ctx)
    dma(ctx)
    mc3p(ctx)
    conf = ctx["config"]["mc3p"]
    