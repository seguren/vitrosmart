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
#include "config.h"
#include "ntc_sensor.h"
#include "ntp_time.h"
#include "relay_control.h"
#include "timer_logic.h"
#include "touch_handler.h"
#include <LiquidCrystal_I2C.h>
#include <Wire.h>

// ============================================================
//  lcd_menu.h — Display LCD I2C (16x2 o 20x4)
//
//  Librería requerida: "LiquidCrystal I2C" by Frank de Brabander
//
//  Layout idle 20x4:
//  Fila 0: "ON   100%   14:22 🔔🕐"  estado + potencia + hora + iconos
//  Fila 1: "Actual:22.3  Set:24C"    temperatura actual / setpoint
//  Fila 2: "Timer:         01:30"    timer (en blanco si detenido)
//  Fila 3: "Calentando...       "    estado termostato / modo
//
//  Layout idle 16x2:
//  Fila 0: "ON  100% 22.3/22C"       estado + potencia + actual/setpoint
//  Fila 1: "T:01:30  14:22 🕐"       timer + hora NTP + scheduler
// ============================================================

static LiquidCrystal_I2C lcd(LCD_ADDR, LCD_COLS, LCD_ROWS);
static bool     lcdBacklightOn    = true;
static uint32_t lcdLastActivityMs = 0;

static byte clockIcon[8]  = { 14, 17, 21, 23, 17, 14, 0, 0 };
static byte buzzerIcon[8] = {  4, 14, 14, 31,  0,  4, 0, 0 };

void lcd_init() {
    Wire.begin(LCD_SDA, LCD_SCL);
    lcd.begin(LCD_COLS, LCD_ROWS);
    lcd.createChar(0, clockIcon);
#if LCD_ROWS == 4
    lcd.createChar(1, buzzerIcon);
#endif
    lcd.backlight();
    lcd.clear();
    lcd.setCursor(0, 0);
#if LCD_ROWS == 4
    lcd.print("    VitroSmart      ");
    lcd.setCursor(0, 1);
    lcd.print("    Iniciando...    ");
#else
    lcd.print("  VitroSmart    ");
    lcd.setCursor(0, 1);
    lcd.print(" Iniciando...   ");
#endif
    delay(1500);
    lcd.clear();
}

// ── Helpers ──────────────────────────────────────────────────
static void lcd_printRow(uint8_t row, const String &s) {
    lcd.setCursor(0, row);
    String padded = s;
    while (padded.length() < LCD_COLS) padded += ' ';
    lcd.print(padded.substring(0, LCD_COLS));
}

// ── Pantallas ────────────────────────────────────────────────

static void lcd_showIdle() {
#if LCD_ROWS == 4
    // ── Fila 0: estado + potencia + hora (20x4) ──────────────
    String row0;
    if (vitro_isOn()) {
        row0  = "ON  ";
        row0 += power_toString();
    } else {
        row0 = "OFF     ";
    }
    while (row0.length() < 12) row0 += ' ';
    row0 += ntp_isSynced() ? ntp_getTimeString() : "--:--";
    row0 += " ";  // col 18=buzzer, col 19=reloj se escriben aparte

    // ── Fila 1: temperaturas ──────────────────────────────────
    char actBuf[6], spBuf[3];
    snprintf(actBuf, sizeof(actBuf), "%4.1f", temp_getCurrent());
    snprintf(spBuf,  sizeof(spBuf),  "%2d",   temp_getSetpoint());
    String row1 = "Actual:";
    row1 += actBuf;
    row1 += "  Set:";
    row1 += spBuf;
    row1 += "C";

    // ── Fila 2: timer ─────────────────────────────────────────
    String row2;
    if (timer_isRunning()) {
        row2 = "Timer:";
        while (row2.length() < 15) row2 += ' ';
        row2 += timer_toString();
    }

    // ── Fila 3: estado termostato ─────────────────────────────
    String row3;
    if (power_isSmartMode() && vitro_isOn()) {
        PowerLevel pwr = power_get();
        if      (pwr == POWER_100)           row3 = "Calentando...";
        else if (pwr == POWER_50)            row3 = "Manteniendo...";
        else if (power_isThermostatPaused()) row3 = "Pausado";
    } else if (vitro_isOn() && !power_isSmartMode()) {
        row3 = "Manual";
    }

    lcd_printRow(0, row0);
    lcd_printRow(1, row1);
    lcd_printRow(2, row2);
    lcd_printRow(3, row3);
    if (buzzer_isEnabled()) {
        lcd.setCursor(18, 0);
        lcd.write(byte(1));
    }
    if (schedule_get().enabled) {
        lcd.setCursor(19, 0);
        lcd.write(byte(0));
    }

#else
    // ── Fila 0: estado + potencia + temp actual/setpoint (16x2)
    char tempBuf[10];
    snprintf(tempBuf, sizeof(tempBuf), "%4.1f/%2dC",
             temp_getCurrent(), temp_getSetpoint());
    String row0;
    if (vitro_isOn()) {
        row0  = "ON ";
        row0 += power_toString();
        row0 += " ";
    } else {
        row0 = "OFF     ";
    }
    row0 += String(tempBuf);

    // ── Fila 1: timer + hora ──────────────────────────────────
    String row1;
    if (timer_isRunning()) {
        row1  = "T:";
        row1 += timer_toString();
        row1 += "  ";
    } else {
        row1 = "         ";
    }
    row1 += ntp_isSynced() ? ntp_getTimeString() : "--:--";

    lcd_printRow(0, row0);
    lcd_printRow(1, row1);
    if (schedule_get().enabled) {
        lcd.setCursor(15, 1);
        lcd.write(byte(0));
    }
#endif
}

static void lcd_showEditTemp() {
#if LCD_ROWS == 4
    String row1 = "        ";
    row1 += String(temp_getSetpoint());
    row1 += " C";
    lcd_printRow(0, ">> Setpoint Temp:   ");
    lcd_printRow(1, row1);
    lcd_printRow(2, "+ ADD      - SUB    ");
    lcd_printRow(3, "TEMP para confirmar ");
#else
    lcd_printRow(0, ">> Set Temp:    ");
    String row1 = "     ";
    row1 += String(temp_getSetpoint());
    row1 += " C";
    lcd_printRow(1, row1);
#endif
}

static void lcd_showEditTime() {
#if LCD_ROWS == 4
    String row1 = "       ";
    row1 += timer_toString();
    lcd_printRow(0, ">> Setpoint Timer:  ");
    lcd_printRow(1, row1);
    lcd_printRow(2, "+ ADD      - SUB    ");
    lcd_printRow(3, "TIME para iniciar   ");
#else
    lcd_printRow(0, ">> Set Timer:   ");
    String row1 = "     ";
    row1 += timer_toString();
    lcd_printRow(1, row1);
#endif
}

// EDIT_SCHED — muestra config del scheduler (solo lectura)
static void lcd_showEditSched() {
    Schedule& s = schedule_get();
    char onBuf[6], offBuf[6];
    if (s.onHour  >= 0) snprintf(onBuf,  sizeof(onBuf),  "%02d:%02d", s.onHour,  s.onMin);
    else                 snprintf(onBuf,  sizeof(onBuf),  "--:--");
    if (s.offHour >= 0) snprintf(offBuf, sizeof(offBuf), "%02d:%02d", s.offHour, s.offMin);
    else                 snprintf(offBuf, sizeof(offBuf), "--:--");

#if LCD_ROWS == 4
    lcd_printRow(0, ">> Scheduler:       ");
    String row1 = "Encendido:    "; row1 += onBuf;
    String row2 = "Apagado:      "; row2 += offBuf;
    lcd_printRow(1, row1);
    lcd_printRow(2, row2);
    lcd_printRow(3, s.enabled ? "Estado:      Activo " : "Estado:    Inactivo ");
#else
    String row0 = "Sched ";
    row0 += s.enabled ? "[ON] " : "[OFF]";
    lcd_printRow(0, row0);
    String row1 = onBuf;
    row1 += " -> ";
    row1 += offBuf;
    lcd_printRow(1, row1);
#endif
}

// ── Backlight ─────────────────────────────────────────────────
bool lcd_isBacklightOn() { return lcdBacklightOn; }

void lcd_activity() {
    lcdLastActivityMs = millis();
    if (!lcdBacklightOn) { lcd.backlight(); lcdBacklightOn = true; }
}

static void lcd_manageBacklight() {
    uint32_t lastActivity = max(lcdLastActivityMs, touch_lastActivityMs());
    bool shouldBeOn = (millis() - lastActivity) < BACKLIGHT_TIMEOUT_MS;
    if (shouldBeOn && !lcdBacklightOn) {
        lcd.backlight();
        lcdBacklightOn = true;
    } else if (!shouldBeOn && lcdBacklightOn) {
        lcd.noBacklight();
        lcdBacklightOn = false;
    }
}

// ── Actualización principal ───────────────────────────────────
void lcd_update() {
    static uint32_t lastUpdate = 0;
    if ((millis() - lastUpdate) < 500) return;
    lastUpdate = millis();

    lcd_manageBacklight();

    switch (touch_getEditMode()) {
        case EDIT_TEMP:  lcd_showEditTemp();  break;
        case EDIT_TIME:  lcd_showEditTime();  break;
        case EDIT_SCHED: lcd_showEditSched(); break;
        default:         lcd_showIdle();      break;
    }
}

// ── Mensaje temporal ──────────────────────────────────────────
void lcd_showMessage(const String &row0, const String &row1, uint16_t ms = 1500) {
    lcd_activity();
    lcd_printRow(0, row0);
    lcd_printRow(1, row1);
#if LCD_ROWS == 4
    lcd_printRow(2, "");
    lcd_printRow(3, "");
#endif
    delay(ms);
    lcd.clear();
}
