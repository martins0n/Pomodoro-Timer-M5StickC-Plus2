#pragma once
#include <M5Unified.h>
#include "config.h"

enum PowerState { PWR_ACTIVE, PWR_DIM, PWR_DISPLAY_OFF, PWR_DEEP_SLEEP };

class PowerManager {
public:
    void begin() {
        _state = PWR_ACTIVE;
        _lastActivity = millis();
        _lastBatteryRead = 0;
        _batteryPct = -1;
        _isCharging = false;
        M5.Display.setBrightness(BRIGHTNESS_ACTIVE);
        setCpuFrequencyMhz(CPU_ACTIVE);
    }

    // Call on any user interaction
    void resetActivity() {
        _lastActivity = millis();
        if (_state != PWR_ACTIVE) {
            _state = PWR_ACTIVE;
            M5.Display.setBrightness(BRIGHTNESS_ACTIVE);
            M5.Display.wakeup();
            setCpuFrequencyMhz(CPU_ACTIVE);
        }
    }

    // Call every loop iteration. menuOnly = true when in menu (allows deep sleep).
    void update(bool menuOnly) {
        unsigned long elapsed = millis() - _lastActivity;

        switch (_state) {
        case PWR_ACTIVE:
            if (elapsed >= TIMEOUT_DIM) {
                _state = PWR_DIM;
                M5.Display.setBrightness(BRIGHTNESS_DIM);
                setCpuFrequencyMhz(CPU_IDLE);
            }
            break;
        case PWR_DIM:
            if (elapsed >= TIMEOUT_DISPLAY_OFF) {
                _state = PWR_DISPLAY_OFF;
                M5.Display.setBrightness(BRIGHTNESS_OFF);
                M5.Display.sleep();
            }
            break;
        case PWR_DISPLAY_OFF:
            if (menuOnly && elapsed >= TIMEOUT_DEEP_SLEEP) {
                _state = PWR_DEEP_SLEEP;
                enterDeepSleep();
            }
            break;
        default:
            break;
        }
    }

    PowerState state() const { return _state; }
    bool isDisplayOn() const { return _state < PWR_DISPLAY_OFF; }

    // Cached battery percentage (updates every BATTERY_READ_INTERVAL)
    int batteryPct() {
        unsigned long now = millis();
        if (_batteryPct < 0 || now - _lastBatteryRead >= BATTERY_READ_INTERVAL) {
            _lastBatteryRead = now;
            _batteryPct = M5.Power.getBatteryLevel();
            _isCharging = M5.Power.isCharging();
        }
        return _batteryPct;
    }

    bool isCharging() {
        batteryPct(); // ensure cache is fresh
        return _isCharging;
    }

private:
    PowerState _state;
    unsigned long _lastActivity;
    unsigned long _lastBatteryRead;
    int _batteryPct;
    bool _isCharging;

    void enterDeepSleep() {
        M5.Display.setBrightness(0);
        M5.Display.sleep();
        // GPIO37 = BtnA wakeup source
        esp_sleep_enable_ext0_wakeup(GPIO_NUM_37, LOW);
        esp_deep_sleep_start();
    }
};
