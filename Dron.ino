/**
 * ============================================================================
 * FIRMWARE DE CONTROL DE VUELO (PID) - ESP32-C3 SUPERMINI
 * Proyecto: Micro-Dron (Arquitectura True-X)
 * ============================================================================
 * Este código lee los datos inerciales del MPU6050, calcula el ángulo real
 * usando un Filtro Complementario, procesa el lazo PID y ajusta la velocidad 
 * de los 4 motores mediante PWM.
 */

#include <Wire.h>

// ==========================================
// 1. ASIGNACIÓN DE PINES (ESP32-C3)
// ==========================================
#define SDA_PIN 8
#define SCL_PIN 9

// Pines PWM para los 4 motores (Módulos DRV8833)
#define MOTOR_DELANTERO_IZQ 0
#define MOTOR_DELANTERO_DER 1
#define MOTOR_TRASERO_IZQ   2
#define MOTOR_TRASERO_DER   3

// ==========================================
// 2. CONSTANTES Y VARIABLES DEL MPU6050
// ==========================================
const int MPU_ADDR = 0x68; // Dirección I2C estándar del MPU6050
int16_t AcX, AcY, AcZ, GyX, GyY, GyZ;

// Variables de ángulos y tiempo
float angulo_pitch_acc, angulo_roll_acc;
float angulo_pitch = 0; // Ángulo real calculado (Adelante/Atrás)
float angulo_roll = 0;  // Ángulo real calculado (Izquierda/Derecha)

unsigned long tiempo_previo, tiempo_actual;
float dt;

// ==========================================
// 3. MATRIZ DEL CONTROLADOR PID
// ==========================================
// Ganancias (Se deben afinar físicamente)
float Kp = 1.2;
float Ki = 0.04;
float Kd = 0.8;

// Objetivos de vuelo (0 grados = vuelo estabilizado)
float setpoint_pitch = 0;
float setpoint_roll = 0;

// Memoria PID para Pitch (Adelante/Atrás)
float error_pitch, error_prev_pitch, integral_pitch, derivativo_pitch, output_pitch;

// Memoria PID para Roll (Izquierda/Derecha)
float error_roll, error_prev_roll, integral_roll, derivativo_roll, output_roll;

// Acelerador base (0 a 255). Para pruebas en mano, usamos un valor bajo.
int throttle_base = 100; 

// ==========================================
// CONFIGURACIÓN INICIAL (SETUP)
// ==========================================
void setup() {
  Serial.begin(115200);
  
  // Configurar pines de motores
  pinMode(MOTOR_DELANTERO_IZQ, OUTPUT);
  pinMode(MOTOR_DELANTERO_DER, OUTPUT);
  pinMode(MOTOR_TRASERO_IZQ, OUTPUT);
  pinMode(MOTOR_TRASERO_DER, OUTPUT);

  // Asegurar que los motores estén apagados por seguridad
  apagarMotores();

  // Iniciar bus I2C
  Wire.begin(SDA_PIN, SCL_PIN);
  
  // Despertar al MPU6050 (Escribir 0 en el registro de energía 0x6B)
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B);
  Wire.write(0x00);
  Wire.endTransmission(true);

  Serial.println("Sistemas de Vuelo Iniciados. Calibrando...");
  delay(1000); // Dar tiempo a estabilizar sensores
  
  tiempo_previo = millis();
}

// ==========================================
// BUCLE PRINCIPAL DE VUELO (LOOP) A ~250Hz
// ==========================================
void loop() {
  // 1. CÁLCULO DE TIEMPO (dt)
  tiempo_actual = millis();
  dt = (tiempo_actual - tiempo_previo) / 1000.0;
  tiempo_previo = tiempo_actual;

  // 2. LECTURA CRUDA DEL MPU6050 (Registros 0x3B al 0x48)
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B); 
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, 14, true); 
  
  AcX = Wire.read()<<8 | Wire.read();
  AcY = Wire.read()<<8 | Wire.read();
  AcZ = Wire.read()<<8 | Wire.read();
  GyX = Wire.read()<<8 | Wire.read(); // Ignoramos Temperatura por ahora
  GyY = Wire.read()<<8 | Wire.read();
  GyZ = Wire.read()<<8 | Wire.read();

  // 3. PROCESAMIENTO DE SEÑALES (FILTRO COMPLEMENTARIO)
  // Convertir aceleraciones crudas a grados (Aprox)
  angulo_pitch_acc = atan2(AcY, AcZ) * 180 / PI;
  angulo_roll_acc  = atan2(-AcX, AcZ) * 180 / PI;

  // Tasa de giro del giroscopio (escala para +/- 250 deg/s)
  float tasa_giro_pitch = GyX / 131.0;
  float tasa_giro_roll  = GyY / 131.0;

  // Filtro Complementario (98% confianza al giroscopio, 2% al acelerómetro)
  angulo_pitch = 0.98 * (angulo_pitch + tasa_giro_pitch * dt) + 0.02 * angulo_pitch_acc;
  angulo_roll  = 0.98 * (angulo_roll  + tasa_giro_roll  * dt) + 0.02 * angulo_roll_acc;

  // 4. CÁLCULO DEL PID (PITCH)
  error_pitch = setpoint_pitch - angulo_pitch;
  integral_pitch += error_pitch * dt;
  derivativo_pitch = (error_pitch - error_prev_pitch) / dt;
  output_pitch = (Kp * error_pitch) + (Ki * integral_pitch) + (Kd * derivativo_pitch);
  error_prev_pitch = error_pitch;

  // 5. CÁLCULO DEL PID (ROLL)
  error_roll = setpoint_roll - angulo_roll;
  integral_roll += error_roll * dt;
  derivativo_roll = (error_roll - error_prev_roll) / dt;
  output_roll = (Kp * error_roll) + (Ki * integral_roll) + (Kd * derivativo_roll);
  error_prev_roll = error_roll;

  // 6. MEZCLADOR DE MOTORES (MIXER TRUE-X)
  // Combina la potencia base con las correcciones del PID
  int vel_del_izq = throttle_base + output_pitch + output_roll;
  int vel_del_der = throttle_base + output_pitch - output_roll;
  int vel_tra_izq = throttle_base - output_pitch + output_roll;
  int vel_tra_der = throttle_base - output_pitch - output_roll;

  // Limitar velocidades para no exceder los límites del PWM (0 a 255)
  vel_del_izq = constrain(vel_del_izq, 0, 255);
  vel_del_der = constrain(vel_del_der, 0, 255);
  vel_tra_izq = constrain(vel_tra_izq, 0, 255);
  vel_tra_der = constrain(vel_tra_der, 0, 255);

  // 7. ENVIAR SEÑAL A LOS MOTORES
  analogWrite(MOTOR_DELANTERO_IZQ, vel_del_izq);
  analogWrite(MOTOR_DELANTERO_DER, vel_del_der);
  analogWrite(MOTOR_TRASERO_IZQ, vel_tra_izq);
  analogWrite(MOTOR_TRASERO_DER, vel_tra_der);

  // Monitor Serial para depuración (Descomentar para ver en pantalla)
  // Serial.print("Pitch: "); Serial.print(angulo_pitch);
  // Serial.print(" | Roll: "); Serial.println(angulo_roll);

  // Mantener un ciclo estable
  delay(4); 
}

// Función de seguridad
void apagarMotores() {
  analogWrite(MOTOR_DELANTERO_IZQ, 0);
  analogWrite(MOTOR_DELANTERO_DER, 0);
  analogWrite(MOTOR_TRASERO_IZQ, 0);
  analogWrite(MOTOR_TRASERO_DER, 0);
}