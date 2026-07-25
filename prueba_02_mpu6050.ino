// =======================================================
// PRUEBA 02: LECTURA Y FILTRADO DEL MPU6050 (Sin Motores)
// =======================================================
#include <Wire.h>

const int SDA_PIN = 10;
const int SCL_PIN = 7;
const int MPU_ADDR = 0x68;

float alpha = 0.15; 
float comp_X_filtrado = 0;
float comp_Y_filtrado = 0;

void setup() {
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN);
  
  // Despertar sensor
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B); 
  Wire.write(0);    
  Wire.endTransmission(true);

  // Filtro Hardware DLPF
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x1A); 
  Wire.write(0x04); 
  Wire.endTransmission(true);
}

void loop() {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, 4, true);

  int16_t AcX = Wire.read()<<8 | Wire.read();
  int16_t AcY = Wire.read()<<8 | Wire.read();

  // Filtro EMA Matemático
  comp_X_filtrado = ((AcX / 150.0) * alpha) + (comp_X_filtrado * (1.0 - alpha));
  comp_Y_filtrado = ((AcY / 150.0) * alpha) + (comp_Y_filtrado * (1.0 - alpha));

  // Imprimir para el Serial Plotter
  Serial.print("Pitch_X:");
  Serial.print(comp_X_filtrado);
  Serial.print(",");
  Serial.print("Roll_Y:");
  Serial.println(comp_Y_filtrado);

  delay(10);
}