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
#include "config.h"
#include "relay_control.h"

// ============================================================
//  timer_logic.h — Cuenta regresiva con apagado automático
// ============================================================

static int      timerMinutes   = TIME_DEFAULT_MIN;
static bool     timerRunning   = false;
static uint32_t timerLastTick  = 0;

void timer_init() {
    timerMinutes  = TIME_DEFAULT_MIN;
    timerRunning  = false;
    timerLastTick = millis();
}

// Inicia el timer cuando el vitro se enciende
void timer_start() {
    timerRunning  = true;
    timerLastTick = millis();
}

void timer_stop()  { timerRunning = false; }
void timer_reset() { timerMinutes = TIME_DEFAULT_MIN; }

void timer_add() {
    timerMinutes = constrain(timerMinutes + TIME_STEP_MIN,
                             TIME_MIN_MIN, TIME_MAX_MIN);
}

void timer_sub() {
    timerMinutes = constrain(timerMinutes - TIME_STEP_MIN,
                             TIME_MIN_MIN, TIME_MAX_MIN);
}

void timer_set(int minutes) {
    timerMinutes = constrain(minutes, TIME_MIN_MIN, TIME_MAX_MIN);
}

int  timer_getMinutes()   { return timerMinutes; }
bool timer_isRunning()    { return timerRunning; }

// String "HH:MM" para LCD
String timer_toString() {
    char buf[6];
    snprintf(buf, sizeof(buf), "%02d:%02d",
             timerMinutes / 60, timerMinutes % 60);
    return String(buf);
}

// Llamar desde loop() — descuenta 1 minuto cada 60s
// Retorna true cuando llega a 0 → el llamador apaga el vitro
bool timer_tick() {
    if (!timerRunning || timerMinutes <= 0) return false;

    uint32_t now = millis();
    if ((now - timerLastTick) >= 60000UL) {
        timerLastTick = now;
        timerMinutes--;
        if (timerMinutes <= 0) {
            timerMinutes = 0;
            timerRunning = false;
            return true;  // ← apagar
        }
    }
    return false;
}
