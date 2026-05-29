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

// ============================================================
//  config.h — Vitroconvector Smart Controller
//  ESP32-S2 Mini v1.0.0
// ============================================================

// ── Credenciales (en secrets.h, fuera del repo) ──────────────
#include "secrets.h"

// ── WiFi ────────────────────────────────────────────────────
#define HTTP_PORT 80

// ── Pines Touch ─────────────────────────────────────────────
#define TOUCH_ONOFF 1 // T1 - GPIO1
#define TOUCH_POWER 2 // T2 - GPIO2
#define TOUCH_TEMP 3  // T3 - GPIO3
#define TOUCH_TIME 5  // T5 - GPIO5
#define TOUCH_ADD 6   // T6 - GPIO6
#define TOUCH_SUB 7   // T7 - GPIO7

// ── Pines Salida ────────────────────────────────────────────
#define PIN_RELAY1 10 // 1000W
#define PIN_RELAY2 11 // 1000W
#define PIN_BUZZER 12 // Activo-bajo, LOW=ON (5V con transistor PNP o pull-up externo)
#define PIN_NTC 4 // ADC1_CH3 (GPIO4) — GPIO14 es ADC2, no disponible con WiFi

// ── I2C LCD ─────────────────────────────────────────────────
#define LCD_SDA 33    // SDA por defecto LOLIN S2 Mini
#define LCD_SCL 35    // SCL por defecto LOLIN S2 Mini
#define LCD_ADDR 0x27 // Dirección PCF8574 (0x3F si no responde)
#define LCD_COLS 16
#define LCD_ROWS 2

// ── Touch ───────────────────────────────────────────────────
#define TOUCH_THRESHOLD 700  // incremento absoluto sobre baseline (counts) para detección a través del vidrio
#define TOUCH_HOLD_MS 150    // ms sostenido para registrar toque
#define TOUCH_RELEASE_MS 80 // ms inactivo antes de cancelar hold (tolerancia ruido)
#define TOUCH_DEBOUNCE_MS 800 // ms entre lecturas válidas

// ── NTC ─────────────────────────────────────────────────────
#define NTC_R_SERIES 47000UL  // Resistencia divisor a GND (47kΩ soldada en PCB)
#define NTC_R_NOMINAL 10000UL // NTC a 25°C (10kΩ)
#define NTC_T_NOMINAL 25.0f   // °C de referencia
#define NTC_BETA 3950.0f      // Coeficiente β (verificar datasheet)
#define NTC_ADC_BITS 12       // ESP32-S2: ADC 12 bits → 4095
#define NTC_VCC 3.3f

// ── Temperatura ─────────────────────────────────────────────
#define TEMP_MIN 18     // °C mínimo seteable
#define TEMP_MAX 30     // °C máximo seteable
#define TEMP_DEFAULT 24 // °C al arranque

// ── Tiempo ──────────────────────────────────────────────────
#define TIME_MIN_MIN 0      // Minutos mínimo
#define TIME_MAX_MIN 240    // Minutos máximo (4hs)
#define TIME_STEP_MIN 10    // Paso por pulsación
#define TIME_DEFAULT_MIN 60 // Minutos al arranque

// ── Timeouts ────────────────────────────────────────────────
#define NTC_INTERVAL_MS 10000      // Intervalo de lectura del sensor NTC (ms)
#define EDIT_TIMEOUT_MS 5000       // Vuelve a IDLE tras 5s sin toque
#define BUZZER_BEEP_MS 80          // Duración beep confirmación
#define BUZZER_HOLD_MS 3000        // ADD+SUB simultáneo para toggle buzzer
#define BACKLIGHT_TIMEOUT_MS 30000 // Apaga backlight LCD tras 30s sin toque

// ── NTP ─────────────────────────────────────────────────────
#define NTP_SERVER1 "pool.ntp.org"
#define NTP_SERVER2 "time.google.com"
#define NTP_GMT_OFFSET_SEC (-3 * 3600) // Argentina: UTC-3 (sin DST)
#define NTP_DST_OFFSET_SEC 0

// ── Scheduler (encendido/apagado programado) ─────────────────
// Formato: hora y minuto en int (ej: 7:30 → hour=7, min=30)
// Se configuran vía web; -1 = deshabilitado
#define SCHED_ON_HOUR_DEF -1
#define SCHED_ON_MIN_DEF -1
#define SCHED_OFF_HOUR_DEF -1
#define SCHED_OFF_MIN_DEF -1

// ── Relés (lógica activa) ────────────────────────────────────
// Ajustar a LOW si el módulo de relés es activo-bajo
#define RELAY_ON HIGH
#define RELAY_OFF LOW