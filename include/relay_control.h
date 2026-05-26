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

// ============================================================
//  relay_control.h — Control de potencia 0% / 50% / 100%
//  El termostato gestiona la potencia directamente (100% para
//  calentar, 50% para mantener). La lógica vive en main.cpp.
// ============================================================

enum PowerLevel { POWER_0 = 0, POWER_50 = 1, POWER_100 = 2 };

static PowerLevel currentPower     = POWER_0;
static bool       vitroOn          = false;
static bool       thermostatPaused = false;
static bool       smartMode        = false;

void relay_init() {
    pinMode(PIN_RELAY1, OUTPUT);
    pinMode(PIN_RELAY2, OUTPUT);
    digitalWrite(PIN_RELAY1, RELAY_OFF);
    digitalWrite(PIN_RELAY2, RELAY_OFF);
}

void relay_apply() {
    if (!vitroOn) {
        digitalWrite(PIN_RELAY1, RELAY_OFF);
        digitalWrite(PIN_RELAY2, RELAY_OFF);
        return;
    }
    switch (currentPower) {
        case POWER_0:
            digitalWrite(PIN_RELAY1, RELAY_OFF);
            digitalWrite(PIN_RELAY2, RELAY_OFF);
            break;
        case POWER_50:
            digitalWrite(PIN_RELAY1, RELAY_ON);
            digitalWrite(PIN_RELAY2, RELAY_OFF);
            break;
        case POWER_100:
            digitalWrite(PIN_RELAY1, RELAY_ON);
            digitalWrite(PIN_RELAY2, RELAY_ON);
            break;
    }
}

// Al encender siempre arranca al 100% para calentar rápido
void vitro_turnOn()  { vitroOn = true; currentPower = POWER_100; relay_apply(); }
void vitro_turnOff() { vitroOn = false; relay_apply(); }
void vitro_toggle()  { vitroOn ? vitro_turnOff() : vitro_turnOn(); }

void power_cycle() {
    if (smartMode) return;
    currentPower     = (PowerLevel)((currentPower + 1) % 3);
    thermostatPaused = false;
    relay_apply();
}

void power_setSmartMode(bool en) {
    smartMode = en;
    thermostatPaused = false;
    currentPower = en ? POWER_100 : POWER_0;
    relay_apply();
}

bool  power_isSmartMode()               { return smartMode; }
void  power_setThermostatPaused(bool v) { thermostatPaused = v; }
bool  power_isThermostatPaused()        { return thermostatPaused; }

void power_set(PowerLevel p) {
    currentPower = p;
    relay_apply();
}

PowerLevel  power_get()  { return currentPower; }
bool        vitro_isOn() { return vitroOn; }

const char* power_toString() {
    if (smartMode) return "SMRT";
    switch (currentPower) {
        case POWER_0:   return "  0%";
        case POWER_50:  return " 50%";
        case POWER_100: return "100%";
    }
    return "???";
}
