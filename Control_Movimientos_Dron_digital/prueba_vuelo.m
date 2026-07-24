% =========================================================================
% PRUEBA DE VUELO - GEMELO DIGITAL
% =========================================================================

% 1. Cargar parámetros
parametros; 

% 2. Empacamos las variables
parametros.m = m; parametros.g = g; parametros.L = L;
parametros.b = b; parametros.d = d;

% 3. SIMULACIÓN: Fuerza suave de despegue (0.25 Newtons por motor)
F1 = 0.25; F2 = 0.25; F3 = 0.25; F4 = 0.25;

% 4. Dinámica
[Acel_Z, Torque_Roll, Torque_Pitch, Torque_Yaw] = dinamica_gemelo(F1, F2, F3, F4, parametros);

disp('--- RESULTADOS DEL VUELO ---');
disp(['Aceleración Vertical (Z): ', num2str(Acel_Z, '%.2f'), ' m/s^2']);

% =========================================================================
% 5. TELEMETRÍA ELÉCTRICA (Motor 1) Ajustada a 3.9V
% =========================================================================
KV = 16000;             % Micro motores tienen KV altísimo (16,000 rev/volt)
R_motor = 0.8;          % Resistencia interna típica de micro motor
Temp_Ambiente = 25;     

% 1. Voltaje necesario considerando eficiencia
w_rad_s = sqrt(F1 / parametros.b); 
RPM = w_rad_s * (60 / (2*pi));
Voltaje_Ideal = RPM / KV; 
Voltaje = Voltaje_Ideal / 0.8; % Eficiencia del 80%

% 2. CORRECCIÓN: Corriente con Back-EMF
Back_EMF = RPM / KV; 
Corriente = (Voltaje - Back_EMF) / R_motor;

% 3. Potencia y Temperatura
Potencia = Voltaje * Corriente;
Temperatura = Temp_Ambiente + ((Potencia * 0.20) / 2);

disp('--- TELEMETRÍA DEL MOTOR (CORREGIDA) ---');
disp(['Velocidad: ', num2str(RPM, '%.0f'), ' RPM']);
disp(['Voltaje Aplicado: ', num2str(Voltaje, '%.2f'), ' V']);
disp(['Corriente Real: ', num2str(Corriente, '%.2f'), ' A']);
disp(['Potencia: ', num2str(Potencia, '%.2f'), ' W']);
disp(['Temperatura: ', num2str(Temperatura, '%.1f'), ' °C']);