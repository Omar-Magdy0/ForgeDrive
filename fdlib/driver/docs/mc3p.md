# MC3P — Three-Phase Motor-Control Driver

## Overview

MC3P provides a shared three-phase motor-control subsystem for generating bridge commands and acquiring synchronized measurements. Its interface supports trapezoidal and space-vector modulation, phase-state control, duty-cycle control, synchronized ADC measurements, background ADC scans, measurement scaling, and optional deadtime compensation.

The driver is generated from configuration. The configured `instances` list describes the physical motor-control instances to emit; the instance names are configuration identifiers used to form generated symbols, not fixed names required by the API.

This document describes the interface and the current configuration model. Hardware-resource details are supplied through each instance's configuration rather than being inherent to a particular instance name.

## Design and timing model

The subsystem has two measurement paths:

- **Synchronized measurements:** samples associated with motor-control timing. The public interface exposes `fd_mc3p_read_sync(...)` and a per-instance generated implementation.
- **Background measurements:** slower monitoring channels acquired separately. The public interface exposes `fd_mc3p_bg_startConv(...)`, a background data buffer in the handle, and an optional post-scan callback.

The configured subsystem tick is independent configuration from the PWM frequency. `tick.freq` sets the subsystem tick frequency, while each instance's `tim` configuration selects the timer and output pins used for the power-stage interface.

The driver supports trapezoidal and SVM sector identifiers. `FD_MC3P_MODE_CALIB` is also defined as a mode value for calibration-related operation.

## Current-sensing direction convention

**The current direction convention is part of the driver interface. Configure sensor scaling and interpret `read_sync` results using the convention for the selected sensing topology.**

### Three-shunt, two-shunt, and inline sensing

For **three-shunt, two-shunt, and inline phase-current sensing**, positive current means current flowing **out of the corresponding motor phase and into the load/motor winding**.

Use this convention consistently when defining current-sensor polarity and scale/offset values. A sensor installed with the opposite electrical polarity must be compensated in its scaling/sign configuration so that the value delivered to the control loop follows this convention.

The synchronized measurement path exposes phase-current channels as `FD_MC3P_CSU`, `FD_MC3P_CSV`, and `FD_MC3P_CSW`. The physical sensor-to-channel mapping is configured per instance; do not assume that a particular ADC channel or pin is permanently associated with a phase.

### Single-shunt sensing

For **single-shunt sensing**, positive current follows the same
convention as the other current-sensing implementations: current
flows out of the motor phase and into the motor winding.

The DC-link shunt measures current in the DC-link return path.
Its electrical sensor polarity and the switching-state-dependent
relationship between DC-link current and phase currents must be
accounted for when interpreting the measurement.

### Scaling and `read_sync`

The synchronized channel scaling is stored in `fd_mc3p_handle_t::sync_scale_q31`. `fd_mc3p_set_sync_scale(...)` configures the scale/offset coefficients used to convert synchronized ADC measurements into the driver's normalized Q31 representation. The `FD_MC3P_VS_TO_FLOAT`, `FD_MC3P_CS_TO_FLOAT`, `FD_MC3P_FLOAT_TO_VS`, and `FD_MC3P_FLOAT_TO_CS` macros convert between normalized values and physical values using the configured voltage/current scale constants.

The scaling coefficients do not make an incorrectly wired sensor polarity correct automatically. Verify sensor polarity, topology, channel mapping, and offset before closing a feedback loop. In particular, confirm that positive measured current agrees with the convention above.

For all current-sensing topologies, the current values delivered
to the control loop must follow the positive-current convention
defined above. Any required polarity correction must be applied
during sensor scaling or measurement processing, as appropriate
to the sensing topology.

## Public types

### `fd_mc3p_cs_t`

Describes the current-sensing topology categories defined by the interface:

| Value | Meaning |
|---|---|
| `FD_MC3P_CS_NONE` | No current sensing |
| `FD_MC3P_CS_TRIPLE_SHUNT` | Three-shunt sensing |
| `FD_MC3P_CS_DOUBLE_SHUNT` | Two-shunt sensing |
| `FD_MC3P_CS_SINGLE_SHUNT` | Single-shunt sensing |
| `FD_MC3P_CS_INLINE` | Inline phase-current sensing |

The configuration's `current_sensing.topology` value is a generator configuration field. Its accepted strings and mapping to these enum values are determined by the configuration/generation implementation.

### `fd_mc3p_sector_t`

Represents the output sector/state selection:

- `FD_MC3P_SECTOR_FLOAT`: floating state, with all MOSFETs off.
- `FD_MC3P_SECTOR_TRAP1` through `FD_MC3P_SECTOR_TRAP6`: trapezoidal sectors.
- `FD_MC3P_SECTOR_SVM1` through `FD_MC3P_SECTOR_SVM6`: SVM sectors.

`IS_TRAP_SECTOR(sector)` and `IS_SVM_SECTOR(sector)` test whether a sector falls in the respective range.

### `fd_mc3p_mode_t`

Defines `FD_MC3P_MODE_NONE`, `FD_MC3P_MODE_TRAP`, `FD_MC3P_MODE_SVM`, and `FD_MC3P_MODE_CALIB`. `fd_mc3p_sector_to_mode(...)` maps trapezoidal and SVM sector values to their corresponding modes and returns `FD_MC3P_MODE_NONE` for other values.

### `fd_mc3p_phase_state_t`

Describes the requested switching state for an individual phase:

| Value | Meaning |
|---|---|
| `FD_MC3P_PHASE_FLOAT` | Phase floating |
| `FD_MC3P_PHASE_H_PWM` | High-side PWM active |
| `FD_MC3P_PHASE_L_PWM` | Low-side PWM active |
| `FD_MC3P_PHASE_COMP` | Complementary PWM active |
| `FD_MC3P_PHASE_H_ON` | High-side held on |
| `FD_MC3P_PHASE_L_ON` | Low-side held on |

The actual safe use of these states depends on the power-stage topology and the configured output implementation.

### `fd_mc3p_config_t`

Contains per-handle PWM settings:

| Field | Meaning |
|---|---|
| `pwm_Hz` | PWM frequency in hertz |
| `deadtime_nS` | Requested deadtime in nanoseconds |
| `duty_max` | Maximum permitted duty cycle |
| `duty_min` | Minimum permitted duty cycle |

### `fd_mc3p_handle_t`

The handle stores configuration and runtime state for an MC3P instance.

| Field | Purpose |
|---|---|
| `config` | PWM configuration |
| `mode` | Current modulation mode |
| `sector_last` | Last selected sector |
| `timer_max_q15` | Timer period represented in Q15-related units |
| `duty_max_q15`, `duty_min_q15` | Duty limits in Q15 representation |
| `dutyu_q15`, `dutyv_q15`, `dutyw_q15` | Per-phase duty commands |
| `sync_scale_q31` | Synchronized measurement scale/offset coefficients |
| `bg_data` | Background ADC scan data buffer |
| `bg_channels` | Number of configured background channels |
| `rank_scale_q31` | Scale/offset data for background conversion ranks |
| `offset_calibration` | Offset-calibration state |
| `offset_calibration_remaining_samples` | Number of calibration samples remaining |
| `offset_calibration_sum` | Accumulated calibration samples |
| `dtc_comp_q15` | Deadtime-compensation amount |
| `dtc_state` | Per-phase/state flags used by deadtime compensation |

These fields are exposed in the header. Applications should generally use the public API rather than modifying runtime bookkeeping directly unless the implementation explicitly requires it.

### Measurement structures

`fd_mc3p_svm_data_t` contains `vbus_q31`, `cu_q31`, `cv_q31`, and `cw_q31` for bus voltage and phase-current data used by SVM-related control.

`fd_mc3p_trap_data_t` contains `vbus_q31`, `vbemf_q31`, and `cbus_q31` for bus voltage, back-EMF, and DC-bus current data used by trapezoidal control.

Current values must follow the current-sensing direction
convention defined in the Current-sensing direction convention
section, regardless of the configured topology.

## API reference

The generator emits instance-specific function symbols for every entry in `mc3p.instances`. The generic macro interface dispatches to the correct generated symbol using the supplied instance token.

### Initialization and PWM configuration

| Interface | Function |
|---|---|
| `fd_mc3p_init(inst)` | Initializes the selected instance. |
| `fd_mc3p_reconfigure_pwm(inst)` | Reconfigures PWM using the handle's configuration. |

The corresponding generated functions accept a pointer to `fd_mc3p_handle_t`.

### Background and synchronized measurements

| Interface | Function |
|---|---|
| `fd_mc3p_bg_startConv(inst)` | Starts a background conversion scan. |
| `fd_mc3p_read_sync(inst, data)` | Reads/processes the synchronized scan data through the generated implementation. |
| `fd_mc3p_set_sync_scale(inst, scales)` | Configures synchronized measurement scaling and offsets. |

The synchronized scaling input is an array of `MC3P_SYNC_CHANNELS_MAX` entries, each containing scale and offset values. Voltage channels use the configured voltage scale; current channels use the configured current scale. Check the implementation's expected units and sensor transfer characteristics when supplying coefficients.

### Phase-state and duty control

| Interface | Function |
|---|---|
| `fd_mc3p_write_phase_state(inst, state_u, state_v, state_w)` | Writes the requested switching state for each phase. |
| `fd_mc3p_write_phase_duty(inst, duty_u, duty_v, duty_w)` | Writes the three phase duty commands in Q15 representation. |
| `fd_mc3p_write_float(inst)` | Requests all phases to float and clears the phase duty commands. |
| `fd_mc3p_write_trap(inst, sector, duty)` | Applies a trapezoidal sector and a common duty command. |
| `fd_mc3p_write_svm(inst, alpha, beta)` | Calculates phase commands from normalized αβ input, selects an SVM sector, applies zero-sequence injection, and writes the resulting duties. |

The `write_trap` interface ignores sector values outside `FD_MC3P_SECTOR_TRAP1` through `FD_MC3P_SECTOR_TRAP6`. The SVM routine derives a sector from the signs of the αβ-derived phase references and applies min/max-based zero-sequence injection.

The direct duty and αβ inputs use fixed-point representations. Respect the expected Q15 range and configured duty limits; do not assume arbitrary integer values represent valid modulation commands.

### Callbacks

The header declares weak callback hooks:

- `fd_mc3p_tickerCallback()`: subsystem ticker callback.
- `fd_mc3p_<instance>_sync_postScanCallback()`: per-instance synchronized-scan post-processing hook.
- `fd_mc3p_<instance>_bg_postScanCallback()`: per-instance background-scan post-processing hook.

Provide a strong definition with the exact generated symbol name to override a weak callback. The instance-specific callback names are generated from the configured instance name.

### Conversion and helper macros

| Macro | Purpose |
|---|---|
| `FD_MC3P_VS_TO_FLOAT(v)` | Converts normalized voltage data to a floating-point voltage using `FD_MC3P_VS_SCALE`. |
| `FD_MC3P_CS_TO_FLOAT(v)` | Converts normalized current data to a floating-point current using `FD_MC3P_CS_SCALE`. |
| `FD_MC3P_FLOAT_TO_VS(v)` | Converts a floating-point voltage to normalized representation. |
| `FD_MC3P_FLOAT_TO_CS(v)` | Converts a floating-point current to normalized representation. |
| `FD_MC3P_TICKFREQ` | Generated subsystem tick frequency. |
| `FD_MC3P_TICKPERIOD_US` | Macro derived from the tick frequency. |
| `FD_MC3P_VS_SCALE` | Configured voltage scaling constant. |
| `FD_MC3P_CS_SCALE` | Configured current scaling constant. |

The header also defines `Q15_HALF`, `Q15_SQRT3_BY_2`, `SATURATE`, `IS_SVM_SECTOR`, and `IS_TRAP_SECTOR` for fixed-point calculations and sector classification.

## Code-generation and templating behavior

MC3P is a configuration-driven template rather than a fixed, manually instantiated driver.

- The shared `mc3p` configuration defines subsystem-wide parameters such as tick timing, voltage/current scales, and current-sensing configuration.
- `mc3p.instances` is a list of physical motor-control instances. Each list entry provides the resources and settings for one generated instance.
- The `name` field is used to form generated C symbols, including initialization, PWM, measurement, modulation, and callback functions. The API does not require any particular instance name.
- Per-instance resources such as timer selection, output pins, ADC channels, sampling times, and background DMA settings are read from the corresponding instance entry.
- The template emits instance-specific function declarations and dispatch macros so application code can use the generic form, for example `fd_mc3p_init(instance_token)`, without manually spelling the generated function name.
- Optional features can conditionally affect generated code. For example, when the deadtime-compensation configuration is present, the template defines `FD_MC3P_DTC_ACTIVE` and includes the corresponding compensation paths.
- Generated symbols and hardware-resource configuration must remain consistent: changing an instance name or resource mapping changes the generated interface or hardware setup accordingly.

The source template is therefore the common implementation pattern, while the configuration determines which physical instances and resource assignments are emitted.

## Driver configuration

The following documents the current MC3P configuration fields and their intended meaning. The actual configuration parser/generator remains authoritative for validation rules and accepted values.

### Shared subsystem fields

| Field | Example | Meaning |
|---|---:|---|
| `mc3p.tick.freq` | `4000` | Subsystem tick frequency in hertz. |
| `mc3p.tick.prio` | `2` | Priority assigned to the subsystem tick mechanism. |
| `mc3p.vs_scale` | `60` | Voltage conversion scale used by the normalized/physical conversion helpers. |
| `mc3p.cs_scale` | `50` | Current conversion scale used by the normalized/physical conversion helpers. |
| `mc3p.current_sensing.topology` | `"two_inline"` | Selects the configured current-sensing topology. Accepted values and their exact interpretation are defined by the generator. |
| `mc3p.current_sensing.calibration_samples_log2` | `10` | Logarithm base 2 of the number of samples used for current-sensor offset calibration; `10` corresponds to \(2^{10}=1024\) samples. |
| `mc3p.instances` | list | Physical instances to generate. |

### Per-instance fields

Each object in `mc3p.instances` has a `name` and resource groups. The following fields describe the current configuration shape.

#### Timer (`tim`)

| Field | Meaning |
|---|---|
| `tim.num` | Timer/peripheral identifier. |
| `tim.vh`, `tim.wh`, `tim.uh` | High-side phase outputs, each with `port`, `pin`, `active`, and `ch` fields. |
| `tim.vl`, `tim.wl`, `tim.ul` | Low-side phase outputs, each with `port`, `pin`, and `active` fields. |

`port` and `pin` select a physical output pin. `active` specifies the configured active level. `ch` selects the output channel for high-side entries where provided. The phase labels are resource names in the configuration; the current-sensing sign convention must still be applied to the corresponding measurement channels independently.

#### ADC (`adc`)

| Field | Meaning |
|---|---|
| `adc.num` | ADC/peripheral identifier. |
| `adc.inj` | Mapping for synchronized/injected measurements. |
| `adc.bg` | Mapping for background monitoring channels. |
| `port`, `pin` | Physical pin for an external analog input, when applicable. |
| `channel` | ADC channel identifier. |
| `sampling_cycles` | Configured ADC sampling duration in cycles. |

The injected-measurement map uses named signals such as `vsbus`, `vsu`, `vsv`, `vsw`, and phase-current signals such as `csu`, `csv`, or `csw`. Only the signals configured for a particular topology and instance should be assumed to be present. The background map uses named signals such as `temp` and `pot`; these names identify configured background inputs, not necessarily distinct ADC channels.

#### Background DMA (`dma.bg`)

| Field | Meaning |
|---|---|
| `dma.bg.num` | DMA controller identifier. |
| `dma.bg.stream` | DMA stream identifier. |
| `dma.bg.channel` | DMA channel/request selection. |
| `dma.bg.prio` | Configured DMA priority. |

The exact combinations of peripheral, channel, stream, and priority must be valid for the selected target and its resource mapping.

### Configuration example

This example illustrates the current schema and example values. It is not a requirement to use the shown instance name or resource assignments.

```python
"mc3p": {
    "tick": {
        "freq": 4000,
        "prio": 2
    },
    "vs_scale": 60,
    "cs_scale": 50,
    "current_sensing": {
        "topology": "two_inline",
        "calibration_samples_log2": 10
    },
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
                    "csu":   {"port": "A", "pin": 5, "channel": 6, "sampling_cycles": 15}
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
            }
        }
    ]
}
```

## Integration checklist

Before using MC3P in a control loop:

1. Confirm the configured current-sensing topology matches the physical hardware and the generator's accepted topology values.
2. Verify that each current sensor's polarity follows the topology-specific positive-current convention described above.
3. Confirm the configured ADC signal names and physical channels match the intended phase measurements.
4. Set voltage/current scale and offset coefficients to match the actual sensor transfer functions and units.
5. Verify PWM frequency, deadtime, duty limits, and output active levels against the power stage.
6. Check that all configured peripheral resources are valid and do not conflict with other subsystems.
7. Confirm the generated instance symbols and callback names match the configured `name` fields.
8. Verify the synchronized and background scan paths independently before enabling closed-loop operation.
