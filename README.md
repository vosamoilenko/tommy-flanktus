# Flanktus v6.0 - Auto-Cycling Hydroponic Pump Controller

![Flanktus preview](preview.png)

the first run was:
19 Apr 2026 18:30

Arduino-based pump controller with temperature-adaptive ON/OFF cycling for a rooftop aeroponic tower.
Two waterproof DS18B20 sensors measure water and air temperature. The pump automatically adjusts its spray timing based on air temperature. Logs data to SD card every 60 minutes as CSV. LCD display shows temps, pump state, cycle countdown, and SD status.

**Setup:** Rooftop, open environment, full sun exposure, Central Europe.

## Hardware

- **Arduino Uno** (USB-B powered)
- **Tongling 250VAC relay module** (5V, 1-channel, 10A)
- **Heissner HSP3000-00** water pump (230V AC)
- **DS18B20 x2** waterproof temperature sensors (water + air)
- **SD card module** (SPI, 3.3V regulator onboard)
- **LCD 2004 20x4 I2C** (I2C backpack, 5V)
- **1 push button** (momentary, normally open)
- **2x 4.7k ohm resistors** (pull-up for each DS18B20)
- Jumper wires
- Wago connectors (already attached to pump cables)
- USB-B cable + any 5V USB charger

## Pin Map

| Pin | Component | Protocol | Notes |
|-----|-----------|----------|-------|
| D2  | DS18B20 #1 | OneWire | Water temp DATA + 4.7k pull-up to 5V |
| D4  | DS18B20 #2 | OneWire | Air temp DATA + 4.7k pull-up to 5V |
| D5  | Button | Digital | Auto mode ON/OFF toggle (10x tap = test mode) |
| D7  | Relay IN | Digital | Pump control signal |
| D10 | SD card CS | SPI | Chip select |
| D11 | SD card MOSI | SPI | Fixed SPI pin |
| D12 | SD card MISO | SPI | Fixed SPI pin |
| D13 | SD card SCK | SPI | Fixed SPI pin |
| A4  | LCD SDA | I2C | Data line |
| A5  | LCD SCL | I2C | Clock line |
| 5V  | All VCC | Power | Relay, DS18B20 x2, SD card, LCD |
| GND | All GND | Power | Everything + button |

## Wiring

Open `docs/flanktus-wiring-guide.html` in your browser for visual diagrams (includes light/dark theme toggle and connection table).

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

### SD Card Module (SPI)

| Arduino | SD Module |
|---------|-----------|
| D10     | CS        |
| D11     | MOSI      |
| D12     | MISO      |
| D13     | SCK       |
| 5V      | VCC       |
| GND     | GND       |

The module has an onboard 3.3V regulator. Feed it 5V from the Uno.

### LCD 2004 I2C

| Arduino | LCD I2C |
|---------|---------|
| A4      | SDA     |
| A5      | SCL     |
| 5V      | VCC     |
| GND     | GND     |

Default I2C address: 0x27.

### Button (no resistor needed)

Uses internal pull-up resistor. Wire between the pin and GND.

| Arduino | Button | Function |
|---------|--------|----------|
| D5      | → GND  | Auto mode toggle / test mode (10x tap) |

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
| ≤ 1 C    | —       | —        | Too cold, pump off |
| 1-25 C   | 1 min   | 10 min   | Spring/autumn |
| 25-30 C  | 1 min   | 5 min    | Summer |
| > 30 C   | 2 min   | 2 min    | Peak heat |

The air temperature is re-read every 5 seconds, so timing adjusts as the day heats up or cools down.

### Test Mode

Tap the button **10 times within 3 seconds** to toggle test mode. Tap 10 times again to return to production mode.

Test mode uses short cycle times for verifying pump/relay operation:

| Air Temp | ON time | OFF time |
|----------|---------|----------|
| ≤ 1 C    | —       | —        |
| 1-25 C   | 10 sec  | 30 sec   |
| 25-30 C  | 10 sec  | 20 sec   |
| > 30 C   | 20 sec  | 10 sec   |

The LCD shows `TEST` on line 3 when test mode is active. Serial debug lines and status also indicate test mode.

### LCD Display (20x4)

| Line | Content | Example |
|------|---------|---------|
| 0 | Water + air temp | `W:23.5C     A:28.1C` |
| 1 | Pump state + mode | `PUMP:ON   AUTO:ON` |
| 2 | Cycle countdown | `ON  ends 00:45` or `OFF next 09:32` |
| 3 | SD status + uptime | `SD:OK  Up:2h15m` |

Line 3 shows `SD:WR` for 2 seconds after each write, `SD:--` if no card, and `TEST` when test mode is active.

### SD Card Logging

Every 60 minutes, the Arduino writes a row to `FLANKTUS.CSV` on the SD card:

```
minutes,water_c,air_c,pump,auto
60,21.5,28.3,1,1
120,21.8,29.1,0,1
```

- `minutes` = minutes since boot
- `water_c` / `air_c` = temperature in Celsius
- `pump` = 1 (on) or 0 (off)
- `auto` = 1 (auto mode) or 0 (manual)

Pull the SD card to read the CSV on any computer.

### Serial Commands

Connect via USB (9600 baud) and send:

| Command | Action |
|---------|--------|
| `s` | Show current sensor readings + status |

### Live Debug Logs

The sketch prints real-time debug output every 5 seconds over USB serial (9600 baud):

```
[DBG 0:05] water=21.5C air=28.3C pump=ON auto=ON sd=OK cycle_left=45s
[DBG 0:10] water=21.6C air=28.4C pump=ON auto=ON sd=OK cycle_left=40s
[DBG 0:15] water=21.5C air=28.3C pump=OFF auto=ON sd=OK cycle_left=280s
```

## Scripts

All scripts are in the `scripts/` directory.

### `scripts/setup.bat` / `scripts/setup.ps1` — First-Time Windows Setup

Installs Arduino AVR core and all libraries (OneWire, DallasTemperature, SD, LiquidCrystal I2C), test-compiles, and checks for a connected board.

```powershell
scripts\setup.bat              # Double-click or run from terminal
```

### `scripts/deploy.sh` / `scripts/deploy.bat` — Compile, Upload, Health Check, Monitor

```bash
./scripts/deploy.sh            # Mac/Linux
```
```powershell
scripts\deploy.bat             # Windows
```

### `scripts/upload.sh` / `scripts/upload.bat` — Compile and Upload

```bash
./scripts/upload.sh            # Mac/Linux
```
```powershell
scripts\upload.bat             # Windows
```

### `scripts/read_logs.sh` / `scripts/read_logs.bat` — Stream Live Debug Logs

```bash
./scripts/read_logs.sh         # Stream until Ctrl+C
./scripts/read_logs.sh 60      # Capture for 60 seconds then stop
```
```powershell
scripts\read_logs.bat          # Stream until Ctrl+C
scripts\read_logs.bat 60       # Capture for 60 seconds
```

### `scripts/clean_csv.sh` / `scripts/clean_csv.bat` — Remove Bad Sensor Rows

Removes rows containing `-99` (disconnected sensor reads) from a CSV file.

```bash
./scripts/clean_csv.sh flanktus_dump.csv              # Print to stdout
./scripts/clean_csv.sh flanktus_dump.csv cleaned.csv   # Save to file
```

## Testing

The logic layer (`flanktus_logic.h`) has no Arduino dependencies and is tested natively with C++:

```bash
g++ -std=c++11 -I . -o test/test_flanktus test/test_flanktus.cpp && ./test/test_flanktus
```

Tests cover all temperature thresholds, timing profiles (production + test mode), boundary conditions, and pump cycle decisions.

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
| > 28 C     | Danger   | Root rot risk. Shade reservoir. |
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
│   ├── setup.sh / .bat       # First-time setup (install core + libs)
│   ├── deploy.sh / .bat      # Compile, upload, health check, monitor
│   ├── upload.sh / .bat      # Compile + upload
│   ├── read_logs.sh / .bat   # Stream live serial logs
│   ├── clean_csv.sh / .bat   # Remove bad rows from CSV
│   └── dump_log.bat          # Export EEPROM log (Windows, legacy)
├── docs/
│   └── flanktus-wiring-guide.html  # Interactive wiring diagrams + connection table
├── logs/
│   └── sensor_log.txt        # Captured serial output
└── README.md
```

## Dependencies

```bash
arduino-cli lib install "OneWire" "DallasTemperature" "SD" "LiquidCrystal I2C"
```

## Setup From Scratch

### 1. Install tools

```bash
# Arduino CLI
curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh
sudo mv bin/arduino-cli /usr/local/bin/  # Linux
# Windows: download from https://arduino.github.io/arduino-cli/installation/
```

### 2. Install board + libraries

```bash
arduino-cli core install arduino:avr
arduino-cli lib install "OneWire" "DallasTemperature" "SD" "LiquidCrystal I2C"
```

Or on Windows, just run `scripts\setup.bat`.

### 3. Clone and upload

```bash
git clone https://github.com/vosamoilenko/tommy-flanktus.git
cd tommy-flanktus
./scripts/upload.sh
```

## Quick Reference

| Task | Command |
|------|---------|
| First-time setup (Windows) | `scripts\setup.bat` |
| Find port | `arduino-cli board list` |
| Compile + upload | `./scripts/upload.sh` or `scripts\upload.bat` |
| Deploy + monitor | `./scripts/deploy.sh` or `scripts\deploy.bat` |
| Stream logs | `./scripts/read_logs.sh` or `scripts\read_logs.bat` |
| Run tests | `g++ -std=c++11 -I . -o test/test_flanktus test/test_flanktus.cpp && ./test/test_flanktus` |
| Install libs | `arduino-cli lib install "OneWire" "DallasTemperature" "SD" "LiquidCrystal I2C"` |

## Troubleshooting

**DS18B20 reads -127 C or -99.9 C** — The 4.7k pull-up resistor between DATA and 5V is missing, or the sensor is disconnected.

**LCD blank or garbled** — Check I2C address with `Wire.h` scanner sketch. Some modules use 0x3F instead of 0x27.

**SD card not detected** — Check wiring (CS=D10). Format card as FAT16 or FAT32. Try a different card.

**Buttons don't respond** — Must connect pin to GND (not 5V). Try rotating 90 degrees on the breadboard.

**Upload fails** — Try different USB port/cable. On Linux: `sudo usermod -a -G dialout $USER` then log out/in.

**Port not detected by scripts** — Make sure `arduino-cli` is installed and the board shows up in `arduino-cli board list`.
