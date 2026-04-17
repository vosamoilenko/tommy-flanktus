# Flanktus v5.0 - Auto-Cycling Hydroponic Pump Controller

Arduino-based pump controller with temperature-adaptive ON/OFF cycling for a rooftop aeroponic tower.
Two waterproof DS18B20 sensors measure water and air temperature. The pump automatically adjusts its spray timing based on air temperature. Logs data to EEPROM every 30 minutes, dump as CSV over USB Serial.

**Setup:** Rooftop, open environment, full sun exposure, Central Europe.

## Hardware

- **Arduino Uno** (USB-B powered)
- **Tongling 250VAC relay module** (5V, 1-channel, 10A)
- **Heissner HSP3000-00** water pump (230V AC)
- **DS18B20 x2** waterproof temperature sensors (water + air)
- **1 push button** (momentary, normally open)
- **2x 4.7k ohm resistors** (pull-up for each DS18B20)
- Jumper wires
- Wago connectors (already attached to pump cables)
- USB-B cable + any 5V USB charger

## Pin Map

| Pin | Component | Notes |
|-----|-----------|-------|
| D2  | DS18B20 #1 | Water temp DATA + 4.7k pull-up to 5V |
| D4  | DS18B20 #2 | Air temp DATA + 4.7k pull-up to 5V |
| D5  | Button | Auto mode ON/OFF toggle |
| D7  | Relay IN | Pump control signal |
| 5V  | Relay VCC, both DS18B20 VCC | |
| GND | Relay GND, both DS18B20 GND, Button | |

## Wiring

Open `flanktus-wiring-guide.html` in your browser for visual diagrams (includes light/dark theme toggle).

### Arduino to Relay (3 wires)

| Arduino | Relay Module |
|---------|-------------|
| D7      | IN (signal) |
| 5V      | VCC         |
| GND     | GND         |

### DS18B20 #1 — Water Temperature

| Arduino | DS18B20        |
|---------|----------------|
| D2      | DATA (yellow)  |
| 5V      | VCC (red)      |
| GND     | GND (black)    |

**4.7k ohm pull-up resistor required between DATA and 5V.**

The waterproof probe goes into the water reservoir. Keep wiring and Arduino away from water.

### DS18B20 #2 — Air Temperature

| Arduino | DS18B20        |
|---------|----------------|
| D4      | DATA (yellow)  |
| 5V      | VCC (red)      |
| GND     | GND (black)    |

**4.7k ohm pull-up resistor required between DATA and 5V.**

Mount outside in open air, shaded from direct sun. The waterproof casing means it survives rain.

### Button (no resistor needed)

| Arduino | Button | Function |
|---------|--------|----------|
| D5      | → GND  | Auto mode toggle |

### Mains wiring (230V via Wago connectors)

| Wire Color    | Where it goes |
|---------------|---------------|
| **Brown (L)** | Cut. One end to relay **COM**, other to relay **NO** |
| **Blue (N)**  | Straight from mains to pump |
| **Yellow/Green (PE)** | Straight from mains to pump |

**Only the brown (live) wire goes through the relay.**

## How It Works

### Auto Pump Cycling

Press the **button** to toggle auto mode ON or OFF.

- **LED solid ON** = auto mode active, pump is cycling
- **LED OFF** = auto mode disabled, pump is off
- **3 fast blinks** = auto mode just turned ON
- **1 long blink** = auto mode just turned OFF
- **2 blinks** = sensor log entry written

When auto mode is ON, the pump cycles automatically based on air temperature:

| Air Temp | ON time | OFF time | Season |
|----------|---------|----------|--------|
| < 10 C   | —       | —        | Too cold, pump off |
| 10-25 C  | 1 min   | 10 min   | Spring/autumn |
| 25-30 C  | 1 min   | 5 min    | Summer |
| > 30 C   | 2 min   | 2 min    | Peak heat |

The air temperature is re-read every 30 seconds, so timing adjusts as the day heats up or cools down.

### Sensor Logging

Every 60 minutes, the Arduino reads both sensors and stores a 6-byte entry in EEPROM:

| Field | Bytes | Range |
|-------|-------|-------|
| Minutes since boot | 2 | 0-65535 (~45 days) |
| Water temp x10 | 2 | -999 to 999 (-99.9 to 99.9 C) |
| Air temp x10 | 2 | -999 to 999 |

**Capacity:** 170 entries = **~7 days** at 60-minute intervals.

### Serial Commands

Connect via USB (9600 baud) and send:

| Command | Action |
|---------|--------|
| `d` | Dump all logged data as CSV |
| `c` | Clear the EEPROM log |
| `s` | Show current sensor readings + status |

### Live Debug Logs

The sketch prints real-time debug output every 5 seconds over USB serial (9600 baud):

```
[DBG 0:05] water=21.5C air=28.3C pump=ON auto=ON cycle_left=45s
[DBG 0:10] water=21.6C air=28.4C pump=ON auto=ON cycle_left=40s
[DBG 0:15] water=21.5C air=28.3C pump=OFF auto=ON cycle_left=280s
```

Each line shows: timestamp (min:sec since boot), both sensor readings, pump state, auto mode state, and time remaining in the current ON/OFF cycle.

**Quick read with the helper script:**

```bash
# Stream to console + save to sensor_log.txt (Ctrl+C to stop)
./read_logs.sh

# Capture for 60 seconds then stop automatically
./read_logs.sh 60
```

The script auto-detects the Arduino port, prints to console, and saves everything to `sensor_log.txt`.

**Manual read (no script):**

```bash
# Find port
arduino-cli board list

# Read serial output (Ctrl+C to stop)
stty -f /dev/cu.usbmodem* 9600 && cat /dev/cu.usbmodem*

# Or save to file
stty -f /dev/cu.usbmodem* 9600 && cat /dev/cu.usbmodem* | tee sensor_log.txt
```

**Sensor reads -99.9 C** — sensor disconnected or missing 4.7kΩ pull-up resistor.

### Exporting EEPROM Data

The Arduino logs sensor data to EEPROM every 60 minutes (170 entries, ~7 days). Export it as a CSV file to analyze or share.

#### Windows — double-click

1. Plug Arduino into USB
2. Double-click **`dump_log.bat`**
3. It auto-detects the Arduino, dumps the log, and saves a timestamped CSV file (e.g. `flanktus_dump_2026-04-17_1430.csv`)
4. Send the CSV file

If auto-detect fails, run manually:

```powershell
powershell -ExecutionPolicy Bypass -File dump_log.ps1 COM3
```

Replace `COM3` with your port — find it in Device Manager → Ports (COM & LPT).

#### Mac/Linux

```bash
# Stream live debug logs
./read_logs.sh

# Or dump EEPROM data manually
stty -f /dev/cu.usbmodem* 9600 && echo 'd' > /dev/cu.usbmodem*
cat /dev/cu.usbmodem* | tee flanktus_dump.csv
# Wait for "--- X/170 entries ---" line, then Ctrl+C
```

#### CSV format

```
minutes,water_c,air_c
60,21.5,28.3
120,21.8,29.1
180,22.1,31.0
```

- `minutes` = minutes since Arduino boot
- `water_c` / `air_c` = temperature in Celsius (one decimal)

#### Serial commands (manual)

Connect via any serial terminal at 9600 baud:

| Command | Action |
|---------|--------|
| `d` | Dump all logged data as CSV |
| `c` | Clear the EEPROM log |
| `s` | Show current sensor readings + status |

## Year-Round Pump Timing Profiles

### Celery on a rooftop in Central Europe

Rooftop = extreme heat in summer. Roof surface radiates extra heat. Wind speeds up root drying.

### Quick Reference Card

| Season        | Temp Range | ON   | OFF (day) | OFF (night) |
|---------------|------------|------|-----------|-------------|
| Winter indoor | 15-20 C    | 1 min | 15 min   | 30 min      |
| Early spring  | 10-18 C    | 1 min | 10 min   | 20 min      |
| Late spring   | 15-25 C    | 1 min | 5-6 min  | 10-12 min   |
| Early summer  | 22-28 C    | 1 min | 4 min    | 8 min       |
| Peak summer   | 28-35+ C   | 2 min | 2-3 min  | 4-6 min     |
| Early autumn  | 15-22 C    | 1 min | 5-8 min  | 10-15 min   |
| Late autumn   | 8-15 C     | 1 min | 10 min   | 20 min      |

Night = 22:00-06:00. Double the OFF time.

### Water Temperature Thresholds

| Water Temp | Status   | Action |
|------------|----------|--------|
| < 16 C     | Cold     | Normal for spring. Growth slow. |
| 16-22 C    | Ideal    | Optimal root zone. |
| 22-26 C    | Warm     | Watch for slime on roots. |
| 26-28 C    | Warning  | Increase spray. Add frozen bottles. |
| > 28 C     | Danger   | Root rot risk. Shade reservoir. Add ice. |
| > 32 C     | Critical | Stop and shade everything immediately. |

## Dependencies

```bash
arduino-cli lib install "OneWire"
arduino-cli lib install "DallasTemperature"
```

## Setup From Scratch

### 1. Install tools

```bash
# Git
sudo apt install git  # Linux
# or download from https://git-scm.com/download/win  # Windows

# Arduino CLI
curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh
sudo mv bin/arduino-cli /usr/local/bin/  # Linux
# Windows: move to ~/arduino-tools/ and add to PATH
```

### 2. Install board + libraries

```bash
arduino-cli core install arduino:avr
arduino-cli lib install "OneWire" "DallasTemperature"
```

### 3. Clone and upload

```bash
git clone https://github.com/vosamoilenko/tommy-flanktus.git
cd tommy-flanktus
arduino-cli compile --fqbn arduino:avr:uno flanktus_pump
arduino-cli upload -p /dev/ttyACM0 --fqbn arduino:avr:uno flanktus_pump
```

Replace `/dev/ttyACM0` with your port (`COM3` on Windows). Find it with `arduino-cli board list`.

## Quick Reference

| Task | Command |
|------|---------|
| Find port | `arduino-cli board list` |
| Compile | `arduino-cli compile --fqbn arduino:avr:uno flanktus_pump` |
| Upload | `arduino-cli upload -p PORT --fqbn arduino:avr:uno flanktus_pump` |
| Install libs | `arduino-cli lib install "OneWire" "DallasTemperature"` |

## Troubleshooting

**DS18B20 reads -127 C** — The 4.7k pull-up resistor between DATA and 5V is missing.

**Buttons don't respond** — Must connect pin to GND (not 5V). Try rotating 90 degrees.

**Upload fails** — Try different USB port/cable. On Linux: `sudo usermod -a -G dialout $USER` then log out/in.

**EEPROM data looks wrong** — Send `c` to clear and start fresh. Power cycling doesn't clear EEPROM.
