// =======================================================
// DRON V7: JOYSTICK VIRTUAL + TRIMS PRE-CALIBRADOS
// =======================================================
#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>

// Pines de Motores
const int PIN_M0 = 0; // Delantero Izquierdo
const int PIN_M1 = 1; // Delantero Derecho
const int PIN_M2 = 3; // Trasero Izquierdo
const int PIN_M3 = 4; // Trasero Derecho

// Pines I2C (Asegúrate de tenerlos en 10 y 7 o los que te funcionaron)
const int SDA_PIN = 10;
const int SCL_PIN = 7;
const int MPU_ADDR = 0x68;

const char* ssid = "Dron_Estable";
const char* password = "password123";
WebServer server(80);

// ==========================================
// GLOBALES DE VUELO Y TELEMETRÍA
// ==========================================
int velocidad_base = 0;
bool aterrizando = false;
unsigned long tiempo_ultimo_descenso = 0;

// Trims pre-calibrados basados en las pruebas reales
int trim_M0 = 35;  // Necesita empuje extra
int trim_M1 = -30; // Necesita frenarse
int trim_M2 = -50; // Trasero con potencia negativa
int trim_M3 = -50; // Trasero con potencia negativa

// Variables de Joystick
int joy_x = 0; 
int joy_y = 0;

// Variables de Motores y Filtros
int v0 = 0, v1 = 0, v2 = 0, v3 = 0; 
float alpha = 0.15; 
float comp_X_filtrado = 0;
float comp_Y_filtrado = 0;

// ==========================================
// PÁGINA WEB (HTML/CSS/JS)
// ==========================================
const char* paginaHTML = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no">
  <title>Estación de Control V7</title>
  <style>
    body { font-family: Arial, sans-serif; text-align: center; background-color: #1a1a1a; color: white; margin: 0; padding: 10px; user-select: none; }
    .panel { background: #2d2d2d; margin: 10px auto; padding: 15px; border-radius: 10px; width: 95%; max-width: 500px; box-shadow: 0 4px 8px rgba(0,0,0,0.5); }
    input[type=range] { width: 100%; height: 35px; cursor: pointer; }
    .val-display { font-size: 2em; font-weight: bold; color: #4CAF50; margin: 5px 0; }
    button { border: none; padding: 12px 15px; font-size: 1.1em; border-radius: 5px; cursor: pointer; margin: 5px; font-weight: bold; width: 45%; }
    .btn-land { background-color: #ff9800; color: white; }
    .btn-stop { background-color: #f44336; color: white; }
    
    .dron-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 10px; margin-top: 15px; }
    .motor-box { background: #3d3d3d; padding: 10px; border-radius: 8px; border: 1px solid #555; }
    .pwm-val { font-size: 1.8em; color: #ffeb3b; font-weight: bold; display: block; margin: 5px 0; }
    .m-title { font-size: 0.9em; color: #aaa; }
    .trim-slider { width: 90%; height: 25px !important; }
    .trim-label { font-size: 0.8em; color: #00bcd4; font-weight: bold; }

    .telemetria-giro { display: grid; grid-template-columns: 1fr 1fr 1fr; grid-template-rows: 1fr 1fr 1fr; gap: 2px; justify-items: center; align-items: center; margin: 15px auto; background: #222; padding: 10px; border-radius: 50%; width: 100px; height: 100px; border: 2px solid #444; }
    .t-val { font-weight: bold; color: #fff; font-size: 0.9em; background: #000; padding: 2px 5px; border-radius: 3px; }
    
    /* Contenedor del Joystick */
    #joy-container { margin: 15px auto; width: 160px; height: 160px; position: relative; }
    canvas { background: transparent; display: block; touch-action: none; }
  </style>
</head>
<body>
  <h2>Centro de Mando V7</h2>
  
  <div class="panel">
    <label>Acelerador General</label><br>
    <input type="range" min="0" max="255" value="0" id="s_potencia" oninput="actPotencia(this.value)" onchange="enviarPotencia(this.value)">
    <div class="val-display" id="v_potencia">0</div>
    <div>
      <button class="btn-land" onclick="aterrizar()">⬇️ ATERRIZAJE</button>
      <button class="btn-stop" onclick="paroEmergencia()">🛑 APAGAR</button>
    </div>
  </div>

  <div class="panel">
    <h3>Joystick de Dirección</h3>
    <div id="joy-container">
      <canvas id="joystick" width="160" height="160"></canvas>
    </div>
  </div>

  <div class="panel">
    <h3>Telemetría y Ajuste Fino</h3>
    <div class="dron-grid">
      <div class="motor-box">
        <span class="m-title">Delantero IZQ (M0)</span><span class="pwm-val" id="pwm0">0</span>
        <input type="range" class="trim-slider" id="t0" min="-50" max="50" value="35" onchange="enviarTrims()" oninput="document.getElementById('lt0').innerText=this.value">
        <span class="trim-label">Trim: <span id="lt0">35</span></span>
      </div>
      <div class="motor-box">
        <span class="m-title">Delantero DER (M1)</span><span class="pwm-val" id="pwm1">0</span>
        <input type="range" class="trim-slider" id="t1" min="-50" max="50" value="-30" onchange="enviarTrims()" oninput="document.getElementById('lt1').innerText=this.value">
        <span class="trim-label">Trim: <span id="lt1">-30</span></span>
      </div>
    </div>

    <div class="telemetria-giro">
      <div style="grid-column:2; grid-row:1;"><span class="t-val" id="y_up">0</span></div>
      <div style="grid-column:1; grid-row:2;"><span class="t-val" id="x_left">0</span></div>
      <div style="grid-column:2; grid-row:2; color:#4CAF50; font-size:1.5em;">✛</div>
      <div style="grid-column:3; grid-row:2;"><span class="t-val" id="x_right">0</span></div>
      <div style="grid-column:2; grid-row:3;"><span class="t-val" id="y_down">0</span></div>
    </div>

    <div class="dron-grid" style="margin-top:0;">
      <div class="motor-box">
        <span class="m-title">Trasero IZQ (M2)</span><span class="pwm-val" id="pwm2">0</span>
        <input type="range" class="trim-slider" id="t2" min="-50" max="50" value="-50" onchange="enviarTrims()" oninput="document.getElementById('lt2').innerText=this.value">
        <span class="trim-label">Trim: <span id="lt2">-50</span></span>
      </div>
      <div class="motor-box">
        <span class="m-title">Trasero DER (M3)</span><span class="pwm-val" id="pwm3">0</span>
        <input type="range" class="trim-slider" id="t3" min="-50" max="50" value="-50" onchange="enviarTrims()" oninput="document.getElementById('lt3').innerText=this.value">
        <span class="trim-label">Trim: <span id="lt3">-50</span></span>
      </div>
    </div>
  </div>

  <script>
    function actPotencia(val) { document.getElementById('v_potencia').innerText = val; }
    function enviarPotencia(val) { fetch(`/potencia?val=${val}`); }
    function aterrizar() { fetch('/aterrizar'); }
    function paroEmergencia() {
      document.getElementById('s_potencia').value = 0; actPotencia(0); fetch('/paro');
    }
    function enviarTrims() {
      let t0 = document.getElementById('t0').value; let t1 = document.getElementById('t1').value;
      let t2 = document.getElementById('t2').value; let t3 = document.getElementById('t3').value;
      fetch(`/trims?m0=${t0}&m1=${t1}&m2=${t2}&m3=${t3}`);
    }

    // --- LÓGICA DEL JOYSTICK VIRTUAL ---
    const canvas = document.getElementById('joystick');
    const ctx = canvas.getContext('2d');
    const centerX = canvas.width / 2; const centerY = canvas.height / 2;
    const radius = 60; 
    let isActive = false; let jX = 0; let jY = 0;

    function drawJoy(x, y) {
      ctx.clearRect(0, 0, canvas.width, canvas.height);
      // Base
      ctx.beginPath(); ctx.arc(centerX, centerY, radius, 0, 2*Math.PI); 
      ctx.fillStyle = '#333'; ctx.fill(); ctx.lineWidth = 2; ctx.strokeStyle = '#555'; ctx.stroke();
      // Palanca
      ctx.beginPath(); ctx.arc(x, y, 25, 0, 2*Math.PI); 
      ctx.fillStyle = '#00bcd4'; ctx.fill();
    }
    drawJoy(centerX, centerY);

    function handleInput(e) {
      let rect = canvas.getBoundingClientRect();
      let x = (e.touches ? e.touches[0].clientX : e.clientX) - rect.left;
      let y = (e.touches ? e.touches[0].clientY : e.clientY) - rect.top;
      
      // Limitar al radio
      let dx = x - centerX; let dy = y - centerY;
      let distance = Math.sqrt(dx*dx + dy*dy);
      if (distance > radius) {
        x = centerX + (dx * radius / distance);
        y = centerY + (dy * radius / distance);
      }
      drawJoy(x, y);
      
      // Mapear a valores de -50 a 50
      jX = Math.round(((x - centerX) / radius) * 50);
      jY = Math.round(((centerY - y) / radius) * 50); // Invertido para que arriba sea positivo
    }

    function sendJoy() { fetch(`/joy?x=${jX}&y=${jY}`); }

    // Eventos Mouse
    canvas.addEventListener('mousedown', (e) => { isActive = true; handleInput(e); sendJoy(); });
    canvas.addEventListener('mousemove', (e) => { if (isActive) { handleInput(e); } });
    canvas.addEventListener('mouseup', () => { isActive = false; jX = 0; jY = 0; drawJoy(centerX, centerY); sendJoy(); });
    canvas.addEventListener('mouseleave', () => { if(isActive){ isActive = false; jX = 0; jY = 0; drawJoy(centerX, centerY); sendJoy(); } });
    
    // Eventos Touch (Celular)
    canvas.addEventListener('touchstart', (e) => { e.preventDefault(); isActive = true; handleInput(e); sendJoy(); }, {passive: false});
    canvas.addEventListener('touchmove', (e) => { e.preventDefault(); if (isActive) { handleInput(e); } }, {passive: false});
    canvas.addEventListener('touchend', (e) => { e.preventDefault(); isActive = false; jX = 0; jY = 0; drawJoy(centerX, centerY); sendJoy(); });

    // Bucle de envío para Joystick (solo si está activo)
    setInterval(() => { if (isActive) sendJoy(); }, 150);

    // --- TELEMETRÍA ---
    setInterval(() => {
      fetch('/telemetria')
        .then(response => response.text())
        .then(data => {
          let partes = data.split(',');
          let valX = parseInt(partes[0]); let valY = parseInt(partes[1]);
          document.getElementById('x_left').innerText = valX > 0 ? valX : "0";
          document.getElementById('x_right').innerText = valX < 0 ? Math.abs(valX) : "0";
          document.getElementById('y_down').innerText = valY > 0 ? valY : "0";
          document.getElementById('y_up').innerText = valY < 0 ? Math.abs(valY) : "0";
          document.getElementById('pwm0').innerText = partes[2];
          document.getElementById('pwm1').innerText = partes[3];
          document.getElementById('pwm2').innerText = partes[4];
          document.getElementById('pwm3').innerText = partes[5];
        }).catch(err => {});
    }, 150);
  </script>
</body>
</html>
)rawliteral";

// ==========================================
// ENDPOINTS DEL SERVIDOR WEB
// ==========================================
void manejarRaiz() { server.send(200, "text/html", paginaHTML); }
void manejarPotencia() { if (server.hasArg("val")) { velocidad_base = server.arg("val").toInt(); aterrizando = false; } server.send(200, "text/plain", "OK"); }

void manejarTrims() {
  if (server.hasArg("m0")) trim_M0 = server.arg("m0").toInt();
  if (server.hasArg("m1")) trim_M1 = server.arg("m1").toInt();
  if (server.hasArg("m2")) trim_M2 = server.arg("m2").toInt();
  if (server.hasArg("m3")) trim_M3 = server.arg("m3").toInt();
  server.send(200, "text/plain", "Trims Actualizados");
}

void manejarJoy() {
  if (server.hasArg("x")) joy_x = server.arg("x").toInt();
  if (server.hasArg("y")) joy_y = server.arg("y").toInt();
  server.send(200, "text/plain", "Joy OK");
}

void manejarAterrizaje() { aterrizando = true; server.send(200, "text/plain", "Aterrizando"); }
void manejarParo() { 
  velocidad_base = 0; aterrizando = false; joy_x = 0; joy_y = 0;
  analogWrite(PIN_M0, 0); analogWrite(PIN_M1, 0); 
  analogWrite(PIN_M2, 0); analogWrite(PIN_M3, 0); 
  server.send(200, "text/plain", "Detenido"); 
}

void manejarTelemetria() { 
  String datos = String((int)comp_X_filtrado) + "," + String((int)comp_Y_filtrado) + "," + 
                 String(v0) + "," + String(v1) + "," + String(v2) + "," + String(v3); 
  server.send(200, "text/plain", datos); 
}

// ==========================================
// CONFIGURACIÓN INICIAL
// ==========================================
void setup() {
  Serial.begin(115200);
  
  pinMode(PIN_M0, OUTPUT); pinMode(PIN_M1, OUTPUT);
  pinMode(PIN_M2, OUTPUT); pinMode(PIN_M3, OUTPUT);
  manejarParo();

  Wire.begin(SDA_PIN, SCL_PIN);
  
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B); Wire.write(0);    
  Wire.endTransmission(true);

  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x1A); Wire.write(0x04); 
  Wire.endTransmission(true);

  WiFi.softAP(ssid, password);
  
  server.on("/", manejarRaiz);
  server.on("/potencia", manejarPotencia);
  server.on("/trims", manejarTrims); 
  server.on("/joy", manejarJoy); // Ruta para recibir los comandos del Joystick
  server.on("/aterrizar", manejarAterrizaje);
  server.on("/paro", manejarParo);
  server.on("/telemetria", manejarTelemetria);
  server.begin();
}

// ==========================================
// CICLO PRINCIPAL DE VUELO
// ==========================================
void loop() {
  server.handleClient(); 

  if (aterrizando && velocidad_base > 0) {
    if (millis() - tiempo_ultimo_descenso > 50) { velocidad_base--; tiempo_ultimo_descenso = millis(); }
  } else if (aterrizando && velocidad_base <= 0) {
    aterrizando = false;
  }

  // Leer giroscopio
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, 4, true);

  int16_t AcX = Wire.read()<<8 | Wire.read();
  int16_t AcY = Wire.read()<<8 | Wire.read();

  // Filtro EMA
  comp_X_filtrado = ((AcX / 150.0) * alpha) + (comp_X_filtrado * (1.0 - alpha));
  comp_Y_filtrado = ((AcY / 150.0) * alpha) + (comp_Y_filtrado * (1.0 - alpha));

  int comp_X_final = (int)comp_X_filtrado;
  int comp_Y_final = (int)comp_Y_filtrado;

  // Mezclador Maestro (Velocidad + Giroscopio + Trims + Joystick)
  if (velocidad_base > 10) {
    // joy_y (Pitch): Positivo inclina hacia adelante (baja fuerza en delanteros, sube en traseros)
    // joy_x (Roll): Positivo inclina hacia la derecha (sube fuerza en izquierdos, baja en derechos)
    
    v0 = constrain(velocidad_base - comp_Y_final + comp_X_final + trim_M0 - joy_y + joy_x, 0, 255); 
    v1 = constrain(velocidad_base - comp_Y_final - comp_X_final + trim_M1 - joy_y - joy_x, 0, 255); 
    v2 = constrain(velocidad_base + comp_Y_final + comp_X_final + trim_M2 + joy_y + joy_x, 0, 255); 
    v3 = constrain(velocidad_base + comp_Y_final - comp_X_final + trim_M3 + joy_y - joy_x, 0, 255); 

    analogWrite(PIN_M0, v0);
    analogWrite(PIN_M1, v1);
    analogWrite(PIN_M2, v2);
    analogWrite(PIN_M3, v3);
  } else {
    v0 = 0; v1 = 0; v2 = 0; v3 = 0;
    analogWrite(PIN_M0, 0); analogWrite(PIN_M1, 0);
    analogWrite(PIN_M2, 0); analogWrite(PIN_M3, 0);
  }

  delay(2); 
}