# Flanktus v5.0 - Auto-Cycling Hydroponic Pump Controller

Arduino-based pump controller with temperature-adaptive ON/OFF cycling for a rooftop aeroponic tower.
Two waterproof DS18B20 sensors measure water and air temperature. The pump automatically adjusts its spray timing based on air temperature. Logs data to EEPROM every 60 minutes, dump as CSV over USB Serial.

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

Open `docs/flanktus-wiring-guide.html` in your browser for visual diagrams (includes light/dark theme toggle).

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

Uses internal pull-up resistor. Wire between the pin and GND.

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

When auto mode is ON, the pump cycles automatically based on air temperature:

| Air Temp | ON time | OFF time | Season |
|----------|---------|----------|--------|
| < 10 C   | —       | —        | Too cold, pump off |
| 10-25 C  | 1 min   | 10 min   | Spring/autumn |
| 25-30 C  | 1 min   | 5 min    | Summer |
| > 30 C   | 2 min   | 2 min    | Peak heat |

The air temperature is re-read every 30 seconds, so timing adjusts as the day heats up or cools down.

### LED Indicators

| LED Pattern | Meaning |
|-------------|---------|
| Solid ON | Auto mode active, pump is cycling |
| OFF | Auto mode disabled, pump is off |
| 3 fast blinks | Auto mode just turned ON |
| 1 long blink | Auto mode just turned OFF |
| 2 blinks | Sensor log entry written |

### Sensor Logging

Every 60 minutes, the Arduino reads both sensors and stores a 6-byte entry in EEPROM:

| Field | Bytes | Range |
|-------|-------|-------|
| Minutes since boot | 2 | 0-65535 (~45 days) |
| Water temp x10 | 2 | -999 to 999 (-99.9 to 99.9 C) |
| Air temp x10 | 2 | -999 to 999 |

**Capacity:** 170 entries = **~7 days** at 60-minute intervals. Uses a ring buffer — oldest entries are overwritten when full.

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

## Scripts

All scripts are in the `scripts/` directory. Make them executable first:

```bash
chmod +x scripts/*.sh
```

### `scripts/upload.sh` / `scripts/upload.bat` — Compile and Upload

Auto-detects the Arduino port, compiles the sketch, and uploads it in one step.

```bash
./scripts/upload.sh            # Mac/Linux
```
```powershell
scripts\upload.bat             # Windows (double-click or run from terminal)
```

### `scripts/read_logs.sh` / `scripts/read_logs.bat` — Stream Live Debug Logs

Streams real-time serial output to the console and saves to `logs/sensor_log.txt`.

```bash
./scripts/read_logs.sh         # Stream until Ctrl+C
./scripts/read_logs.sh 60      # Capture for 60 seconds then stop
```
```powershell
scripts\read_logs.bat          # Stream until Ctrl+C
scripts\read_logs.bat 60       # Capture for 60 seconds
scripts\read_logs.bat 60 COM3  # Specify port manually
```

### `scripts/clear_eeprom.sh` / `scripts/clear_eeprom.bat` — Clear EEPROM Log

Sends the `c` command to the Arduino to wipe all logged sensor data.

```bash
./scripts/clear_eeprom.sh      # Mac/Linux
```
```powershell
scripts\clear_eeprom.bat       # Windows (double-click or run from terminal)
```

### `scripts/clean_csv.sh` / `scripts/clean_csv.bat` — Remove Bad Sensor Rows

Removes rows containing `-99` (disconnected sensor reads) from an exported CSV file.

```bash
./scripts/clean_csv.sh flanktus_dump.csv              # Print to stdout
./scripts/clean_csv.sh flanktus_dump.csv cleaned.csv   # Save to file
```
```powershell
scripts\clean_csv.bat flanktus_dump.csv                # Print to console
scripts\clean_csv.bat flanktus_dump.csv cleaned.csv    # Save to file
```

### `scripts/dump_log.bat` / `scripts/dump_log.ps1` — Windows EEPROM Export

For Windows users. Double-click `dump_log.bat` or run from PowerShell:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\dump_log.ps1
```

- Auto-detects Arduino by USB vendor ID (Arduino, CH340, FTDI)
- Falls back to asking the user to select a COM port
- Saves a timestamped CSV file (e.g. `flanktus_dump_2026-04-17_1430.csv`)

To specify a port manually:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\dump_log.ps1 COM3
```

Find your port in Device Manager → Ports (COM & LPT).

### Exporting EEPROM Data (Mac/Linux, no script)

```bash
# Find port
arduino-cli board list

# Send dump command and capture output
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

## Testing

The logic layer (`flanktus_logic.h`) has no Arduino dependencies and is tested natively with C++:

```bash
g++ -std=c++11 -I . -o test/test_flanktus test/test_flanktus.cpp && ./test/test_flanktus
```

Tests cover all temperature thresholds, timing profiles, EEPROM layout, ring buffer wrap-around, and pump cycle decisions.

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

## Project Structure

```
tommy-flanktus/
├── flanktus_pump/
│   ├── flanktus_pump.ino     # Main Arduino sketch
│   └── flanktus_logic.h      # Pure logic (testable without Arduino)
├── test/
│   └── test_flanktus.cpp     # Native C++ unit tests
├── scripts/
│   ├── upload.sh / .bat      # Compile + upload
│   ├── read_logs.sh / .bat   # Stream live serial logs
│   ├── clear_eeprom.sh / .bat # Clear logged sensor data
│   ├── clean_csv.sh / .bat   # Remove bad rows from CSV
│   ├── dump_log.bat          # Export EEPROM log (Windows, double-click)
│   └── dump_log.ps1          # Export EEPROM log (PowerShell)
├── docs/
│   └── flanktus-wiring-guide.html  # Interactive wiring diagrams
├── logs/
│   └── sensor_log.txt        # Captured serial output
└── README.md
```

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
./scripts/upload.sh
```

Or manually:

```bash
arduino-cli compile --fqbn arduino:avr:uno flanktus_pump
arduino-cli upload -p /dev/ttyACM0 --fqbn arduino:avr:uno flanktus_pump
```

Replace `/dev/ttyACM0` with your port (`COM3` on Windows). Find it with `arduino-cli board list`.

## Quick Reference

| Task | Command |
|------|---------|
| Find port | `arduino-cli board list` |
| Compile + upload | `./scripts/upload.sh` or `scripts\upload.bat` |
| Stream logs | `./scripts/read_logs.sh` or `scripts\read_logs.bat` |
| Clear EEPROM | `./scripts/clear_eeprom.sh` or `scripts\clear_eeprom.bat` |
| Clean CSV | `./scripts/clean_csv.sh input.csv` or `scripts\clean_csv.bat input.csv` |
| Export log (Windows) | Double-click `scripts/dump_log.bat` |
| Run tests | `g++ -std=c++11 -I . -o test/test_flanktus test/test_flanktus.cpp && ./test/test_flanktus` |
| Install libs | `arduino-cli lib install "OneWire" "DallasTemperature"` |

## Troubleshooting

**DS18B20 reads -127 C or -99.9 C** — The 4.7k pull-up resistor between DATA and 5V is missing, or the sensor is disconnected.

**Buttons don't respond** — Must connect pin to GND (not 5V). Try rotating 90 degrees on the breadboard.

**Upload fails** — Try different USB port/cable. On Linux: `sudo usermod -a -G dialout $USER` then log out/in.

**EEPROM data looks wrong** — Send `c` to clear and start fresh, or run `./scripts/clear_eeprom.sh`. Power cycling doesn't clear EEPROM.

**Port not detected by scripts** — Make sure `arduino-cli` is installed and the board shows up in `arduino-cli board list`.
