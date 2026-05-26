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

// ============================================================
//  VitroSmart — Vitroconvector Liliana Smart Controller
//  Hardware : ESP32-S2 Mini v1.0.0
//  Versión  : 1.1.0
//
//  Librerías requeridas (Library Manager):
//    - LiquidCrystal I2C  (Frank de Brabander)
//
//  Librerías incluidas en ESP32 Arduino core:
//    - WiFi, WebServer, Wire, Preferences, time.h
// ============================================================
#include <Arduino.h>
#include "config.h"
#include "relay_control.h"
#include "ntc_sensor.h"
#include "timer_logic.h"
#include "ntp_time.h"
#include "touch_handler.h"
#include "lcd_menu.h"
#include "web_server.h"

void wifi_connect();
void wifi_showStatus();

static uint32_t lastNtcMs      = 0;
static uint32_t lastSchedMs    = 0;
static uint32_t lastWifiCheckMs = 0;

// ============================================================
//  SETUP
// ============================================================
void setup() {
    Serial.begin(115200);

    relay_init();
    touch_calibrate();
    {
        char r0[17], r1[17];
        snprintf(r0, 17, "ON:%lu", touch_getBaseline(TOUCH_ONOFF));
        snprintf(r1, 17, "ADD:%lu", touch_getBaseline(TOUCH_ADD));
        lcd_showMessage(r0, r1, 4000);
    }
    buzzer_init();
    ntc_init();        // carga setpoint desde NVS
    timer_init();
    lcd_init();

    wifi_connect();
    wifi_showStatus();

    ntp_init();        // carga scheduler desde NVS, sincroniza NTP
    if (ntp_isSynced()) {
        lcd_showMessage("NTP OK", ntp_getTimeString());
    } else {
        lcd_showMessage("NTP: sin sync", "scheduler OFF");
    }

    webserver_init();

    lcd_showMessage("VitroSmart", "Listo!");
}

// ============================================================
//  LOOP
// ============================================================
void loop() {
    uint32_t now = millis();

    if (!lcd_isBacklightOn()) {
        if (touch_anyActive()) {
            lcd_activity();
            touch_resetHoldTimers();
        }
    } else {
        touch_handle();
    }
    webserver_handle();
    lcd_update();

    // ── NTC + termostato (cada 10s) ──────────────────────────
    if ((now - lastNtcMs) >= NTC_INTERVAL_MS) {
        lastNtcMs = now;
        float t = ntc_readCelsius();

        // Termostato: 100% para calentar, 50% para mantener (histéresis ±1°C)
        if (power_isSmartMode() && t > -50.0f && vitro_isOn()) {
            float sp  = (float)temp_getSetpoint();
            PowerLevel pwr = power_get();
            if (t >= sp + 2.0f && pwr != POWER_0) {
                power_set(POWER_0);
                power_setThermostatPaused(true);
                lcd_showMessage("Termostato", "Pausado");
            } else if (t >= sp + 1.0f && pwr == POWER_100) {
                power_set(POWER_50);
                lcd_showMessage("Termostato", "Manteniendo...");
            } else if (t < sp - 1.0f && pwr == POWER_50) {
                power_set(POWER_100);
                lcd_showMessage("Termostato", "Calentando...");
            } else if (power_isThermostatPaused() && pwr == POWER_0 && t < sp + 1.0f) {
                power_set(POWER_50);
                power_setThermostatPaused(false);
                lcd_showMessage("Termostato", "Manteniendo...");
            }
        }
    }

    // ── Timer cuenta regresiva ───────────────────────────────
    if (timer_tick()) {
        vitro_turnOff();
        buzzer_doubleBeep();
        lcd_showMessage("Timer vencido", "Vitro apagado");
    }

    // ── Scheduler NTP (cada 1s) ──────────────────────────────
    if ((now - lastSchedMs) >= 1000UL) {
        lastSchedMs = now;
        int action = schedule_check();
        if (action == 1) {
            vitro_turnOn();
            buzzer_beep();
            lcd_showMessage("Scheduler", "Encendiendo...");
        } else if (action == -1) {
            vitro_turnOff();
            buzzer_doubleBeep();
            lcd_showMessage("Scheduler", "Apagando...");
        }
    }

    // ── Reconexión WiFi (cada 30s si está desconectado) ──────
    if ((now - lastWifiCheckMs) >= 30000UL) {
        lastWifiCheckMs = now;
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("WiFi: reconectando...");
            WiFi.disconnect();
            WiFi.begin(WIFI_SSID, WIFI_PASS);
        }
    }
}

// ============================================================
//  WiFi
// ============================================================
void wifi_connect() {
    lcd_showMessage("Conectando WiFi", WIFI_SSID, 100);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    uint8_t attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        attempts++;
        Serial.print('.');
    }
    Serial.println();
}

void wifi_showStatus() {
    if (WiFi.status() == WL_CONNECTED) {
        String ip = WiFi.localIP().toString();
        Serial.println("WiFi OK -> " + ip);
        String row0 = "WiFi: ";
        row0 += String(WIFI_SSID).substring(0, 10);
        lcd_showMessage(row0, ip, 3000);
    } else {
        Serial.println("WiFi FAIL - modo offline");
        lcd_showMessage("WiFi: sin red", "Modo offline", 3000);
    }
}
