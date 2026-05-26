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
//  test_touch.ino — Diagnóstico de pines touch ESP32-S2
//  Board: LOLIN S2 Mini
//
//  IMPORTANTE — Antes de subir en Arduino IDE:
//    Herramientas > USB CDC On Boot > Enabled
//    Herramientas > Upload Speed > 921600
//
//  Abrí el Serial Monitor a 115200 baud.
//  Presioná RST y esperá 3 segundos para ver el output.
// ============================================================

#define NUM_PADS 6

const uint8_t PADS[]  = { 1,          2,          3,         5,          6,        7        };
const char*   NAMES[] = { "ONOFF(1)", "POWER(2)", "TEMP(3)", "TIME(5)", "ADD(6)", "SUB(7)" };

uint32_t baseline[NUM_PADS];

void setup() {
    Serial.begin(115200);
    delay(3000);  // espera que USB CDC se establezca

    Serial.println("\n=== TEST TOUCH ESP32-S2 — LOLIN S2 Mini ===");
    Serial.println("NO toques nada durante la calibracion...");
    delay(500);

    for (int i = 0; i < NUM_PADS; i++) {
        uint32_t sum = 0;
        for (int j = 0; j < 20; j++) {
            sum += touchRead(PADS[i]);
            delay(10);
        }
        baseline[i] = sum / 20;
        Serial.printf("  %-10s GPIO%d  baseline = %lu\n", NAMES[i], PADS[i], baseline[i]);
    }

    Serial.println("\n>>> Ahora toca cada resorte uno por uno <<<");
    Serial.println();
    Serial.println("PAD          GPIO   BASE     AHORA    DELTA    %CAMBIO   ACTIVO?");
    Serial.println("--------------------------------------------------------------------");
}

void loop() {
    for (int i = 0; i < NUM_PADS; i++) {
        uint32_t val  = touchRead(PADS[i]);
        int32_t  delta = (int32_t)val - (int32_t)baseline[i];
        float    pct   = (baseline[i] > 0) ? ((float)delta / (float)baseline[i]) * 100.0f : 0.0f;

        // Activo si SUBE mas de 10% sobre baseline (ESP32-S2: toque aumenta el valor)
        bool active = (val > baseline[i]) && (baseline[i] > 0) &&
                      ((val - baseline[i]) > baseline[i] * 10 / 100);

        Serial.printf("%-12s GPIO%d  %7lu  %7lu  %+7ld  %+7.1f%%   %s\n",
            NAMES[i], PADS[i],
            baseline[i], val, delta, pct,
            active ? "SI <---" : "no");
    }
    Serial.println("--------------------------------------------------------------------");
    delay(800);
}
