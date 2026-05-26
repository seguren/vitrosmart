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
#include <WebServer.h>
#include <WiFi.h>

// ============================================================
//  web_server.h — HTTP WebServer + HTML embebido
// ============================================================

static WebServer server(HTTP_PORT);

// ── Autenticación ─────────────────────────────────────────────
static bool checkAuth() {
    if (!server.authenticate(WEB_USER, WEB_PASS)) {
        server.requestAuthentication();
        return false;
    }
    return true;
}

// ── HTML ──────────────────────────────────────────────────────
static const char HTML_PAGE[] PROGMEM = R"rawhtml(
<!DOCTYPE html>
<html lang="es">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>VitroSmart</title>
<style>
  body{font-family:sans-serif;max-width:420px;margin:0 auto;padding:16px;background:#1a1a2e;color:#eee}
  h1{text-align:center;color:#e94560;margin-bottom:4px}
  .sub{text-align:center;color:#aaa;font-size:13px;margin-bottom:14px}
  .vitro-banner{text-align:center;font-size:22px;font-weight:bold;padding:14px;
                border-radius:10px;margin-bottom:14px;letter-spacing:3px}
  .vitro-banner.on{background:#0d2b1a;color:#2ecc71}
  .vitro-banner.off{background:#2b0d0d;color:#e74c3c}
  .card{background:#16213e;border-radius:10px;padding:14px;margin-bottom:14px}
  .card h2{margin:0 0 10px;font-size:15px;color:#0f9b8e;text-transform:uppercase;letter-spacing:1px}
  .row{display:flex;justify-content:space-between;align-items:center;margin-bottom:8px}
  .label{color:#aaa;font-size:13px}
  .value{font-weight:bold;font-size:15px}
  .on{color:#2ecc71} .off{color:#e74c3c} .warn{color:#f39c12}
  a.btn,button.btn{display:inline-block;padding:8px 18px;border-radius:6px;text-decoration:none;
        font-size:14px;font-weight:bold;margin:3px;color:#fff;border:none;cursor:pointer}
  .btn-green{background:#27ae60} .btn-red{background:#c0392b}
  .btn-blue{background:#2980b9}  .btn-gray{background:#555}
  .btn-orange{background:#e67e22}
  .btn-lg{font-size:18px;padding:14px 8px}
  .btn-active{outline:3px solid rgba(255,255,255,0.8);outline-offset:2px}
  .onoff-grid{display:grid;grid-template-columns:1fr 1fr;gap:8px;margin-bottom:12px}
  .onoff-grid form{margin:0}
  .onoff-grid button{width:100%}
  input[type=number]{width:70px;padding:5px;border-radius:4px;border:none;
                     background:#0f3460;color:#fff;font-size:14px;text-align:center}
  input[type=time]{background:#0f3460;color:#fff;border:none;border-radius:4px;
                   padding:6px 8px;font-size:15px}
  input[type=range]{width:100%;height:6px;border-radius:3px;background:#0f3460;
                    outline:none;cursor:pointer;-webkit-appearance:none;appearance:none}
  input[type=range]::-webkit-slider-thumb{-webkit-appearance:none;width:24px;height:24px;
    border-radius:50%;background:#e94560;cursor:pointer}
  input[type=range]::-moz-range-thumb{width:24px;height:24px;border-radius:50%;
    background:#e94560;cursor:pointer;border:none}
  input[type=submit]{padding:7px 16px;border:none;border-radius:6px;background:#e94560;
                     color:#fff;font-weight:bold;cursor:pointer;font-size:14px}
  .slider-row{display:flex;align-items:center;gap:8px;margin:10px 0}
  .slider-row input{flex:1}
  .temp-display{text-align:center;margin:8px 0}
  .temp-big{font-size:40px;font-weight:bold;color:#0f9b8e}
  .temp-unit{font-size:20px;color:#aaa}
  .divider{border:none;border-top:1px solid #333;margin:10px 0}
  .time-big{font-size:28px;font-weight:bold;text-align:center;color:#0f9b8e;letter-spacing:2px}
</style>
</head>
<body>
<h1>&#x1F525; VitroSmart</h1>
<div class="sub">Liliana Smart Controller</div>

<!-- Banner estado -->
<div id="vitro-banner" class="vitro-banner %VITRO_CLASS%">%VITRO_STATE%</div>

<!-- Estado -->
<div class="card">
  <h2>Estado</h2>
  <div class="row">
    <span class="label">Potencia</span>
    <span id="power-val" class="value warn">%POWER%</span>
  </div>
  <div class="row">
    <span class="label">Termostato</span>
    <span id="thermo-val" class="value %THERMO_CLASS%">%THERMO_STATE%</span>
  </div>
  <div class="row">
    <span class="label">Temp actual</span>
    <span id="temp-now" class="value">%TEMP_NOW%</span>
  </div>
  <div class="row">
    <span class="label">Setpoint</span>
    <span class="value"><span id="temp-set-display">%TEMP_SET%</span> &#xB0;C</span>
  </div>
  <div class="row">
    <span class="label">Timer</span>
    <span id="timer-val" class="value">%TIMER%</span>
  </div>
  <div class="row">
    <span class="label">Hora</span>
    <span id="time-now" class="value time-big">%TIME_NOW%</span>
  </div>
</div>

<!-- Control ON/OFF + potencia -->
<div class="card">
  <h2>Control</h2>
  <div class="onoff-grid">
    <form method="post" action="/on">
      <button class="btn btn-green btn-lg" type="submit">&#x23FB; Encender</button>
    </form>
    <form method="post" action="/off">
      <button class="btn btn-red btn-lg" type="submit">&#x23FC; Apagar</button>
    </form>
  </div>
  <hr class="divider">
  <div style="text-align:center">
    <span class="label">Potencia: </span>
    <form method="post" action="/power/0"   style="display:inline">
      <button id="pwr0"   class="btn btn-gray %PWR0_ACTIVE%"   type="submit">0%</button>
    </form>
    <form method="post" action="/power/50"  style="display:inline">
      <button id="pwr50"  class="btn btn-blue %PWR50_ACTIVE%"  type="submit">50%</button>
    </form>
    <form method="post" action="/power/100" style="display:inline">
      <button id="pwr100" class="btn btn-blue %PWR100_ACTIVE%" type="submit">100%</button>
    </form>
    <form method="post" action="/smarton"   style="display:inline">
      <button id="pwr-smart" class="btn btn-orange %SMART_ACTIVE%" type="submit">&#x26A1; SMART</button>
    </form>
  </div>
  <hr class="divider">
  <div style="text-align:center">
    <span class="label">Buzzer: </span>
    <form method="post" action="/buzzeron"  style="display:inline">
      <button id="bzron"  class="btn btn-green %BZON_ACTIVE%"  type="submit">&#x1F50A; Activar</button>
    </form>
    <form method="post" action="/buzzeroff" style="display:inline">
      <button id="bzroff" class="btn btn-gray %BZOFF_ACTIVE%"  type="submit">&#x1F507; Silenciar</button>
    </form>
  </div>
</div>

<!-- Temperatura -->
<div class="card">
  <h2>Temperatura deseada</h2>
  <form action="/settemp" method="post">
    <div class="slider-row">
      <span class="label">%TEMP_MIN%&#xB0;</span>
      <input type="range" name="t" min="%TEMP_MIN%" max="%TEMP_MAX%" value="%TEMP_SET%"
             oninput="document.getElementById('tval').textContent=this.value">
      <span class="label">%TEMP_MAX%&#xB0;</span>
    </div>
    <div class="temp-display">
      <span id="tval" class="temp-big">%TEMP_SET%</span>
      <span class="temp-unit"> &#xB0;C</span>
    </div>
    <div style="text-align:center">
      <input type="submit" value="Aplicar setpoint">
    </div>
  </form>
</div>

<!-- Timer -->
<div class="card">
  <h2>Timer de apagado</h2>
  <form action="/settimer" method="post">
    <div class="row">
      <span class="label">Minutos</span>
      <input type="number" name="m" value="%TIMER_MIN%" min="0" max="240" step="10">
      <input type="submit" value="Aplicar">
    </div>
  </form>
  <div style="text-align:center;margin-top:6px">
    <form method="post" action="/timerstart" style="display:inline">
      <button class="btn btn-gray" type="submit">&#x25B6; Iniciar</button>
    </form>
    <form method="post" action="/timerstop" style="display:inline">
      <button class="btn btn-red"  type="submit">&#x23F9; Detener</button>
    </form>
  </div>
</div>

<!-- Scheduler -->
<div class="card">
  <h2>Programar encendido/apagado</h2>
  <form action="/setschedule" method="post">
    <div class="row">
      <span class="label">Encender a</span>
      <input type="time" name="ont"  value="%SCHED_ON_T%">
    </div>
    <div class="row">
      <span class="label">Apagar a</span>
      <input type="time" name="offt" value="%SCHED_OFF_T%">
    </div>
    <div style="text-align:center;margin-top:8px">
      <input type="submit" value="Guardar">
    </div>
  </form>
  <div style="text-align:center;margin-top:6px">
    <form method="post" action="/scheduleoff" style="display:inline">
      <button class="btn btn-red" type="submit">Deshabilitar</button>
    </form>
  </div>
  <div class="row" style="margin-top:8px">
    <span class="label">Estado scheduler</span>
    <span id="sched-state" class="value %SCHED_CLASS%">%SCHED_STATE%</span>
  </div>
</div>

<script>
function el(id){return document.getElementById(id)}
function refreshStatus(){
  fetch('/status',{credentials:'same-origin'})
  .then(function(r){return r.json()})
  .then(function(d){
    var b=el('vitro-banner');
    b.textContent=d.vitroOn?'ENCENDIDO':'APAGADO';
    b.className='vitro-banner '+(d.vitroOn?'on':'off');
    var smart=d.smartMode;
    el('power-val').textContent=smart?'SMART':d.power;
    [0,50,100].forEach(function(p){
      var btn=el('pwr'+p);
      if(btn){btn.classList.toggle('btn-active',!smart&&d.powerPct===p);btn.disabled=smart;}
    });
    var ps=el('pwr-smart');
    if(ps)ps.classList.toggle('btn-active',smart);
    el('thermo-val').textContent=d.thermoState;
    el('thermo-val').className='value '+d.thermoClass;
    el('temp-now').textContent=d.tempNow;
    el('temp-set-display').textContent=d.tempSet;
    el('tval').textContent=d.tempSet;
    var slider=document.querySelector('input[name=t]');
    if(slider)slider.value=d.tempSet;
    el('timer-val').textContent=d.timer;
    el('time-now').textContent=d.timeNow;
    el('sched-state').textContent=d.schedState;
    el('sched-state').className='value '+d.schedClass;
    var bzron=el('bzron'); var bzroff=el('bzroff');
    if(bzron) bzron.classList.toggle('btn-active',d.buzzerOn);
    if(bzroff)bzroff.classList.toggle('btn-active',!d.buzzerOn);
  })
  .catch(function(){});
}
setInterval(refreshStatus,5000);
</script>
</body>
</html>
)rawhtml";

// ── Variables de plantilla ─────────────────────────────────────
static void getThermoState(String &state, String &cls) {
    if (!vitro_isOn()) {
        state = "--"; cls = "off";
    } else if (!power_isSmartMode()) {
        state = "Manual"; cls = "warn";
    } else {
        switch (power_get()) {
            case POWER_0:   state = "Pausado";     cls = "off";  break;
            case POWER_50:  state = "Manteniendo"; cls = "on";   break;
            default:        state = "Calentando";  cls = "warn"; break;
        }
    }
}

static String buildPage() {
    String html = String(HTML_PAGE);
    Schedule &s  = schedule_get();
    bool smart   = power_isSmartMode();
    int pct      = (power_get() == POWER_0) ? 0 : (power_get() == POWER_50) ? 50 : 100;

    html.replace("%VITRO_STATE%",   vitro_isOn() ? "ENCENDIDO" : "APAGADO");
    html.replace("%VITRO_CLASS%",   vitro_isOn() ? "on" : "off");
    html.replace("%POWER%",         smart ? "SMART" : (String(pct) + "%"));
    html.replace("%PWR0_ACTIVE%",   (!smart && pct == 0)   ? "btn-active" : "");
    html.replace("%PWR50_ACTIVE%",  (!smart && pct == 50)  ? "btn-active" : "");
    html.replace("%PWR100_ACTIVE%", (!smart && pct == 100) ? "btn-active" : "");
    html.replace("%SMART_ACTIVE%",  smart ? "btn-active" : "");
    html.replace("%BZON_ACTIVE%",   buzzer_isEnabled() ? "btn-active" : "");
    html.replace("%BZOFF_ACTIVE%",  buzzer_isEnabled() ? "" : "btn-active");

    String thermoState, thermoClass;
    getThermoState(thermoState, thermoClass);
    html.replace("%THERMO_STATE%", thermoState);
    html.replace("%THERMO_CLASS%", thermoClass);

    html.replace("%TEMP_NOW%",  temp_currentString());
    html.replace("%TEMP_SET%",  String(temp_getSetpoint()));
    html.replace("%TEMP_MIN%",  String(TEMP_MIN));
    html.replace("%TEMP_MAX%",  String(TEMP_MAX));
    html.replace("%TIMER%",     timer_toString());
    html.replace("%TIMER_MIN%", String(timer_getMinutes()));
    html.replace("%TIME_NOW%",  ntp_getTimeString());

    char onT[6] = "00:00", offT[6] = "00:00";
    if (s.onHour  >= 0) snprintf(onT,  sizeof(onT),  "%02d:%02d", s.onHour,  s.onMin);
    if (s.offHour >= 0) snprintf(offT, sizeof(offT), "%02d:%02d", s.offHour, s.offMin);
    html.replace("%SCHED_ON_T%",  String(onT));
    html.replace("%SCHED_OFF_T%", String(offT));
    html.replace("%SCHED_STATE%", s.enabled ? "ACTIVO" : "INACTIVO");
    html.replace("%SCHED_CLASS%", s.enabled ? "on" : "off");

    return html;
}

// ── Endpoint JSON para auto-refresco ──────────────────────────
static void handleStatus() {
    if (!checkAuth()) return;

    bool smart = power_isSmartMode();
    int pct    = (power_get() == POWER_0) ? 0 : (power_get() == POWER_50) ? 50 : 100;

    String thermoState, thermoClass;
    getThermoState(thermoState, thermoClass);

    Schedule &s = schedule_get();

    String json = "{";
    json += "\"vitroOn\":"       + String(vitro_isOn() ? "true" : "false") + ",";
    json += "\"power\":\""       + String(pct) + "%\",";
    json += "\"powerPct\":"      + String(pct) + ",";
    json += "\"smartMode\":"     + String(smart ? "true" : "false") + ",";
    json += "\"thermoState\":\"" + thermoState + "\",";
    json += "\"thermoClass\":\"" + thermoClass + "\",";
    json += "\"tempNow\":\""     + temp_currentString() + "\",";
    json += "\"tempSet\":"       + String(temp_getSetpoint()) + ",";
    json += "\"timer\":\""       + timer_toString() + "\",";
    json += "\"timeNow\":\""     + ntp_getTimeString() + "\",";
    json += "\"schedState\":\""  + String(s.enabled ? "ACTIVO" : "INACTIVO") + "\",";
    json += "\"schedClass\":\""  + String(s.enabled ? "on" : "off") + "\",";
    json += "\"buzzerOn\":"      + String(buzzer_isEnabled() ? "true" : "false");
    json += "}";

    server.send(200, "application/json", json);
}

// ── Rutas ─────────────────────────────────────────────────────
static void handleRoot() {
    if (!checkAuth()) return;
    server.send(200, "text/html", buildPage());
}

static void redirect() {
    server.sendHeader("Location", "/");
    server.send(303);
}

static void handleOn()   { if (!checkAuth()) return; vitro_turnOn();      redirect(); }
static void handleOff()  { if (!checkAuth()) return; vitro_turnOff();     redirect(); }

static void handlePower0()   { if (!checkAuth()) return; power_setSmartMode(false); power_set(POWER_0);   redirect(); }
static void handlePower50()  { if (!checkAuth()) return; power_setSmartMode(false); power_set(POWER_50);  redirect(); }
static void handlePower100() { if (!checkAuth()) return; power_setSmartMode(false); power_set(POWER_100); redirect(); }
static void handleSmartOn()  { if (!checkAuth()) return; power_setSmartMode(true);  redirect(); }
static void handleSmartOff() { if (!checkAuth()) return; power_setSmartMode(false); redirect(); }

static void handleSetTemp() {
    if (!checkAuth()) return;
    if (server.hasArg("t")) temp_setSetpoint(server.arg("t").toInt());
    redirect();
}

static void handleSetTimer() {
    if (!checkAuth()) return;
    if (server.hasArg("m")) timer_set(server.arg("m").toInt());
    redirect();
}

static void handleTimerStart() { if (!checkAuth()) return; timer_start(); redirect(); }
static void handleTimerStop()  { if (!checkAuth()) return; timer_stop();  redirect(); }

static void handleSetSchedule() {
    if (!checkAuth()) return;
    int onh = -1, onm = -1, offh = -1, offm = -1;
    if (server.hasArg("ont") && server.arg("ont").length() >= 4) {
        onh = server.arg("ont").substring(0, 2).toInt();
        onm = server.arg("ont").substring(3, 5).toInt();
    }
    if (server.hasArg("offt") && server.arg("offt").length() >= 4) {
        offh = server.arg("offt").substring(0, 2).toInt();
        offm = server.arg("offt").substring(3, 5).toInt();
    }
    schedule_set(onh, onm, offh, offm);
    redirect();
}

static void handleScheduleOff() { if (!checkAuth()) return; schedule_disable();       redirect(); }
static void handleBuzzerOn()    { if (!checkAuth()) return; buzzer_setEnabled(true);  redirect(); }
static void handleBuzzerOff()   { if (!checkAuth()) return; buzzer_setEnabled(false); redirect(); }

// ── Init ──────────────────────────────────────────────────────
void webserver_init() {
    server.on("/",            HTTP_GET,  handleRoot);
    server.on("/status",      HTTP_GET,  handleStatus);
    server.on("/on",          HTTP_POST, handleOn);
    server.on("/off",         HTTP_POST, handleOff);
    server.on("/power/0",     HTTP_POST, handlePower0);
    server.on("/power/50",    HTTP_POST, handlePower50);
    server.on("/power/100",   HTTP_POST, handlePower100);
    server.on("/settemp",     HTTP_POST, handleSetTemp);
    server.on("/settimer",    HTTP_POST, handleSetTimer);
    server.on("/timerstart",  HTTP_POST, handleTimerStart);
    server.on("/timerstop",   HTTP_POST, handleTimerStop);
    server.on("/setschedule", HTTP_POST, handleSetSchedule);
    server.on("/scheduleoff", HTTP_POST, handleScheduleOff);
    server.on("/smarton",     HTTP_POST, handleSmartOn);
    server.on("/smartoff",    HTTP_POST, handleSmartOff);
    server.on("/buzzeron",    HTTP_POST, handleBuzzerOn);
    server.on("/buzzeroff",   HTTP_POST, handleBuzzerOff);
    server.begin();
}

void webserver_handle() { server.handleClient(); }
