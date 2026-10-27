config = {
    "usb": {
        "vid": 0xFFFF,
        "pid": 0x0001,
        "langid_string": 0x0409,

        "manufacturer_string": "ForgeDrive",

        "product": {
            "hs": "ForgeDriveMC",
            "fs": "ForgeDriveMC",
        },

        "configuration": {
            "hs": "Default Configuration",
            "fs": "Default Configuration",
        },

        "interface": {
            "hs": "PMSM Drive",
            "fs": "PMSM Drive",
        },
    },
    "usbxch": {
        "instances": [
            {
                "name": "USB1",
                "tx_bufsize" : 4096,
                "rx_bufsize" : 256
            }
        ]
    },
    "hall": {

    },
    "tim": {
        "instances": [
            {
                "name": "TIMxMC",
                "num": 10,
                "irq": {
                    "tim": {"prio": 2}
                }
            }
        ]
    },
    "mc3p": {
        # Shared subsystem configuration tied specifically to mc3p
        "vs_scale" : 60,
        "cs_scale": 50,
        "current_sensing": {
            "topology": "two_inline",
            "calibration_samples_log2": 10
        },
        # List of physical motor instances using this subsystem
        "instances": [
            {
                "name": "MC3P1",
                "tim": {
                    "num": 1,
                    "vh": {"port": "A", "pin": 8, "active": 1, "ch": 1},
                    "wh": {"port": "A", "pin": 9, "active": 1, "ch": 2},
                    "uh": {"port": "A", "pin": 10, "active": 1, "ch": 3},
                    "vl": {"port": "B", "pin": 13, "active": 1},
                    "wl": {"port": "B", "pin": 14, "active": 1},
                    "ul": {"port": "B", "pin": 15, "active": 1}
                },
                "adc": {
                    "num": 1,
                    "inj": {
                        "vsbus": {"port": "A", "pin": 0, "channel": 0, "sampling_cycles": 15},
                        "vsu":   {"port": "A", "pin": 1, "channel": 1, "sampling_cycles": 15},
                        "vsv":   {"port": "A", "pin": 2, "channel": 2, "sampling_cycles": 15},
                        "vsw":   {"port": "A", "pin": 3, "channel": 3, "sampling_cycles": 15},
                        "csw":   {"port": "A", "pin": 4, "channel": 4, "sampling_cycles": 15},
                        "csu":   {"port": "A", "pin": 5, "channel": 6, "sampling_cycles": 15},
                    },
                    "bg": {
                        "temp": {"channel": 18, "sampling_cycles": 56},
                        "pot":  {"channel": 18, "sampling_cycles": 56}
                    }
                },
                "dma": {
                    "bg": {
                        "num": 2,
                        "stream": 0,
                        "channel": 0,
                        "prio": "medium"
                    }
                },
                "irq": {
                    "sync": {"prio": 0},
                    "tim": {"prio": 1},
                    "bkin": {"prio": 0},
                    "bg": {"prio": 5}
                }
            }
            
        ]
    }
}