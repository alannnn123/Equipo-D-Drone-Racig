% =========================================================================
% PARÁMETROS FÍSICOS DEL DRON - EQUIPO D
% =========================================================================
% 1. Constantes del entorno
g = 9.81;               % Gravedad (m/s^2)

% 2. Masa y dimensiones
m = 0.087;              % Masa total: 87 gramos
L = 0.04;               % Distancia del centro al motor: ~4 cm (Micro dron)

% 3. Propiedades de Inercia (Escaladas a un dron de juguete/miniatura)
Ixx = 2.5e-5;
Iyy = 2.5e-5;
Izz = 5.0e-5;

% 4. Constantes de los motores y hélices miniatura
b = 1.5e-8;             % Coeficiente de empuje para micro hélices
d = 1.5e-10;            % Coeficiente de torque

% Matriz de Inercia completa
Inercia = [Ixx, 0, 0; 
           0, Iyy, 0; 
           0, 0, Izz];

disp('✅ Parámetros del MICRO DRON cargados correctamente.');