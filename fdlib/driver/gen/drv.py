

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
        if conf["current_sensing"]["topology"] in ["two_shunt","two_inline"]:
            inst["cs_found"] = {
                phase[2]
                for phase in inst["adc"]["inj"]
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
                "channel": inst["adc"]["inj"]["vs" + phase_names[phases.index("FLOAT")]]["channel"],
                "sign": 1
            }
            for phases in conf["trap_sectors"]
        ]
        #generate current sense vector for trapezoidal commutation current sense
        if conf["current_sensing"]["topology"] == "three_shunt":
            inst["trapCs_vector"] = [
                {
                    "phase": phase_names[phases.index("L_ON")].upper(),
                    "channel": inst["adc"]["inj"]["cs" + phase_names[phases.index("L_ON")]]["channel"],
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
                            "channel": inst["adc"]["inj"]["cs" + phase]["channel"],
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
            
    

def drv(ctx):
    mc3p(ctx)
    conf = ctx["config"]["mc3p"]
    