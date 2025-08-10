#include <DHT.h>
#include <WiFi.h>
#include <WiFiAP.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <LittleFS.h>

const char* ssid = "Mattress";
const char* password = "117658";

IPAddress local_ip(192,168,1,1);
IPAddress gateway(192,168,1,1);
IPAddress subnet(255,255,255,0);

WebServer server(80);

// MAX FREQUENCY: 20 kHz or 20000
// Duty cycle depends on resolution                 

// PWM Configuration Constants
#define PWM_FREQ 5000   // Frequency in Hz (adjust based on the driver).
#define PWM_RES 8      // Resolution (8-bit gives values from 0 to 255). Goes from 1 to 65,536. Increased resolution = Finer control over current

// PWM Channels
const int pwm_channel_fan_1 = 0;
const int pwm_channel_fan_2 = 1;
const int pwm_channel_fan_3 = 2;
const int pwm_channel_fan_4 = 3;
const int pwm_channel_fan_5 = 4;
const int pwm_channel_fan_6 = 5;
const int pwm_channel_fan_tec_1 = 6;
const int pwm_channel_fan_tec_2 = 7;
const int pwm_channel_fan_tec_3 = 8;
const int pwm_channel_fan_tec_4 = 9;
const int pwm_channel_fan_tec_5 = 10;
const int pwm_channel_fan_tec_6 = 11;
const int pwm_channel_tec_1 = 12;
const int pwm_channel_tec_2 = 13;

// PWM Pins - GPIO connected to the LED driver's PWM input (CHANGE)
const int pwm_pin_fan_1 = 16;
const int pwm_pin_fan_2 = 2; 
const int pwm_pin_fan_3 = 4;
const int pwm_pin_fan_4 = 1;  
const int pwm_pin_fan_5 = 22;  
const int pwm_pin_fan_6 = 32;  
const int pwm_pin_fan_tec_1 = 13;
const int pwm_pin_fan_tec_2 = 12;
const int pwm_pin_fan_tec_3 = 21;
const int pwm_pin_fan_tec_4 = 15;
const int pwm_pin_fan_tec_5 = 17;  
const int pwm_pin_fan_tec_6 = 3;

const int pwm_pin_tec_1 = 5;
//const int pwm_pin_tec_2 = 10;

// Direction pins
//const int dir1_pin_tec_1 = 14; // currently omitted
//const int dir2_pin_tec_2 = 18; // currently omitted

// DHT Sensors
#define DHTTYPE DHT11
struct DHT_Sensor {
  DHT dht;
  float temperature;
};

DHT_Sensor sensors[] = {
  {DHT(25, DHTTYPE)},
  {DHT(26, DHTTYPE)},
  {DHT(33, DHTTYPE)},
  {DHT(18, DHTTYPE)},
  {DHT(19, DHTTYPE)},
  {DHT(27, DHTTYPE)}
};
const int numSensors = sizeof(sensors) / sizeof(sensors[0]);

bool systemOn = false;
bool manualMode = true;
float desiredTemperature = 20;
int desiredFanSpeedLeft = 128;
int desiredTecFanSpeedLeft = 255;
int desiredFanSpeedRight = 128;
int desiredTecFanSpeedRight = 255;

void handleRoot();
void handleSetDesiredTemp();
void handleSetTecFanSpeedLeft();
void handleSetTecFanSpeedRight();
void handleSetFanSpeedLeft();
void handleSetFanSpeedRight();
void handleTurnOn();
void handleTurnOff();
void handleToggleMode();
void handleGetSensorData();
void handleNotFound();
void updateSystem();

// void listDir(fs::FS &fs, const char * dirname, uint8_t levels) {
//   Serial.printf("Listing directory: %s\n", dirname);

//   File root = fs.open(dirname);
//   if(!root){
//     Serial.println("Failed to open directory");
//     return;
//   }
//   if(!root.isDirectory()){
//     Serial.println("Not a directory");
//     return;
//   }

//   File file = root.openNextFile();
//   while(file){
//     if(file.isDirectory()){
//       Serial.print("  DIR : ");
//       Serial.println(file.name());
//       if(levels){
//         listDir(fs, file.name(), levels -1);
//       }
//     } else {
//       Serial.print("  FILE: ");
//       Serial.print(file.name());
//       Serial.print("  SIZE: ");
//       Serial.println(file.size());
//     }
//     file = root.openNextFile();
//   }
// }

void setup() {
  Serial.begin(115200);
  // // H-Bridge TEC 1 - Subsystem
  //pinMode(dir1_pin_tec_1, OUTPUT); // Set the GPIO as OUTPUT
  ledcAttachChannel(pwm_pin_tec_1, PWM_FREQ, PWM_RES, pwm_channel_tec_1); // Generate the PWM waveform on this pin
  //digitalWrite(dir1_pin_tec_1, LOW); // Default direction

  // // H-Bridge TEC 2 - Subsystem
  // pinMode(dir2_pin_tec_2, OUTPUT);
  // ledcAttachChannel(pwm_pin_tec_2, PWM_FREQ, PWM_RES, pwm_channel_tec_2); 
  // digitalWrite(dir2_pin_tec_2, LOW); // Default direction

  // Fan 1
  ledcAttachChannel(pwm_pin_fan_1, PWM_FREQ, PWM_RES, pwm_channel_fan_1); 
  // Fan 2
  ledcAttachChannel(pwm_pin_fan_2, PWM_FREQ, PWM_RES, pwm_channel_fan_2);
  // Fan 3
  ledcAttachChannel(pwm_pin_fan_3, PWM_FREQ, PWM_RES, pwm_channel_fan_3); 
  // Fan 4
  ledcAttachChannel(pwm_pin_fan_4, PWM_FREQ, PWM_RES, pwm_channel_fan_4); 
  // Fan 5
  ledcAttachChannel(pwm_pin_fan_5, PWM_FREQ, PWM_RES, pwm_channel_fan_5); 
  // Fan 6
  ledcAttachChannel(pwm_pin_fan_6, PWM_FREQ, PWM_RES, pwm_channel_fan_6); 

  // // // TEC Fan 1
  ledcAttachChannel(pwm_pin_fan_tec_1, PWM_FREQ, PWM_RES, pwm_channel_fan_tec_1);
  // TEC Fan 2
  ledcAttachChannel(pwm_pin_fan_tec_2, PWM_FREQ, PWM_RES, pwm_channel_fan_tec_2); 
  // TEC Fan 3
  ledcAttachChannel(pwm_pin_fan_tec_3, PWM_FREQ, PWM_RES, pwm_channel_fan_tec_3); 
  // TEC Fan 4
  ledcAttachChannel(pwm_pin_fan_tec_4, PWM_FREQ, PWM_RES, pwm_channel_fan_tec_4);
  // TEC Fan 5
  ledcAttachChannel(pwm_pin_fan_tec_5, PWM_FREQ, PWM_RES, pwm_channel_fan_tec_5); 
  // TEC Fan 6
  ledcAttachChannel(pwm_pin_fan_tec_6, PWM_FREQ, PWM_RES, pwm_channel_fan_tec_6); 

  Serial.print("Got to LittleFS");
  if (!LittleFS.begin()) {
    Serial.print("Failed to mount LittleFS");
    return;
  }
  Serial.print("LittleFS mounted successfully");

  // listDir(LittleFS, "/", 0);

  // Initialize DHT Sensors
  for(int i = 0; i < numSensors; i++) {
    sensors[i].dht.begin();
    Serial.print("DHT Sensor ");
    Serial.print(i + 1);
    Serial.println(" initialized.");
  }

  // Start wifi in AP
  WiFi.softAP(ssid, password); // soft access point. 'soft' bc no interface to wireless network
  WiFi.softAPConfig(local_ip, gateway, subnet);

  // GET - Retrieve info from server, and data is sent as part of url
  // POST - Send data to server
  // register route for post/get requests
  server.on("/", HTTP_GET, handleRoot); // when client visits http://<ESP32_IP>/, handleRoot is called and the webpage is sent to client
  server.on("/setTemperature", HTTP_POST, handleSetDesiredTemp); // client sends POST request to server at route http://<ESP32_IP>/setTemperature, executing handleSetDesiredTemp
  server.on("/setFanSpeedLeft", HTTP_POST, handleSetFanSpeedLeft);
  server.on("/setFanSpeedRight", HTTP_POST, handleSetFanSpeedRight);
  server.on("/setTecFanSpeedLeft", HTTP_POST, handleSetTecFanSpeedLeft);
  server.on("/setTecFanSpeedRight", HTTP_POST, handleSetTecFanSpeedRight);
  server.on("/turnOn", HTTP_POST, handleTurnOn);
  server.on("/turnOff", HTTP_POST, handleTurnOff);
  server.on("/toggleMode", HTTP_POST, handleToggleMode);
  server.on("/getSensorData", HTTP_GET, handleGetSensorData);
  Serial.println("Route /getSensorData registered");

  server.serveStatic("/", LittleFS, "/");

  server.onNotFound(handleNotFound);

  // // Start server
  server.begin();
  Serial.println("HTTP server started");
}

// server.send(statusCode, contentType, content);

//     statusCode: The HTTP response status code (e.g., 200 OK, 404 Not Found, etc.).
//     contentType: The type of the response (e.g., text/plain, text/html, application/json).
//     content: The response body, which contains the data you want to send back.

void loop() {
  server.handleClient(); 

  // Test Fans
  // ledcWrite(pwm_pin_fan_1, 255);
  // ledcWrite(pwm_pin_fan_tec_1, 255);
  // ledcWrite(pwm_pin_fan_2, 255);
  // ledcWrite(pwm_pin_fan_tec_2, 255);
  // ledcWrite(pwm_pin_fan_3, 255);
  // ledcWrite(pwm_pin_fan_tec_3, 255);
  // ledcWrite(pwm_pin_fan_4, 255);
  // ledcWrite(pwm_pin_fan_tec_4, 255);
  // ledcWrite(pwm_pin_fan_5, 255);
  // ledcWrite(pwm_pin_fan_tec_5, 255);
  // ledcWrite(pwm_pin_fan_6, 255);
  // ledcWrite(pwm_pin_fan_tec_6, 255);

  // ledcWrite(pwm_pin_tec_1, 120);

  // Test Sensors
  // for(int i = 0; i < numSensors; ++i) {
  //   float t = sensors[i].dht.readTemperature();
  //   if (isnan(t)) {
  //   Serial.print("Failed to read from DHT sensor ");
  //   Serial.print(i + 1);
  //   Serial.println("!");
  //   } else {
  //     Serial.print("Sensor ");
  //     Serial.print(i + 1);
  //     Serial.print(" temperature: ");
  //     Serial.println(t);
  //   }
  // }
  
  updateSystem();
}

//<span id="fanSpeedValue">128</span>
void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>Bed Control System</title>
  <style>
    body { font-family: Arial, sans-serif; text-align: center; }
    .container { width: 80%; margin: auto; }
    .button { padding: 10px 20px; margin: 5px; font-size: 16px; }
    .slider { width: 50%; }
    #chartContainer { width: 100%; height: 400px; }
  </style>
  <script src="/chart.min.js"></script>
</head>
<body>
  <div class="container">
    <h1>Bed Control System</h1>
    
    <div>
      <button class="button" onclick="turnOn()">Turn On</button>
      <button class="button" onclick="turnOff()">Turn Off</button>
      <button class="button" onclick="toggleMode(this)">Manual Mode</button>
    </div>
    
    <div>
      <label for="temp">Desired Temperature (°C): </label>
      <input type="number" id="temp" name="temp" value="20" step="0.1">
      <button class="button" onclick="setTemperature()">Set Temperature</button>
    </div>
    
    <div>
      <label for="fanSpeedLeft">Fan Speed Left: </label>
      <input type="range" id="fanSpeedLeft" name="fanSpeedLeft" min="0" max="255" value="128" class="slider">
      <span id="fanSpeedValueLeft">128</span> <!-- Added Span -->
      <button class="button" onclick="setFanSpeedLeft()">Set Left Bed Fan Speed</button>
    </div>
    
    <div>
      <label for="fanSpeedRight">Fan Speed Right: </label>
      <input type="range" id="fanSpeedRight" name="fanSpeedRight" min="0" max="255" value="128" class="slider">
      <span id="fanSpeedValueRight">128</span> <!-- Added Span -->
      <button class="button" onclick="setFanSpeedRight()">Set Right Bed Fan Speed</button>
    </div>
    
    <div>
      <label for="tecFanSpeedLeft">TEC Fan Speed Left: </label>
      <input type="range" id="tecFanSpeedLeft" name="tecFanSpeedLeft" min="0" max="255" value="128" class="slider">
      <span id="tecFanSpeedValueLeft">128</span> <!-- Added Span -->
      <button class="button" onclick="setTecFanSpeedLeft()">Set Left TEC Fan Speed</button>
    </div>
    
    <div>
      <label for="tecFanSpeedRight">TEC Fan Speed Right: </label>
      <input type="range" id="tecFanSpeedRight" name="tecFanSpeedRight" min="0" max="255" value="128" class="slider">
      <span id="tecFanSpeedValueRight">128</span> <!-- Added Span -->
      <button class="button" onclick="setTecFanSpeedRight()">Set Right TEC Fan Speed</button>
    </div>
    
    <h2>Sensor Data</h2>
    <canvas id="sensorChart"></canvas>
    
  </div>
  
  <script>
    // Initialize Chart.js
    const ctx = document.getElementById('sensorChart').getContext('2d');
    const sensorChart = new Chart(ctx, {
      type: 'line',
      data: {
        labels: [], // Time labels
        datasets: []
      },
      options: {
        responsive: true,
        scales: {
          x: { title: { display: true, text: 'Time' } },
          y: { title: { display: true, text: 'Temperature (°C)' }, suggestedMin: 0, suggestedMax: 50 }
        }
      }
    });

    const maxDataPoints = 100;

    // Function to update the chart with new data
    function updateChart(data) {
      const time = new Date().toLocaleTimeString();
      sensorChart.data.labels.push(time);
      
      data.sensors.forEach((sensor, index) => {
        if (!sensorChart.data.datasets[index]) {
          sensorChart.data.datasets[index] = {
            label: 'Sensor ' + sensor.id,
            data: [],
            borderColor: getRandomColor(),
            fill: false
          };
        }
        sensorChart.data.datasets[index].data.push(sensor.temperature);
        if (sensorChart.data.datasets[index].data.length > maxDataPoints) {
          sensorChart.data.datasets[index].data.shift();
        }
      });

      // Remove old labels and data
      if (sensorChart.data.labels.length > maxDataPoints) {
        sensorChart.data.labels.shift();
      }

      sensorChart.update();
    }

    // Function to fetch sensor data periodically
    setInterval(() => {
      fetch('/getSensorData')
        .then(response => response.json())
        .then(data => {
          updateChart(data);
        })
        .catch(error => console.error('Error fetching sensor data:', error));
    }, 5000); // Update every 5 seconds

    // Function to generate random colors for datasets
    function getRandomColor() {
      const letters = '0123456789ABCDEF';
      let color = '#';
      for (let i = 0; i < 6; i++) {
        color += letters[Math.floor(Math.random() * 16)];
      }
      return color;
    }

    // Update fan speed left display
    const fanSpeedSliderLeft = document.getElementById('fanSpeedLeft');
    const fanSpeedValueLeft = document.getElementById('fanSpeedValueLeft');
    fanSpeedSliderLeft.oninput = function() {
      fanSpeedValueLeft.innerHTML = this.value;
    }

    // Update fan speed right display
    const fanSpeedSliderRight = document.getElementById('fanSpeedRight');
    const fanSpeedValueRight = document.getElementById('fanSpeedValueRight');
    fanSpeedSliderRight.oninput = function() {
      fanSpeedValueRight.innerHTML = this.value;
    }

    // Update TEC fan speed display
    const tecFanSpeedSliderLeft = document.getElementById('tecFanSpeedLeft');
    const tecFanSpeedValueLeft = document.getElementById('tecFanSpeedValueLeft');
    tecFanSpeedSliderLeft.oninput = function() {
      tecFanSpeedValueLeft.innerHTML = this.value;
    }

    const tecFanSpeedSliderRight = document.getElementById('tecFanSpeedRight');
    const tecFanSpeedValueRight = document.getElementById('tecFanSpeedValueRight');
    tecFanSpeedSliderRight.oninput = function() {
      tecFanSpeedValueRight.innerHTML = this.value;
    }

    // Control Functions
    function turnOn() {
      fetch('/turnOn', { method: 'POST' })
        .then(response => response.json())
        .then(data => alert(data.status))
        .catch(error => console.error('Error:', error));
    }

    function turnOff() {
      fetch('/turnOff', { method: 'POST' })
        .then(response => response.json())
        .then(data => alert(data.status))
        .catch(error => console.error('Error:', error));
    }

    function toggleMode(button) {
      fetch('/toggleMode', { method: 'POST' })
        .then(response => response.json())
        .then(data => alert(data.status + ': ' + (data.manualMode ? 'Manual' : 'Auto')))
        .catch(error => console.error('Error:', error));

      if (button.innerText === "Manual Mode") {
        button.innerText = "Auto Mode";
      } else {
        button.innerText = "Manual Mode";
      }
    }

    function setTemperature() {
      const temp = document.getElementById('temp').value;
      fetch('/setTemperature', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: 'temperature=' + temp
      })
        .then(response => response.json())
        .then(data => alert(data.status))
        .catch(error => console.error('Error:', error));
    }
    
    function setFanSpeedLeft() {
      const fanSpeed = document.getElementById('fanSpeedLeft').value;
      fetch('/setFanSpeedLeft', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: 'fanSpeed=' + fanSpeed
      })
        .then(response => response.json())
        .then(data => alert(data.status))
        .catch(error => console.error('Error:', error));
    }

    function setFanSpeedRight() {
      const fanSpeed = document.getElementById('fanSpeedRight').value;
      fetch('/setFanSpeedRight', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: 'fanSpeed=' + fanSpeed
      })
        .then(response => response.json())
        .then(data => alert(data.status))
        .catch(error => console.error('Error:', error));
    }

    function setTecFanSpeedLeft() {
      const fanSpeed = document.getElementById('tecFanSpeedLeft').value;
      fetch('/setTecFanSpeedLeft', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: 'fanSpeed=' + fanSpeed
      })
        .then(response => response.json())
        .then(data => alert(data.status))
        .catch(error => console.error('Error:', error));
    }

    function setTecFanSpeedRight() {
      const fanSpeed = document.getElementById('tecFanSpeedRight').value;
      fetch('/setTecFanSpeedRight', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: 'fanSpeed=' + fanSpeed
      })
        .then(response => response.json())
        .then(data => alert(data.status))
        .catch(error => console.error('Error:', error));
    }
  </script>
</body>
</html>
)rawliteral";

  server.send(200, "text/html", html); // send response to client (send html to client)
}

void handleSetDesiredTemp() {
  if(server.hasArg("temperature")) {
    desiredTemperature = server.arg("temperature").toFloat();
    Serial.print("Desired Temperature set to: ");
    Serial.println(desiredTemperature);
    server.send(200, "application/json", "{\"status\":\"Temperature updated\"}"); 
  } else {
    server.send(400, "application/json", "{\"error\":\"Missing temperature parameter\"}");
  }
}

void handleSetFanSpeedLeft() {
  if(server.hasArg("fanSpeed")) {
    desiredFanSpeedLeft = constrain(server.arg("fanSpeed").toInt(), 0, 255);
    Serial.print("Desired Fan Speed set to: ");
    Serial.println(desiredFanSpeedLeft);
    server.send(200, "application/json", "{\"status\":\"Fan speed updated\"}");
  } else {
    server.send(400, "application/json", "{\"error\":\"Missing fanSpeed parameter\"}");
  }
}

void handleSetFanSpeedRight() {
  if(server.hasArg("fanSpeed")) {
    desiredFanSpeedRight = constrain(server.arg("fanSpeed").toInt(), 0, 255);
    Serial.print("Desired Fan Speed set to: ");
    Serial.println(desiredFanSpeedRight);
    server.send(200, "application/json", "{\"status\":\"Fan speed updated\"}");
  } else {
    server.send(400, "application/json", "{\"error\":\"Missing fanSpeed parameter\"}");
  }
}

void handleSetTecFanSpeedLeft() {
  if(server.hasArg("fanSpeed")) {
    desiredTecFanSpeedLeft = constrain(server.arg("tecFanSpeed").toInt(), 0, 255);
    Serial.print("Desired Tec Fan Speed set to: ");
    Serial.println(desiredTecFanSpeedLeft);
    server.send(200, "application/json", "{\"status\":\"TEC Fan speed updated\"}");
  } else {
    server.send(400, "application/json", "{\"error\":\"Missing tecFanSpeed parameter\"}");
  }
}

void handleSetTecFanSpeedRight() {
  if(server.hasArg("fanSpeed")) {
    desiredTecFanSpeedRight = constrain(server.arg("tecFanSpeed").toInt(), 0, 255);
    Serial.print("Desired Tec Fan Speed set to: ");
    Serial.println(desiredTecFanSpeedRight);
    server.send(200, "application/json", "{\"status\":\"TEC Fan speed updated\"}");
  } else {
    server.send(400, "application/json", "{\"error\":\"Missing tecFanSpeed parameter\"}");
  }
}

void handleTurnOn() {
  systemOn = true;
  Serial.println("System Turned ON");
  server.send(200, "application/json", "{\"status\":\"System turned on\"}");
}

void handleTurnOff() {
  systemOn = false;
  // Turn off all outputs
  ledcWrite(pwm_pin_fan_1, 0);
  ledcWrite(pwm_pin_fan_2, 0);
  ledcWrite(pwm_pin_fan_3, 0);
  ledcWrite(pwm_pin_fan_4, 0);
  ledcWrite(pwm_pin_fan_5, 0);
  ledcWrite(pwm_pin_fan_6, 0);

  ledcWrite(pwm_pin_fan_tec_1, 0);
  ledcWrite(pwm_pin_fan_tec_2, 0);
  ledcWrite(pwm_pin_fan_tec_3, 0);
  ledcWrite(pwm_pin_fan_tec_4, 0);
  ledcWrite(pwm_pin_fan_tec_5, 0);
  ledcWrite(pwm_pin_fan_tec_6, 0);

  ledcWrite(pwm_pin_tec_1, 0);
  Serial.println("System Turned OFF");
  server.send(200, "application/json", "{\"status\":\"System turned off\"}");
}

void handleToggleMode() {
  manualMode = !manualMode;
  Serial.print("Manual Mode set to: ");
  Serial.println(manualMode ? "ON" : "OFF");
  String response = "{\"status\":\"Mode toggled\",\"manualMode\":" + String(manualMode ? "true" : "false") + "}";
  server.send(200, "application/json", response);
}

void handleGetSensorData() {
  StaticJsonDocument<1024> jsonDoc;
  JsonArray sensorsArray = jsonDoc.createNestedArray("sensors");
  
  for(int i = 0; i < numSensors; i++) {
    JsonObject sensorObj = sensorsArray.createNestedObject();
    sensorObj["id"] = i + 1;
    sensorObj["temperature"] = sensors[i].temperature;
  }

  String json;
  serializeJson(jsonDoc, json);

  Serial.print("Sending JSON data:");
  Serial.print(json);

  server.send(200, "application/json", json);
}

void handleNotFound(){
  server.send(404, "text/plain", "404: Not Found");
}

void updateSystem() {
  if(systemOn) {
    // Read all sensor temperatures
    for(int i = 0; i < numSensors; i++) {
      sensors[i].temperature = sensors[i].dht.readTemperature();
      if(isnan(sensors[i].temperature)) {
        Serial.print("Failed to read from DHT sensor ");
        Serial.println(i+1);
        sensors[i].temperature = 0.0; // Default to 0 if failed
      }
    }

    // Example: Use the average temperature to control TEC and fans
    float avgTemp = 0.0;
    for(int i = 0; i < numSensors; i++) {
      avgTemp += sensors[i].temperature;
    }
    avgTemp /= numSensors;

    if(!manualMode) {
      // Auto Mode: Control TEC based on desired temperature
      if(avgTemp > desiredTemperature + 1) { // Adding hysteresis
        // Cool down
        //digitalWrite(dir1_pin_tec_1, LOW); // Set direction for cooling
        //digitalWrite(dir2_pin_tec_2, LOW);
        ledcWrite(pwm_pin_tec_1, map(avgTemp, desiredTemperature, desiredTemperature + 10, 0, 255));
        //ledcWrite(pwm_channel_tec_2, map(avgTemp, desiredTemperature, desiredTemperature + 10, 0, 255));
        ledcWrite(pwm_pin_fan_tec_1, 255);
        ledcWrite(pwm_pin_fan_tec_2, 255);
        ledcWrite(pwm_pin_fan_tec_3, 255);
        ledcWrite(pwm_pin_fan_tec_4, 255);
        ledcWrite(pwm_pin_fan_tec_5, 255);
        ledcWrite(pwm_pin_fan_tec_6, 255);
      }
      // NO HEATING RIGHT NOW
      // else if(avgTemp < desiredTemperature - 1) {
      //   // Heat up
      //   digitalWrite(dir1_pin_tec_1, HIGH); // Set direction for heating
      //   digitalWrite(dir2_pin_tec_2, HIGH);
      //   ledcWrite(pwm_channel_tec_1, map(desiredTemperature - avgTemp, 0, 10, 0, 255));
      //   ledcWrite(pwm_channel_tec_2, map(desiredTemperature - avgTemp, 0, 10, 0, 255));
      //}
      else {
        // Maintain current temperature
        ledcWrite(pwm_pin_tec_1, 0);
        // ledcWrite(pwm_pin_tec_2, 0);
        ledcWrite(pwm_pin_fan_tec_1, 255);
        ledcWrite(pwm_pin_fan_tec_2, 255);
        ledcWrite(pwm_pin_fan_tec_3, 255);
        ledcWrite(pwm_pin_fan_tec_4, 255);
        ledcWrite(pwm_pin_fan_tec_5, 255);
        ledcWrite(pwm_pin_fan_tec_6, 255);
      }
    }

    // Manual Mode: Fan speed is set by user
    if(manualMode) {
      ledcWrite(pwm_pin_fan_1, desiredFanSpeedLeft);
      ledcWrite(pwm_pin_fan_2, desiredFanSpeedLeft);
      ledcWrite(pwm_pin_fan_3, desiredFanSpeedLeft);
      ledcWrite(pwm_pin_fan_4, desiredFanSpeedRight);
      ledcWrite(pwm_pin_fan_5, desiredFanSpeedRight);
      ledcWrite(pwm_pin_fan_6, desiredFanSpeedRight);

      ledcWrite(pwm_pin_fan_tec_1, desiredTecFanSpeedLeft);
      ledcWrite(pwm_pin_fan_tec_2, desiredTecFanSpeedLeft);
      ledcWrite(pwm_pin_fan_tec_3, desiredTecFanSpeedLeft);
      ledcWrite(pwm_pin_fan_tec_4, desiredTecFanSpeedRight);
      ledcWrite(pwm_pin_fan_tec_5, desiredTecFanSpeedRight);
      ledcWrite(pwm_pin_fan_tec_6, desiredTecFanSpeedRight);
    } else {
      // Auto Mode: Adjust fan speed based on temperature
      int fanSpeed = map(avgTemp, desiredTemperature - 10, desiredTemperature + 10, 50, 255);
      fanSpeed = constrain(fanSpeed, 50, 255);
      ledcWrite(pwm_pin_fan_1, fanSpeed);
      ledcWrite(pwm_pin_fan_2, fanSpeed);
      ledcWrite(pwm_pin_fan_3, fanSpeed);
      ledcWrite(pwm_pin_fan_4, fanSpeed);
      ledcWrite(pwm_pin_fan_5, fanSpeed);
      ledcWrite(pwm_pin_fan_6, fanSpeed);
    }

    // Throttle TEC if any sensor is too high
    for(int i = 0; i < numSensors; i++) {
      if(sensors[i].temperature > 30) {
        ledcWrite(pwm_pin_tec_1, 0);
        ledcWrite(pwm_pin_fan_tec_1, 255);
        ledcWrite(pwm_pin_fan_tec_2, 255);
        ledcWrite(pwm_pin_fan_tec_3, 255);
        ledcWrite(pwm_pin_fan_tec_4, 255);
        ledcWrite(pwm_pin_fan_tec_5, 255);
        ledcWrite(pwm_pin_fan_tec_6, 255);
        // ledcWrite(pwm_pin_tec_2, 0);
        Serial.println("Temperature exceeded threshold! Throttling TEC.");
        break;
      }
    }
  }
  else {
    // System is off, ensure all outputs are off
    ledcWrite(pwm_pin_fan_1, 0);
    ledcWrite(pwm_pin_fan_2, 0);
    ledcWrite(pwm_pin_fan_3, 0);
    ledcWrite(pwm_pin_fan_4, 0);
    ledcWrite(pwm_pin_fan_5, 0);
    ledcWrite(pwm_pin_fan_6, 0);

    ledcWrite(pwm_pin_fan_tec_1, 0);
    ledcWrite(pwm_pin_fan_tec_2, 0);
    ledcWrite(pwm_pin_fan_tec_3, 0);
    ledcWrite(pwm_pin_fan_tec_4, 0);
    ledcWrite(pwm_pin_fan_tec_5, 0);
    ledcWrite(pwm_pin_fan_tec_6, 0);

    ledcWrite(pwm_pin_tec_1, 0);
    // ledcWrite(pwm_pin_tec_2, 0);
  }
}