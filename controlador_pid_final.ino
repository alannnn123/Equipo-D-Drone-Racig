// =======================================================
// v8_controlador_pid_final.ino
// DRON V8: CONTROLADOR PID + FILTRO COMPLEMENTARIO
// =======================================================
#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>

const int PIN_M0 = 0; // Delantero Izquierdo
const int PIN_M1 = 1; // Delantero Derecho
const int PIN_M2 = 3; // Trasero Izquierdo
const int PIN_M3 = 4; // Trasero Derecho

const int SDA_PIN = 10;
const int SCL_PIN = 7;
const int MPU_ADDR = 0x68;

const char* ssid = "Dron_Estable";
const char* password = "password123";
WebServer server(80);

// ==========================================
// GLOBALES Y AFINACIÓN PID (Tuning)
// ==========================================
int velocidad_base = 0;
bool aterrizando = false;

// Trims ampliados (-150 a 255) calibrados estructuralmente
int trim_M0 = 35;  
int trim_M1 = -30; 
int trim_M2 = -50; 
int trim_M3 = -50; 

int joy_x = 0; 
int joy_y = 0;
int v0 = 0, v1 = 0, v2 = 0, v3 = 0; 

// --- VARIABLES PID ---
// Kp = Fuerza de reacción inmediata
// Ki = Corrección de motores débiles a lo largo del tiempo
// Kd = Freno predictivo para evitar vibraciones
float Kp = 1.2;  
float Ki = 0.05; 
float Kd = 0.8;  

float pid_p_x = 0, pid_i_x = 0, pid_d_x = 0, pid_total_x = 0;
float pid_p_y = 0, pid_i_y = 0, pid_d_y = 0, pid_total_y = 0;
float error_x = 0, error_anterior_x = 0;
float error_y = 0, error_anterior_y = 0;

// Variables de sensores y cálculo de tiempo (dt)
unsigned long tiempo_anterior, tiempo_actual;
float angulo_x_pitch = 0, angulo_y_roll = 0;

// ==========================================
// PÁGINA WEB (HTML/CSS/JS)
// ==========================================
const char* paginaHTML = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no">
  <title>Estación PID V8</title>
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
    #joy-container { margin: 15px auto; width: 160px; height: 160px; position: relative; }
    canvas { background: transparent; display: block; touch-action: none; }
  </style>
</head>
<body>
  <h2>Centro de Mando PID V8</h2>
  
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
    <div id="joy-container"><canvas id="joystick" width="160" height="160"></canvas></div>
  </div>

  <div class="panel">
    <h3>Telemetría y Trims Extendidos</h3>
    <div class="dron-grid">
      <div class="motor-box">
        <span class="m-title">Delantero IZQ (M0)</span><span class="pwm-val" id="pwm0">0</span>
        <!-- RANGO EXTENDIDO A -150 Y 255 -->
        <input type="range" class="trim-slider" id="t0" min="-150" max="255" value="35" onchange="enviarTrims()" oninput="document.getElementById('lt0').innerText=this.value">
        <span class="trim-label">Trim: <span id="lt0">35</span></span>
      </div>
      <div class="motor-box">
        <span class="m-title">Delantero DER (M1)</span><span class="pwm-val" id="pwm1">0</span>
        <input type="range" class="trim-slider" id="t1" min="-150" max="255" value="-30" onchange="enviarTrims()" oninput="document.getElementById('lt1').innerText=this.value">
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
        <input type="range" class="trim-slider" id="t2" min="-150" max="255" value="-50" onchange="enviarTrims()" oninput="document.getElementById('lt2').innerText=this.value">
        <span class="trim-label">Trim: <span id="lt2">-50</span></span>
      </div>
      <div class="motor-box">
        <span class="m-title">Trasero DER (M3)</span><span class="pwm-val" id="pwm3">0</span>
        <input type="range" class="trim-slider" id="t3" min="-150" max="255" value="-50" onchange="enviarTrims()" oninput="document.getElementById('lt3').innerText=this.value">
        <span class="trim-label">Trim: <span id="lt3">-50</span></span>
      </div>
    </div>
  </div>

  <script>
    function actPotencia(val) { document.getElementById('v_potencia').innerText = val; }
    function enviarPotencia(val) { fetch(`/potencia?val=${val}`); }
    function aterrizar() { fetch('/aterrizar'); }
    function paroEmergencia() { document.getElementById('s_potencia').value = 0; actPotencia(0); fetch('/paro'); }
    
    function enviarTrims() {
      let t0 = document.getElementById('t0').value; let t1 = document.getElementById('t1').value;
      let t2 = document.getElementById('t2').value; let t3 = document.getElementById('t3').value;
      fetch(`/trims?m0=${t0}&m1=${t1}&m2=${t2}&m3=${t3}`);
    }

    // Lógica de Canvas para el Joystick Virtual
    const canvas = document.getElementById('joystick');
    const ctx = canvas.getContext('2d');
    const centerX = canvas.width / 2; const centerY = canvas.height / 2;
    const radius = 60; let isActive = false; let jX = 0; let jY = 0;

    function drawJoy(x, y) {
      ctx.clearRect(0, 0, canvas.width, canvas.height);
      ctx.beginPath(); ctx.arc(centerX, centerY, radius, 0, 2*Math.PI); 
      ctx.fillStyle = '#333'; ctx.fill(); ctx.lineWidth = 2; ctx.strokeStyle = '#555'; ctx.stroke();
      ctx.beginPath(); ctx.arc(x, y, 25, 0, 2*Math.PI); ctx.fillStyle = '#00bcd4'; ctx.fill();
    }
    drawJoy(centerX, centerY);

    function handleInput(e) {
      let rect = canvas.getBoundingClientRect();
      let x = (e.touches ? e.touches[0].clientX : e.clientX) - rect.left;
      let y = (e.touches ? e.touches[0].clientY : e.clientY) - rect.top;
      let dx = x - centerX; let dy = y - centerY;
      let distance = Math.sqrt(dx*dx + dy*dy);
      if (distance > radius) { x = centerX + (dx * radius / distance); y = centerY + (dy * radius / distance); }
      drawJoy(x, y);
      jX = Math.round(((x - centerX) / radius) * 20); // El joystick pide +-20 grados
      jY = Math.round(((centerY - y) / radius) * 20); 
    }

    function sendJoy() { fetch(`/joy?x=${jX}&y=${jY}`); }
    canvas.addEventListener('mousedown', (e) => { isActive = true; handleInput(e); sendJoy(); });
    canvas.addEventListener('mousemove', (e) => { if (isActive) { handleInput(e); } });
    canvas.addEventListener('mouseup', () => { isActive = false; jX = 0; jY = 0; drawJoy(centerX, centerY); sendJoy(); });
    canvas.addEventListener('mouseleave', () => { if(isActive){ isActive = false; jX = 0; jY = 0; drawJoy(centerX, centerY); sendJoy(); } });
    canvas.addEventListener('touchstart', (e) => { e.preventDefault(); isActive = true; handleInput(e); sendJoy(); }, {passive: false});
    canvas.addEventListener('touchmove', (e) => { e.preventDefault(); if (isActive) { handleInput(e); } }, {passive: false});
    canvas.addEventListener('touchend', (e) => { e.preventDefault(); isActive = false; jX = 0; jY = 0; drawJoy(centerX, centerY); sendJoy(); });
    setInterval(() => { if (isActive) sendJoy(); }, 150);

    // Bucle asíncrono para Telemetría
    setInterval(() => {
      fetch('/telemetria')
        .then(response => response.text())
        .then(data => {
          let partes = data.split(',');
          document.getElementById('x_left').innerText = partes[0] > 0 ? partes[0] : "0";
          document.getElementById('x_right').innerText = partes[0] < 0 ? Math.abs(partes[0]) : "0";
          document.getElementById('y_down').innerText = partes[1] > 0 ? partes[1] : "0";
          document.getElementById('y_up').innerText = partes[1] < 0 ? Math.abs(partes[1]) : "0";
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
// FUNCIONES WEB (Endpoints)
// ==========================================
void manejarRaiz() { server.send(200, "text/html", paginaHTML); }
void manejarPotencia() { if (server.hasArg("val")) { velocidad_base = server.arg("val").toInt(); aterrizando = false; } server.send(200, "text/plain", "OK"); }
void manejarTrims() {
  if (server.hasArg("m0")) trim_M0 = server.arg("m0").toInt();
  if (server.hasArg("m1")) trim_M1 = server.arg("m1").toInt();
  if (server.hasArg("m2")) trim_M2 = server.arg("m2").toInt();
  if (server.hasArg("m3")) trim_M3 = server.arg("m3").toInt();
  server.send(200, "text/plain", "Trims OK");
}
void manejarJoy() {
  if (server.hasArg("x")) joy_x = server.arg("x").toInt();
  if (server.hasArg("y")) joy_y = server.arg("y").toInt();
  server.send(200, "text/plain", "Joy OK");
}
void manejarAterrizaje() { aterrizando = true; server.send(200, "text/plain", "OK"); }
void manejarParo() { velocidad_base = 0; aterrizando = false; analogWrite(PIN_M0,0); analogWrite(PIN_M1,0); analogWrite(PIN_M2,0); analogWrite(PIN_M3,0); server.send(200, "text/plain", "OK"); }
void manejarTelemetria() { 
  String datos = String((int)angulo_x_pitch) + "," + String((int)angulo_y_roll) + "," + String(v0) + "," + String(v1) + "," + String(v2) + "," + String(v3); 
  server.send(200, "text/plain", datos); 
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_M0, OUTPUT); pinMode(PIN_M1, OUTPUT); pinMode(PIN_M2, OUTPUT); pinMode(PIN_M3, OUTPUT);
  manejarParo();

  // Iniciar Bus I2C en pines limpios
  Wire.begin(SDA_PIN, SCL_PIN);
  
  // Despertar MPU6050
  Wire.beginTransmission(MPU_ADDR); Wire.write(0x6B); Wire.write(0); Wire.endTransmission(true);
  
  // Activar Filtro Digital Low-Pass (DLPF) en el Hardware para mitigar vibraciones de los motores
  Wire.beginTransmission(MPU_ADDR); Wire.write(0x1A); Wire.write(0x04); Wire.endTransmission(true);

  WiFi.softAP(ssid, password);
  server.on("/", manejarRaiz); server.on("/potencia", manejarPotencia); server.on("/trims", manejarTrims); 
  server.on("/joy", manejarJoy); server.on("/aterrizar", manejarAterrizaje); server.on("/paro", manejarParo); server.on("/telemetria", manejarTelemetria);
  server.begin();
  
  // Guardar marca de tiempo inicial para cálculos de dt
  tiempo_anterior = micros();
}

void loop() {
  server.handleClient(); 
  
  // Calcular diferencial de tiempo (dt)
  tiempo_actual = micros();
  float dt = (tiempo_actual - tiempo_anterior) / 1000000.0; // Segundos
  tiempo_anterior = tiempo_actual;

  // LECTURA DE ACELERÓMETRO Y GIROSCOPIO (14 Bytes)
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, 14, true);

  int16_t AcX = Wire.read()<<8 | Wire.read();
  int16_t AcY = Wire.read()<<8 | Wire.read();
  int16_t AcZ = Wire.read()<<8 | Wire.read();
  Wire.read()<<8 | Wire.read(); // Descartar temperatura
  int16_t GyX = Wire.read()<<8 | Wire.read();
  int16_t GyY = Wire.read()<<8 | Wire.read();
  int16_t GyZ = Wire.read()<<8 | Wire.read(); // Descartar Yaw (no tenemos brújula)

  // Convertir a grados y ratio de giro
  float accel_angulo_x = atan2(AcY, AcZ) * 180 / PI; // Pitch
  float accel_angulo_y = atan2(-AcX, AcZ) * 180 / PI; // Roll
  float gyro_rate_x = GyX / 131.0; 
  float gyro_rate_y = GyY / 131.0;

  // ==========================================
  // FILTRO COMPLEMENTARIO (FUSIÓN DE SENSORES)
  // ==========================================
  // Combina la rapidez del giroscopio con la referencia absoluta del acelerómetro
  angulo_x_pitch = 0.98 * (angulo_x_pitch + gyro_rate_x * dt) + 0.02 * accel_angulo_x;
  angulo_y_roll = 0.98 * (angulo_y_roll + gyro_rate_y * dt) + 0.02 * accel_angulo_y;

  // ==========================================
  // LAZO DE CONTROL PID
  // ==========================================
  if (velocidad_base > 10) {
    // 1. Calcular Error (SetPoint [Joystick] vs Actual [Sensor])
    error_x = joy_y - angulo_x_pitch; // Pitch 
    error_y = joy_x - angulo_y_roll;  // Roll 

    // 2. Control Proporcional (Reacción inmediata)
    pid_p_x = Kp * error_x;
    pid_p_y = Kp * error_y;

    // 3. Control Integral (Corrección de motores débiles en el tiempo)
    pid_i_x += Ki * error_x;
    pid_i_y += Ki * error_y;
    // Anti-windup (Límite de acumulación integral)
    pid_i_x = constrain(pid_i_x, -50, 50);
    pid_i_y = constrain(pid_i_y, -50, 50);

    // 4. Control Derivativo (Freno predictivo amortiguador)
    pid_d_x = Kd * ((error_x - error_anterior_x) / dt);
    pid_d_y = Kd * ((error_y - error_anterior_y) / dt);

    // Guardar error para la próxima derivada
    error_anterior_x = error_x;
    error_anterior_y = error_y;

    // Suma final PID
    pid_total_x = pid_p_x + pid_i_x + pid_d_x;
    pid_total_y = pid_p_y + pid_i_y + pid_d_y;

    // ==========================================
    // MEZCLADOR MATEMÁTICO A PWM
    // ==========================================
    v0 = constrain(velocidad_base + pid_total_x + pid_total_y + trim_M0, 0, 255); 
    v1 = constrain(velocidad_base + pid_total_x - pid_total_y + trim_M1, 0, 255); 
    v2 = constrain(velocidad_base - pid_total_x + pid_total_y + trim_M2, 0, 255); 
    v3 = constrain(velocidad_base - pid_total_x - pid_total_y + trim_M3, 0, 255); 

    analogWrite(PIN_M0, v0); analogWrite(PIN_M1, v1);
    analogWrite(PIN_M2, v2); analogWrite(PIN_M3, v3);
  } else {
    // Apagado de seguridad y limpieza de memoria integral
    pid_i_x = 0; pid_i_y = 0; error_anterior_x = 0; error_anterior_y = 0;
    v0 = 0; v1 = 0; v2 = 0; v3 = 0;
    analogWrite(PIN_M0, 0); analogWrite(PIN_M1, 0); analogWrite(PIN_M2, 0); analogWrite(PIN_M3, 0);
  }
}