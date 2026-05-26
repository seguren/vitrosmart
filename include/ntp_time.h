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
#include <time.h>
#include "config.h"

// ============================================================
//  ntp_time.h — Sincronización NTP + Scheduler
// ============================================================

struct Schedule {
    int  onHour  = SCHED_ON_HOUR_DEF;
    int  onMin   = SCHED_ON_MIN_DEF;
    int  offHour = SCHED_OFF_HOUR_DEF;
    int  offMin  = SCHED_OFF_MIN_DEF;
    bool enabled = false;
};

static Schedule schedule;
static bool     ntpSynced        = false;
static int      lastFiredMinute  = -1;  // evita retrigger en el mismo minuto

// ── Init NTP ─────────────────────────────────────────────────
void ntp_init() {
    // Cargar scheduler guardado en NVS
    Preferences p;
    p.begin("sched", true);
    schedule.onHour  = p.getInt("onh",  SCHED_ON_HOUR_DEF);
    schedule.onMin   = p.getInt("onm",  SCHED_ON_MIN_DEF);
    schedule.offHour = p.getInt("offh", SCHED_OFF_HOUR_DEF);
    schedule.offMin  = p.getInt("offm", SCHED_OFF_MIN_DEF);
    schedule.enabled = p.getBool("en",  false);
    p.end();

    configTime(NTP_GMT_OFFSET_SEC, NTP_DST_OFFSET_SEC, NTP_SERVER1, NTP_SERVER2);

    struct tm t;
    uint32_t start = millis();
    while (!getLocalTime(&t) && (millis() - start) < 10000) {
        delay(500);
    }
    ntpSynced = getLocalTime(&t);
}

// ── Hora actual ──────────────────────────────────────────────
bool ntp_getTime(int &hour, int &minute, int &second) {
    struct tm t;
    if (!getLocalTime(&t)) return false;
    hour   = t.tm_hour;
    minute = t.tm_min;
    second = t.tm_sec;
    return true;
}

String ntp_getTimeString() {
    int h, m, s;
    if (!ntp_getTime(h, m, s)) return "--:--";
    char buf[6];
    snprintf(buf, sizeof(buf), "%02d:%02d", h, m);
    return String(buf);
}

bool ntp_isSynced() { return ntpSynced; }

// ── Scheduler ────────────────────────────────────────────────
static void schedule_savePrefs() {
    Preferences p;
    p.begin("sched", false);
    p.putInt("onh",  schedule.onHour);
    p.putInt("onm",  schedule.onMin);
    p.putInt("offh", schedule.offHour);
    p.putInt("offm", schedule.offMin);
    p.putBool("en",  schedule.enabled);
    p.end();
}

void schedule_set(int onH, int onM, int offH, int offM) {
    schedule.onHour  = onH;
    schedule.onMin   = onM;
    schedule.offHour = offH;
    schedule.offMin  = offM;
    schedule.enabled = (onH >= 0 && offH >= 0);
    schedule_savePrefs();
}

void schedule_disable() {
    schedule.enabled = false;
    schedule.onHour  = -1;
    schedule.onMin   = -1;
    schedule.offHour = -1;
    schedule.offMin  = -1;
    schedule_savePrefs();
}

// Llama desde loop() cada segundo.
// Retorna:  1 → debe encenderse
//          -1 → debe apagarse
//           0 → sin acción
// Usa lastFiredMinute para disparar una vez por minuto sin depender del segundo exacto.
int schedule_check() {
    if (!schedule.enabled || !ntpSynced) return 0;

    int h, m, s;
    if (!ntp_getTime(h, m, s)) return 0;

    int currentMinute = h * 60 + m;
    if (currentMinute == lastFiredMinute) return 0;

    if (h == schedule.onHour  && m == schedule.onMin)  { lastFiredMinute = currentMinute; return  1; }
    if (h == schedule.offHour && m == schedule.offMin) { lastFiredMinute = currentMinute; return -1; }
    return 0;
}

Schedule& schedule_get() { return schedule; }
