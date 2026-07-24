# Proyecto: Dron Estable V2
**Equipo:** <div align="center">
DIEGO BETANCOURT MENDEZ <br>
LUCANO ESTRADA MANUEL ALEJANDRO <br>
ALAN BLADIMIR ALMARAZ CORTES <br>
CRISTIAN YERAM GONZALEZ MIRANDA <br>
**FALCONS**<br>

<img src="LOGO.jpeg" width="300">
</div>

## Datos del Dron
El presente proyecto utiliza una arquitectura de hardware personalizada, montada sobre el chasis de un dron comercial, integrando microcontroladores modernos y sensores de estabilización para el control de vuelo mediante Wi-Fi.

**Estructura y Propulsión** (Hardware Base)<br>
*Chasis y Brazos:* Estructura plástica ultraligera recuperada de un dron de juguete comercial.

*Motores (4x):* Motores de corriente continua sin núcleo (Coreless DC Motors). Operan a altas revoluciones y bajo torque, configurados de manera cruzada (dos girando en sentido horario CW y dos en sentido antihorario CCW) para cancelar el momento angular.

*Hélices:* De paso fijo, optimizadas para los motores coreless de juguete.

**Unidad de Control** <br>
Microcontrolador: ESP Mini (basado en la arquitectura ESP32/ESP8266).

*Procesamiento:* Frecuencia de reloj de alto rendimiento (hasta 240 MHz), lo cual permite ejecutar el bucle de estabilización y el servidor web simultáneamente.

*Telemetría y Control:* Utiliza el módulo Wi-Fi integrado a 2.4 GHz configurado como Punto de Acceso (SoftAP) para enviar y recibir comandos de vuelo y telemetría en tiempo real sin necesidad de un router externo.

**Sistema de Estabilización** (Sensores)
Sensor Inercial (IMU): Módulo MPU6050.

*Grados de Libertad:* Integra un giroscopio de 3 ejes y un acelerómetro de 3 ejes en el mismo chip.

*Comunicación:* Se comunica con el microcontrolador mediante el protocolo I2C, entregando datos precisos sobre la inclinación (Pitch y Roll) para la retroalimentación del sistema de control.

**Potencia** (Controladores de Motor)<br>
Actuadores: 2 Módulos de Controlador de Motor Puente H (H-Bridge).

*Funcionamiento:* El ESP Mini solo entrega señales lógicas de bajo voltaje y baja corriente (3.3V), los puentes H actúan como intermediarios. Reciben las señales de Modulación por Ancho de Pulso (PWM) del microcontrolador y permiten el paso de la corriente directa de la batería hacia los 4 motores, logrando un control de velocidad independiente y preciso para cada motor.

