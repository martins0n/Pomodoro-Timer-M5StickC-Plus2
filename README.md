# Pomodoro Timer — M5StickC Plus2

A Pomodoro focus timer for the M5StickC Plus2 with 5 presets, a custom regime editor, power saving, and persistent settings.

Based on the [M5 StickC Plus 2 as a Pomodoro Timer](https://www.hackster.io/asamakerlab/m5-stickc-plus-2-as-a-pomodoro-timer-be4c75) project by **asamakerlab** on Hackster.io (MIT License). This version adds multiple presets, a custom regime editor with flash persistence, full power management, and double-buffered rendering.

## Presets

| # | Name | Work | Break | Loops |
|---|------|------|-------|-------|
| 1 | Pomodoro 25/5 | 25 min | 5 min | 4x |
| 2 | Short Focus | 15 min | 3 min | 6x |
| 3 | Long Session | 50 min | 10 min | 2x |
| 4 | Quick Test | 10 sec | 5 sec | 2x |
| 5 | Custom | editable | editable | editable |

Custom values are saved to flash and persist across reboots.

## Button Controls

**A** = front button, **B** = side button, **PWR** = power/side button

### Menu
| Button | Action |
|--------|--------|
| A | Next preset |
| B | Previous preset |
| Hold A | Select & start timer |
| PWR | Enter custom editor (on "Custom" preset) |

### Custom Editor
| Button | Action |
|--------|--------|
| A | Increment value |
| B | Decrement value |
| PWR | Confirm field & advance (Work → Break → Loops → Start) |
| Hold B | Cancel, back to menu |

### Timer Running
| Button | Action |
|--------|--------|
| A | Pause |
| Hold A | Skip current phase |
| Hold B | Exit dialog |

### Timer Paused
| Button | Action |
|--------|--------|
| A | Resume |
| Hold A | Skip current phase |
| Hold B | Exit dialog |

### Exit Dialog
| Button | Action |
|--------|--------|
| A | Confirm exit |
| B | Cancel (return to paused) |

## Custom Editor Ranges
- **Work**: 1–60 min (step 1)
- **Break**: 1–30 min (step 1)
- **Loops**: 1–10 or Infinite

## Power Saving
- **30 seconds** idle → screen dims
- **2 minutes** idle → screen off
- **15 minutes** idle in menu → deep sleep (press A to wake)
- **30 minutes** idle when paused → deep sleep (press A to wake)
- Running timer never enters deep sleep
- CPU scales down when idle (240 → 80 MHz)
- **Motion wake**: picking up or tilting the device wakes the screen when dimmed or off (uses MPU6886 accelerometer at 5 Hz, ~0.5 mA)

## Build & Flash

Requires [PlatformIO](https://platformio.org/).

```bash
# Build
pio run

# Flash to device (connect via USB)
pio run -t upload

# Monitor serial output
pio device monitor
```
