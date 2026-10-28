# GPIO (port, pin, mode, pull, speed)
# PWM (active, channel) (with negative denoting complementary)
# Analog channels (channel, sample_cycles)


config = {
    "usb": {
        "vid": 0xFFFF,
        "pid": 0x0001,
        "langid_string": 0x0409,
        "class": "usbxch",
        "manufacturer_string": "ForgeDrive",
        "product": "ForgeDriveMC",
        "configuration": "Default Configuration",
        "interface": "PMSM Drive",
        "gpio": {
            "dp": ("A", 11, "AF10_PP", None, "very_high"),
            "dm": ("A", 12, "AF10_PP", None, "very_high")
        },
        "irq": {
            "usb": 6
        },
    },
    "remap": {},
    "usbxch": {
        "name": "USB1",
        "tx_bufsize" : 4096,
        "rx_bufsize" : 256,
    },
    "hall": {

    },
    "timer": {
        "instances": [
            {
                "name": "TIMERxMC",
                "tim": 10,
                "irq": {
                    "tim": 2
                },
            }
        ]
    },
    "mc3p": {
        "vs_scale" : 60,
        "cs_scale": 50,
        "current_sensing": {
            "topology": "two_inline",
            "calibration_samples_log2": 10
        },
        "instances": [
            {
                "name": "MC3P1",
                "pwm": {
                    "tim": 1,
                    "vh": (1, 1),
                    "wh": (1, 2),
                    "uh": (1, 3),
                    "vl": (1,-1),
                    "wl": (1,-2),
                    "ul": (1,-3),
                },
                "analog": {
                    "adc": 1,
                    "inj": {
                        "vsbus": (0, 15),
                        "vsu":   (1, 15),
                        "vsv":   (2, 15),
                        "vsw":   (3, 15),
                        "csw":   (4, 15),
                        "csu":   (6, 15),
                    },
                    "bg": {
                        "temp": (18, 56),
                        "pot":  (18, 56)
                    }
                },
                "dma": {
                    "bg": ("DMA1_STREAM0_CH1","medium")
                },
                "irq": {
                    "sync": 0,
                    "tim": 1,
                    "bkin": 0,
                    "bg": 5
                },
                "gpio": {
                    "vh": ("A", 8, "AF1_PP", None, "low"),
                    "wh": ("A", 9, "AF1_PP", None, "low"),
                    "uh": ("A", 10, "AF1_PP", None, "low"),
                    "vl": ("B", 13, "AF1_PP", None, "low"),
                    "wl": ("B", 14, "AF1_PP", None, "low"),
                    "ul": ("B", 15, "AF1_PP", None, "low"),
                    "vsbus": ("A", 0, "AN", None, "low"),
                    "vsu": ("A", 1, "AN", None, "low"),
                    "vsv": ("A", 2, "AN", None, "low"),
                    "vsw": ("A", 3, "AN", None, "low"),
                    "csw": ("A", 4, "AN", None, "low"),
                    "csu": ("A", 5, "AN", None, "low"),
                    "temp": ("A", 6, "AN", None, "low"),
                    "pot":  ("A", 7, "AN", None, "low"),
                },
            }
        ]
    }
}