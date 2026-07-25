// =======================================================
// DRON V5: LIVE TUNING, PWM TELEMETRY & FAST GYRO
// =======================================================
#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>

// Pines de Motores
const int PIN_M0 = 0; // Delantero Izquierdo
const int PIN_M1 = 1; // Delantero Derecho
const int PIN_M2 = 3; // Trasero Izquierdo
const int PIN_M3 = 4; // Trasero Derecho

// Pines I2C (Seguros)
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

// Trims (Ajustables en vivo desde la web)
int trim_M0 = 0;
int trim_M1 = 0;
int trim_M2 = 0;
int trim_M3 = 0;

// Variables de Motores y Filtros (Globales para que la web las lea)
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
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Estación de Control V5</title>
  <style>
    body { font-family: Arial, sans-serif; text-align: center; background-color: #1a1a1a; color: white; margin: 0; padding: 10px; }
    .panel { background: #2d2d2d; margin: 10px auto; padding: 15px; border-radius: 10px; width: 95%; max-width: 500px; box-shadow: 0 4px 8px rgba(0,0,0,0.5); }
    input[type=range] { width: 100%; height: 35px; cursor: pointer; }
    .val-display { font-size: 2em; font-weight: bold; color: #4CAF50; margin: 5px 0; }
    button { border: none; padding: 12px 15px; font-size: 1.1em; border-radius: 5px; cursor: pointer; margin: 5px; font-weight: bold; width: 45%; }
    .btn-land { background-color: #ff9800; color: white; }
    .btn-stop { background-color: #f44336; color: white; }
    
    /* Diseño del Dron (Motores) */
    .dron-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 10px; margin-top: 15px; }
    .motor-box { background: #3d3d3d; padding: 10px; border-radius: 8px; border: 1px solid #555; }
    .pwm-val { font-size: 1.8em; color: #ffeb3b; font-weight: bold; display: block; margin: 5px 0; }
    .m-title { font-size: 0.9em; color: #aaa; }
    .trim-slider { width: 90%; height: 25px !important; }
    .trim-label { font-size: 0.8em; color: #00bcd4; font-weight: bold; }

    /* Diseño del Giroscopio Central */
    .telemetria-giro { display: grid; grid-template-columns: 1fr 1fr 1fr; grid-template-rows: 1fr 1fr 1fr; gap: 2px; justify-items: center; align-items: center; margin: 15px auto; background: #222; padding: 10px; border-radius: 50%; width: 120px; height: 120px; border: 2px solid #444; }
    .t-val { font-weight: bold; color: #fff; font-size: 1em; background: #000; padding: 2px 5px; border-radius: 3px; }
  </style>
</head>
<body>
  <h2>Centro de Mando V5</h2>
  
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
    <h3>Telemetría de Motores y Giroscopio</h3>
    
    <div class="dron-grid">
      <!-- Motor Delantero Izquierdo -->
      <div class="motor-box">
        <span class="m-title">Delantero IZQ (M0)</span>
        <span class="pwm-val" id="pwm0">0</span>
        <input type="range" class="trim-slider" id="t0" min="-50" max="50" value="0" onchange="enviarTrims()" oninput="document.getElementById('lt0').innerText=this.value">
        <span class="trim-label">Trim: <span id="lt0">0</span></span>
      </div>
      
      <!-- Motor Delantero Derecho -->
      <div class="motor-box">
        <span class="m-title">Delantero DER (M1)</span>
        <span class="pwm-val" id="pwm1">0</span>
        <input type="range" class="trim-slider" id="t1" min="-50" max="50" value="0" onchange="enviarTrims()" oninput="document.getElementById('lt1').innerText=this.value">
        <span class="trim-label">Trim: <span id="lt1">0</span></span>
      </div>
    </div>

    <!-- Cruz del Giroscopio -->
    <div class="telemetria-giro">
      <div style="grid-column:2; grid-row:1;"><span class="t-val" id="y_up">0</span></div>
      <div style="grid-column:1; grid-row:2;"><span class="t-val" id="x_left">0</span></div>
      <div style="grid-column:2; grid-row:2; color:#4CAF50; font-size:1.5em;">✛</div>
      <div style="grid-column:3; grid-row:2;"><span class="t-val" id="x_right">0</span></div>
      <div style="grid-column:2; grid-row:3;"><span class="t-val" id="y_down">0</span></div>
    </div>

    <div class="dron-grid" style="margin-top:0;">
      <!-- Motor Trasero Izquierdo -->
      <div class="motor-box">
        <span class="m-title">Trasero IZQ (M2)</span>
        <span class="pwm-val" id="pwm2">0</span>
        <input type="range" class="trim-slider" id="t2" min="-50" max="50" value="0" onchange="enviarTrims()" oninput="document.getElementById('lt2').innerText=this.value">
        <span class="trim-label">Trim: <span id="lt2">0</span></span>
      </div>

      <!-- Motor Trasero Derecho -->
      <div class="motor-box">
        <span class="m-title">Trasero DER (M3)</span>
        <span class="pwm-val" id="pwm3">0</span>
        <input type="range" class="trim-slider" id="t3" min="-50" max="50" value="0" onchange="enviarTrims()" oninput="document.getElementById('lt3').innerText=this.value">
        <span class="trim-label">Trim: <span id="lt3">0</span></span>
      </div>
    </div>
  </div>

  <script>
    function actPotencia(val) { document.getElementById('v_potencia').innerText = val; }
    function enviarPotencia(val) { fetch(`/potencia?val=${val}`); }
    function aterrizar() { fetch('/aterrizar'); }
    function paroEmergencia() {
      document.getElementById('s_potencia').value = 0;
      actPotencia(0);
      fetch('/paro');
    }

    // Función para enviar los ajustes de los sliders de TRIM al ESP32
    function enviarTrims() {
      let t0 = document.getElementById('t0').value;
      let t1 = document.getElementById('t1').value;
      let t2 = document.getElementById('t2').value;
      let t3 = document.getElementById('t3').value;
      fetch(`/trims?m0=${t0}&m1=${t1}&m2=${t2}&m3=${t3}`);
    }

    // Actualización super rápida (10 veces por segundo - 100ms)
    setInterval(() => {
      fetch('/telemetria')
        .then(response => response.text())
        .then(data => {
          // El formato que envía el ESP es: "GiroX,GiroY,PWM0,PWM1,PWM2,PWM3"
          let partes = data.split(',');
          let valX = parseInt(partes[0]);
          let valY = parseInt(partes[1]);
          
          // Actualizar Giroscopio
          document.getElementById('x_left').innerText = valX > 0 ? valX : "0";
          document.getElementById('x_right').innerText = valX < 0 ? Math.abs(valX) : "0";
          document.getElementById('y_down').innerText = valY > 0 ? valY : "0";
          document.getElementById('y_up').innerText = valY < 0 ? Math.abs(valY) : "0";

          // Actualizar Fuerza Real de Motores (PWM)
          document.getElementById('pwm0').innerText = partes[2];
          document.getElementById('pwm1').innerText = partes[3];
          document.getElementById('pwm2').innerText = partes[4];
          document.getElementById('pwm3').innerText = partes[5];
        })
        .catch(err => {});
    }, 100);
  </script>
</body>
</html>
)rawliteral";

// ==========================================
// ENDPOINTS DEL SERVIDOR WEB
// ==========================================
void manejarRaiz() { server.send(200, "text/html", paginaHTML); }

void manejarPotencia() { 
  if (server.hasArg("val")) { velocidad_base = server.arg("val").toInt(); aterrizando = false; } 
  server.send(200, "text/plain", "OK"); 
}

void manejarTrims() {
  if (server.hasArg("m0")) trim_M0 = server.arg("m0").toInt();
  if (server.hasArg("m1")) trim_M1 = server.arg("m1").toInt();
  if (server.hasArg("m2")) trim_M2 = server.arg("m2").toInt();
  if (server.hasArg("m3")) trim_M3 = server.arg("m3").toInt();
  server.send(200, "text/plain", "Trims Actualizados");
}

void manejarAterrizaje() { aterrizando = true; server.send(200, "text/plain", "Aterrizando"); }

void manejarParo() { 
  velocidad_base = 0; aterrizando = false; 
  analogWrite(PIN_M0, 0); analogWrite(PIN_M1, 0); 
  analogWrite(PIN_M2, 0); analogWrite(PIN_M3, 0); 
  server.send(200, "text/plain", "Detenido"); 
}

void manejarTelemetria() { 
  // Empaquetamos giroscopio y estado de los 4 motores en un solo mensaje
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
  Wire.write(0x1A); Wire.write(0x04); // Filtro DLPF
  Wire.endTransmission(true);

  WiFi.softAP(ssid, password);
  
  server.on("/", manejarRaiz);
  server.on("/potencia", manejarPotencia);
  server.on("/trims", manejarTrims); // Nueva ruta para escuchar ajustes
  server.on("/aterrizar", manejarAterrizaje);
  server.on("/paro", manejarParo);
  server.on("/telemetria", manejarTelemetria);
  server.begin();
}

// ==========================================
// CICLO PRINCIPAL DE VUELO
// ==========================================
void loop() {
  server.handleClient(); // Atender página web

  if (aterrizando && velocidad_base > 0) {
    if (millis() - tiempo_ultimo_descenso > 50) { velocidad_base--; tiempo_ultimo_descenso = millis(); }
  } else if (aterrizando && velocidad_base <= 0) {
    aterrizando = false;
  }

  // Leer y filtrar giroscopio
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, 4, true);

  int16_t AcX = Wire.read()<<8 | Wire.read();
  int16_t AcY = Wire.read()<<8 | Wire.read();

  comp_X_filtrado = ((AcX / 150.0) * alpha) + (comp_X_filtrado * (1.0 - alpha));
  comp_Y_filtrado = ((AcY / 150.0) * alpha) + (comp_Y_filtrado * (1.0 - alpha));

  int comp_X_final = (int)comp_X_filtrado;
  int comp_Y_final = (int)comp_Y_filtrado;

  // Cortacorriente anti-volcaduras (Mata los motores si se pone de cabeza)
  if (abs(AcX) > 12000 || abs(AcY) > 12000) { velocidad_base = 0; aterrizando = false; }

  // Mezclador (Aplicando Base + Giroscopio + Trim Dinámico)
  if (velocidad_base > 10) {
    v0 = constrain(velocidad_base - comp_Y_final + comp_X_final + trim_M0, 0, 255); 
    v1 = constrain(velocidad_base - comp_Y_final - comp_X_final + trim_M1, 0, 255); 
    v2 = constrain(velocidad_base + comp_Y_final + comp_X_final + trim_M2, 0, 255); 
    v3 = constrain(velocidad_base + comp_Y_final - comp_X_final + trim_M3, 0, 255); 

    analogWrite(PIN_M0, v0);
    analogWrite(PIN_M1, v1);
    analogWrite(PIN_M2, v2);
    analogWrite(PIN_M3, v3);
  } else {
    v0 = 0; v1 = 0; v2 = 0; v3 = 0; // Variables a 0 para que la web lo refleje
    analogWrite(PIN_M0, 0); analogWrite(PIN_M1, 0);
    analogWrite(PIN_M2, 0); analogWrite(PIN_M3, 0);
  }

  delay(2); 
}