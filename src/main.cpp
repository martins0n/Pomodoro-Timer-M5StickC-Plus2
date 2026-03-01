// Pomodoro Timer for M5StickC Plus2
// Based on: https://www.hackster.io/asamakerlab/m5-stickc-plus-2-as-a-pomodoro-timer-be4c75
// Original author: asamakerlab (MIT License)

#include <M5Unified.h>
#include <Preferences.h>
#include "config.h"
#include "power_manager.h"

// ── App states ─────────────────────────────────────────────────────
enum AppState {
    STATE_MENU,
    STATE_CUSTOM_EDITOR,
    STATE_TIMER_RUNNING,
    STATE_TIMER_PAUSED,
    STATE_CONFIRM_EXIT
};

// ── Timer phase ────────────────────────────────────────────────────
enum TimerPhase { PHASE_WORK, PHASE_BREAK };

// ── Preset definition ──────────────────────────────────────────────
struct Preset {
    const char* name;
    uint32_t workSec;
    uint32_t breakSec;
    uint8_t  loops;    // 0 = infinite
};

// ── Presets (index 4 = Custom, values loaded from flash) ───────────
Preset presets[] = {
    { "Pomodoro 25/5",  25 * 60,  5 * 60, 4 },
    { "Short Focus",    15 * 60,  3 * 60, 6 },
    { "Long Session",   50 * 60, 10 * 60, 2 },
    { "Quick Test",         10,       5,  2 },
    { "Custom",         25 * 60,  5 * 60, 4 },
};
const int PRESET_COUNT = sizeof(presets) / sizeof(presets[0]);
const int CUSTOM_IDX = PRESET_COUNT - 1;

// ── Globals ────────────────────────────────────────────────────────
M5Canvas canvas(&M5.Display);
PowerManager power;
Preferences prefs;

AppState appState = STATE_MENU;
int menuIndex = 0;

// Timer state
TimerPhase phase;
uint8_t currentLoop;
uint32_t phaseRemainMs;
unsigned long phaseStartedAt;
uint32_t totalWorkSec, totalBreakSec;
uint8_t totalLoops;

// Custom editor state
int editorField = 0;           // 0=work, 1=break, 2=loops
int editorWork, editorBreak, editorLoops;

// Button timing
unsigned long btnADownAt = 0, btnBDownAt = 0;
bool btnAHandled = false, btnBHandled = false;

// Display refresh
unsigned long lastDrawMs = 0;
const unsigned long DRAW_INTERVAL = 100; // 10 fps

// ── Preferences helpers ────────────────────────────────────────────
void loadCustomPreset() {
    prefs.begin("pomo", true);
    presets[CUSTOM_IDX].workSec  = prefs.getUInt("workSec",  25 * 60);
    presets[CUSTOM_IDX].breakSec = prefs.getUInt("breakSec",  5 * 60);
    presets[CUSTOM_IDX].loops    = prefs.getUChar("loops",    4);
    prefs.end();
}

void saveCustomPreset() {
    prefs.begin("pomo", false);
    prefs.putUInt("workSec",  presets[CUSTOM_IDX].workSec);
    prefs.putUInt("breakSec", presets[CUSTOM_IDX].breakSec);
    prefs.putUChar("loops",   presets[CUSTOM_IDX].loops);
    prefs.end();
}

// ── Speaker helper ─────────────────────────────────────────────────
void beep(int count = 1) {
    for (int i = 0; i < count; i++) {
        M5.Speaker.tone(BEEP_FREQ, BEEP_DURATION);
        delay(BEEP_DURATION + 50);
    }
}

// ── Format seconds as MM:SS or HH:MM:SS ───────────────────────────
String formatTime(uint32_t totalSec) {
    uint32_t h = totalSec / 3600;
    uint32_t m = (totalSec % 3600) / 60;
    uint32_t s = totalSec % 60;
    char buf[12];
    if (h > 0)
        snprintf(buf, sizeof(buf), "%u:%02u:%02u", h, m, s);
    else
        snprintf(buf, sizeof(buf), "%02u:%02u", m, s);
    return String(buf);
}

// ── Battery icon drawing ───────────────────────────────────────────
void drawBattery(int x, int y) {
    int pct = power.batteryPct();
    uint16_t color = (pct > 50) ? CLR_GREEN : (pct > 20) ? CLR_ORANGE : CLR_RED;

    // Battery outline 24x12
    canvas.drawRect(x, y, 22, 12, CLR_TEXT);
    canvas.fillRect(x + 22, y + 3, 3, 6, CLR_TEXT);

    // Fill
    int fillW = (pct * 18) / 100;
    if (fillW > 0)
        canvas.fillRect(x + 2, y + 2, fillW, 8, color);

    // Percentage text
    canvas.setTextSize(1);
    canvas.setTextColor(CLR_DIM_TEXT);
    char buf[8];
    if (power.isCharging())
        snprintf(buf, sizeof(buf), "CHG");
    else
        snprintf(buf, sizeof(buf), "%d%%", pct);
    canvas.setCursor(x + 28, y + 2);
    canvas.print(buf);
}

// ── Draw: Menu ─────────────────────────────────────────────────────
void drawMenu() {
    canvas.fillSprite(CLR_BG);

    // Title
    canvas.setTextColor(CLR_ACCENT);
    canvas.setTextSize(2);
    canvas.setCursor(10, 6);
    canvas.print("POMODORO");

    // Battery
    drawBattery(170, 6);

    // Preset list — show 3 around selection
    int startIdx = menuIndex - 1;
    if (startIdx < 0) startIdx = 0;
    if (startIdx > PRESET_COUNT - 3) startIdx = PRESET_COUNT - 3;
    if (startIdx < 0) startIdx = 0;

    for (int i = 0; i < 3 && (startIdx + i) < PRESET_COUNT; i++) {
        int idx = startIdx + i;
        int yPos = 32 + i * 32;
        bool selected = (idx == menuIndex);

        if (selected) {
            canvas.fillRoundRect(4, yPos - 2, 232, 28, 4, 0x1082);
            canvas.setTextColor(CLR_ACCENT);
        } else {
            canvas.setTextColor(CLR_DIM_TEXT);
        }

        canvas.setTextSize(2);
        canvas.setCursor(12, yPos + 4);
        canvas.print(presets[idx].name);

        // Show details for selected
        if (selected) {
            canvas.setTextSize(1);
            canvas.setTextColor(CLR_TEXT);
            char detail[32];
            if (presets[idx].workSec >= 60)
                snprintf(detail, sizeof(detail), "%um/%um x%u",
                    (unsigned)(presets[idx].workSec / 60),
                    (unsigned)(presets[idx].breakSec / 60),
                    presets[idx].loops == 0 ? 99 : presets[idx].loops);
            else
                snprintf(detail, sizeof(detail), "%us/%us x%u",
                    (unsigned)presets[idx].workSec,
                    (unsigned)presets[idx].breakSec,
                    presets[idx].loops);
            canvas.setCursor(SCREEN_W - 10 - strlen(detail) * 6, yPos + 6);
            canvas.print(detail);
        }
    }

    // Scroll indicators
    canvas.setTextSize(1);
    canvas.setTextColor(CLR_DIM_TEXT);
    if (startIdx > 0) {
        canvas.setCursor(116, 26);
        canvas.print("^");
    }
    if (startIdx + 3 < PRESET_COUNT) {
        canvas.setCursor(116, 126);
        canvas.print("v");
    }

    // Bottom hint — context-aware
    canvas.setTextSize(1);
    canvas.setTextColor(CLR_DIM_TEXT);
    canvas.setCursor(4, 126);
    if (menuIndex == CUSTOM_IDX)
        canvas.print("[PWR]Edit [Hold A]Start");
    else
        canvas.print("[A]Next [B]Prev [Hold A]Start");

    canvas.pushSprite(0, 0);
}

// ── Draw: Custom Editor ────────────────────────────────────────────
void drawCustomEditor() {
    canvas.fillSprite(CLR_BG);

    canvas.setTextColor(CLR_ACCENT);
    canvas.setTextSize(2);
    canvas.setCursor(20, 6);
    canvas.print("CUSTOM SETUP");

    const char* labels[] = { "Work (min)", "Break (min)", "Loops" };
    int values[] = { editorWork, editorBreak, editorLoops };

    for (int i = 0; i < 3; i++) {
        int yPos = 34 + i * 30;
        bool active = (i == editorField);

        if (active) {
            canvas.fillRoundRect(4, yPos - 2, 232, 26, 4, 0x1082);
            canvas.setTextColor(CLR_ACCENT);
        } else {
            canvas.setTextColor(CLR_DIM_TEXT);
        }

        canvas.setTextSize(2);
        canvas.setCursor(12, yPos + 3);
        canvas.print(labels[i]);

        // Value
        canvas.setCursor(180, yPos + 3);
        if (i == 2 && values[i] > CUSTOM_LOOPS_MAX) {
            canvas.print("INF");
        } else {
            canvas.print(values[i]);
        }

        // Arrows for active field
        if (active) {
            canvas.setTextColor(CLR_TEXT);
            canvas.setCursor(165, yPos + 3);
            canvas.print("<");
            canvas.setCursor(220, yPos + 3);
            canvas.print(">");
        }
    }

    canvas.setTextSize(1);
    canvas.setTextColor(CLR_DIM_TEXT);
    canvas.setCursor(4, 126);
    canvas.print("[A]+  [B]-  [PWR]OK  [Hold B]Back");

    canvas.pushSprite(0, 0);
}

// ── Draw: Timer ────────────────────────────────────────────────────
void drawTimer() {
    canvas.fillSprite(CLR_BG);

    bool paused = (appState == STATE_TIMER_PAUSED);
    uint16_t phaseColor = (phase == PHASE_WORK) ? CLR_WORK : CLR_BREAK;

    // Phase label
    canvas.setTextColor(paused ? CLR_PAUSE : phaseColor);
    canvas.setTextSize(2);
    canvas.setCursor(10, 8);
    canvas.print(phase == PHASE_WORK ? "WORK" : "BREAK");

    if (paused) {
        canvas.setTextColor(CLR_PAUSE);
        canvas.setCursor(140, 8);
        canvas.print("PAUSED");
    }

    // Countdown
    uint32_t remainSec;
    if (paused) {
        remainSec = (phaseRemainMs + 999) / 1000;
    } else {
        unsigned long elapsed = millis() - phaseStartedAt;
        if (elapsed >= phaseRemainMs)
            remainSec = 0;
        else
            remainSec = (phaseRemainMs - elapsed + 999) / 1000;
    }

    canvas.setTextColor(CLR_TEXT);
    canvas.setTextSize(5);
    String timeStr = formatTime(remainSec);
    // Center the time
    int tw = timeStr.length() * 30;
    canvas.setCursor((SCREEN_W - tw) / 2, 38);
    canvas.print(timeStr);

    // Progress bar
    uint32_t totalPhaseSec = (phase == PHASE_WORK) ? totalWorkSec : totalBreakSec;
    float progress = 1.0f - (float)remainSec / (float)totalPhaseSec;
    if (progress < 0) progress = 0;
    if (progress > 1) progress = 1;
    int barW = (int)(progress * (SCREEN_W - 20));
    canvas.fillRoundRect(10, 90, SCREEN_W - 20, 8, 3, 0x1082);
    if (barW > 0)
        canvas.fillRoundRect(10, 90, barW, 8, 3, phaseColor);

    // Loop counter
    canvas.setTextSize(2);
    canvas.setTextColor(CLR_DIM_TEXT);
    canvas.setCursor(10, 108);
    char loopBuf[16];
    if (totalLoops == 0)
        snprintf(loopBuf, sizeof(loopBuf), "Loop %u/INF", currentLoop);
    else
        snprintf(loopBuf, sizeof(loopBuf), "Loop %u/%u", currentLoop, totalLoops);
    canvas.print(loopBuf);

    // Hints
    canvas.setTextSize(1);
    canvas.setTextColor(CLR_DIM_TEXT);
    canvas.setCursor(140, 115);
    if (paused)
        canvas.print("[A]Resume");
    else
        canvas.print("[A]Pause");

    canvas.pushSprite(0, 0);
}

// ── Draw: Confirm Exit ─────────────────────────────────────────────
void drawConfirmExit() {
    canvas.fillSprite(CLR_BG);

    canvas.setTextColor(CLR_WORK);
    canvas.setTextSize(2);
    canvas.setCursor(50, 20);
    canvas.print("EXIT TIMER?");

    canvas.setTextColor(CLR_TEXT);
    canvas.setTextSize(2);
    canvas.setCursor(20, 60);
    canvas.print("[A] Yes  [B] No");

    canvas.pushSprite(0, 0);
}

// ── Start timer from current preset ────────────────────────────────
void startTimer(int presetIdx) {
    Preset& p = presets[presetIdx];
    totalWorkSec  = p.workSec;
    totalBreakSec = p.breakSec;
    totalLoops    = p.loops;
    currentLoop   = 1;
    phase         = PHASE_WORK;
    phaseRemainMs = totalWorkSec * 1000UL;
    phaseStartedAt = millis();
    appState = STATE_TIMER_RUNNING;
    beep(1);
}

// ── Advance to next phase or finish ────────────────────────────────
void advancePhase() {
    if (phase == PHASE_WORK) {
        // Go to break
        phase = PHASE_BREAK;
        phaseRemainMs = totalBreakSec * 1000UL;
        phaseStartedAt = millis();
        beep(2);
    } else {
        // Break finished — next loop or done
        if (totalLoops != 0 && currentLoop >= totalLoops) {
            // All done
            beep(3);
            appState = STATE_MENU;
            return;
        }
        currentLoop++;
        phase = PHASE_WORK;
        phaseRemainMs = totalWorkSec * 1000UL;
        phaseStartedAt = millis();
        beep(1);
    }
}

// ── Button handling ────────────────────────────────────────────────
void handleButtons() {
    M5.update();

    // Wake guard: if screen was dimmed/off, first press only wakes display
    bool anyPressed = M5.BtnA.wasPressed() || M5.BtnB.wasPressed() || M5.BtnPWR.wasClicked();
    if (anyPressed && power.state() != PWR_ACTIVE) {
        power.resetActivity();
        return; // consume the press — don't trigger actions
    }

    // ── Button A (front, M5.BtnA) ──────────────────────────────────
    if (M5.BtnA.wasPressed()) {
        btnADownAt = millis();
        btnAHandled = false;
        power.resetActivity();
    }

    if (M5.BtnA.isPressed() && !btnAHandled) {
        if (millis() - btnADownAt >= LONG_PRESS_MS) {
            btnAHandled = true;
            // Long press A
            switch (appState) {
            case STATE_MENU:
                startTimer(menuIndex);
                break;
            case STATE_TIMER_RUNNING:
            case STATE_TIMER_PAUSED:
                // Skip current phase
                advancePhase();
                break;
            default:
                break;
            }
        }
    }

    if (M5.BtnA.wasReleased() && !btnAHandled) {
        // Short press A
        switch (appState) {
        case STATE_MENU:
            menuIndex = (menuIndex + 1) % PRESET_COUNT;
            break;
        case STATE_CUSTOM_EDITOR:
            // Increment current field
            if (editorField == 0) {
                editorWork++;
                if (editorWork > CUSTOM_WORK_MAX) editorWork = CUSTOM_WORK_MIN;
            } else if (editorField == 1) {
                editorBreak++;
                if (editorBreak > CUSTOM_BREAK_MAX) editorBreak = CUSTOM_BREAK_MIN;
            } else {
                editorLoops++;
                if (editorLoops > CUSTOM_LOOPS_MAX + 1) editorLoops = CUSTOM_LOOPS_MIN;
            }
            break;
        case STATE_TIMER_RUNNING:
            // Pause
            {
                unsigned long elapsed = millis() - phaseStartedAt;
                if (elapsed >= phaseRemainMs)
                    phaseRemainMs = 0;
                else
                    phaseRemainMs -= elapsed;
                appState = STATE_TIMER_PAUSED;
            }
            break;
        case STATE_TIMER_PAUSED:
            // Resume
            phaseStartedAt = millis();
            appState = STATE_TIMER_RUNNING;
            break;
        case STATE_CONFIRM_EXIT:
            // Confirm exit
            appState = STATE_MENU;
            break;
        }
    }

    // ── Button B (side, M5.BtnB) ───────────────────────────────────
    if (M5.BtnB.wasPressed()) {
        btnBDownAt = millis();
        btnBHandled = false;
        power.resetActivity();
    }

    if (M5.BtnB.isPressed() && !btnBHandled) {
        if (millis() - btnBDownAt >= LONG_PRESS_MS) {
            btnBHandled = true;
            // Long press B
            switch (appState) {
            case STATE_CUSTOM_EDITOR:
                // Cancel editor
                appState = STATE_MENU;
                break;
            case STATE_TIMER_RUNNING:
            case STATE_TIMER_PAUSED:
                // Show exit dialog
                if (appState == STATE_TIMER_RUNNING) {
                    unsigned long elapsed = millis() - phaseStartedAt;
                    if (elapsed >= phaseRemainMs)
                        phaseRemainMs = 0;
                    else
                        phaseRemainMs -= elapsed;
                }
                appState = STATE_CONFIRM_EXIT;
                break;
            default:
                break;
            }
        }
    }

    if (M5.BtnB.wasReleased() && !btnBHandled) {
        // Short press B
        switch (appState) {
        case STATE_MENU:
            menuIndex--;
            if (menuIndex < 0) menuIndex = PRESET_COUNT - 1;
            break;
        case STATE_CUSTOM_EDITOR:
            // Decrement current field
            if (editorField == 0) {
                editorWork--;
                if (editorWork < CUSTOM_WORK_MIN) editorWork = CUSTOM_WORK_MAX;
            } else if (editorField == 1) {
                editorBreak--;
                if (editorBreak < CUSTOM_BREAK_MIN) editorBreak = CUSTOM_BREAK_MAX;
            } else {
                editorLoops--;
                if (editorLoops < CUSTOM_LOOPS_MIN) editorLoops = CUSTOM_LOOPS_MAX + 1;
            }
            break;
        case STATE_CONFIRM_EXIT:
            // Cancel exit — resume paused
            phaseStartedAt = millis();
            appState = STATE_TIMER_PAUSED;
            break;
        default:
            break;
        }
    }

    // ── Power button (M5.BtnPWR) — short press only ────────────────
    if (M5.BtnPWR.wasClicked()) {
        power.resetActivity();
        switch (appState) {
        case STATE_MENU:
            if (menuIndex == CUSTOM_IDX) {
                // Enter custom editor
                editorWork  = presets[CUSTOM_IDX].workSec / 60;
                editorBreak = presets[CUSTOM_IDX].breakSec / 60;
                editorLoops = presets[CUSTOM_IDX].loops;
                if (editorLoops == 0) editorLoops = CUSTOM_LOOPS_MAX + 1;
                editorField = 0;
                appState = STATE_CUSTOM_EDITOR;
            }
            break;
        case STATE_CUSTOM_EDITOR:
            // Confirm field and move to next
            editorField++;
            if (editorField > 2) {
                // Save and start
                presets[CUSTOM_IDX].workSec  = editorWork * 60;
                presets[CUSTOM_IDX].breakSec = editorBreak * 60;
                presets[CUSTOM_IDX].loops    = (editorLoops > CUSTOM_LOOPS_MAX) ? 0 : editorLoops;
                saveCustomPreset();
                startTimer(CUSTOM_IDX);
            }
            break;
        default:
            break;
        }
    }
}

// ── Timer tick (check if phase time elapsed) ───────────────────────
void updateTimer() {
    if (appState != STATE_TIMER_RUNNING) return;

    unsigned long elapsed = millis() - phaseStartedAt;
    if (elapsed >= phaseRemainMs) {
        advancePhase();
    }
}

// ── Setup ──────────────────────────────────────────────────────────
void setup() {
    auto cfg = M5.config();
    cfg.internal_spk = true;
    M5.begin(cfg);

    M5.Display.setRotation(1);   // landscape
    canvas.createSprite(SCREEN_W, SCREEN_H);
    canvas.setSwapBytes(true);

    M5.Speaker.setVolume(128);

    power.begin();
    loadCustomPreset();

    Serial.println("Pomodoro Timer ready");
}

// ── Loop ───────────────────────────────────────────────────────────
void loop() {
    handleButtons();
    updateTimer();

    bool menuOnly = (appState == STATE_MENU);
    power.update(menuOnly);

    // Redraw at DRAW_INTERVAL (skip if display off)
    unsigned long now = millis();
    if (power.isDisplayOn() && (now - lastDrawMs >= DRAW_INTERVAL)) {
        lastDrawMs = now;
        switch (appState) {
        case STATE_MENU:           drawMenu();        break;
        case STATE_CUSTOM_EDITOR:  drawCustomEditor(); break;
        case STATE_TIMER_RUNNING:
        case STATE_TIMER_PAUSED:   drawTimer();       break;
        case STATE_CONFIRM_EXIT:   drawConfirmExit(); break;
        }
    }

    delay(10); // yield
}
