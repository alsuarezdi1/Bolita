#include <Wire.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <MPU6050.h>

// Configuración del punto de acceso
const char* ssid = "ESP32-AP";
const char* password = "12345678";

// Inicialización del MPU6050 y el servidor
MPU6050 mpu;
AsyncWebServer server(80);

// Página HTML+JavaScript
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>ESP32 GY-521 Control</title>
  <style>
    body { margin: 0; display: flex; justify-content: center; align-items: center; height: 100vh; background-color: #f0f0f0; flex-direction: column; }
    #container { width: 300px; height: 300px; border: 2px solid #000; position: relative; }
    #ball { width: 50px; height: 50px; background-color: red; border-radius: 50%; position: absolute; }
    #coords { font-size: 20px; margin-top: 10px; }
  </style>
</head>
<body>
  <div id="container">
    <div id="ball"></div>
  </div>
  <div id="coords">X: 0, Y: 0</div>
  <script>
    const ball = document.getElementById('ball');
    const container = document.getElementById('container');
    const coordsDisplay = document.getElementById('coords');
    let x = container.clientWidth / 2 - ball.clientWidth / 2;
    let y = container.clientHeight / 2 - ball.clientHeight / 2;
    const speed = 0.01; // Ajusta este valor para cambiar la sensibilidad del movimiento

    function updatePosition(ax, ay) {
      x -= ax * speed; // Invertir movimiento en el eje X
      y += ay * speed;

      // Limitar el movimiento dentro del contenedor
      x = Math.max(0, Math.min(container.clientWidth - ball.clientWidth, x));
      y = Math.max(0, Math.min(container.clientHeight - ball.clientHeight, y));

      ball.style.transform = `translate(${x}px, ${y}px)`;
      coordsDisplay.innerText = `X: ${Math.round(x)}, Y: ${Math.round(y)}`;
    }

    async function getGyroData() {
      const response = await fetch('/gyro');
      const data = await response.json();
      console.log("Gyro Data:", data);  // Depuración: Imprimir datos en la consola
      updatePosition(data.ax, data.ay);
    }

    setInterval(getGyroData, 100); // Actualizar la posición cada 100 ms
  </script>
</body>
</html>
)rawliteral";

void setup() {
  // Iniciar la conexión I2C
  Wire.begin(21, 22);  // SDA en GPIO 21, SCL en GPIO 22

  // Inicializar el MPU6050
  mpu.initialize();

  // Verificar la conexión con el MPU6050
  if (!mpu.testConnection()) {
    Serial.println("MPU6050 connection failed");
    while (1);
  }

  // Iniciar el puerto serie para depuración
  Serial.begin(115200);

  // Configurar el ESP32 como un punto de acceso
  WiFi.softAP(ssid, password);

  // Mostrar la dirección IP asignada al AP
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  // Configurar el servidor web para enviar la página HTML+JS
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html", index_html);
  });

  // Configurar el servidor web para enviar los datos del acelerómetro
  server.on("/gyro", HTTP_GET, [](AsyncWebServerRequest *request){
    int16_t ax, ay, az;
    mpu.getAcceleration(&ax, &ay, &az);

    // Imprimir los valores de depuración en el puerto serie
    Serial.print("ax: ");
    Serial.print(ax);
    Serial.print(", ay: ");
    Serial.print(ay);
    Serial.print(", az: ");
    Serial.println(az);

    String json = "{\"ax\":" + String(ax) + ", \"ay\":" + String(ay) + ", \"az\":" + String(az) + "}";
    request->send(200, "application/json", json);
  });

  // Iniciar el servidor web
  server.begin();
}

void loop() {
  // No se necesita código en el loop
}

