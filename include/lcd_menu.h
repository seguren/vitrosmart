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
//  lcd_menu.h — Display 16x2 I2C
//
//  Librería requerida: "LiquidCrystal I2C" by Frank de Brabander
//
//  Layout idle:
//  Fila 0: "ON  100% 22.3/22C"  → estado + potencia + actual/setpoint
//  Fila 0: "OFF      22.3/22C"  → cuando apagado
//  Fila 1: "T:01:30  14:22  "   → timer (si corre) + hora NTP
//  Fila 1: "         14:22  "   → solo hora si timer detenido
// ============================================================

static LiquidCrystal_I2C lcd(LCD_ADDR, LCD_COLS, LCD_ROWS);
static bool     lcdBacklightOn    = true;
static uint32_t lcdLastActivityMs = 0;  // reseteado por toques Y por mensajes

static byte clockIcon[8] = { 14, 17, 21, 23, 17, 14, 0, 0 };

void lcd_init() {
    Wire.begin(LCD_SDA, LCD_SCL);
    lcd.begin(LCD_COLS, LCD_ROWS);
    lcd.createChar(0, clockIcon);
    lcd.backlight();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("  VitroSmart    ");
    lcd.setCursor(0, 1);
    lcd.print(" Iniciando...   ");
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

// IDLE:
// Fila 0: "ON  100% 22.3/22C"  (16 chars)
//          ^^^^ ^^^^ ^^^^^^^^
//          ON   pot  actual/setpoint
// Fila 1: "T:01:30  14:22  "   (timer corriendo)
//         "         14:22  "   (timer detenido)
static void lcd_showIdle() {
    // ── Fila 0 ───────────────────────────────────────────────
    char tempBuf[10];
    snprintf(tempBuf, sizeof(tempBuf), "%4.1f/%2dC",
             temp_getCurrent(), temp_getSetpoint());

    String row0;
    if (vitro_isOn()) {
        row0  = "ON ";
        row0 += power_toString();  // 4 chars: "100%", " 50%", "  0%"
        row0 += " ";               // separador entre % y temperatura
    } else {
        row0 = "OFF     ";         // 8 chars, alinea con el bloque de temp
    }
    row0 += String(tempBuf);       // "22.3/22C" — 8 chars

    // ── Fila 1 ───────────────────────────────────────────────
    String row1;
    if (timer_isRunning()) {
        row1  = "T:";
        row1 += timer_toString();  // "HH:MM" — 5 chars
        row1 += "  ";              // separador (total izq: 9 chars)
    } else {
        row1 = "         ";        // 9 espacios — mantiene la hora alineada
    }
    row1 += ntp_isSynced() ? ntp_getTimeString() : "--:--";

    lcd_printRow(0, row0);
    lcd_printRow(1, row1);
    if (schedule_get().enabled) {
        lcd.setCursor(15, 1);
        lcd.write(byte(0));
    }
}

// EDIT_TEMP:
// Fila 0: ">> Set Temp:    "
// Fila 1: "     22 C       "
static void lcd_showEditTemp() {
    lcd_printRow(0, ">> Set Temp:    ");
    String row1 = "     ";
    row1 += String(temp_getSetpoint());
    row1 += " C";
    lcd_printRow(1, row1);
}

// EDIT_TIME:
// Fila 0: ">> Set Timer:   "
// Fila 1: "     01:30      "
static void lcd_showEditTime() {
    lcd_printRow(0, ">> Set Timer:   ");
    String row1 = "     ";
    row1 += timer_toString();
    lcd_printRow(1, row1);
}

// ── Backlight ─────────────────────────────────────────────────
// Cualquier evento (toque o mensaje) llama a lcd_activity() para
// resetear el timer y garantizar N segundos de pantalla encendida.
bool lcd_isBacklightOn() { return lcdBacklightOn; }

void lcd_activity() {
    lcdLastActivityMs = millis();
    if (!lcdBacklightOn) { lcd.backlight(); lcdBacklightOn = true; }
}

static void lcd_manageBacklight() {
    // Usa el más reciente entre toque táctil y evento de sistema
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
        case EDIT_TEMP: lcd_showEditTemp(); break;
        case EDIT_TIME: lcd_showEditTime(); break;
        default:        lcd_showIdle();     break;
    }
}

// ── Mensaje temporal ──────────────────────────────────────────
void lcd_showMessage(const String &row0, const String &row1, uint16_t ms = 1500) {
    lcd_activity();   // enciende y resetea el timer de N segundos
    lcd_printRow(0, row0);
    lcd_printRow(1, row1);
    delay(ms);
    lcd.clear();
}
