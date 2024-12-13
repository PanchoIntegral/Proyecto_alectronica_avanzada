#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BME680.h"
#include <GP2YDustSensor.h>

// WiFi credentials
const char* ssid = "INFINITUM648A_2.4";
const char* password = "mHUEpCD53N";

// Pins for sensors
const uint8_t SHARP_LED_PIN = 19;
const uint8_t SHARP_VO_PIN = 34;
const int MQ135_SENSOR_PIN = 36;
int sensibilidad = 150;

// Pressure at sea level
#define SEALEVELPRESSURE_HPA (1013.25)

// Create instances for sensors
Adafruit_BME680 bme;
GP2YDustSensor dustSensor(GP2YDustSensorType::GP2Y1010AU0F, SHARP_LED_PIN, SHARP_VO_PIN);

// Create server instance
AsyncWebServer server(80);

// HTML for the web page with Chart.js
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8">
  <title>Datos Del Sistema de calidad del aire</title>
  <style>
    body {
      font-family: 'Roboto', sans-serif;
      background-color: #f4f7f6;
      margin: 0;
      padding: 0;
      text-align: center;
    }
    h1 {
      color: #fff;
      background-color: #00796b;
      padding: 20px 0;
      margin: 0;
      box-shadow: 0 2px 4px rgba(0, 0, 0, 0.1);
      position: fixed;
      width: 100%;
      top: 0;
      z-index: 1000;
      transition: top 0.3s;
    }
    .content {
      margin-top: 100px; /* espacio para la barra superior fija */
    }
    .container {
      background-color: #fff;
      margin: 20px auto;
      padding: 20px;
      box-shadow: 0 2px 4px rgba(0, 0, 0, 0.1);
      border-radius: 8px;
      max-width: 800px;
      transition: transform 0.2s;
    }
    .container:hover {
      transform: scale(1.02);
    }
    canvas {
      max-width: 100%;
    }
    .chart-container {
      margin: 20px auto;
      max-width: 90%;
    }
    footer {
      background-color: #00796b;
      color: #fff;
      padding: 10px 0;
      position: fixed;
      width: 100%;
      bottom: 0;
      text-align: center;
    }
  </style>
  <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
  <script>
    const updateInterval = 2000; // Update interval in milliseconds

    let temperatureChart, pressureChart, humidityChart, altitudeChart, dustDensityChart, ppmChart;

    function createChart(context, label, color) {
      return new Chart(context, {
        type: 'line',
        data: {
          labels: [],
          datasets: [{
            label: label,
            data: [],
            fill: false,
            borderColor: color,
            tension: 0.1
          }]
        },
        options: {
          scales: {
            x: {
              type: 'time',
              time: {
                unit: 'second',
                tooltipFormat: 'HH:mm:ss'
              },
              distribution: 'linear',
              ticks: {
                source: 'data'
              },
              grid: {
                display: true,
                color: 'rgba(200, 200, 200, 0.3)'
              }
            },
            y: {
              ticks: {
                beginAtZero: true
              },
              grid: {
                display: true,
                color: 'rgba(200, 200, 200, 0.3)'
              }
            }
          },
          plugins: {
            legend: {
              display: true,
              position: 'top',
              labels: {
                color: '#00796b'
              }
            }
          }
        }
      });
    }

    function updateCharts(data) {
      const now = new Date();
      const maxPoints = 20; // Maximum number of data points to show

      const charts = [
        { chart: temperatureChart, data: data.temperature },
        { chart: pressureChart, data: data.pressure },
        { chart: humidityChart, data: data.humidity },
        { chart: altitudeChart, data: data.altitude },
        { chart: dustDensityChart, data: data.dust_density },
        { chart: ppmChart, data: data.ppm }
      ];

      charts.forEach(item => {
        item.chart.data.labels.push(now);
        item.chart.data.datasets[0].data.push(item.data);

        if (item.chart.data.labels.length > maxPoints) {
          item.chart.data.labels.shift();
          item.chart.data.datasets[0].data.shift();
        }

        item.chart.update();
      });
    }

    async function fetchData() {
      try {
        const response = await fetch('/sensor');
        const data = await response.json();
        updateCharts(data);
      } catch (error) {
        console.error('Error fetching sensor data:', error);
      }
    }

    document.addEventListener('DOMContentLoaded', function() {
      const temperatureCtx = document.getElementById('temperatureChart').getContext('2d');
      const pressureCtx = document.getElementById('pressureChart').getContext('2d');
      const humidityCtx = document.getElementById('humidityChart').getContext('2d');
      const altitudeCtx = document.getElementById('altitudeChart').getContext('2d');
      const dustDensityCtx = document.getElementById('dustDensityChart').getContext('2d');
      const ppmCtx = document.getElementById('ppmChart').getContext('2d');

      temperatureChart = createChart(temperatureCtx, 'Temperatura', 'rgb(255, 99, 132)');
      pressureChart = createChart(pressureCtx, 'Presión', 'rgb(54, 162, 235)');
      humidityChart = createChart(humidityCtx, 'Humedad', 'rgb(75, 192, 192)');
      altitudeChart = createChart(altitudeCtx, 'Altitud', 'rgb(153, 102, 255)');
      dustDensityChart = createChart(dustDensityCtx, 'Densidad del Polvo', 'rgb(255, 159, 64)');
      ppmChart = createChart(ppmCtx, 'PPM', 'rgb(255, 205, 86)');

      setInterval(fetchData, updateInterval);
    });

    let lastScrollTop = 0;
    window.addEventListener('scroll', function() {
      const currentScroll = window.pageYOffset || document.documentElement.scrollTop;
      if (currentScroll > lastScrollTop) {
        document.querySelector('h1').style.top = '-100px';
      } else {
        document.querySelector('h1').style.top = '0';
      }
      lastScrollTop = currentScroll <= 0 ? 0 : currentScroll;
    });
  </script>
</head>
<body>
  <h1>Datos De Sistema De Calidad Del Aire</h1>
  <div class="content">
    <div class="container chart-container">
      <canvas id="temperatureChart"></canvas>
    </div>
    <div class="container chart-container">
      <canvas id="pressureChart"></canvas>
    </div>
    <div class="container chart-container">
      <canvas id="humidityChart"></canvas>
    </div>
    <div class="container chart-container">
      <canvas id="altitudeChart"></canvas>
    </div>
    <div class="container chart-container">
      <canvas id="dustDensityChart"></canvas>
    </div>
    <div class="container chart-container">
      <canvas id="ppmChart"></canvas>
    </div>
  </div>
  <footer>
    Datos actualizados cada 2 segundos
  </footer>
</body>
</html>
)rawliteral";

String interpretar_calidad_aire(int ppm) {
  if (ppm < 400) {
    return "Buena";
  } else if (ppm < 800) {
    return "Moderada";
  } else if (ppm < 1000) {
    return "Mala";
  } else {
    return "Peligrosa";
  }
}

int calcular_ppm(int valor_sensor) {
  int ppm = valor_sensor * sensibilidad / 1023;
  return ppm;
}

void setup() {
  Serial.begin(115200);

  // Conectar a la red WiFi
  Serial.println("Conectando a la red WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nConectado a la red WiFi");
  Serial.print("Dirección IP del ESP32: ");
  Serial.println(WiFi.localIP());

  // Iniciar el servidor web
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });

  server.on("/sensor", HTTP_GET, [](AsyncWebServerRequest *request){
    float temperature = bme.temperature;
    float pressure = bme.pressure / 100.0;
    float humidity = bme.humidity;
    float altitude = bme.readAltitude(SEALEVELPRESSURE_HPA);
    int dust_density = dustSensor.getDustDensity();
    int valor_sensor = analogRead(MQ135_SENSOR_PIN);
    int ppm = calcular_ppm(valor_sensor);
    String air_quality = interpretar_calidad_aire(ppm);

    String json = "{";
    json += "\"temperature\":" + String(temperature) + ",";
    json += "\"pressure\":" + String(pressure) + ",";
    json += "\"humidity\":" + String(humidity) + ",";
    json += "\"altitude\":" + String(altitude) + ",";
    json += "\"dust_density\":" + String(dust_density) + ",";
    json += "\"ppm\":" + String(ppm) + ",";
    json += "\"air_quality\":\"" + air_quality + "\"";
    json += "}";

    request->send(200, "application/json", json);
  });

  server.begin();
  Serial.println("Servidor web iniciado");

  // Inicializar BME680
  if (!bme.begin()) {
    Serial.println(F("No se pudo detectar el sensor BME680, intenta de nuevo"));
    while (1);
  }
  Serial.println("Sensor BME680 detectado");

  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150); // 320°C for 150 ms

  // Inicializar GP2YDustSensor
  dustSensor.setBaseline(1.0);
  dustSensor.setCalibrationFactor(0.25);
  dustSensor.begin();
  Serial.println("Sensor GP2YDustSensor iniciado");
}

void loop() {
  // Leer datos del BME680
  unsigned long endTime = bme.beginReading();
  if (endTime == 0) {
    Serial.println(F("Hubo un fallo en el inicio de la lectura"));
    return;
  }

  delay(50);

  if (!bme.endReading()) {
    Serial.println(F("Hubo un fallo en finalizar la lectura"));
    return;
  }

  // Leer y mostrar datos de los sensores
  Serial.print("Temperatura: ");
  Serial.print(bme.temperature);
  Serial.println(" °C");

  Serial.print("Presión: ");
  Serial.print(bme.pressure / 100.0);
  Serial.println(" hPa");

  Serial.print("Humedad: ");
  Serial.print(bme.humidity);
  Serial.println(" %");

  Serial.print("Altitud: ");
  Serial.print(bme.readAltitude(SEALEVELPRESSURE_HPA));
  Serial.println(" m");

  int densidadPolvo = dustSensor.getDustDensity();
  if (densidadPolvo > 600) densidadPolvo = 600;

  Serial.print("Densidad del polvo: ");
  Serial.print(densidadPolvo);
  Serial.println(" ug/m3");

  int valor_sensor = analogRead(MQ135_SENSOR_PIN);
  int ppm = calcular_ppm(valor_sensor);
  String estado_calidad_aire = interpretar_calidad_aire(ppm);

  Serial.print("Valor del sensor MQ135: ");
  Serial.println(valor_sensor);
  Serial.print("Concentración de CO2 y otros gases nocivos: ");
  Serial.print(ppm);
  Serial.println(" PPM");
  Serial.print("Calidad del aire: ");
  Serial.println(estado_calidad_aire);

  delay(2000);
}
