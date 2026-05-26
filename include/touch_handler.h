// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Sebastian Rodriguez
//
// This file is part of VitroSmart.
// VitroSmart is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// VitroSmart is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with VitroSmart. If not, see <https://www.gnu.org/licenses/>.

#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include "config.h"
#include "relay_control.h"
#include "ntc_sensor.h"
#include "timer_logic.h"

// ============================================================
//  touch_handler.h — Touch capacitivo ESP32-S2 Mini + Buzzer
// ============================================================

// ── Buzzer ───────────────────────────────────────────────────
static bool buzzerEnabled = true;

static void buzzer_savePrefs() {
    Preferences p;
    p.begin("buzz", false);
    p.putBool("en", buzzerEnabled);
    p.end();
}

void buzzer_init() {
    pinMode(PIN_BUZZER, OUTPUT);
    digitalWrite(PIN_BUZZER, HIGH); // HIGH = apagado (lógica activa-baja)
    Preferences p;
    p.begin("buzz", true);
    buzzerEnabled = p.getBool("en", true);
    p.end();
}

void buzzer_beep(uint16_t ms = BUZZER_BEEP_MS) {
    if (!buzzerEnabled) return;
    tone(PIN_BUZZER, 2500, ms);
    delay(ms + 10);
    noTone(PIN_BUZZER);
}

void buzzer_doubleBeep() {
    if (!buzzerEnabled) return;
    tone(PIN_BUZZER, 2500, 60);
    delay(70);
    tone(PIN_BUZZER, 2500, 60);
    delay(70);
    noTone(PIN_BUZZER);
}

bool buzzer_isEnabled() { return buzzerEnabled; }

void buzzer_setEnabled(bool en) {
    buzzerEnabled = en;
    buzzer_savePrefs();
    if (en) buzzer_beep();  // confirmación al activar
}

// ── Pads definidos ───────────────────────────────────────────
static const uint8_t TOUCH_GPIO_LIST[] = {
    TOUCH_ONOFF, TOUCH_POWER, TOUCH_TEMP,
    TOUCH_TIME,  TOUCH_ADD,   TOUCH_SUB
};
static const uint8_t NUM_TOUCH = 6;

// ── Baseline por pad ─────────────────────────────────────────
static uint32_t touchBaseline[14] = {0};

static uint32_t readAvgTouch(uint8_t gpio) {
    uint32_t sum = 0;
    for (uint8_t i = 0; i < 10; i++) {
        sum += touchRead(gpio);
        delay(5);
    }
    return sum / 10;
}

void touch_calibrate() {
    for (uint8_t i = 0; i < NUM_TOUCH; i++) {
        touchBaseline[TOUCH_GPIO_LIST[i]] = readAvgTouch(TOUCH_GPIO_LIST[i]);
    }
}

uint32_t touch_getBaseline(uint8_t gpio) { return touchBaseline[gpio]; }

void touch_printBaselines() {
    const char* names[] = {"ON/OFF","POWER","TEMP","TIME","ADD","SUB"};
    Serial.println("-- Touch baselines --");
    for (uint8_t i = 0; i < NUM_TOUCH; i++) {
        Serial.printf("  %s (GPIO%d): %lu\n",
            names[i], TOUCH_GPIO_LIST[i],
            touchBaseline[TOUCH_GPIO_LIST[i]]);
    }
}

// ── Detección ────────────────────────────────────────────────
static bool touch_isActive(uint8_t gpio) {
    uint32_t base = touchBaseline[gpio];
    if (base == 0) return false;
    uint32_t val  = touchRead(gpio);
    uint32_t limit = base + (base * TOUCH_THRESHOLD / 100);
    return (val > limit);
}

static uint32_t lastTouchMs[14]        = {0};
static uint32_t touchActiveStart[14]   = {0};
static uint32_t touchInactiveStart[14] = {0};

static bool touch_isPressed(uint8_t gpio) {
    uint32_t now = millis();
    if (touch_isActive(gpio)) {
        touchInactiveStart[gpio] = 0;
        if (touchActiveStart[gpio] == 0) touchActiveStart[gpio] = now;
        if ((now - touchActiveStart[gpio]) < TOUCH_HOLD_MS) return false;
        if ((now - lastTouchMs[gpio]) < TOUCH_DEBOUNCE_MS) return false;
        lastTouchMs[gpio] = now;
        touchActiveStart[gpio] = 0;
        return true;
    }
    // Gracia: solo cancela el hold si la inactividad supera TOUCH_RELEASE_MS.
    // Evita que fluctuaciones breves reseteen el timer.
    if (touchInactiveStart[gpio] == 0) touchInactiveStart[gpio] = now;
    if ((now - touchInactiveStart[gpio]) >= TOUCH_RELEASE_MS) {
        touchActiveStart[gpio] = 0;
    }
    return false;
}

// ── Estado de edición ────────────────────────────────────────
enum EditMode { EDIT_NONE, EDIT_TEMP, EDIT_TIME };

static EditMode editMode   = EDIT_NONE;
static uint32_t lastEditMs = 0;

EditMode touch_getEditMode() { return editMode; }

static void setEditMode(EditMode m) {
    editMode   = m;
    lastEditMs = millis();
}

void touch_checkTimeout() {
    if (editMode != EDIT_NONE &&
        (millis() - lastEditMs) > EDIT_TIMEOUT_MS) {
        editMode = EDIT_NONE;
    }
}

// ── Handlers ─────────────────────────────────────────────────
static void handle_onoff() {
    vitro_toggle();
    vitro_isOn() ? buzzer_beep() : buzzer_doubleBeep();
}

static void handle_power() {
    if (power_isSmartMode()) return;  // en smart mode el ciclo manual está bloqueado
    power_cycle();
    buzzer_beep();
}
static void handle_temp() {
    editMode == EDIT_TEMP ? setEditMode(EDIT_NONE) : setEditMode(EDIT_TEMP);
    buzzer_beep();
}
static void handle_time() {
    if (editMode == EDIT_TIME) {
        setEditMode(EDIT_NONE);
        if (timer_getMinutes() > 0) timer_start();
        else                        timer_stop();
    } else {
        setEditMode(EDIT_TIME);
    }
    buzzer_beep();
}

static void handle_add() {
    if (editMode == EDIT_NONE) setEditMode(EDIT_TEMP);
    lastEditMs = millis();
    if (editMode == EDIT_TEMP) temp_add();
    else if (editMode == EDIT_TIME) timer_add();
    buzzer_beep();
}

static void handle_sub() {
    if (editMode == EDIT_NONE) setEditMode(EDIT_TEMP);
    lastEditMs = millis();
    if (editMode == EDIT_TEMP) temp_sub();
    else if (editMode == EDIT_TIME) timer_sub();
    buzzer_beep();
}

// ── Loop principal ────────────────────────────────────────────
static uint32_t lastTouchActivityMs = 0;
static uint32_t addSubHoldStart     = 0;
static bool     addSubTriggered     = false;
static uint32_t powerHoldStart      = 0;
static uint32_t powerReleaseStart   = 0;
static bool     powerLongFired      = false;

uint32_t touch_lastActivityMs() { return lastTouchActivityMs; }

bool touch_anyActive() {
    for (uint8_t i = 0; i < NUM_TOUCH; i++)
        if (touch_isActive(TOUCH_GPIO_LIST[i])) return true;
    return false;
}

void touch_resetHoldTimers() {
    for (uint8_t i = 0; i < NUM_TOUCH; i++) {
        touchActiveStart[TOUCH_GPIO_LIST[i]]   = 0;
        touchInactiveStart[TOUCH_GPIO_LIST[i]] = 0;
    }
    powerHoldStart = powerReleaseStart = 0;
    powerLongFired = false;
    addSubHoldStart = 0;
    addSubTriggered = false;
}

void touch_handle() {
    touch_checkTimeout();

    // Gesto: ADD+SUB simultáneo por BUZZER_HOLD_MS → toggle buzzer
    bool addActive = touch_isActive(TOUCH_ADD);
    bool subActive = touch_isActive(TOUCH_SUB);

    if (addActive && subActive) {
        if (addSubHoldStart == 0) addSubHoldStart = millis();
        if (!addSubTriggered && (millis() - addSubHoldStart) >= BUZZER_HOLD_MS) {
            addSubTriggered = true;
            buzzer_setEnabled(!buzzerEnabled);
            lastTouchActivityMs = millis();
            return;
        }
    } else {
        addSubHoldStart = 0;
        addSubTriggered = false;
    }

    // ── POWER: press corto → cycle potencia / press largo (2s) → smart mode ──
    bool pwrActive = touch_isActive(TOUCH_POWER);
    if (pwrActive) {
        powerReleaseStart = 0;
        if (powerHoldStart == 0) powerHoldStart = millis();  // usa var de archivo
        if (!powerLongFired && (millis() - powerHoldStart) >= 2000UL) {
            powerLongFired = true;
            bool entering = !power_isSmartMode();
            power_setSmartMode(entering);
            entering ? buzzer_doubleBeep() : buzzer_beep();
            lastTouchActivityMs = millis();
        }
    } else if (powerHoldStart > 0) {
        if (powerReleaseStart == 0) powerReleaseStart = millis();
        if ((millis() - powerReleaseStart) >= TOUCH_RELEASE_MS) {
            // Confirmado el release — si fue corto, ejecutar cycle
            if (!powerLongFired && (powerReleaseStart - powerHoldStart) >= TOUCH_HOLD_MS) {
                handle_power();
                lastTouchActivityMs = millis();
            }
            powerHoldStart    = 0;
            powerReleaseStart = 0;
            powerLongFired    = false;
        }
    }

    bool any = false;
    if (touch_isPressed(TOUCH_ONOFF)) { handle_onoff(); any = true; }
    if (touch_isPressed(TOUCH_TEMP))  { handle_temp();  any = true; }
    if (touch_isPressed(TOUCH_TIME))  { handle_time();  any = true; }
    if (touch_isPressed(TOUCH_ADD))   { handle_add();   any = true; }
    if (touch_isPressed(TOUCH_SUB))   { handle_sub();   any = true; }
    if (any) lastTouchActivityMs = millis();
}
