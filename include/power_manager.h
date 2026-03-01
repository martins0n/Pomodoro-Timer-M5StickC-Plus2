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
        _lastMotionCheck = 0;
        _batteryPct = -1;
        _isCharging = false;
        M5.Display.setBrightness(BRIGHTNESS_ACTIVE);
        setCpuFrequencyMhz(CPU_ACTIVE);
        // Take initial accelerometer baseline
        M5.Imu.getAccel(&_baseAx, &_baseAy, &_baseAz);
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
        unsigned long now = millis();
        unsigned long elapsed = now - _lastActivity;

        switch (_state) {
        case PWR_ACTIVE:
            if (elapsed >= TIMEOUT_DIM) {
                _state = PWR_DIM;
                M5.Display.setBrightness(BRIGHTNESS_DIM);
                setCpuFrequencyMhz(CPU_IDLE);
                // Refresh baseline so small drift doesn't false-trigger
                M5.Imu.getAccel(&_baseAx, &_baseAy, &_baseAz);
            }
            break;
        case PWR_DIM:
            checkMotionWake(now);
            if (_state != PWR_ACTIVE && elapsed >= TIMEOUT_DISPLAY_OFF) {
                _state = PWR_DISPLAY_OFF;
                M5.Display.setBrightness(BRIGHTNESS_OFF);
                M5.Display.sleep();
            }
            break;
        case PWR_DISPLAY_OFF:
            checkMotionWake(now);
            if (_state != PWR_ACTIVE && menuOnly && elapsed >= TIMEOUT_DEEP_SLEEP) {
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
    unsigned long _lastMotionCheck;
    int _batteryPct;
    bool _isCharging;
    float _baseAx, _baseAy, _baseAz;

    void checkMotionWake(unsigned long now) {
        if (now - _lastMotionCheck < MOTION_CHECK_INTERVAL) return;
        _lastMotionCheck = now;

        float ax, ay, az;
        M5.Imu.getAccel(&ax, &ay, &az);

        float dx = ax - _baseAx;
        float dy = ay - _baseAy;
        float dz = az - _baseAz;
        float delta = dx * dx + dy * dy + dz * dz;

        // Update baseline for next check
        _baseAx = ax;
        _baseAy = ay;
        _baseAz = az;

        if (delta >= MOTION_THRESHOLD * MOTION_THRESHOLD) {
            resetActivity();
        }
    }

    void enterDeepSleep() {
        M5.Display.setBrightness(0);
        M5.Display.sleep();
        M5.Imu.sleep();
        // GPIO37 = BtnA wakeup source
        esp_sleep_enable_ext0_wakeup(GPIO_NUM_37, LOW);
        esp_deep_sleep_start();
    }
};
