function [Acel_Z, Torque_Roll, Torque_Pitch, Torque_Yaw] = dinamica_gemelo(F1, F2, F3, F4, parametros)
    % =====================================================================
    % GEMELO DIGITAL - DINÁMICA FÍSICA
    % Esta función calcula cómo se acelera y gira el dron con base en los motores
    % =====================================================================
    
    % Extraemos los parámetros de la memoria
    m = parametros.m;
    g = parametros.g;
    L = parametros.L;
    d = parametros.d; % Coeficiente de giro (Drag)
    b = parametros.b; % Coeficiente de empuje
    
    % 1. FUERZA TOTAL (Empuje hacia arriba)
    Empuje_Total = F1 + F2 + F3 + F4;
    
    % Aceleración en el eje Z (Vertical)
    % Si el empuje es mayor que el peso (m*g), el dron sube.
    Acel_Z = (Empuje_Total / m) - g;
    
    % 2. TORQUES (Fuerzas de giro en los 3 ejes)
    % Roll (Giro lateral): Diferencia de fuerza entre lado izquierdo y derecho
    Torque_Roll = L * (F4 + F1 - F2 - F3);
    
    % Pitch (Giro frontal): Diferencia de fuerza entre adelante y atrás
    Torque_Pitch = L * (F3 + F4 - F1 - F2);
    
    % Yaw (Giro sobre su propio eje): Reacción física de las hélices girando
    % Motores impares (1,3) giran hacia un lado, pares (2,4) hacia el otro
    Torque_Yaw = (d / b) * (F1 - F2 + F3 - F4);
    
end