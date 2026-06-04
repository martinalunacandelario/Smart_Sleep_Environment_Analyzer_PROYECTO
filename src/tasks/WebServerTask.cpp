#include "WebServerTask.h"
#include "../../include/config.h"
#include <ArduinoJson.h>
#include <SD.h>

// ============================================================================
// INICIALIZACIÓN DE MIEMBROS ESTÁTICOS
// ============================================================================
TaskHandle_t      WebServerTask::_taskHandle  = nullptr;
WebServer         WebServerTask::server(WEB_SERVER_PORT);
SensorData        WebServerTask::_currentData = {0};
SemaphoreHandle_t WebServerTask::_dataMutex   = nullptr;

// ============================================================================
// FUNCIÓN HELPER - Extrae solo el nombre del archivo sin path
// ============================================================================
static String sdBasename(const char* fullPath) {
    String s = String(fullPath);
    int lastSlash = s.lastIndexOf('/');
    if (lastSlash >= 0) {
        return s.substring(lastSlash + 1);
    }
    return s;
}

// ============================================================================
// HTML, CSS y JS alojado en memoria FLASH (PROGMEM) para ahorrar mucha RAM
// ============================================================================
const char INDEX_HTML[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="es">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1.0">
<title>Sleep Environment Analyzer</title>
<script src="https://cdn.jsdelivr.net/npm/chart.js@4.4.0/dist/chart.umd.min.js"></script>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',sans-serif;background:#0f0c1a;color:#e8e6f0;min-height:100vh;padding:20px}
.container{max-width:980px;margin:0 auto}

/* topbar */
.topbar{display:flex;align-items:center;justify-content:space-between;margin-bottom:22px;padding-bottom:16px;border-bottom:1px solid rgba(255,255,255,0.08)}
.topbar h1{font-size:16px;font-weight:500;display:flex;align-items:center;gap:9px;color:#e8e6f0}
.status-row{display:flex;align-items:center;gap:8px}
.sl{font-size:11px;color:#8880a0}

/* badges */
.badge{font-size:11px;font-weight:500;padding:3px 10px;border-radius:20px;display:inline-flex;align-items:center;gap:4px}
.b-ok  {background:rgba(99,153,34,0.18);color:#a4d65e;border:1px solid rgba(99,153,34,0.28)}
.b-warn{background:rgba(186,117,23,0.18);color:#f0b94a;border:1px solid rgba(186,117,23,0.28)}
.b-bad {background:rgba(226,75,74,0.18);color:#f07070;border:1px solid rgba(226,75,74,0.28)}
.b-off {background:rgba(255,255,255,0.06);color:#8880a0;border:1px solid rgba(255,255,255,0.1)}
.live-dot{width:6px;height:6px;border-radius:50%;background:#a4d65e;flex-shrink:0;animation:lp 1.4s infinite}
@keyframes lp{0%,100%{opacity:1}50%{opacity:.2}}

/* infobar */
.infobar{display:grid;grid-template-columns:repeat(3,1fr);gap:10px;margin-bottom:20px}
@media(max-width:600px){.infobar{grid-template-columns:1fr 1fr}}
.info-cell{background:rgba(255,255,255,0.03);border:1px solid rgba(255,255,255,0.07);border-radius:12px;padding:12px 14px}
.info-cell .ic-label{font-size:10px;color:#8880a0;text-transform:uppercase;letter-spacing:.07em;margin-bottom:5px}
.info-cell .ic-val{font-size:14px;font-weight:500;color:#c8c4d8}

/* controles */
.controls{display:flex;gap:10px;justify-content:center;margin-bottom:22px}
.btn{padding:8px 22px;border-radius:24px;cursor:pointer;font-size:13px;font-weight:500;border:1px solid;display:inline-flex;align-items:center;gap:6px;transition:opacity .15s}
.btn:hover{opacity:.75}
.btn-start{background:rgba(99,153,34,0.15);color:#a4d65e;border-color:rgba(99,153,34,0.35)}
.btn-stop {background:rgba(226,75,74,0.15);color:#f07070;border-color:rgba(226,75,74,0.35)}

/* métricas */
.grid4{display:grid;grid-template-columns:repeat(4,1fr);gap:11px;margin-bottom:20px}
@media(max-width:580px){.grid4{grid-template-columns:repeat(2,1fr)}}
.metric{background:rgba(255,255,255,0.04);border:1px solid rgba(255,255,255,0.08);border-radius:14px;padding:15px}
.metric .m-lbl{font-size:10px;color:#8880a0;text-transform:uppercase;letter-spacing:.07em;margin-bottom:8px;display:flex;align-items:center;gap:5px}
.metric .m-val{font-size:28px;font-weight:500;line-height:1;letter-spacing:-.5px}
.metric .m-unit{font-size:11px;color:#8880a0;margin-top:4px}
.c-co2  {color:#a393e8}
.c-temp {color:#f07b50}
.c-hum  {color:#50c8a0}
.c-light{color:#f0c040}

/* estado ambiental */
.env-state{background:rgba(255,255,255,0.03);border:1px solid rgba(255,255,255,0.07);border-radius:14px;padding:16px;margin-bottom:20px}
.env-state h2{font-size:11px;font-weight:500;color:#8880a0;text-transform:uppercase;letter-spacing:.07em;margin-bottom:12px}
.env-grid{display:grid;grid-template-columns:repeat(3,1fr);gap:8px}
@media(max-width:500px){.env-grid{grid-template-columns:1fr}}
.env-item{border-radius:10px;padding:11px 14px;border:1px solid;opacity:.35;transition:opacity .3s,box-shadow .3s}
.env-item .env-title{font-size:12px;font-weight:500;margin-bottom:3px}
.env-item .env-desc{font-size:11px;opacity:.7}
.env-optimo   {background:rgba(99,153,34,0.12);border-color:rgba(99,153,34,0.25)}
.env-optimo   .env-title{color:#a4d65e}
.env-aceptable{background:rgba(186,117,23,0.12);border-color:rgba(186,117,23,0.25)}
.env-aceptable .env-title{color:#f0b94a}
.env-malo     {background:rgba(226,75,74,0.12);border-color:rgba(226,75,74,0.25)}
.env-malo     .env-title{color:#f07070}
.env-item.active{opacity:1}

/* card */
.card{background:rgba(255,255,255,0.03);border:1px solid rgba(255,255,255,0.08);border-radius:16px;padding:18px;margin-bottom:16px}
.card h2{font-size:11px;font-weight:500;color:#8880a0;text-transform:uppercase;letter-spacing:.07em;margin-bottom:14px;display:flex;align-items:center;gap:6px}
.grid2{display:grid;grid-template-columns:1fr 1fr;gap:14px}
@media(max-width:600px){.grid2{grid-template-columns:1fr}}

/* session items */
.si{display:flex;align-items:center;gap:10px;padding:9px 11px;border-radius:10px;margin-bottom:7px;cursor:pointer;border:1px solid rgba(255,255,255,0.06);transition:background .15s}
.si:hover{background:rgba(255,255,255,0.06)}
.rn-wrap{width:24px;height:24px;border-radius:50%;display:flex;align-items:center;justify-content:center;font-size:11px;font-weight:600;flex-shrink:0}
.r1{background:rgba(186,117,23,0.28);color:#f0b94a}
.r2{background:rgba(180,180,180,0.15);color:#c0bbc8}
.r3{background:rgba(180,90,50,0.22);color:#e0845a}
.rn{background:rgba(255,255,255,0.05);color:#8880a0}
.si-info{flex:1;min-width:0}
.si-title{font-size:13px;font-weight:500;white-space:nowrap;overflow:hidden;text-overflow:ellipsis}
.si-sub{font-size:11px;color:#8880a0;margin-top:1px}
.sp{font-size:12px;font-weight:600;padding:3px 10px;border-radius:20px;flex-shrink:0}
.sp-good{background:rgba(99,153,34,0.18);color:#a4d65e;border:1px solid rgba(99,153,34,0.25)}
.sp-mid {background:rgba(186,117,23,0.18);color:#f0b94a;border:1px solid rgba(186,117,23,0.25)}
.sp-low {background:rgba(226,75,74,0.18);color:#f07070;border:1px solid rgba(226,75,74,0.25)}
.empty{text-align:center;padding:24px;color:#8880a0;font-size:13px}

/* modal */
.modal{display:none;position:fixed;inset:0;background:rgba(0,0,0,0.88);z-index:999;justify-content:center;align-items:flex-start;overflow-y:auto;padding:20px}
.modal.open{display:flex}
.mbox{background:#1a1728;border:1px solid rgba(255,255,255,0.1);border-radius:20px;width:100%;max-width:860px;padding:24px;position:relative;margin:auto}
.mbox h2{font-size:15px;font-weight:500;margin-bottom:18px;color:#e8e6f0}
.mclose{position:absolute;top:14px;right:18px;font-size:24px;cursor:pointer;color:#8880a0;background:none;border:none;line-height:1}
.mclose:hover{color:#e8e6f0}

/* bloques detalle */
.sb{background:rgba(255,255,255,0.03);border:1px solid rgba(255,255,255,0.07);border-radius:12px;padding:15px;margin-bottom:12px}
.sb h3{font-size:10px;font-weight:500;color:#8880a0;text-transform:uppercase;letter-spacing:.08em;margin-bottom:10px}
.sb p{font-size:13px;line-height:2;color:#c8c4d8}

/* info general sesión */
.ig-grid{display:grid;grid-template-columns:repeat(3,1fr);gap:8px}
@media(max-width:500px){.ig-grid{grid-template-columns:1fr 1fr}}
.ig-cell{background:rgba(255,255,255,0.04);border-radius:10px;padding:10px 12px;border:1px solid rgba(255,255,255,0.07)}
.ig-cell .ig-lbl{font-size:10px;color:#8880a0;text-transform:uppercase;letter-spacing:.07em;margin-bottom:4px}
.ig-cell .ig-val{font-size:13px;font-weight:500;color:#c8c4d8}

/* alertas */
.alert-list{list-style:none}
.alert-list li{background:rgba(226,75,74,0.1);border-left:3px solid #f07070;border-radius:0 8px 8px 0;padding:8px 12px;margin:6px 0;font-size:12px;color:#f09090}
.alert-list li.ok{background:rgba(99,153,34,0.1);border-color:#a4d65e;color:#a4d65e}

/* gráficas */
.tab-btns{display:flex;gap:6px;flex-wrap:wrap;margin-bottom:12px}
.tab-btn{background:rgba(255,255,255,0.06);border:1px solid rgba(255,255,255,0.1);padding:5px 14px;border-radius:20px;cursor:pointer;color:#c8c4d8;font-size:12px;transition:background .15s}
.tab-btn.active{background:rgba(163,147,232,0.28);border-color:rgba(163,147,232,0.4);color:#c8bff8}
.tab-content{display:none}
.tab-content.active{display:block}
.chart-wrap{background:rgba(0,0,0,0.18);border-radius:10px;padding:14px;margin-top:4px}
canvas{max-height:180px;width:100%!important}

/* recomendaciones */
.rec-item{display:flex;align-items:flex-start;gap:10px;padding:8px 11px;border-radius:8px;margin-bottom:6px;background:rgba(163,147,232,0.08);border:1px solid rgba(163,147,232,0.18)}
.rec-item .rec-time{font-size:11px;color:#a393e8;font-weight:600;white-space:nowrap;min-width:38px}
.rec-item .rec-text{font-size:12px;color:#c8c4d8;line-height:1.5}

/* mejor franja */
.best-hour{background:rgba(99,153,34,0.1);border:1px solid rgba(99,153,34,0.22);border-radius:10px;padding:12px 14px;font-size:13px;color:#a4d65e;font-weight:500}

.spinner{text-align:center;padding:36px;color:#8880a0;font-size:13px}
</style>
</head>
<body>
<div class="container">

<div class="topbar">
  <h1>
    <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="#a393e8" stroke-width="1.8" stroke-linecap="round"><path d="M21 12.79A9 9 0 1 1 11.21 3a7 7 0 0 0 9.79 9.79z"/></svg>
    Sleep Environment Analyzer
  </h1>
  <div class="status-row">
    <span class="sl">Ambiente</span>
    <span class="badge b-off" id="env_badge">--</span>
    <span class="sl">Sesión</span>
    <span class="badge b-off" id="session_badge">--</span>
  </div>
</div>

<div class="infobar">
  <div class="info-cell">
    <div class="ic-label">Hora actual</div>
    <div class="ic-val" id="info_time">--:--:--</div>
  </div>
  <div class="info-cell">
    <div class="ic-label">Fecha actual</div>
    <div class="ic-val" id="info_date">--</div>
  </div>
  <div class="info-cell">
    <div class="ic-label">Estado de sesión</div>
    <div class="ic-val" id="info_session">Inactiva</div>
  </div>
</div>

<div class="controls">
  <button class="btn btn-start" onclick="startSession()">
    <svg width="11" height="11" viewBox="0 0 24 24" fill="currentColor"><polygon points="5,3 19,12 5,21"/></svg>
    Iniciar sesión
  </button>
  <button class="btn btn-stop" onclick="stopSession()">
    <svg width="11" height="11" viewBox="0 0 24 24" fill="currentColor"><rect x="4" y="4" width="16" height="16" rx="2"/></svg>
    Finalizar sesión
  </button>
</div>

<div class="grid4">
  <div class="metric">
    <div class="m-lbl"><svg width="12" height="12" viewBox="0 0 24 24" fill="none" stroke="#a393e8" stroke-width="2"><path d="M12 2L8 8H4l4 5H4l8 9 8-9h-4l4-5h-4z"/></svg>CO₂</div>
    <div class="m-val c-co2" id="co2_val">--</div>
    <div class="m-unit">ppm</div>
  </div>
  <div class="metric">
    <div class="m-lbl"><svg width="12" height="12" viewBox="0 0 24 24" fill="none" stroke="#f07b50" stroke-width="2"><path d="M14 14.76V3.5a2.5 2.5 0 0 0-5 0v11.26a4.5 4.5 0 1 0 5 0z"/></svg>Temperatura</div>
    <div class="m-val c-temp" id="temp_val">--</div>
    <div class="m-unit">°C</div>
  </div>
  <div class="metric">
    <div class="m-lbl"><svg width="12" height="12" viewBox="0 0 24 24" fill="none" stroke="#50c8a0" stroke-width="2"><path d="M12 2.69l5.66 5.66a8 8 0 1 1-11.31 0z"/></svg>Humedad</div>
    <div class="m-val c-hum" id="hum_val">--</div>
    <div class="m-unit">%</div>
  </div>
  <div class="metric">
    <div class="m-lbl"><svg width="12" height="12" viewBox="0 0 24 24" fill="none" stroke="#f0c040" stroke-width="2"><circle cx="12" cy="12" r="4"/><line x1="12" y1="2" x2="12" y2="4"/><line x1="12" y1="20" x2="12" y2="22"/><line x1="4.22" y1="4.22" x2="5.64" y2="5.64"/><line x1="18.36" y1="18.36" x2="19.78" y2="19.78"/><line x1="2" y1="12" x2="4" y2="12"/><line x1="20" y1="12" x2="22" y2="12"/><line x1="4.22" y1="19.78" x2="5.64" y2="18.36"/><line x1="18.36" y1="5.64" x2="19.78" y2="4.22"/></svg>Luz</div>
    <div class="m-val c-light" id="light_val">--</div>
    <div class="m-unit">lux</div>
  </div>
</div>

<div class="env-state">
  <h2>Estado ambiental actual</h2>
  <div class="env-grid">
    <div class="env-item env-optimo" id="env-optimo">
      <div class="env-title">Condiciones óptimas</div>
      <div class="env-desc">Temperatura, CO₂, humedad y luz en rangos ideales para el sueño</div>
    </div>
    <div class="env-item env-aceptable" id="env-aceptable">
      <div class="env-title">Condiciones aceptables</div>
      <div class="env-desc">Algún parámetro ligeramente fuera del rango óptimo</div>
    </div>
    <div class="env-item env-malo" id="env-malo">
      <div class="env-title">Condiciones desfavorables</div>
      <div class="env-desc">Uno o más parámetros en rango perjudicial para el descanso</div>
    </div>
  </div>
</div>

<div class="grid2">
  <div class="card">
    <h2>
      <svg width="12" height="12" viewBox="0 0 24 24" fill="none" stroke="#f0b94a" stroke-width="2"><path d="M12 2l3.09 6.26L22 9.27l-5 4.87 1.18 6.88L12 17.77l-6.18 3.25L7 14.14 2 9.27l6.91-1.01z"/></svg>
      Ranking de sesiones
    </h2>
    <div id="ranking-list"><div class="empty">Cargando...</div></div>
  </div>
  <div class="card">
    <h2>
      <svg width="12" height="12" viewBox="0 0 24 24" fill="none" stroke="#8880a0" stroke-width="2"><circle cx="12" cy="12" r="10"/><polyline points="12 6 12 12 16 14"/></svg>
      Historial de sesiones
    </h2>
    <div id="history-list"><div class="empty">Cargando...</div></div>
    <p style="font-size:11px;color:#8880a0;margin-top:10px;text-align:center">Toca una sesión para ver los detalles</p>
  </div>
</div>

</div><div id="session-modal" class="modal" onclick="if(event.target===this)closeModal()">
  <div class="mbox">
    <button class="mclose" onclick="closeModal()">&#215;</button>
    <div id="modal-body"><div class="spinner">Cargando...</div></div>
  </div>
</div>

<script>
var _charts = {};
var _pollTimer = null;
var _pollErrors = 0;
var _lastStatus = null;   

function tickClock() {
  var now = new Date();
  var pad = function(n){ return String(n).padStart(2,'0'); };
  document.getElementById('info_time').textContent =
    pad(now.getHours())+':'+pad(now.getMinutes())+':'+pad(now.getSeconds());
  document.getElementById('info_date').textContent =
    pad(now.getDate())+'/'+pad(now.getMonth()+1)+'/'+now.getFullYear();
}
setInterval(tickClock, 1000);
tickClock();

function spClass(s){ return s>=80?'sp-good':s>=60?'sp-mid':'sp-low'; }
function fmtDur(secs){
  if(!secs) return '--';
  var h=Math.floor(secs/3600), m=Math.floor((secs%3600)/60);
  return h>0?h+'h '+m+'min':m+'min';
}

async function pollStatus() {
  try {
    var r = await fetch('/api/status',{cache:'no-store'});
    if (!r.ok) throw new Error('http '+r.status);
    var d = await r.json();
    _pollErrors = 0;

    document.getElementById('co2_val').textContent   = d.co2         !== undefined ? Math.round(d.co2)        : '--';
    document.getElementById('temp_val').textContent  = d.temperature !== undefined ? d.temperature.toFixed(1) : '--';
    document.getElementById('hum_val').textContent   = d.humidity    !== undefined ? Math.round(d.humidity)   : '--';
    document.getElementById('light_val').textContent = d.light       !== undefined ? Math.round(d.light)      : '--';

    var eb = document.getElementById('env_badge');
    eb.className = 'badge';
    var st = d.status || 'DESCONOCIDO';
    if      (st==='OPTIMO')   { eb.textContent='Óptimo';    eb.classList.add('b-ok'); }
    else if (st==='ACEPTABLE'){ eb.textContent='Aceptable'; eb.classList.add('b-warn'); }
    else if (st==='MALO')     { eb.textContent='Malo';      eb.classList.add('b-bad'); }
    else                      { eb.textContent='--';        eb.classList.add('b-off'); }

    ['env-optimo','env-aceptable','env-malo'].forEach(function(id){
      document.getElementById(id).classList.remove('active');
      document.getElementById(id).style.boxShadow='none';
    });
    if      (st==='OPTIMO')   { document.getElementById('env-optimo').classList.add('active');    document.getElementById('env-optimo').style.boxShadow='0 0 0 2px #a4d65e'; }
    else if (st==='ACEPTABLE'){ document.getElementById('env-aceptable').classList.add('active'); document.getElementById('env-aceptable').style.boxShadow='0 0 0 2px #f0b94a'; }
    else if (st==='MALO')     { document.getElementById('env-malo').classList.add('active');      document.getElementById('env-malo').style.boxShadow='0 0 0 2px #f07070'; }
    else { ['env-optimo','env-aceptable','env-malo'].forEach(function(id){ document.getElementById(id).classList.add('active'); }); }

    var sb = document.getElementById('session_badge');
    var is = document.getElementById('info_session');
    sb.className = 'badge';
    if (d.sessionActive) {
      sb.innerHTML='<span class="live-dot"></span>Activa'; sb.classList.add('b-ok');
      is.textContent='Activa'; is.style.color='#a4d65e';
    } else {
      sb.textContent='Inactiva'; sb.classList.add('b-off');
      is.textContent='Inactiva'; is.style.color='#8880a0';
    }

    if (_lastStatus !== null && _lastStatus !== d.sessionActive) {
      loadLists();
    }
    _lastStatus = d.sessionActive;

  } catch(e) {
    _pollErrors++;
    if (_pollErrors >= 3) {
      document.getElementById('env_badge').className='badge b-off';
      document.getElementById('env_badge').textContent='Sin conexión';
    }
  }
}

async function startSession() {
  try {
    var r = await fetch('/api/session',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({sessionActive:true})});
    var d = await r.json();
    if (d.success) { pollStatus(); }
  } catch(e){}
}
async function stopSession() {
  try {
    var r = await fetch('/api/session',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({sessionActive:false})});
    var d = await r.json();
    if (d.success) { setTimeout(loadLists, 1500); pollStatus(); }
  } catch(e){}
}

async function loadLists() {
  try {
    var r = await fetch('/api/sessions',{cache:'no-store'});
    var sessions = await r.json();

    if (!sessions || sessions.length === 0) {
      var msg = '<div class="empty">No hay sesiones registradas</div>';
      document.getElementById('ranking-list').innerHTML = msg;
      document.getElementById('history-list').innerHTML = msg;
      return;
    }

    var ranked = sessions.slice().sort(function(a,b){ return (b.sleepScore||0)-(a.sleepScore||0); });
    var rHtml = '';
    ranked.forEach(function(s,i){
      var nc = i===0?'r1':i===1?'r2':i===2?'r3':'rn';
      var pc = spClass(s.sleepScore||0);
      rHtml += '<div class="si" onclick="showDetail('+s.id+')">'
             + '<div class="rn-wrap '+nc+'">'+(i+1)+'</div>'
             + '<div class="si-info">'
             + '<div class="si-title">Sesión #'+s.id+'</div>'
             + '<div class="si-sub">'+fmtDur(s.duration)+(s.date?' · '+s.date:'')+'</div>'
             + '</div>'
             + '<span class="sp '+pc+'">'+(s.sleepScore!==undefined?s.sleepScore:'--')+'</span>'
             + '</div>';
    });
    document.getElementById('ranking-list').innerHTML = rHtml;

    var hist = sessions.slice().sort(function(a,b){ return b.id-a.id; });
    var hHtml = '';
    hist.forEach(function(s){
      var pc = spClass(s.sleepScore||0);
      hHtml += '<div class="si" onclick="showDetail('+s.id+')">'
             + '<div class="si-info">'
             + '<div class="si-title">Sesión #'+s.id+'</div>'
             + '<div class="si-sub">'+fmtDur(s.duration)+(s.date?' · '+s.date:'')+'</div>'
             + '</div>'
             + '<span class="sp '+pc+'">'+(s.sleepScore!==undefined?s.sleepScore:'--')+'</span>'
             + '</div>';
    });
    document.getElementById('history-list').innerHTML = hHtml;
  } catch(e){ console.error('[loadLists]',e); }
}

async function showDetail(sid) {
  document.getElementById('modal-body').innerHTML='<div class="spinner">Cargando datos de la sesión...</div>';
  document.getElementById('session-modal').classList.add('open');
  try {
    var results = await Promise.all([
      fetch('/api/session/stats?id='+sid,  {cache:'no-store'}),
      fetch('/api/session/alerts?id='+sid, {cache:'no-store'}),
      fetch('/api/session/data?id='+sid,   {cache:'no-store'})
    ]);
    var stats    = await results[0].json();
    var alerts   = await results[1].json();
    var timeline = await results[2].json();

    var pc = spClass(stats.sleepScore||0);
    var scoreColor = pc==='sp-good'?'#a4d65e':pc==='sp-mid'?'#f0b94a':'#f07070';

    var igHtml =
      '<div class="ig-grid">'
      +'<div class="ig-cell"><div class="ig-lbl">Fecha</div><div class="ig-val">'+(stats.date||'--')+'</div></div>'
      +'<div class="ig-cell"><div class="ig-lbl">Hora de inicio</div><div class="ig-val">'+(stats.startTime||'--')+'</div></div>'
      +'<div class="ig-cell"><div class="ig-lbl">Hora de fin</div><div class="ig-val">'+(stats.endTime||'--')+'</div></div>'
      +'<div class="ig-cell"><div class="ig-lbl">Duración total</div><div class="ig-val">'+fmtDur(stats.duration)+'</div></div>'
      +'<div class="ig-cell"><div class="ig-lbl">Sleep Score</div><div class="ig-val" style="color:'+scoreColor+';font-size:18px;font-weight:600">'+(stats.sleepScore!==undefined?stats.sleepScore:'--')+'/100</div></div>'
      +'<div class="ig-cell"><div class="ig-lbl">Interpretación</div><div class="ig-val" style="font-size:12px">'+(stats.interpretation||'--')+'</div></div>'
      +'</div>';

    var statsHtml =
      '<p>CO₂ → máx <strong>'+(stats.co2&&stats.co2.max!==undefined?stats.co2.max:'--')+'</strong> · mín <strong>'+(stats.co2&&stats.co2.min!==undefined?stats.co2.min:'--')+'</strong> · promedio <strong>'+(stats.co2&&stats.co2.avg!==undefined?stats.co2.avg:'--')+'</strong> ppm</p>'
     +'<p>Temperatura → máx <strong>'+(stats.temperature&&stats.temperature.max!==undefined?stats.temperature.max:'--')+'</strong> · mín <strong>'+(stats.temperature&&stats.temperature.min!==undefined?stats.temperature.min:'--')+'</strong> · promedio <strong>'+(stats.temperature&&stats.temperature.avg!==undefined?stats.temperature.avg:'--')+'</strong> °C</p>'
     +'<p>Humedad → máx <strong>'+(stats.humidity&&stats.humidity.max!==undefined?stats.humidity.max:'--')+'</strong> · mín <strong>'+(stats.humidity&&stats.humidity.min!==undefined?stats.humidity.min:'--')+'</strong> · promedio <strong>'+(stats.humidity&&stats.humidity.avg!==undefined?stats.humidity.avg:'--')+'</strong> %</p>'
     +'<p>Luz → máx <strong>'+(stats.light&&stats.light.max!==undefined?stats.light.max:'--')+'</strong> · mín <strong>'+(stats.light&&stats.light.min!==undefined?stats.light.min:'--')+'</strong> · promedio <strong>'+(stats.light&&stats.light.avg!==undefined?stats.light.avg:'--')+'</strong> lux</p>';

    var alertsHtml = '';
    if (alerts.alerts && alerts.alerts.length > 0) {
      alerts.alerts.forEach(function(a){
        alertsHtml += '<li><strong>'+(a.time||'')+'</strong> → '+(a.type||'')+': '+(a.message||'')+'</li>';
      });
    } else {
      alertsHtml = '<li class="ok">Sin alertas registradas</li>';
    }

    var bestHour = '--';
    if (stats.bestHour && stats.bestHour.start !== undefined) {
      var fmt = function(s){ return String(Math.floor(s/60)).padStart(2,'0')+':'+String(s%60).padStart(2,'0'); };
      bestHour = fmt(stats.bestHour.start)+' – '+fmt(stats.bestHour.end)+' → Condiciones óptimas';
    }

    var recsHtml = '';
    if (stats.recommendations && stats.recommendations.length > 0) {
      stats.recommendations.forEach(function(rec){
        recsHtml += '<div class="rec-item"><span class="rec-time">'+(rec.time||'')+'</span><span class="rec-text">'+(rec.message||rec)+'</span></div>';
      });
    } else if (alerts.alerts && alerts.alerts.length > 0) {
      alerts.alerts.forEach(function(a){
        var msg='', t=(a.type||'').toLowerCase();
        if      (t.indexOf('co2')>=0)                                    msg='Ventilar la habitación para reducir el CO₂';
        else if (t.indexOf('temp')>=0)                                   msg='Ajustar la temperatura de la habitación';
        else if (t.indexOf('hum')>=0)                                    msg='Regular la humedad ambiental';
        else if (t.indexOf('luz')>=0||t.indexOf('light')>=0)             msg='Reducir la iluminación ambiental';
        else                                                             msg=a.message||'Revisar condiciones ambientales';
        recsHtml += '<div class="rec-item"><span class="rec-time">'+(a.time||'')+'</span><span class="rec-text">'+msg+'</span></div>';
      });
    } else {
      recsHtml = '<p style="color:#8880a0;font-size:13px">Sin recomendaciones — las condiciones fueron óptimas durante toda la sesión.</p>';
    }

    document.getElementById('modal-body').innerHTML =
      '<h2>Detalle de sesión #'+sid+'</h2>'
      +'<div class="sb"><h3>Información general de sesión</h3>'+igHtml+'</div>'
      +'<div class="sb"><h3>Estadísticas ambientales</h3>'+statsHtml+'</div>'
      +'<div class="sb"><h3>Alertas generadas</h3><ul class="alert-list">'+alertsHtml+'</ul></div>'
      +'<div class="sb"><h3>Sleep timeline</h3>'
      +'<div class="tab-btns">'
      +'<button class="tab-btn active" onclick="switchTab(\'co2\',event)">CO₂</button>'
      +'<button class="tab-btn" onclick="switchTab(\'temp\',event)">Temperatura</button>'
      +'<button class="tab-btn" onclick="switchTab(\'hum\',event)">Humedad</button>'
      +'<button class="tab-btn" onclick="switchTab(\'light\',event)">Luz</button>'
      +'</div>'
      +'<div id="tab-co2"   class="tab-content active"><div class="chart-wrap"><canvas id="ch-co2"></canvas></div></div>'
      +'<div id="tab-temp"  class="tab-content"><div class="chart-wrap"><canvas id="ch-temp"></canvas></div></div>'
      +'<div id="tab-hum"   class="tab-content"><div class="chart-wrap"><canvas id="ch-hum"></canvas></div></div>'
      +'<div id="tab-light" class="tab-content"><div class="chart-wrap"><canvas id="ch-light"></canvas></div></div>'
      +'</div>'
      +'<div class="sb"><h3>Mejor franja horaria detectada</h3><div class="best-hour">'+bestHour+'</div></div>'
      +'<div class="sb"><h3>Recomendaciones generadas</h3>'+recsHtml+'</div>';

    Object.values(_charts).forEach(function(c){ if(c) c.destroy(); });
    _charts = {};

    if (timeline.timestamps && timeline.timestamps.length > 0) {
      var labels = timeline.timestamps.map(function(ts){
        var sec=Math.floor(ts/1000), h=Math.floor(sec/3600)%24, m=Math.floor(sec/60)%60;
        return String(h).padStart(2,'0')+':'+String(m).padStart(2,'0');
      });
      function mkChart(id, label, data, color) {
        return new Chart(document.getElementById(id),{
          type:'line',
          data:{ labels:labels, datasets:[{ label:label, data:data,
            borderColor:color, backgroundColor:color+'22',
            fill:true, tension:0.3, pointRadius:0, borderWidth:1.5 }]},
          options:{ responsive:true, maintainAspectRatio:true, animation:false,
            plugins:{ legend:{ labels:{ color:'#8880a0', font:{size:11} } } },
            scales:{
              x:{ ticks:{ color:'#8880a0', font:{size:10}, maxRotation:45, autoSkip:true, maxTicksLimit:10 }, grid:{color:'rgba(255,255,255,0.05)'} },
              y:{ ticks:{ color:'#8880a0', font:{size:10} }, grid:{color:'rgba(255,255,255,0.05)'} }
            }
          }
        });
      }
      _charts.co2   = mkChart('ch-co2',  'CO₂ (ppm)',        timeline.co2         ||[],'#a393e8');
      _charts.temp  = mkChart('ch-temp', 'Temperatura (°C)', timeline.temperature ||[],'#f07b50');
      _charts.hum   = mkChart('ch-hum',  'Humedad (%)',      timeline.humidity    ||[],'#50c8a0');
      _charts.light = mkChart('ch-light','Luz (lux)',        timeline.light       ||[],'#f0c040');
    } else {
      document.querySelectorAll('.tab-content').forEach(function(el){
        el.innerHTML='<div class="empty">Sin datos suficientes para las gráficas</div>';
      });
    }
  } catch(e) {
    document.getElementById('modal-body').innerHTML='<div class="spinner">Error al cargar los datos de la sesión</div>';
    console.error('[showDetail]',e);
  }
}

function switchTab(tab, event) {
  document.querySelectorAll('.tab-content').forEach(function(el){ el.classList.remove('active'); });
  document.querySelectorAll('.tab-btn').forEach(function(el){ el.classList.remove('active'); });
  document.getElementById('tab-'+tab).classList.add('active');
  if (event && event.target) event.target.classList.add('active');
  setTimeout(function(){ if(_charts[tab]) _charts[tab].resize(); }, 80);
}

function closeModal() {
  document.getElementById('session-modal').classList.remove('open');
}
document.addEventListener('keydown', function(e){ if(e.key==='Escape') closeModal(); });

pollStatus();
loadLists();
setInterval(pollStatus, 3000);   
setInterval(loadLists,  30000);  
</script>
</body>
</html>)rawliteral";

// ============================================================================
// start() - Configura el AP, registra rutas y crea la tarea
// ============================================================================
void WebServerTask::start() {
    _dataMutex = xSemaphoreCreateMutex();

    setupAccessPoint();

    server.on("/",            handleRoot);
    server.on("/api/status",          handleApiStatus);
    server.on("/api/session",  HTTP_POST, handleApiSession);
    server.on("/api/sessions",        handleApiSessions);
    server.on("/api/session/stats",   handleApiSessionStats);
    server.on("/api/session/alerts",  handleApiSessionAlerts);
    server.on("/api/session/data",    handleApiSessionData);
    server.onNotFound(handleNotFound);

    server.begin();
    Serial.println("[Web] Servidor iniciado");
    Serial.printf("[Web] http://%s\n", WiFi.softAPIP().toString().c_str());

    xTaskCreatePinnedToCore(
        taskFunction,
        "WebServerTask",
        WEB_TASK_STACK,
        nullptr,
        WEB_TASK_PRIORITY,
        &_taskHandle,
        1
    );
}

void WebServerTask::setupAccessPoint() {
    Serial.println("[Web] Configurando Access Point...");
    WiFi.mode(WIFI_AP);
    bool ok = WiFi.softAP(AP_SSID, AP_PASSWORD, AP_CHANNEL, AP_HIDDEN);
    if (ok) {
        Serial.printf("[Web] AP OK · SSID: %s · IP: %s\n", AP_SSID, WiFi.softAPIP().toString().c_str());
    } else {
        Serial.println("[Web] Error al crear el Access Point");
    }
}

void WebServerTask::taskFunction(void* pvParams) {
    while (true) {
        server.handleClient();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void WebServerTask::updateCurrentData(const SensorData &data) {
    if (_dataMutex && xSemaphoreTake(_dataMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        _currentData = data;
        xSemaphoreGive(_dataMutex);
    }
}

void WebServerTask::handleApiStatus() {
    SensorData snap = {0};
    if (_dataMutex && xSemaphoreTake(_dataMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        snap = _currentData;
        xSemaphoreGive(_dataMutex);
    }

    StaticJsonDocument<320> doc;
    doc["co2"]         = snap.co2;
    doc["temperature"] = snap.temperature;
    doc["humidity"]    = snap.humidity;
    doc["light"]       = snap.light;

    String status = "DESCONOCIDO";
    if (snap.co2 > 0) {
        bool malo = false, regular = false;

        if (snap.co2 > CO2_ACCEPTABLE_MAX) malo = true;
        else if (snap.co2 > CO2_GOOD_MAX)  regular = true;

        if (snap.temperature < TEMP_GOOD_MIN ||
            snap.temperature > TEMP_ACCEPTABLE_MAX) malo = true;
        else if (snap.temperature > TEMP_GOOD_MAX)  regular = true;

        if (snap.humidity < HUM_ACCEPTABLE_MIN1 ||
            snap.humidity > HUM_ACCEPTABLE_MAX2) malo = true;
        else if ((snap.humidity >= HUM_ACCEPTABLE_MIN1 && snap.humidity < HUM_GOOD_MIN) ||
                 (snap.humidity > HUM_GOOD_MAX && snap.humidity <= HUM_ACCEPTABLE_MAX2)) regular = true;

        if (snap.light >= LIGHT_ACCEPTABLE_MAX) malo = true;
        else if (snap.light >= LIGHT_GOOD_MAX)  regular = true;

        if (malo)         status = "MALO";
        else if (regular) status = "ACEPTABLE";
        else              status = "OPTIMO";
    }
    doc["status"]        = status;
    doc["sessionActive"] = SessionManager::isSessionActive();

    unsigned long secs = millis() / 1000;
    char timeStr[12];
    sprintf(timeStr, "%02d:%02d:%02d",
            (int)(secs / 3600) % 24,
            (int)(secs / 60) % 60,
            (int)(secs % 60));
    doc["uptime"] = timeStr;

    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
}

void WebServerTask::handleApiSession() {
    StaticJsonDocument<128> doc;
    String payload = server.hasArg("plain") ? server.arg("plain") : server.arg(0);
    DeserializationError err = deserializeJson(doc, payload);
    
    if (err) {
        server.send(400, "application/json", "{\"error\":\"JSON invalido\"}");
        return;
    }

    bool active = doc["sessionActive"] | false;
    bool ok = active ? SessionManager::startSession() : SessionManager::stopSession();

    StaticJsonDocument<64> resp;
    resp["success"]       = ok;
    resp["sessionActive"] = SessionManager::isSessionActive();
    String out;
    serializeJson(resp, out);
    server.send(200, "application/json", out);
}

void WebServerTask::handleApiSessions() {
    DynamicJsonDocument doc(8192);
    JsonArray sessions = doc.to<JsonArray>();

    File root = SD.open(SD_BASE_PATH);
    if (!root) {
        server.send(200, "application/json", "[]");
        return;
    }

    File file = root.openNextFile();
    while (file) {
        String fullPath = String(file.name());
        String name = sdBasename(fullPath.c_str());
        file.close();

        if (name.indexOf("_stats") >= 0) {
            int firstUnderscore = name.indexOf('_');
            int secondUnderscore = name.indexOf('_', firstUnderscore + 1);
            
            if (firstUnderscore >= 0 && secondUnderscore > firstUnderscore) {
                String idStr = name.substring(firstUnderscore + 1, secondUnderscore);
                int sessionId = idStr.toInt();
                
                if (sessionId > 0) {
                    String fullPathFile = String(SD_BASE_PATH) + "/" + name;
                    File sf = SD.open(fullPathFile.c_str(), FILE_READ);
                    if (sf) {
                        DynamicJsonDocument sd(2048);
                        DeserializationError e = deserializeJson(sd, sf);
                        sf.close();
                        if (!e) {
                            JsonObject s = sessions.createNestedObject();
                            s["id"]         = sessionId;
                            s["sleepScore"] = sd["sleepScore"]  | 0;
                            s["duration"]   = sd["duration"]    | 0;
                            s["date"]       = sd["date"]        | "";
                            s["startTime"]  = sd["startTime"]   | "";
                            s["endTime"]    = sd["endTime"]     | "";
                        }
                    }
                }
            }
        }
        file = root.openNextFile();
    }
    root.close();

    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
}

void WebServerTask::handleApiSessionStats() {
    if (!server.hasArg("id")) {
        server.send(400, "application/json", "{\"error\":\"Falta el parametro id\"}");
        return;
    }

    int idNum = server.arg("id").toInt();
    String statsPath = "";
    
    File root = SD.open(SD_BASE_PATH);
    if (root) {
        File file = root.openNextFile();
        while (file) {
            String fullPath = String(file.name());
            String name = sdBasename(fullPath.c_str());
            file.close();
            
            char searchPattern[32];
            snprintf(searchPattern, sizeof(searchPattern), "session_%03d_stats", idNum);
            
            if (name.indexOf(searchPattern) >= 0) {
                statsPath = String(SD_BASE_PATH) + "/" + name;
                break;
            }
            file = root.openNextFile();
        }
        root.close();
    }
    
    if (statsPath.length() == 0) {
        server.send(404, "application/json", "{\"error\":\"Sesion no encontrada\"}");
        return;
    }
    
    File f = SD.open(statsPath.c_str(), FILE_READ);
    if (!f) {
        server.send(500, "application/json", "{\"error\":\"No se pudo abrir el archivo\"}");
        return;
    }
    
    String content = f.readString();
    f.close();
    server.send(200, "application/json", content);
}

void WebServerTask::handleApiSessionAlerts() {
    if (!server.hasArg("id")) {
        server.send(400, "application/json", "{\"error\":\"Falta el parametro id\"}");
        return;
    }

    int idNum = server.arg("id").toInt();
    String alertsPath = "";
    
    File root = SD.open(SD_BASE_PATH);
    if (root) {
        File file = root.openNextFile();
        while (file) {
            String fullPath = String(file.name());
            String name = sdBasename(fullPath.c_str());
            file.close();
            
            char searchPattern[32];
            snprintf(searchPattern, sizeof(searchPattern), "session_%03d_alerts", idNum);
            
            if (name.indexOf(searchPattern) >= 0) {
                alertsPath = String(SD_BASE_PATH) + "/" + name;
                break;
            }
            file = root.openNextFile();
        }
        root.close();
    }
    
    if (alertsPath.length() == 0) {
        char emptyResp[128];
        snprintf(emptyResp, sizeof(emptyResp), "{\"sessionId\":%d,\"alerts\":[]}", idNum);
        server.send(200, "application/json", emptyResp);
        return;
    }
    
    File f = SD.open(alertsPath.c_str(), FILE_READ);
    if (!f) {
        char emptyResp[128];
        snprintf(emptyResp, sizeof(emptyResp), "{\"sessionId\":%d,\"alerts\":[]}", idNum);
        server.send(200, "application/json", emptyResp);
        return;
    }
    
    String content = f.readString();
    f.close();
    server.send(200, "application/json", content);
}

void WebServerTask::handleApiSessionData() {
    if (!server.hasArg("id")) {
        server.send(400, "application/json", "{\"error\":\"Falta el parametro id\"}");
        return;
    }

    int idNum = server.arg("id").toInt();
    char prefixStr[32];
    snprintf(prefixStr, sizeof(prefixStr), "session_%03d_", idNum);
    String prefix = String(prefixStr);

    String csvPath = "";
    File root = SD.open(SD_BASE_PATH);
    if (root) {
        File f = root.openNextFile();
        while (f) {
            String fullPath = String(f.name());
            String name = sdBasename(fullPath.c_str());
            f.close();

            if (name.startsWith(prefix) && name.indexOf("_alerts") < 0 && name.indexOf("_stats") < 0) {
                csvPath = String(SD_BASE_PATH) + "/" + name;
                break;
            }
            f = root.openNextFile();
        }
        root.close();
    }

    if (csvPath.length() == 0) {
        server.send(404, "application/json", "{\"error\":\"CSV no encontrado\"}");
        return;
    }

    File dataFile = SD.open(csvPath.c_str(), FILE_READ);
    if (!dataFile) {
        server.send(500, "application/json", "{\"error\":\"No se pudo abrir el CSV\"}");
        return;
    }

    DynamicJsonDocument doc(32768);
    JsonArray timestamps = doc.createNestedArray("timestamps");
    JsonArray co2Arr     = doc.createNestedArray("co2");
    JsonArray tempArr    = doc.createNestedArray("temperature");
    JsonArray humArr     = doc.createNestedArray("humidity");
    JsonArray lightArr   = doc.createNestedArray("light");

    bool headerSkipped = false;
    while (dataFile.available()) {
        String line = dataFile.readStringUntil('\n');
        line.trim();
        if (line.length() == 0 || line.startsWith("#")) continue;

        if (!headerSkipped && (line.startsWith("timestamp") || line.startsWith("time"))) {
            headerSkipped = true;
            continue;
        }
        if (!headerSkipped) continue;

        int i1 = line.indexOf(',');
        int i2 = line.indexOf(',', i1 + 1);
        int i3 = line.indexOf(',', i2 + 1);
        int i4 = line.indexOf(',', i3 + 1);
        if (i1 < 0 || i2 < 0 || i3 < 0 || i4 < 0) continue;

        timestamps.add(line.substring(0, i1).toInt());
        co2Arr.add(line.substring(i1 + 1, i2).toFloat());
        tempArr.add(line.substring(i2 + 1, i3).toFloat());
        humArr.add(line.substring(i3 + 1, i4).toFloat());
        lightArr.add(line.substring(i4 + 1).toFloat());
    }
    dataFile.close();

    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
}

void WebServerTask::handleNotFound() {
    server.send(404, "text/plain", "404: Not Found");
}

void WebServerTask::handleRoot() {
    server.send_P(200, "text/html", INDEX_HTML);
}