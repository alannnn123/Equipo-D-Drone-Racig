// =======================================================
// PRUEBA 03: REACCIÓN DE MOTORES AL GIROSCOPIO
// =======================================================
#include <Wire.h>

const int PIN_M0 = 0; const int PIN_M1 = 1;
const int PIN_M2 = 3; const int PIN_M3 = 4;
const int SDA_PIN = 10; const int SCL_PIN = 7;
const int MPU_ADDR = 0x68;

int velocidad_base = 120; // Potencia base para la prueba
float alpha = 0.15;
float comp_X_filtrado = 0; float comp_Y_filtrado = 0;

void setup() {
  Serial.begin(115200);
  pinMode(PIN_M0, OUTPUT); pinMode(PIN_M1, OUTPUT);
  pinMode(PIN_M2, OUTPUT); pinMode(PIN_M3, OUTPUT);
  apagarMotores();

  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.beginTransmission(MPU_ADDR); Wire.write(0x6B); Wire.write(0); Wire.endTransmission(true);
  Wire.beginTransmission(MPU_ADDR); Wire.write(0x1A); Wire.write(0x04); Wire.endTransmission(true);

  delay(3000); 
}

void loop() {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, 4, true);

  int16_t AcX = Wire.read()<<8 | Wire.read();
  int16_t AcY = Wire.read()<<8 | Wire.read();

  comp_X_filtrado = ((AcX / 150.0) * alpha) + (comp_X_filtrado * (1.0 - alpha));
  comp_Y_filtrado = ((AcY / 150.0) * alpha) + (comp_Y_filtrado * (1.0 - alpha));

  int cX = (int)comp_X_filtrado;
  int cY = (int)comp_Y_filtrado;

  // Mezclador (Se restan/suman los ejes para compensar la caída)
  int v0 = velocidad_base - cY + cX; 
  int v1 = velocidad_base - cY - cX; 
  int v2 = velocidad_base + cY + cX; 
  int v3 = velocidad_base + cY - cX; 

  analogWrite(PIN_M0, constrain(v0, 0, 255));
  analogWrite(PIN_M1, constrain(v1, 0, 255));
  analogWrite(PIN_M2, constrain(v2, 0, 255));
  analogWrite(PIN_M3, constrain(v3, 0, 255));

  delay(5);
}

void apagarMotores() {
  analogWrite(PIN_M0, 0); analogWrite(PIN_M1, 0);
  analogWrite(PIN_M2, 0); analogWrite(PIN_M3, 0);
}