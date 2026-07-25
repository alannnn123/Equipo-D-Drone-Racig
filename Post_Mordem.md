
**Dificultades y Errores obtuvidos**<br>
***Ing Sofware*** <br>Alan Almaraz: Programación del microcontrolador (ESP32-C3), diseño de la interfaz web de control y optimización de la comunicación cliente-servidor.

1. Cuello de botella en la telemetría y bloqueos del ciclo principal

El Problema: Durante las primeras pruebas, noté que la lectura de los datos del giroscopio hacia la página web era extremadamente lenta. No podía ver los valores en tiempo real, lo que me dejaba a ciegas para saber cómo retroalimentar los motores. El ciclo principal (loop) se estaba pausando demasiado.

La Causa: Inicialmente estaba utilizando la función delay() de forma ineficiente y el servidor web bloqueaba la ejecución mientras enviaba toda la página HTML cada vez que yo quería ver un dato nuevo.

La Solución: Tuve que reestructurar la arquitectura del código. Implementé un bucle de vuelo de alta velocidad (con un retardo mínimo de 2ms). Para la web, separé el código: el ESP32 ahora solo envía la interfaz HTML una vez al conectarse, y creé un endpoint específico (/telemetria). En el lado del cliente (JavaScript), programé peticiones asíncronas con fetch() que consultan los datos del sensor y el estado PWM de los motores cada 100 milisegundos, actualizando el panel en tiempo real sin recargar la página.

2. Falso positivo en el sistema de seguridad (Kill Switch)

El Problema: Implementé una medida de seguridad en el código: si el dron se inclinaba más de 45 grados, los motores debían apagarse automáticamente para evitar accidentes. Sin embargo, al hacer pruebas con los motores encendidos a media potencia, el dron se apagaba solo, incluso si yo lo sostenía firmemente con la mano.

La Causa: Me di cuenta de que estaba leyendo los valores "crudos" del sensor. Los motores de corriente continua (DC) generan micro-vibraciones mecánicas altísimas. El software estaba leyendo esas vibraciones de alta frecuencia como picos de fuerza G enormes, engañando a la condición del código, que interpretaba el ruido mecánico como una "volcadura" inminente.

La Solución: Tuve que eliminar por completo el bloque condicional de apagado automático por ángulo. Decidí que era mejor dejar que el algoritmo peleara contra la inercia sin restricciones de software, delegando el apagado únicamente al botón de "Paro de Emergencia" de mi interfaz.

3. Desarrollo del "Centro de Mando" (Live Tuning)

El Problema: Cada vez que notaba que un motor (específicamente el delantero izquierdo) no levantaba con la misma fuerza, tenía que desconectar el dron, modificar la variable de potencia base en el IDE de Arduino, compilar y volver a subir el código. Este proceso hacía que calibrar el dron fuera una tarea casi imposible y lentísima.

La Solución: Diseñé y programé una estación de control completa utilizando HTML, CSS y JavaScript alojada en la memoria del ESP32.

Trims Dinámicos: Agregué sliders independientes para cada motor que me permiten enviar valores que van desde -150 hasta 255 mediante peticiones HTTP. Esto me permitió inyectar o restar potencia base (PWM) a cada motor de forma cruzada en pleno vuelo para compensar la falta de fuerza física sin necesidad de reprogramar.

Joystick Virtual: Para el control direccional, programé un elemento Canvas en HTML5 que captura eventos táctiles (touchstart, touchmove) y de ratón. Al mover el joystick, el código mapea las coordenadas (X, Y) y las envía al ESP32 para modificar las variables de Pitch (cabeceo) y Roll (alabeo) en tiempo real, dándome control manual sobre las correcciones matemáticas.

***Ing Electronica*** <br>
Responsabilidad: Cableado, asignación de pines, alimentación de energía, integración del bus I2C y protección de componentes físicos.

1. Caídas de Tensión (Brownouts) y Reinicios del ESP32

El Problema: Al intentar encender el dron con la batería conectada, la red Wi-Fi "Dron_Estable" desaparecía súbitamente o la placa entraba en un bucle de reinicios infinitos, impidiendo cualquier conexión.

La Causa: Descubrí que se trataba de un problema de distribución de energía. El chip Wi-Fi del ESP32 exige un pico de corriente muy alto exactamente en el instante en que crea la red. Como la batería LiPo estaba algo descargada por las pruebas previas y compartía la línea de alimentación con el puente H de los motores, el voltaje general colapsaba por debajo de los 3.3V requeridos. El detector interno del ESP32 (Brownout Detector) registraba esta caída y forzaba un reinicio de seguridad.

La Solución: Implementé un protocolo de aislamiento. Conecté el ESP32 únicamente por USB para confirmar que el hardware seguía vivo (la red apareció de inmediato). A partir de ahí, establecí la regla de iniciar las pruebas de vuelo solo con la LiPo cargada a su voltaje máximo (4.2V) para que tuviera la capacidad de absorber el pico de encendido de la antena sin afectar al procesador.

2. Secuestro del Procesador por "Strapping Pins" (Bloqueo de Arranque)

El Problema: Llegó un punto crítico donde la placa quedó completamente "brickeada" (muda). Mi computadora dejó de reconocer el puerto COM y el IDE de Arduino me arrojaba un error de Write timeout al intentar subir nuevo código. Los botones físicos de Reset y Boot tampoco respondían.

La Causa: Analizando la hoja de datos (datasheet) del ESP32-C3, me di cuenta de un error grave de diseño en mi ruteo de cables. Había conectado el sensor giroscópico MPU6050 a los pines 8 y 9 (y en un intento posterior, al pin 2). En esta arquitectura, esos son Strapping Pins: pines que el procesador lee durante el primer milisegundo de encendido para decidir en qué modo arrancar. El MPU6050 tiene resistencias pull-up integradas que inyectaban voltaje "alto" en esos pines, mintiéndole al ESP32 y forzándolo a entrar en un modo de error del cual no podía salir.

La Solución: Temporalmente, apliqué una técnica de "Conexión en caliente" (Hot-Plug), donde encendía la placa con el sensor desconectado y lo conectaba 2 segundos después. Sin embargo, como solución de ingeniería definitiva, reasigné los cables del bus I2C (SDA y SCL) a los Pines 10 y 7, los cuales son pines 100% "limpios" que no interfieren con la secuencia de arranque.

3. Diagnóstico del Bus I2C y Protección del MPU6050

El Problema: En varias ocasiones la telemetría comenzó a mostrar valores congelados en cero (0,0). El procesador funcionaba, pero el giroscopio parecía haber muerto repentinamente.

La Causa: Me enfrenté a dos escenarios. El primero fue físico: los cables de prototipo (Dupont) sufren fracturas internas invisibles al manipularlos, o a veces conectaba SDA y SCL invertidos. El segundo escenario fue eléctrico: los motores de corriente continua generan picos de voltaje inverso (Flyback) al apagarse bruscamente. Al no tener capacitores suficientes para absorber ese golpe, el ruido eléctrico viajaba por la placa, desestabilizando (o en el peor de los casos, quemando) el chip de silicio del MPU6050.

La Solución: Para dejar de adivinar, desarrollé e implementé un código de diagnóstico exclusivo: un "Escáner I2C Universal". Este código aislaba el hardware (sin encender Wi-Fi ni motores) y barría todas las frecuencias para confirmar si el MPU6050 respondía en la dirección 0x68. Sumado a esto, verifiqué la alimentación física revisando el LED de estado del módulo y reemplacé el cableado defectuoso, logrando estabilizar la comunicación I2C en los pines definitivos.


***Ing Mecanico*** <br> Yeram Gonzales
Encargado del diseño en 3D para el dron. A lo largo del proyecto, realicé múltiples iteraciones digitales y pruebas en simuladores para lograr una estructura lo más ligera y resistente posible. <br>
Pasé de un diseño inicial voluminoso de 150 gramos a una versión mucho más aerodinámica de tan solo 36 gramos. Aunque logramos reducir el peso drásticamente, por restricciones de tiempo decidimos emplear la carcasa comercial del dron, que pesaba 8 gramos. Con más tiempo para reimprimir, se habría logrado un peso aún menor aplicando un patrón de malla y paredes más delgadas. <br>
•	Evolución del peso: Se partió de un prototipo inicial rígido que pesaba 150 g (sumando caja y armazón) hasta alcanzar una versión aerodinámica de 36 g.<br>
•	Decisión final: Por limitaciones de tiempo, se optó por utilizar la carcasa original del dron, la cual pesaba únicamente 8 g. <br>
•	 Conclusión y trabajo futuro: A través de técnicas como el aligeramiento mediante estructura de malla (lattice) y la reducción del grosor de las paredes, habría sido posible alcanzar un peso aún más óptimo en impresión 3D en iteraciones futuras. <br>

***Ing Control*** <br> Manuel Lucano: 
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

***Ing Comunicaciones***<br> Diego Betancourt:
Problema principal
Durante las pruebas finales del proyecto vimos que los cuatro motores sí respondían a las instrucciones enviadas por el ESP32-S3, por lo que en un principio parecía que todo funcionaba correctamente. Sin embargo, al momento de intentar volar el dron nos dimos cuenta de que algunos motores giraban más rápido que otros.
Al no generar todos la misma fuerza, el dron perdía el equilibrio y comenzaba a girar sobre sí mismo en lugar de despegar de forma estable. Como resultado, nunca pudo mantenerse en el aire ni realizar un vuelo controlado.
Como consecuencia de este problema:
•	El dron no logró despegar correctamente. 
•	Comenzaba a girar sobre su propio eje. 
•	No era posible mantenerlo estable. 
•	No se pudo lograr un vuelo estacionario. 

*Causas identificadas*
1. Diferencia en la velocidad de los motores
La principal falla fue que los motores no alcanzaban la misma velocidad, aunque recibían prácticamente las mismas instrucciones.
Esto pudo deberse a varios factores, por ejemplo:
•	Los ESC no estaban calibrados exactamente igual. 
•	Algunos motores podían tener pequeñas diferencias de fabricación. 
•	La alimentación eléctrica no era completamente uniforme. 
•	La configuración utilizada para controlar los motores aún necesitaba ajustes. 

2. Falta de un sistema de estabilización
Nuestro sistema enviaba órdenes a los motores, pero no tenía un mecanismo que corrigiera automáticamente los movimientos del dron mientras estaba intentando volar.
En otras palabras, si un motor comenzaba a girar más rápido que otro, el sistema no era capaz de detectarlo y compensarlo para recuperar el equilibrio.

3. Calibración insuficiente
Otro aspecto importante fue la calibración de algunos componentes.
No se realizó una calibración completa de elementos como:
•	Los ESC. 
•	El giroscopio y acelerómetro. 
•	Los sensores encargados de medir la orientación del dron. 
Una mala calibración puede hacer que el dron interprete incorrectamente su posición y termine reaccionando de forma inestable.

4. Distribución del peso
Aunque el mayor problema fueron los motores, también consideramos que la distribución del peso pudo haber influido en el comportamiento del dron.
Si el peso no está bien balanceado, mantener la estabilidad resulta mucho más complicado, especialmente durante el despegue.

*Posibles soluciones* <br>
Para una siguiente versión del proyecto consideramos que sería importante implementar las siguientes mejoras: <br>
•	Calibrar individualmente cada ESC para que todos los motores respondan de manera similar. <br>
•	Incorporar un sistema de estabilización que utilice la información del giroscopio y acelerómetro para corregir automáticamente cualquier desequilibrio. <br>
•	Verificar que los cuatro motores alcancen velocidades muy parecidas antes de realizar las pruebas de vuelo. <br>
•	Optimizar la comunicación entre el transmisor y el ESP32-S3 para que las órdenes se envíen con la menor latencia posible. <br>
•	Realizar más pruebas de ajuste antes del primer vuelo para encontrar la configuración más estable. <br>

Este proyecto nos dejó varias enseñanzas importantes.
La primera es que el hecho de que todos los componentes funcionen de manera individual no significa que el sistema completo vaya a funcionar correctamente cuando se integren.
También aprendimos que la estabilidad de un dron depende del trabajo conjunto de todos sus elementos: sensores, motores, controladores y comunicación. Si uno de ellos presenta algún problema, el desempeño general se ve afectado.
Otra lección importante fue la necesidad de dedicar más tiempo a la calibración y a las pruebas antes de intentar un vuelo real. Detectar estos detalles desde etapas tempranas puede evitar muchos problemas durante la integración final.
Finalmente, entendimos que desarrollar un dron implica mucho más que hacer que los motores enciendan. Es necesario lograr que todos los componentes trabajen de forma sincronizada para conseguir un vuelo estable y seguro.


