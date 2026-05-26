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
#include <math.h>
#include "config.h"

// ============================================================
//  ntc_sensor.h — Lectura NTC / Steinhart-Hart
//
//  Circuito: 3.3V → R_SERIES → nodo → NTC → GND
//                               ↑
//                            PIN_NTC (ADC)
//
//  Nota: NTC en lado GND para que el voltaje en el nodo
//  quede en rango del ADC del ESP32-S2 (~0.5-0.8V a 20-30°C).
// ============================================================

#define NTC_SAMPLES 10

static int   tempSetpoint = TEMP_DEFAULT;
static float tempCurrent  = 0.0f;

void ntc_init() {
    Preferences p;
    p.begin("ntc", true);
    tempSetpoint = p.getInt("sp", TEMP_DEFAULT);
    p.end();

    analogReadResolution(NTC_ADC_BITS);
    analogSetAttenuation(ADC_ATTENDB_MAX);
}

static float ntc_readMillivolts() {
    uint32_t sum = 0;
    for (int i = 0; i < NTC_SAMPLES; i++) {
        sum += analogReadMilliVolts(PIN_NTC);
        delay(2);
    }
    return (float)sum / NTC_SAMPLES;
}

float ntc_readCelsius() {
    float mv   = ntc_readMillivolts();
    float vout = mv / 1000.0f;
    float rNTC = NTC_R_SERIES * vout / (NTC_VCC - vout); // NTC lado GND
    if (vout <= 0.01f || vout >= NTC_VCC * 0.99f) { tempCurrent = -99.0f; return -99.0f; }
    if (rNTC <= 0) { tempCurrent = -99.0f; return -99.0f; }

    float tK = 1.0f / (
        (1.0f / (NTC_T_NOMINAL + 273.15f)) +
        (1.0f / NTC_BETA) * log(rNTC / NTC_R_NOMINAL)
    );

    tempCurrent = tK - 273.15f;
    return tempCurrent;
}

void temp_setSetpoint(int t) {
    tempSetpoint = constrain(t, TEMP_MIN, TEMP_MAX);
    Preferences p;
    p.begin("ntc", false);
    p.putInt("sp", tempSetpoint);
    p.end();
}
void temp_add() { temp_setSetpoint(tempSetpoint + 1); }
void temp_sub() { temp_setSetpoint(tempSetpoint - 1); }

int   temp_getSetpoint() { return tempSetpoint; }
float temp_getCurrent()  { return tempCurrent; }

String temp_currentString() {
    char buf[8];
    snprintf(buf, sizeof(buf), "%4.1fC", tempCurrent);
    return String(buf);
}
