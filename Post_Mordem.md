
**Dificultades y Errores obtuvidos**<br>
*Ing Sofware* <br>Alan Almaraz

*Ing Electronica* <br>

*Ing Mecanico* <br> Yeram Gonzales

*Ing Contro* <br> Manuel Lucano: 
Del simulador a la realidad no todo es perfecto En las simulaciones realizadas en MATLAB, el dron virtual se comportaba de manera ideal. Pero en la vida real, factores que no se ven en la pantalla —como ligeras corrientes de aire, la distribución real del peso, los cables o las caídas de voltaje en la batería— hacían que la física del dron físico fuera muy diferente a la teoría.

Domar los motores de juguete: Reutilizar motores "coreless" de un dron comercial fue un verdadero reto. Estos motores no siempre responden con la misma fuerza exacta a las señales PWM que les manda el ESP Mini, lo que volvía muy complejo el proceso de encontrar la sintonización adecuada para estabilizarlos.

Las vibraciones volvían locos a los sensores: En la simulación, los datos de inclinación del dron son impecables. Sin embargo, en la práctica, en cuanto los motores empezaban a girar a toda velocidad, las vibraciones del chasis metían muchísimo "ruido" al sensor MPU6050. Esto engañaba al algoritmo de control y hacía que el dron intentara corregir movimientos que en realidad no estaban ocurriendo.

Lo mas dificil fue pasar la teoría matemática al "cerebro" físico del dron.
Definitivamente, el mayor desafío fue tomar todos los cálculos teóricos de control de MATLAB y hacer que encajaran y funcionaran dentro de los recursos limitados de un microcontrolador real (el ESP Mini).

En la computadora, el poder de procesamiento sobra; pero en el mundo físico, fue complicado ajustar la programación para que el ESP Mini lograra leer el sensor, calcular la fórmula de control (el PID) y mandar la orden de potencia a los motores en una fracción de segundo y de manera constante. Ajustar ese tiempo de ejecución (Loop Time) para que el sistema no se desfasara o se "trabara" fue el mayor obstáculo técnico a nivel de control.

Oportunidades de Mejora (Si retomamos el proyecto)
Si en el futuro decidimos hacer una nueva versión, en la parte de control se debería implementar lo siguiente para lograr un vuelo completamente estable:

Mejorar la lectura de los sensores: En lugar de usar filtros complementarios básicos para limpiar el ruido del sensor, implementaría un Filtro de Kalman. Es un algoritmo mucho más robusto que mitigaría casi por completo las vibraciones del chasis, dándonos el ángulo real y exacto del dron.

Usar datos de vuelos reales (System ID): En vez de adivinar o calcular los pesos y la física del dron en papel, realizaríamos vuelos de prueba para grabar los datos de telemetría y subirlos a MATLAB. Así, se podría realizar una Identificación de Sistemas para obtener un modelo matemático 100% fiel a nuestro dron específico y calcular las ganancias ideales.

Control de vuelo más avanzado: Evolucionar el código para pasar de un control PID simple a un PID en Cascada (un sistema doble: un lazo externo que controle la inclinación general y un lazo interno que controle qué tan rápido gira para alcanzar esa inclinación). Esto haría que los movimientos en el aire sean mucho más fluidos y evitaría correcciones bruscas.

*Ing Comunicaciones*<br> Diego Betancourt

