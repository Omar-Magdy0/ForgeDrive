# ForgeDrive Naming Convention

This guide defines identifier spelling, semantic boundaries, and engineering-unit suffixes for ForgeDrive software. Apply it to new code and use it when touching existing code; it does not require a repository-wide rename.

## 1. Identifier structure

Identifiers may contain up to three semantic parts:

```text
Prefix_Name_suffix
```

- `Prefix` identifies an owning module or namespace when ownership is not already clear from scope.
- `Name` describes the entity or quantity.
- `suffix` adds a required unit, numeric representation, or other defining qualifier.
- Underscores mark boundaries between these parts. Do not use underscores inside a part, except where a language or tool requires a spelling such as a macro name.
- Omit a part when it adds no information. Do not repeat scope that is already unambiguous.

The spelling of each part depends on its role. The structure does not mean every part has the same capitalization.

Examples:

```cpp
FdMc3p::speed_radpsQ31   // namespace, quantity, unit + representation
FdMc3p_speed_radpsQ31    // C/global-style module prefix, quantity, unit + representation
pwm_hz                   // quantity name + unit; frequency is implicit in Hz
iq_amp                   // q-axis current + unit
```

## 2. Casing by identifier category

| Category | Convention | Example |
|---|---|---|
| C++ namespaces | PascalCase acronym spelling | `FdMc3p` |
| C module/global prefixes | Uppercase acronym, no internal separators | `FDMC3P` |
| Variables and parameters | lowercase snake_case words | `mod_index` |
| Functions | PascalCase, acronym words normalized | `PwmWriteSvm()` |
| Classes and structures | PascalCase | `PmsmControlCore`, `PmsmModel` |
| Enumerations/types | PascalCase | `PmsmError` |
| Enum members | PascalCase | `InvalidModel` |
| Constants | PascalCase | `InvSqrt3Q31` |
| Template parameters | PascalCase | `ValueType` |
| Macros | UPPER_SNAKE_CASE | `FDMC3P_PWM_ENABLE` |
| Files | lowercase snake_case | `pmsm_control.h` |
| Directories | lowercase snake_case | `pmsm_control/` |

Use the C++ namespace spelling for namespace declarations and the compact uppercase spelling for C macros and other global C identifiers. Do not write a module prefix as `FD_MC3P`; the module name is `FDMC3P` (or `FdMc3p` as a C++ namespace). Macro tokens may still use underscores to separate words: `FDMC3P_PWM_ENABLE`.

## 3. Prefixes and ownership

Use a module prefix for global C identifiers, macros, and other names that need collision protection. In C++, prefer namespace ownership where practical. Within a module namespace, do not repeat the namespace in each member name.

```cpp
namespace FdMc3p {
    int32_t speed_radpsQ31;
    void PwmWriteSvm();
}

#define FDMC3P_PWM_MAX_DUTY
```

Avoid generic prefixes such as `Fd` when the actual module is known. Avoid redundant prefixes such as `FdMc3p_FdMc3p...`.

## 4. Engineering quantities and units

Include a unit in the suffix when it is needed to understand the value, especially for public interfaces, stored configuration, conversions, and values that could be confused with another unit. The unit establishes the measurement; do not repeat the unit's meaning in the quantity name.

Use this order:

```text
quantity_unit[representation]
```

Examples:

```cpp
uint32_t pwm_hz;          // PWM rate in hertz; don't write pwm_frequency_hz
int32_t iq_amp;            // q-axis current in amperes
int32_t vq_vlt;            // q-axis voltage in volts
int32_t speed_radpsQ31;    // angular speed in radians per second, Q31
float speed_radpsF;        // angular speed in radians per second, floating point
uint32_t sample_period_us; // period in microseconds
```

Unit aliases are lowercase, compact, and documented here. Use these spellings consistently; do not invent a new alias in a local identifier. Add an alias to this table before adopting it in shared code.

| Unit | Alias | Meaning |
|---|---|---|
| ampere | `amp` | electric current |
| volt | `vlt` | electric potential / voltage |
| hertz | `hz` | cycles per second, including frequency |
| radian per second | `rps` | angular speed |
| revolution per minute | `rpm` | rotational speed |
| second | `s` | time |
| millisecond | `ms` | time |
| microsecond | `us` | time |
| nanosecond | `ns` | time |
| meter | `m` | length |
| meter per second | `mps` | linear speed |
| ohm | `ohm` | resistance |
| watt | `w` | power |
| degree Celsius | `degc` | temperature |

Use the quantity name to disambiguate distinct quantities that share a unit, such as `bus_vlt` and `vq_vlt`. Use established SI prefixes in the alias (`ms`, `us`, `ns`) instead of spelling their scales into the quantity. If a value is dimensionless, do not add a unit; document the scale or meaning where needed (for example, `duty_q31` or `mod_index`).

## 5. Numeric representations

When representation is part of the value's contract, append it immediately after the unit, without another separator:

```text
_rpsQ31
_ampUQ16
_vltF
```

- `Qxx` means a signed fixed-point value with `xx` fractional bits.
- `UQxx` means an unsigned fixed-point value with `xx` fractional bits.
- `F` means floating-point storage when the representation must be explicit.
- Put the representation after the unit: `speed_radpsQ31`, not `speed_Q31_radps`.
- The storage type determines total width; `Qxx` alone does not specify it.

| Storage type | Format | Fractional bits | Magnitude bits |
|---|---:|---:|---:|
| `int16_t` | Q12 | 12 | 3 |
| `int32_t` | Q12 | 12 | 19 |
| `uint16_t` | UQ12 | 12 | 4 |
| `uint32_t` | UQ12 | 12 | 20 |

Do not append a representation when the declared type and local API already make it clear and the identifier is not crossing an interface. Keep it when values with different representations may be mixed or the representation is part of the contract.

## 6. Acronyms and technical notation

In PascalCase identifiers, write acronyms as words. In macros, use uppercase. Keep fixed-point notation and standard unit symbols in their established forms.

| Acronym | PascalCase | Macro form |
|---|---|---|
| PWM | `Pwm` | `PWM` |
| ADC | `Adc` | `ADC` |
| PMSM | `Pmsm` | `PMSM` |
| FOC | `Foc` | `FOC` |
| SVM | `Svm` | `SVM` |
| RPC | `Rpc` | `RPC` |
| DAQ | `Daq` | `DAQ` |
| USB | `Usb` | `USB` |
| CAN | `Can` | `CAN` |
| PID | `Pid` | `PID` |

Examples of representation tokens: `Q31`, `Q15`, `UQ12`.

## 7. Functions, types, and enumerations

- Prefer `enum class` in C++.
- Use unscoped enums when C interoperability requires them.
- Specify an underlying type when the representation is part of a hardware, serialization, or ABI contract.
- Keep function names as verbs or verb phrases. Use PascalCase and normalize acronyms (`PwmWriteSvm`).

```cpp
enum class PmsmError : uint8_t {
    Ok,
    InvalidModel,
    OverCurrent
};
```

## 8. Macros

Use UPPER_SNAKE_CASE and a compact uppercase module prefix for public or globally visible macros. Prefer `constexpr`, typed constants, or inline functions when they can replace a macro safely.

```c
#define FDMC3P_PWM_MAX_DUTY
#define FDRPC_MAX_PAYLOAD
```

## 9. Files and directories

Use lowercase snake_case for files and directories. Keep source/header pairs aligned and name files after their primary module, type, or interface, names follow module underlying name, or main class name.

```text
PmsmControl/
    PmsmControlCore.h
    PmsmControlCore.cpp
    PmsmControlApi.h
    PmsmControlApi.cpp
```
### 10. Semantic name modifiers

Modifiers that qualify a quantity must consistently appear **at the beginning of the semantic name**, before the quantity.

Modifiers and the quantity form one camelCase component.

| Modifier | Example       |
| -------- | ------------- |
| `max`    | `maxCurrent`  |
| `min`    | `minCurrent`  |
| `mag`    | `magVoltage`  |
| `peak`   | `peakCurrent` |
| `rms`    | `rmsVoltage`  |
| `avg`    | `avgSpeed`    |

Examples:

```
maxCurrent_ampQ31
minCurrent_ampQ31
magVoltage_vltQ31
peakCurrent_ampQ31
rmsVoltage_vltQ31
avgSpeed_radpsQ31
```

**Rule:** Property modifiers must always precede the quantity. Semantic role modifiers must follow the quantity. Both belong to the same camelCase name component.

Naming order:

`[Property] + Quantity + [Role] + _ + EngineeringSuffix`

Examples:

```cpp
maxCurrentSp_ampQ31
minCurrentFb_ampQ31
magVoltageRef_vltQ31
avgSpeed_radpsQ31
```

Property modifiers include `max`, `min`, `mag`, `peak`, `rms`, and `avg`.

Semantic role modifiers include `Sp` (setpoint), `Fb` (feedback), and `Ref` (reference).

Do not use underscores between these modifiers and the quantity.


## 12. Identifier Abbreviations

Use the following standardized abbreviations when naming identifiers. Prefer the abbreviated form consistently instead of mixing abbreviated and full-length words.

Abbreviations follow the normal identifier casing rules:
- PascalCase: `BufferSize` → `BufferSz`
- camelCase: `bufferSize` → `bufferSz`
- Compound identifiers retain their established component separators.

### 12.1 General

| Full term | Abbreviation |
|---|---|
| Size | `Sz` |
| Length | `Len` |
| Index | `Idx` |
| Position | `Pos` |
| Count | `Cnt` |
| Capacity | `Cap` |
| Maximum | `Max` |
| Minimum | `Min` |
| Average | `Avg` |
| Magnitude | `Mag` |
| Maximum magnitude | `MaxMag` |
| Minimum magnitude | `MinMag` |
| Value | `Val` |
| Number | `Num` |
| Amount | `Amt` |
| Rate | `Rate` |
| Ratio | `Ratio` |
| Factor | `Fac` |
| Offset | `Ofs` |
| Difference | `Diff` |
| Threshold | `Thres` |
| Limit | `Lim` |
| Range | `Rng` |
| Step | `Step` |
| Period | `Per` |
| Interval | `Intvl` |
| Resolution | `Res` |
| Scale | `Scl` |

### 12.2 Memory and Data

| Full term | Abbreviation |
|---|---|
| Buffer | `Buf` |
| Pointer | `Ptr` |
| Reference | `Ref` |
| Address | `Addr` |
| Memory | `Mem` |
| Array | `Arr` |
| Element | `Elem` |
| Entry | `Entry` |
| Data | `Data` |
| Structure | `Struct` |
| Configuration | `Cfg` |
| Descriptor | `Desc` |
| Context | `Ctx` |
| Parameter | `Param` |
| Argument | `Arg` |
| Payload | `Payload` |
| Header | `Hdr` |
| Field | `Field` |
| Record | `Rec` |

### 12.3 Communication and Protocols

| Full term | Abbreviation |
|---|---|
| Transmit | `Tx` |
| Receive | `Rx` |
| Transmission | `Tx` |
| Reception | `Rx` |
| Message | `Msg` |
| Packet | `Pkt` |
| Frame | `Frame` |
| Command | `Cmd` |
| Request | `Req` |
| Response | `Rsp` |
| Service | `Svc` |
| Channel | `Ch` |
| Interface | `If` |
| Protocol | `Proto` |
| Sequence | `Seq` |
| Timeout | `Tmo` |
| Retry | `Retry` |
| Checksum | `Csum` |
| Cyclic redundancy check | `Crc` |
| Encoding | `Enc` |
| Decoding | `Dec` |
| Serializer | `Ser` |
| Deserializer | `Des` |

### 12.4 State and Control

| Full term | Abbreviation |
|---|---|
| State | `St` |
| Status | `Sts` |
| Event | `Evt` |
| Error | `Err` |
| Result | `Res` |
| Flag | `Flag` |
| Mode | `Mode` |
| Control | `Ctrl` |
| Controller | `Ctrl` |
| Reference | `Ref` |
| Setpoint | `Sp` |
| Feedback | `Fb` |
| Target | `Tgt` |
| Commanded | `Cmd` |
| Actual | `Act` |
| Enable | `En` |
| Disable | `Dis` |
| Active | `Act` |
| Inactive | `Inact` |
| Ready | `Rdy` |
| Busy | `Busy` |
| Valid | `Valid` |
| Invalid | `Invalid` |

### 12.5 Hardware and Embedded Systems

| Full term | Abbreviation |
|---|---|
| Driver | `Drv` |
| Device | `Dev` |
| Hardware | `Hw` |
| Peripheral | `Periph` |
| Register | `Reg` |
| Interrupt | `Irq` |
| Interrupt service routine | `Isr` |
| Direct memory access | `Dma` |
| Clock | `Clk` |
| Timer | `Tim` |
| Counter | `Cnt` |
| Prescaler | `Psc` |
| Compare | `Cmp` |
| Capture | `Cap` |
| Input | `In` |
| Output | `Out` |
| General-purpose input/output | `Gpio` |
| Analog-to-digital converter | `Adc` |
| Digital-to-analog converter | `Dac` |
| Pulse-width modulation | `Pwm` |
| Complementary | `Comp` |
| Polarity | `Pol` |
| Deadtime | `Dtc` |

### 12.6 Motor Control and Electrical Quantities

| Full term | Abbreviation |
|---|---|
| Current | `Amp` |
| Voltage | `Vlt` |
| Power | `Pwr` |
| Torque | `Trq` |
| Speed | `Speed` |
| Velocity | `Vel` |
| Position | `Pos` |
| Angle | `Ang` |
| Frequency | `Freq` |
| Temperature | `Temp` |
| Resistance | `Res` |
| Inductance | `Ind` |
| Capacitance | `Cap` |
| Flux | `Flux` |
| Phase | `Ph` |
| Electrical | `Elec` |
| Mechanical | `Mech` |
| Magnitude | `Mag` |
| Direct axis | `D` |
| Quadrature axis | `Q` |
| Bus | `Bus` |
| Sensor | `Sens` |
| Calibration | `Cal` |
| Compensation | `Comp` |
| Estimation | `Est` |
| Reference | `Ref` |
| Setpoint | `Sp` |
| Feedback | `Fb` |

### 12.7 Mathematical and Signal Processing Terms

| Full term | Abbreviation |
|---|---|
| Sine | `Sin` |
| Cosine | `Cos` |
| Square root | `Sqrt` |
| Absolute | `Abs` |
| Reciprocal | `Inv` |
| Derivative | `Dv` |
| Integral | `Int` |
| Proportional | `Prop` |
| Integral (controller term) | `Int` |
| Derivative (controller term) | `Dv` |
| Filter | `Filt` |
| Coefficient | `Cof` |
| Sample | `Spl` |
| Sampling | `Spl` |
| Moving average | `MovAvg` |
| Root mean square | `Rms` |
| Peak | `Peak` |
| Normalized | `Norm` |
| Transform | `Xform` |
| Interpolation | `Interp` |

### 12.8 Naming Rules

1. Prefer the standardized abbreviation whenever one is listed.
2. Do not abbreviate words that are already short and unambiguous, such as `Data`, `Bus`, `Mode`, and `Step`.
3. Preserve established engineering abbreviations such as `Pwm`, `Adc`, `Dma`, `Foc`, `Svm`, and `Pmsm`.
4. Do not introduce additional underscores between abbreviated words. They remain part of the same camelCase or PascalCase component.
5. Engineering units and numerical representations remain separate suffix components, following the established `Prefix_Name_Suffix` convention.
6. Use abbreviations consistently across declarations, definitions, APIs, and generated code.

## 11. Applying the convention

Treat this guide as the naming standard for new code. When editing existing code, follow the convention for the identifiers you add or change; rename surrounding identifiers only when needed for consistency or correctness. Third-party code keeps its upstream naming style.

