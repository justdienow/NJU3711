# NJU3711 Hardware Guide

Complete hardware reference for the NJU3711 8-bit serial-to-parallel converter.

## Table of Contents

- [IC Specifications](#ic-specifications)
- [Pin Configuration](#pin-configuration)
- [Basic Wiring](#basic-wiring)
- [7-Segment Display Wiring](#7-segment-display-wiring)
- [Multi-Digit Display Wiring](#multi-digit-display-wiring)
- [PCB Design Notes](#pcb-design-notes)
- [Power Supply](#power-supply)
- [Current Limiting](#current-limiting)

---

## IC Specifications

### NJU3711 Features

- **Function:** 8-bit Serial-In, Parallel-Out Shift Register with Latches
- **Operating Voltage:** 5V ± 10% (4.5V to 5.5V)
- **Maximum Clock Frequency:** 5MHz
- **Output Current:** 25mA per pin (can drive LEDs directly)
- **Input Type:** Hysteresis inputs (Schmitt trigger) - 0.5V typical
- **Technology:** C-MOS
- **Packages:** DIP14, DMP14, SSOP14

### Electrical Characteristics

| Parameter | Symbol | Min | Typ | Max | Unit | Conditions |
|-----------|--------|-----|-----|-----|------|------------|
| Supply Voltage | VDD | 4.5 | 5.0 | 5.5 | V | - |
| Operating Current | IDDS | - | - | 0.1 | mA | VIH=VDD, VIL=VSS |
| High-level Input | VIH | 0.7VDD | - | VDD | V | - |
| Low-level Input | VIL | VSS | - | 0.3VDD | V | - |
| Input Leakage | ILI | -10 | - | 10 | µA | VI=0~VDD |
| High Output @ 25mA | VOHD | VDD-1.5 | - | VDD | V | IOH=-25mA |
| High Output @ 15mA | VOHD | VDD-1.0 | - | VDD | V | IOH=-15mA |
| High Output @ 10mA | VOHD | VDD-0.5 | - | VDD | V | IOH=-10mA |
| Low Output @ 25mA | VOLD | VSS | - | 1.5 | V | IOL=+25mA |
| Low Output @ 15mA | VOLD | VSS | - | 0.8 | V | IOL=+15mA |
| Low Output @ 10mA | VOLD | VSS | - | 0.4 | V | IOL=+10mA |

### Timing Specifications

| Parameter | Symbol | Min | Typ | Max | Unit |
|-----------|--------|-----|-----|-----|------|
| Setup Time (DATA-CLK) | tSD | 20 | - | - | ns |
| Hold Time (CLK-DATA) | tHD | 20 | - | - | ns |
| Setup Time (STB-CLK) | tSSTB | 30 | - | - | ns |
| Hold Time (CLK-STB) | tHSTB | 30 | - | - | ns |
| Output Delay (CLK-Px) | tpd PCK | - | - | 100 | ns |
| Output Delay (STB-Px) | tpd PSTB | - | - | 80 | ns |
| Output Delay (CLR-Px) | tpd PCLR | - | - | 80 | ns |
| Max Frequency | fMAX | 5 | - | - | MHz |

---

## Pin Configuration

### DIP14/DMP14/SSOP14 Pinout

```
         NJU3711
     ┌─────────────┐
  P3 │1    ∪    14│ VDD
  P4 │2         13│ P2
  P5 │3         12│ P1
 VSS │4         11│ CLR
  P6 │5         10│ STB
  P7 │6          9│ CLK
  P8 │7          8│ DATA
     └─────────────┘
```

### Pin Descriptions

| Pin | Name | Type | Description |
|-----|------|------|-------------|
| 1 | P3 | Output | Parallel output 3 |
| 2 | P4 | Output | Parallel output 4 |
| 3 | P5 | Output | Parallel output 5 |
| 4 | VSS | Power | Ground (0V) |
| 5 | P6 | Output | Parallel output 6 |
| 6 | P7 | Output | Parallel output 7 |
| 7 | P8 | Output | Parallel output 8 |
| 8 | DATA | Input | Serial data input (with hysteresis) |
| 9 | CLK | Input | Clock input (data shifts on rising edge) |
| 10 | STB | Input | Strobe/Latch (HIGH=shift, LOW=latch) |
| 11 | CLR | Input | Clear (LOW=clear all outputs) |
| 12 | P1 | Output | Parallel output 1 |
| 13 | P2 | Output | Parallel output 2 |
| 14 | VDD | Power | Power supply (+5V) |

### Data Flow

1. **DATA** → Shift Register (on CLK rising edge)
2. **Shift Register** → Latches (when STB goes LOW)
3. **Latches** → Output Buffers (P1-P8)

### Pin States

| CLK | STB | CLR | Operation |
|-----|-----|-----|-----------|
| ↑ | HIGH | HIGH | Data shifts into register |
| - | ↓ HIGH | HIGH | Register contents latch to outputs |
| - | - | LOW | All outputs cleared to LOW |

---

## Basic Wiring

### Minimum Configuration (CLR Strapped HIGH)

This is the most common configuration when you don't need software clear control.

```
Arduino          NJU3711
                 ┌─────┐
Pin 2  ────────→ │DATA │
Pin 3  ────────→ │CLK  │
Pin 4  ────────→ │STB  │
                 │     │
+5V    ────────→ │VDD  │
+5V    ────────→ │CLR  │ (strapped HIGH)
GND    ────────→ │VSS  │
                 └─────┘
```

**Arduino Code:**
```cpp
NJU3711 expander(2, 3, 4); // DATA, CLK, STB
```

### With Software CLR Control

If you need to clear outputs via software:

```
Arduino          NJU3711
                 ┌─────┐
Pin 2  ────────→ │DATA │
Pin 3  ────────→ │CLK  │
Pin 4  ────────→ │STB  │
Pin 5  ────────→ │CLR  │
                 │     │
+5V    ────────→ │VDD  │
GND    ────────→ │VSS  │
                 └─────┘
```

**Arduino Code:**
```cpp
NJU3711 expander(2, 3, 4, 5); // DATA, CLK, STB, CLR
```

### Connecting LEDs Directly

The NJU3711 can drive LEDs directly (up to 25mA per output).

```
                  NJU3711
                  ┌─────┐
                  │ P1  ├────┬─── LED1+ ───▷|─── [220Ω] ─── GND
                  │ P2  ├────┤
                  │ P3  ├────┤
                  │ P4  ├────┤    (Repeat for each output)
                  │ P5  ├────┤
                  │ P6  ├────┤
                  │ P7  ├────┤
                  │ P8  ├────┘
                  └─────┘
```

**Current Limiting Resistor Calculation:**
```
R = (VDD - Vf) / Iled

Example for Red LED:
R = (5V - 2V) / 0.015A = 200Ω (use 220Ω standard value)
```

---

## 7-Segment Display Wiring

### Pin Mapping Reference

The library uses this specific mapping from NJU3711 outputs to 7-segment segments:

```
    AAA          Bit 7 = A (top)
   F   B         Bit 6 = F (top left)
   F   B         Bit 5 = B (top right)
    GGG          Bit 4 = G (middle)
   E   C         Bit 3 = E (bottom left)
   E   C         Bit 2 = C (bottom right)
 DP DDD          Bit 1 = D (bottom)
                 Bit 0 = DP (decimal point)
```

### Common Cathode Display (ACTIVE_LOW)

Most common configuration - this is what the PCB uses.

```
           NJU3711                    7-Segment Display
           ┌─────┐                    (Common Cathode)
           │ P8  ├─── [R] ─────────→ A (pin 7)
           │ P7  ├─── [R] ─────────→ F (pin 9)
           │ P6  ├─── [R] ─────────→ B (pin 6)
           │ P5  ├─── [R] ─────────→ G (pin 10)
           │ P4  ├─── [R] ─────────→ E (pin 1)
           │ P3  ├─── [R] ─────────→ C (pin 4)
           │ P2  ├─── [R] ─────────→ D (pin 2)
           │ P1  ├─── [R] ─────────→ DP (pin 5)
           └─────┘
                                      Common ─→ GND
```

**Resistor Value:** Use 220-470Ω per segment

**Arduino Code:**
```cpp
NJU3711_7Segment display(2, 3, 4, ACTIVE_LOW);
```

**Important:** LEDs are ON when NJU3711 output is LOW!

### Common Anode Display (ACTIVE_HIGH)

Less common, but supported:

```
           NJU3711                    7-Segment Display
           ┌─────┐                    (Common Anode)
           │ P8  ├─── [R] ─────────→ A
           │ P7  ├─── [R] ─────────→ F
           │ P6  ├─── [R] ─────────→ B
           │ P5  ├─── [R] ─────────→ G
           │ P4  ├─── [R] ─────────→ E
           │ P3  ├─── [R] ─────────→ C
           │ P2  ├─── [R] ─────────→ D
           │ P1  ├─── [R] ─────────→ DP
           └─────┘
                                      Common ─→ +5V
```

**Arduino Code:**
```cpp
NJU3711_7Segment display(2, 3, 4, ACTIVE_HIGH);
```

### Standard 7-Segment Display Pinout

Most 7-segment displays follow this pinout:

```
     Top View (looking at display face)
     
     ┌───────────────┐
   1 │•             •│ 10
   2 │•             •│ 9
   3 │•             •│ 8
   4 │•             •│ 7
   5 │•             •│ 6
     └───────────────┘

Pin 1:  E
Pin 2:  D
Pin 3:  Common (cathode/anode)
Pin 4:  C
Pin 5:  DP
Pin 6:  B
Pin 7:  A
Pin 8:  Common (cathode/anode)
Pin 9:  F
Pin 10: G
```

**Always verify with datasheet - pinouts vary!**

---

## Multi-Digit Display Wiring

### 3-Digit Multiplexed Display

The PCB design uses **high-side PNP switching** for digit selection.

```
Arduino          NJU3711              Displays
                 ┌─────┐
Pin 2  ────────→ │DATA │
Pin 3  ────────→ │CLK  ├─ Shared segments to all 3 displays
Pin 4  ────────→ │STB  │  (through resistors)
                 └─────┘
                 
                 Digit Selection (High-Side PNP Switching)
                 
Pin 5  ────┬─── [R] ─── B ───┐
           │               E ├─── PNP1 ─── Digit 1 Common
          GND              C ─── +5V
          
Pin 6  ────┬─── [R] ─── B ───┐
           │               E ├─── PNP2 ─── Digit 2 Common
          GND              C ─── +5V
          
Pin 7  ────┬─── [R] ─── B ───┐
           │               E ├─── PNP3 ─── Digit 3 Common
          GND              C ─── +5V

Note: Base resistor ~1kΩ, PNP transistors like 2N3906
```

**Arduino Code:**
```cpp
NJU3711_7Segment_Multi display(2, 3, 4,  // NJU3711 pins
                                5, 6, 7,  // Digit select pins
                                ACTIVE_LOW);
```

### How Multiplexing Works

1. **Turn OFF all digits** (PNP transistors OFF)
2. **Write segment data** for digit 1
3. **Turn ON digit 1** (brief blanking period prevents ghosting)
4. **Wait ~2ms** (digit stays lit)
5. **Repeat** for digits 2 and 3

The cycle repeats fast enough (~167Hz) that persistence of vision makes all digits appear continuously lit.

### Cascaded 6-Digit Display

For the cascaded clock example with two 3-digit PCBs:

```
Arduino          PCB 1 & PCB 2 (Shared Bus)
                 ┌─────┐  ┌─────┐
Pin 2  ────────→ │DATA │──│DATA │
Pin 3  ────────→ │CLK  │──│CLK  │
Pin 4  ────────→ │STB  │──│STB  │
                 └─────┘  └─────┘

Individual Digit Select:
Pin 5  ────→ Digit 1 (leftmost - hours tens)
Pin 6  ────→ Digit 2 (hours ones)
Pin 7  ────→ Digit 3 (minutes tens)
Pin 8  ────→ Digit 4 (minutes ones)
Pin 9  ────→ Digit 5 (seconds tens)
Pin 10 ────→ Digit 6 (rightmost - seconds ones)
```

---

## PCB Design Notes

### Your PCB Configuration

Based on the PCB images provided:

**Left Side (Control):**
- IC1: NJU3711 shift register
- CN1: 8-pin header for Arduino connection
  - STB, DAT, CLK pins
  - F3, F2, F1 (footprints for filtering/protection)
  - D3, D2, D1 pins
- TR1: Transistor for power control
- TR2: Additional transistor

**Right Side (Display):**
- 24 LEDs arranged as three 7-segment displays
- LED1-LED8: First digit
- LED9-LED16: Second digit (middle)
- LED17-LED24: Third digit
- Current limiting resistors (1A, 1F, 1B, etc.)
- Decimal points between digits

**Power:**
- Red jumpers/traces for +5V distribution
- Common ground plane

### Design Considerations

1. **Current Limiting Resistors**
   - Individual resistor per segment (not per digit)
   - Placed close to LEDs
   - Value: 220-470Ω depending on desired brightness

2. **Transistor Switching**
   - High-side PNP switching
   - Allows proper multiplexing
   - Base resistors ~1kΩ

3. **Trace Width**
   - Power traces should handle total current
   - Example: 3 digits × 8 segments × 15mA = 360mA peak (but multiplexed)
   - Actual average: ~120mA

4. **Decoupling**
   - Add 100nF ceramic capacitor near VDD/VSS
   - Optional: 10µF electrolytic for bulk decoupling

5. **CLR Pin**
   - Your PCB has CLR strapped to VDD (always HIGH)
   - This is the recommended configuration
   - Saves one Arduino pin

---

## Power Supply

### Power Requirements

**NJU3711 IC:**
- Operating current: <0.1mA (logic only)
- No significant current through IC - outputs drive external loads

**LED Displays:**
- Per segment: 10-20mA typical, 25mA maximum
- Per digit (all segments): up to 200mA
- **Multiplexed 3-digit display:** 60-80mA average

**Total System:**
- Arduino: ~50mA
- NJU3711: <1mA
- LEDs (3-digit multiplexed): 60-80mA
- **Total: ~150mA typical**

### Power Supply Recommendations

1. **USB Power (5V):** Suitable for most applications
2. **Wall Adapter:** 5V, 500mA minimum, 1A recommended
3. **Battery:** 
   - 3× AA (4.5V) works but dimmer
   - 4× AA (6V) needs voltage regulator
   - 9V battery not recommended (inefficient with regulator)

### Power Distribution

```
Power Source (+5V)
       │
       ├──→ Arduino VIN/5V
       ├──→ NJU3711 VDD (pin 14)
       └──→ Display Common Anodes (if used)

Common Ground
       │
       ├──→ Arduino GND
       ├──→ NJU3711 VSS (pin 4)
       └──→ Display Common Cathodes (if used)
```

**Important:** All grounds must be connected together!

---

## Current Limiting

### Why Current Limiting is Critical

1. **LED Protection:** LEDs can burn out without current limiting
2. **IC Protection:** NJU3711 outputs rated for 25mA maximum
3. **Brightness Control:** Resistor value affects brightness
4. **Power Consumption:** Lower current = longer battery life

### Resistor Calculation

**Formula:**
```
R = (Vsupply - Vf) / Iled

Where:
  Vsupply = Supply voltage (5V)
  Vf = LED forward voltage (1.8-2.2V for red, 3.0-3.4V for blue/white)
  Iled = Desired LED current (10-20mA typical)
```

**Example Calculations:**

| LED Color | Vf | Iled | Calculation | Standard Value |
|-----------|-----|------|-------------|----------------|
| Red | 2.0V | 15mA | (5-2)/0.015 = 200Ω | **220Ω** |
| Red (bright) | 2.0V | 20mA | (5-2)/0.020 = 150Ω | **150Ω** or **180Ω** |
| Red (dim) | 2.0V | 10mA | (5-2)/0.010 = 300Ω | **330Ω** |
| Green | 2.2V | 15mA | (5-2.2)/0.015 = 187Ω | **220Ω** |
| Blue/White | 3.2V | 15mA | (5-3.2)/0.015 = 120Ω | **120Ω** or **150Ω** |

**Recommended:** 220Ω for red/green, 150Ω for blue/white

### Multiplexed Display Considerations

In multiplexed displays, each digit is only ON 1/3 of the time (for 3 digits).

**Brightness Compensation Options:**

1. **Increase current** (use lower resistor)
   - Example: 150Ω instead of 220Ω
   - Risk: May exceed 25mA limit if all segments on
   
2. **Increase duty cycle** (faster multiplexing)
   - Reduce multiplex delay: `setMultiplexDelay(1000)` (1ms instead of 2ms)
   - Benefit: Same brightness with same current
   
3. **Balance both**
   - 180Ω resistor + 1.5ms delay
   - Best compromise

### Power Dissipation in Resistors

**For 220Ω resistor at 15mA:**
```
P = I² × R = 0.015² × 220 = 0.05W (50mW)
```

Use **1/4W (250mW) resistors** - standard through-hole size is fine.

---

## Troubleshooting Hardware Issues

### Display Not Working

1. **Check power:** Verify 5V at VDD (pin 14) and 0V at VSS (pin 4)
2. **Check connections:** Verify DATA, CLK, STB pins
3. **Test IC:** Run test pattern: `expander.write(0xFF);`
4. **Check resistors:** Verify current-limiting resistors present

### Segments Always On or Always Off

1. **Wrong display mode:** Check `ACTIVE_LOW` vs `ACTIVE_HIGH`
2. **Missing resistors:** Segments may be damaged without current limiting
3. **Reversed polarity:** Common cathode/anode swapped

### Flickering Display

1. **Multiplexing too slow:** Reduce `setMultiplexDelay()` to 1500-2000µs
2. **Missing `update()` call:** Must call `display.update()` every loop
3. **Power supply issues:** Check for voltage drop under load

### Ghosting (Wrong Digits Lit)

1. **Blanking time too short:** Increase with `setBlankingTime(50)` or higher
2. **Transistor switching issue:** Check base resistors (~1kΩ)
3. **Update frequency:** Ensure `update()` called consistently

### Dim Display

1. **Resistors too high:** Try lower value (220Ω → 150Ω)
2. **Multiplex delay too short:** Increase to 2000-3000µs
3. **Power supply:** Check for voltage drop
4. **LED quality:** Some LEDs are naturally dimmer

---

## Safety Notes

1. **Never exceed 25mA per output** - Can damage NJU3711
2. **Always use current-limiting resistors** - Protects LEDs and IC
3. **Check polarity** - Reversed power can destroy ICs
4. **ESD precautions** - C-MOS ICs are static sensitive
5. **Heat sinks** - Not needed for normal operation
6. **Short circuits** - Can damage outputs - use proper wiring

---

## See Also

- [API Reference](API_Reference.md) - Complete method documentation
- [Getting Started](Getting_Started.md) - First project tutorial
- [Advanced Usage](Advanced_Usage.md) - Complex applications
- [Troubleshooting](Troubleshooting.md) - Common issues