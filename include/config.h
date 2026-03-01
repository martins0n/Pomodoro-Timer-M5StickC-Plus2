#pragma once

// ── Display ────────────────────────────────────────────────────────
#define SCREEN_W 240
#define SCREEN_H 135

// ── Brightness ─────────────────────────────────────────────────────
#define BRIGHTNESS_ACTIVE  80
#define BRIGHTNESS_DIM     20
#define BRIGHTNESS_OFF      0

// ── Power-saving timeouts (ms) ─────────────────────────────────────
#define TIMEOUT_DIM         30000UL   // 30 s  → dim
#define TIMEOUT_DISPLAY_OFF 120000UL  // 2 min → display off
#define TIMEOUT_DEEP_SLEEP  900000UL  // 15 min → deep sleep (menu/editor)
#define TIMEOUT_PAUSE_SLEEP 1800000UL // 30 min → deep sleep (paused timer)

// ── CPU frequencies (MHz) ──────────────────────────────────────────
#define CPU_ACTIVE 240
#define CPU_IDLE    80

// ── Battery reading cache ──────────────────────────────────────────
#define BATTERY_READ_INTERVAL 10000UL // 10 s

// ── Colors (RGB565) ────────────────────────────────────────────────
#define CLR_BG         0x0000  // black
#define CLR_WORK       0xFBE0  // orange
#define CLR_BREAK      0x07E0  // green
#define CLR_ACCENT     0x07FF  // cyan
#define CLR_TEXT       0xFFFF  // white
#define CLR_DIM_TEXT   0x8410  // grey
#define CLR_RED        0xF800
#define CLR_ORANGE     0xFD20
#define CLR_GREEN      0x07E0
#define CLR_PAUSE      0xFFE0  // yellow

// ── Custom editor ranges ───────────────────────────────────────────
#define CUSTOM_WORK_MIN   1
#define CUSTOM_WORK_MAX  60
#define CUSTOM_BREAK_MIN  1
#define CUSTOM_BREAK_MAX 30
#define CUSTOM_LOOPS_MIN  1
#define CUSTOM_LOOPS_MAX 10   // 11 = infinite

// ── Long-press threshold (ms) ──────────────────────────────────────
#define LONG_PRESS_MS 600

// ── Motion wake (IMU) ─────────────────────────────────────────────
#define MOTION_CHECK_INTERVAL 200     // ms between accel reads (5 Hz)
#define MOTION_THRESHOLD      0.3f   // delta in G to trigger wake

// ── Speaker ────────────────────────────────────────────────────────
#define BEEP_FREQ     2000
#define BEEP_DURATION  150
