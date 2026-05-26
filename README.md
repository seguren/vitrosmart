# VitroSmart — Controlador inteligente de vitroconvector

Firmware para ESP32-S2 que convierte un vitroconvector (calefactor eléctrico) en un dispositivo IoT controlable por red local. Permite encender/apagar, regular la potencia, programar horarios y controlar la temperatura desde cualquier navegador o con botones táctiles físicos.

---

## Características

- **Control de potencia en 3 niveles**: 0% / 50% (1000W) / 100% (2000W) mediante dos relés independientes
- **Modo Smart (termostato automático)**: el firmware gestiona la potencia automáticamente para alcanzar y mantener el setpoint de temperatura, sin intervención del usuario
- **Termostato integrado**: activo en modo Smart; cicla entre Calentando (100%), Manteniendo (50%) y Pausado (0%) con histéresis de ±1–2°C
- **Timer de apagado**: cuenta regresiva configurable de 0 a 240 minutos con apagado automático y alarma sonora
- **Scheduler NTP**: enciende y apaga el equipo a horas programadas, sincronizado con servidores de tiempo de internet
- **Interfaz web**: dashboard responsive accesible desde cualquier dispositivo en la red local
- **Controles táctiles**: 6 pads capacitivos para control físico sin conexión a red
- **Pantalla LCD 16×2**: muestra estado, temperatura, timer, hora y modo activo en tiempo real
- **Backlight inteligente**: se apaga tras inactividad; cualquier toque lo enciende sin ejecutar la acción
- **Persistencia en flash**: setpoint de temperatura y configuración del scheduler sobreviven reinicios
- **Modo offline**: touch y LCD siguen funcionando aunque no haya conexión WiFi

---

## Hardware requerido

| Componente | Detalle |
|---|---|
| Microcontrolador | LOLIN S2 Mini (ESP32-S2) |
| Módulo de relés | 2 canales, 10A / 220V CA, lógica activa-alta |
| Pantalla | LCD 16×2 con módulo I2C PCF8574 (dirección 0x27) |
| Sensor de temperatura | NTC 10kΩ (β = 3950) |
| Resistencia divisor | 47kΩ entre 3.3V y el nodo ADC |
| Buzzer | Piezoeléctrico pasivo (usa `tone()` a 2500 Hz); activo-bajo, GPIO es el lado negativo |
| Pads táctiles | 6 electrodos capacitivos conectados a los pines touch del ESP32 |

### Diagrama de conexiones

```
ESP32-S2 Mini
│
├── GPIO 1  (Touch T1) ── Pad ONOFF
├── GPIO 2  (Touch T2) ── Pad POWER
├── GPIO 3  (Touch T3) ── Pad TEMP
├── GPIO 5  (Touch T5) ── Pad TIME
├── GPIO 6  (Touch T6) ── Pad ADD (+)
├── GPIO 7  (Touch T7) ── Pad SUB (-)
│
├── GPIO 10 (OUTPUT)   ── Relé 1 (1000W)
├── GPIO 11 (OUTPUT)   ── Relé 2 (1000W)
├── GPIO 12 (OUTPUT)   ── Buzzer (lado negativo; positivo a 5V vía R 10Ω)
├── GPIO 4  (ADC1_CH3) ── Sensor NTC
│
├── GPIO 33 (SDA)      ──┐
└── GPIO 35 (SCL)      ──┴── LCD I2C 16×2
```

**Circuito NTC:**
```
3.3V ── 47kΩ ── GPIO4 ── NTC(10kΩ) ── GND
```

> **Nota:** GPIO4 es ADC1 en el ESP32-S2. GPIO11–GPIO20 corresponden a ADC2, que no está disponible cuando WiFi está activo — no usar esos pines para lectura analógica.

---

## Software y dependencias

- **Framework**: Arduino (vía PlatformIO)
- **Plataforma**: `espressif32`
- **Board**: `lolin_s2_mini`
- **Librería externa**: `LiquidCrystal I2C` v1.1.4 (Frank de Brabander)
- **Incluidas en el ESP32 core**: `WiFi`, `WebServer`, `Wire`, `Preferences`, `time.h`

---

## Instalación y build

### 1. Clonar el repositorio

```bash
git clone <url-del-repo>
cd vitroconvector
```

### 2. Configurar credenciales

Crear el archivo `include/secrets.h` (no está en el repo por seguridad):

```cpp
#pragma once
#define WIFI_SSID "nombre-de-tu-red"
#define WIFI_PASS "contraseña-wifi"
#define WEB_USER  "admin"
#define WEB_PASS  "tu-contraseña-web"
```

### 3. Compilar y flashear

```bash
# Compilar
pio run

# Compilar y subir al dispositivo
pio run --target upload

# Monitor serie
pio device monitor
```

---

## Interfaz web

Una vez conectado a la red, el dispositivo muestra su IP en el LCD (también por el monitor serie). Abriendo esa IP en un navegador aparece el dashboard.

**Requiere autenticación** (usuario y contraseña definidos en `secrets.h`).

### Características del dashboard

- **Auto-refresco automático**: el estado (temperatura, timer, hora, potencia activa) se actualiza cada 5 segundos vía `fetch('/status')` sin recargar la página
- **Banner de estado prominente**: indicador grande ENCENDIDO / APAGADO en la parte superior con fondo verde o rojo
- **Botón de potencia activa destacado**: el nivel activo (0% / 50% / 100% / SMART) aparece resaltado con contorno blanco
- **Botón SMART**: activa el modo Smart (termostato automático); deshabilita los controles de potencia manual mientras está activo
- **Slider de temperatura**: arrastrando el control se ve el valor en tiempo real antes de confirmar
- **Selector de hora nativo**: el scheduler usa `<input type="time">` — selector de reloj del sistema operativo en móvil y desktop
- **Tema oscuro responsive**: optimizado para pantallas de hasta 420px (uso desde celular)

### Estado mostrado

| Campo | Descripción |
|---|---|
| Banner | ENCENDIDO / APAGADO con color de fondo |
| Potencia | Nivel activo actual (0% / 50% / 100% / SMART) |
| Termostato | Manual / Calentando / Manteniendo / Pausado |
| Temp actual | Lectura del NTC en °C |
| Setpoint | Temperatura deseada |
| Timer | Tiempo restante en HH:MM |
| Hora | Hora NTP actual |

### Controles disponibles

- Encender / Apagar (botones grandes en grid)
- Selección de potencia manual: 0% / 50% / 100%
- Activar / Desactivar modo Smart (botón SMART)
- Slider de setpoint de temperatura con confirmación
- Configuración y control del timer (minutos + inicio/parada)
- Programación de horario de encendido y apagado con selector de hora
- Activar / Desactivar buzzer

---

## Controles táctiles

### Acciones principales

| Pad | Press corto | Press largo (2s) |
|---|---|---|
| **ONOFF** | Encender / Apagar | — |
| **POWER** | Cicla potencia manual: 0% → 50% → 100% → 0% | Activa / desactiva **Modo Smart** |
| **TEMP** | Entra / sale del modo edición de temperatura | — |
| **TIME** | Primera vez: entra en modo edición de timer. Segunda vez: confirma y **arranca el timer** (o lo detiene si el valor es 0) | — |
| **ADD (+)** | Si no hay modo edición activo: entra en edición temperatura. Si hay modo activo: +1°C / +10 minutos | — |
| **SUB (-)** | Si no hay modo edición activo: entra en edición temperatura. Si hay modo activo: -1°C / -10 minutos | — |

> **Modo edición**: al entrar en EDIT_TEMP o EDIT_TIME, el LCD muestra el valor a editar. Presionar el mismo pad nuevamente (TEMP o TIME) confirma y sale. La edición también se cancela automáticamente tras `EDIT_TIMEOUT_MS` de inactividad.

### Gesto buzzer

Mantener **ADD y SUB simultáneos** durante 3 segundos activa o desactiva el buzzer. El estado persiste en flash (namespace `buzz`, clave `en`) y también se puede controlar desde la interfaz web.

### Wake-on-touch (backlight apagado)

Cuando el backlight del LCD está apagado, **el primer toque en cualquier pad únicamente enciende la pantalla**, sin ejecutar la acción asociada al pad. El hold timer se resetea al momento de encender, por lo que hay que soltar y volver a presionar para realizar una acción. Esto evita acciones accidentales al despertar el display.

---

## Pantalla LCD

### Layout en reposo (modo idle)

```
Fila 0: ON  100% 22.3/22C    ← estado + potencia + temperatura actual/setpoint
Fila 0: ON  SMRT 22.3/22C    ← cuando está activo el modo Smart
Fila 0: OFF      22.3/22C    ← cuando apagado

Fila 1: T:01:30  14:22  🕐   ← timer corriendo + hora NTP + ícono scheduler
Fila 1:          14:22  🕐   ← timer detenido + hora NTP + ícono scheduler
Fila 1:          14:22       ← sin scheduler activo
```

El **ícono de reloj** (🕐) en la posición 16 de la fila 1 indica que el scheduler está habilitado.

### Mensajes temporales

Durante eventos (encendido del termostato, scheduler, timer, etc.) el LCD muestra mensajes de 1–4 segundos y vuelve al modo idle automáticamente.

### Diagnóstico al arranque

Al iniciar, el LCD muestra brevemente los valores de baseline de los pads táctiles ON/OFF y ADD (útil para verificar la calibración):

```
ON:<baseline>
ADD:<baseline>
```

---

## Modo Smart (termostato automático)

El modo Smart delega el control de potencia al firmware. Al activarlo, el vitro arranca automáticamente en **100%** y el termostato gestiona los relés según la temperatura del NTC.

### Activación y desactivación

- **Botonera física**: mantener presionado el pad **POWER** durante **2 segundos**. El buzzer confirma con doble beep al entrar y un beep simple al salir.
- **Interfaz web**: botón **⚡ SMART** en el dashboard.

Al activar Smart Mode, la potencia manual queda bloqueada (botones 0% / 50% / 100% deshabilitados tanto en la web como en la botonera). Al desactivarlo, la potencia queda en 0% para que el usuario la ajuste manualmente.

### Lógica del termostato en Smart Mode

El termostato se evalúa cada `NTC_INTERVAL_MS` (10 segundos) mientras el vitro está encendido y el modo Smart está activo:

| Estado | Condición | Potencia | Descripción |
|---|---|---|---|
| **Calentando** | `temp < setpoint - 1°C` y potencia=50% | → 100% | Acelera el calentamiento |
| **Manteniendo** | `temp >= setpoint + 1°C` y potencia=100% | → 50% | Reduce consumo al alcanzar la temperatura |
| **Pausado** | `temp >= setpoint + 2°C` y potencia≠0% | → 0% | Corta todo si hay sobretemperatura |
| **Recuperando** | `temp < setpoint + 1°C` y potencia=0% y pausado | → 50% | Retoma desde Pausado cuando baja la temperatura |

La histéresis diferencial (±1°C para Calentando/Manteniendo, +2°C para Pausado) evita el ciclo rápido de los relés.

> **Nota:** En modo manual (Smart desactivado), los botones de potencia controlan libremente los relés y el termostato no interfiere.

---

## Backlight LCD

La retroiluminación se apaga automáticamente tras `BACKLIGHT_TIMEOUT_MS` (30 segundos por defecto) sin actividad táctil ni eventos del sistema. Se enciende ante cualquier evento: toque, scheduler, timer expirado, cambio de termostato.

**Wake-on-touch**: cuando el backlight está apagado, el primer toque enciende la pantalla pero no ejecuta ninguna acción. Ver sección [Controles táctiles](#controles-táctiles).

---

## Scheduler (encendido/apagado programado)

Requiere sincronización NTP exitosa. Se configura desde la web indicando hora de encendido y hora de apagado (formato HH:MM). La configuración persiste en flash y sobrevive reinicios.

Cuando el scheduler está habilitado, aparece un **ícono de reloj** en la posición 16 de la fila 1 del LCD.

El scheduler se almacena con el namespace `sched` en la NVS del ESP32 (Preferences).

---

## Persistencia (NVS)

| Dato | Namespace | Clave |
|---|---|---|
| Setpoint de temperatura | `ntc` | `sp` |
| Hora de encendido (h/m) | `sched` | `onh`, `onm` |
| Hora de apagado (h/m) | `sched` | `offh`, `offm` |
| Scheduler habilitado | `sched` | `en` |
| Buzzer habilitado | `buzz` | `en` |

---

## Estructura del proyecto

```
vitroconvector/
├── include/
│   ├── config.h          # Constantes de configuración (pines, rangos, NTP)
│   ├── secrets.h         # Credenciales WiFi y web (NO en el repo)
│   ├── relay_control.h   # Control de relés, niveles de potencia, modo Smart
│   ├── ntc_sensor.h      # Lectura del sensor NTC (Steinhart-Hart)
│   ├── timer_logic.h     # Timer de cuenta regresiva
│   ├── ntp_time.h        # Sincronización NTP y scheduler
│   ├── touch_handler.h   # Pads capacitivos y buzzer
│   ├── lcd_menu.h        # Pantalla LCD I2C 16×2
│   └── web_server.h      # HTTP server + HTML embebido
├── src/
│   └── main.cpp          # Setup, loop principal, reconexión WiFi
└── platformio.ini        # Configuración del proyecto PlatformIO
```

---

## Configuración avanzada

Todos los parámetros ajustables están en `include/config.h`:

| Parámetro | Valor actual | Descripción |
|---|---|---|
| `TEMP_MIN` / `TEMP_MAX` | 18 / 30°C | Rango del setpoint |
| `TEMP_DEFAULT` | 24°C | Setpoint inicial |
| `TIME_MAX_MIN` | 240 min | Duración máxima del timer |
| `TIME_STEP_MIN` | 10 min | Paso del timer por toque |
| `NTC_INTERVAL_MS` | 10000 ms | Intervalo de lectura del sensor y evaluación del termostato |
| `EDIT_TIMEOUT_MS` | 5000 ms | Tiempo hasta salir del modo edición por inactividad |
| `BACKLIGHT_TIMEOUT_MS` | 30000 ms | Tiempo hasta apagar el backlight LCD |
| `BUZZER_BEEP_MS` | 80 ms | Duración del beep de confirmación |
| `BUZZER_HOLD_MS` | 3000 ms | Tiempo de ADD+SUB simultáneos para toggle buzzer |
| `TOUCH_THRESHOLD` | 10% | Incremento mínimo sobre el baseline para detectar toque (ESP32-S2: el valor sube al tocar) |
| `TOUCH_HOLD_MS` | 300 ms | Tiempo sostenido requerido para registrar un toque válido |
| `TOUCH_RELEASE_MS` | 80 ms | Gracia antes de resetear el hold timer (tolerancia a ruido/EMI) |
| `TOUCH_DEBOUNCE_MS` | 800 ms | Tiempo mínimo entre lecturas válidas del mismo pad |
| `NTC_R_SERIES` | 47000 Ω | Resistencia del divisor (lado 3.3V) |
| `NTC_BETA` | 3950 | Coeficiente β del termistor |
| `NTP_GMT_OFFSET_SEC` | -10800 | Zona horaria (UTC-3, Argentina) |
| `RELAY_ON` / `RELAY_OFF` | HIGH / LOW | Cambiar si el módulo de relés es activo-bajo |

### Nota sobre la detección táctil en ESP32-S2

El ESP32-S2 **incrementa** el valor de `touchRead()` al detectar un toque, a diferencia del ESP32 original que lo decrementaba. El umbral de detección es:

```
val > baseline + (baseline × TOUCH_THRESHOLD / 100)
```

Al arrancar, el firmware mide el baseline de cada pad 10 veces y promedia. Los valores se muestran en el LCD durante 4 segundos para facilitar la calibración.

---

## Seguridad

- Las credenciales WiFi y de acceso web **no se incluyen en el repositorio**. El archivo `include/secrets.h` está en `.gitignore`.
- El dashboard web requiere autenticación HTTP Basic.
- Todas las acciones de control usan método **POST** (protección básica contra CSRF).
- El servidor corre en HTTP (puerto 80). Para mayor seguridad en redes no confiables, limitar el acceso por firewall/router.

---

## Licencia

Copyright (C) 2026 Sebastian Rodriguez

Este proyecto está licenciado bajo la **GNU General Public License v3.0** (GPLv3).

Podés usarlo, modificarlo y distribuirlo libremente, siempre que cualquier versión derivada se publique bajo la misma licencia y con el código fuente disponible. Ver el archivo [LICENSE](LICENSE) para el texto completo.
