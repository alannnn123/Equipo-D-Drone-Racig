// =======================================================
// PRUEBA 04: MANTENIMIENTO DE ALTURA (LÁSER ToF + MOTORES)
// =======================================================
#include <Wire.h>
#include <Adafruit_VL53L0X.h> // Librería estándar para sensor láser I2C

Adafruit_VL53L0X sensor_altura = Adafruit_VL53L0X();

const int PIN_M0 = 0; const int PIN_M1 = 1;
const int PIN_M2 = 3; const int PIN_M3 = 4;
const int SDA_PIN = 10; const int SCL_PIN = 7;

// Configuración de Altura
const int ALTURA_OBJETIVO_MM = 1000; // Queremos que flote a 1 metro (1000 mm)
int potencia_base = 120; // Potencia de flotación aproximada
float Kp_altura = 0.5;   // Ganancia proporcional de altura

void setup() {
  Serial.begin(115200);
  pinMode(PIN_M0, OUTPUT); pinMode(PIN_M1, OUTPUT);
  pinMode(PIN_M2, OUTPUT); pinMode(PIN_M3, OUTPUT);
  apagarMotores();

  Wire.begin(SDA_PIN, SCL_PIN);
  
  if (!sensor_altura.begin()) {
    Serial.println("Fallo al iniciar el sensor VL53L0X");
    while(1); // Detener el código si no hay sensor
  }
  Serial.println("Sensor Láser Listo. Iniciando motores...");
  delay(3000);
}

void loop() {
  VL53L0X_RangingMeasurementData_t medida;
  sensor_altura.rangingTest(&medida, false);

  if (medida.RangeStatus != 4) {  // Si la lectura es válida
    int altura_actual = medida.RangeMilliMeter;
    
    // Calcular el error de altura
    int error_altura = ALTURA_OBJETIVO_MM - altura_actual;
    
    // Ajustar la potencia (Si estamos muy abajo, el error es positivo -> acelera)
    int compensacion = error_altura * Kp_altura;
    int potencia_final = constrain(potencia_base + compensacion, 0, 255);

    // Enviar potencia a todos los motores
    analogWrite(PIN_M0, potencia_final);
    analogWrite(PIN_M1, potencia_final);
    analogWrite(PIN_M2, potencia_final);
    analogWrite(PIN_M3, potencia_final);

    Serial.print("Altura Actual (mm): "); Serial.print(altura_actual);
    Serial.print(" | Potencia: "); Serial.println(potencia_final);
  } else {
    Serial.println("Fuera de rango");
    apagarMotores(); // Seguridad si pierde la lectura
  }
  
  delay(50); // El sensor láser toma unos milisegundos en disparar y leer
}

void apagarMotores() {
  analogWrite(PIN_M0, 0); analogWrite(PIN_M1, 0);
  analogWrite(PIN_M2, 0); analogWrite(PIN_M3, 0);
}