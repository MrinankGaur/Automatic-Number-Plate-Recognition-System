
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "esp_camera.h"
/* I2C and OLED Display Includes ------------------------------------------- */
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
//http server header files
#include <WebServer.h>
// ESP32-CAM doesn't have dedicated I2C pins, so we define our own
#define I2C_SDA 15
#define I2C_SCL 14
TwoWire I2Cbus = TwoWire(0);
// Display defines
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

// Web server instance
WebServer server(80);


Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &I2Cbus, OLED_RESET);
const char* ssid = "*******";       
const char* password = "********";    
String serverName = "www.circuitdigest.cloud";  
String serverPath = "/readnumberplate";              
const int serverPort = 443;                    
String apiKey = "**********";             
#define triggerButton 13  
#define flashLight 4      
int count = 0;           
WiFiClientSecure client; 
// Camera GPIO pins - adjust based on your ESP32-CAM board
#define PWDN_GPIO_NUM 32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 0
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27
#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22
String NoPlateData = "";
String imageLink = "";
bool npState = false;

void handleRoot() {
  String plateColor = npState ? "#10B981" : "#EF4444"; // Green/Red colors
  String statusText = npState ? "Number Plate Detected: " + NoPlateData : "No Plate Detected";
  String cardBorderColor = npState ? "3px solid #10B981" : "3px solid #EF4444";

  String html = R"=====(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>ANPR System | Plate Detection</title>
  <style>
    :root {
      --bg-color: #f8f9fa;
      --text-color: #2d3748;
      --header-bg: #ffffff;
      --card-bg: #ffffff;
      --border-color: #e2e8f0;
      --primary: #3B82F6;
      --success: #10B981;
      --error: #EF4444;
      --footer-text: #718096;
    }
    
    @media (prefers-color-scheme: dark) {
      :root {
        --bg-color: #1a202c;
        --text-color: #e2e8f0;
        --header-bg: #2d3748;
        --card-bg: #2d3748;
        --border-color: #4a5568;
        --footer-text: #a0aec0;
      }
    }
    
    * {
      margin: 0;
      padding: 0;
      box-sizing: border-box;
      font-family: 'Segoe UI', Roboto, Oxygen, Ubuntu, Cantarell, sans-serif;
    }
    
    html, body {
      height: 100%;
      width: 100%;
      overflow: hidden;
    }
    
    body {
      background: var(--bg-color);
      color: var(--text-color);
      display: flex;
      flex-direction: column;
      line-height: 1.6;
    }
    
    header {
      background: var(--header-bg);
      padding: 1rem 0;
      text-align: center;
      box-shadow: 0 2px 4px rgba(0,0,0,0.1);
      flex-shrink: 0;
    }
    
    h1 {
      font-size: 1.5rem;
      font-weight: 700;
      color: var(--text-color);
    }
    
    .main-content {
      flex: 1;
      display: flex;
      flex-direction: column;
      padding: 1rem;
      overflow: hidden;
    }
    
    .card {
      background: var(--card-bg);
      border-radius: 12px;
      padding: 1.5rem;
      box-shadow: 0 4px 6px rgba(0,0,0,0.05);
      border: 1px solid var(--border-color);
      border-left: )=====" + cardBorderColor + R"=====(;
      text-align: center;
      flex: 1;
      display: flex;
      flex-direction: column;
      overflow: hidden;
    }
    
    .image-container {
      flex: 1;
      min-height: 0;
      display: flex;
      justify-content: center;
      align-items: center;
      margin-bottom: 1rem;
    }
    
    .captured-image {
      max-width: 100%;
      max-height: 100%;
      object-fit: contain;
      border-radius: 8px;
      box-shadow: 0 2px 4px rgba(0,0,0,0.1);
    }
    
    .plate-info {
      flex-shrink: 0;
    }
    
    .plate-number {
      font-family: 'Courier New', monospace;
      font-size: 1.8rem;
      font-weight: 700;
      margin: 0.5rem 0;
      color: )=====" + plateColor + R"=====(;
      letter-spacing: 2px;
    }
    
    .plate-status {
      font-size: 1.1rem;
      margin-bottom: 1rem;
      color: )=====" + plateColor + R"=====(;
    }
    
    .footer {
      text-align: center;
      padding: 1rem;
      color: var(--footer-text);
      font-size: 0.8rem;
      flex-shrink: 0;
      background: var(--header-bg);
    }
    
    .footer-title {
      font-weight: 600;
    }
    
    .loader {
      display: none;
      width: 40px;
      height: 40px;
      margin: 0.5rem auto;
      border: 4px solid rgba(16, 185, 129, 0.2);
      border-radius: 50%;
      border-top-color: )=====" + plateColor + R"=====(;
      animation: spin 1s linear infinite;
    }
    
    @keyframes spin {
      to { transform: rotate(360deg); }
    }
    
    .btn {
      background: var(--primary);
      color: white;
      border: none;
      padding: 0.75rem 1.5rem;
      border-radius: 8px;
      font-weight: 600;
      cursor: pointer;
      transition: all 0.2s;
      margin-top: 1rem;
      width: 100%;
      max-width: 300px;
      margin-left: auto;
      margin-right: auto;
    }
    
    .btn:hover {
      background: #2563EB;
      transform: translateY(-1px);
    }
    
    .btn:disabled {
      background: #9CA3AF;
      cursor: not-allowed;
      transform: none;
    }
  </style>
</head>
<body>
  <header>
    <h1>Automatic Number Plate Recognition</h1>
  </header>
  
  <div class="main-content">
    <div class="card">
      <div class="image-container">
        <img id="plateImage" class="captured-image" src="http://)=====" + imageLink + R"=====(" alt="Captured Number Plate">
      </div>
      
      <div class="plate-info">
        <div id="plateDisplay" class="plate-number">)=====" + NoPlateData + R"=====(</div>
        <div class="plate-status">)=====" + statusText + R"=====(</div>
        
        <div id="loader" class="loader"></div>
        
        <button id="captureBtn" class="btn" onclick="captureImage()">Capture New Image</button>
      </div>
    </div>
  </div>
  
  <div class="footer">
    <div class="footer-title">ANPR System</div>
    <div>Real-time Computer Vision Analysis</div>
  </div>
  
  <script>
    function captureImage() {
      const btn = document.getElementById('captureBtn');
      const loader = document.getElementById('loader');
      const plateDisplay = document.getElementById('plateDisplay');
      const plateStatus = document.querySelector('.plate-status');
      const plateImage = document.getElementById('plateImage');
      
      // Show loading state
      btn.disabled = true;
      loader.style.display = 'block';
      plateDisplay.textContent = 'Processing...';
      plateStatus.textContent = 'Processing...';
      
      fetch('/capture')
        .then(response => {
          if (response.ok) {
            // Force refresh image
            plateImage.src = "http://)=====" + imageLink + R"=====(" + '?t=' + Date.now();
            setTimeout(() => location.reload(), 1500);
          } else {
            throw new Error('Capture failed');
          }
        })
        .catch(error => {
          console.error(error);
          plateDisplay.textContent = 'Error';
          plateStatus.textContent = 'Capture Failed';
          loader.style.display = 'none';
          btn.disabled = false;
        });
    }
  </script>
</body>
</html>
)=====";

  server.send(200, "text/html", html);
}


void handleCapture() {
  sendPhoto();
  
  server.send(200, "text/plain", "Capture completed");
}




// Function to extract a JSON string value by key
String extractJsonStringValue(const String& jsonString, const String& key) {
  int keyIndex = jsonString.indexOf(key);
  if (keyIndex == -1) {
    return "";
  }
  int startIndex = jsonString.indexOf(':', keyIndex) + 2;
  int endIndex = jsonString.indexOf('"', startIndex);
  if (startIndex == -1 || endIndex == -1) {
    return "";
  }
  return jsonString.substring(startIndex, endIndex);
}
// Function to display text on OLED
void displayText(String text) {

  Serial.println(text);
}
void setup() {
  // Disable brownout detector
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  Serial.begin(115200);
  pinMode(flashLight, OUTPUT);
  pinMode(triggerButton, INPUT);
  digitalWrite(flashLight, LOW);
  // Connect to WiFi
  WiFi.mode(WIFI_STA);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.print("ESP32-CAM IP Address: ");
  Serial.println(WiFi.localIP());
  // Configure camera
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  // Adjust frame size and quality based on PSRAM availability
  if (psramFound()) {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 5;  // Lower number means higher quality (0-63)
    config.fb_count = 2;
    Serial.println("PSRAM found");
  } else {
    config.frame_size = FRAMESIZE_CIF;
    config.jpeg_quality = 12;  // Lower number means higher quality (0-63)
    config.fb_count = 1;
  }
  // Initialize camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    delay(1000);
    ESP.restart();
  }
  // Initialize I2C with our defined pins
  I2Cbus.begin(I2C_SDA, I2C_SCL, 100000);
  // Initialize OLED display
  
  //server setup
  server.on("/", handleRoot);
  server.on("/capture", handleCapture);
  Serial.println(WiFi.localIP());
  // Start the server
  server.begin();
  Serial.println("Server started");
}
void loop() {
  // Check if trigger button is pressed
  
  server.handleClient();
  
}
// Function to capture and send photo to the server
void sendPhoto() {
  camera_fb_t* fb = NULL;
  // Turn on flash light and capture image
  // digitalWrite(flashLight, HIGH);
  delay(100);
  fb = esp_camera_fb_get();
  delay(100);
  if (!fb) {
    Serial.println("Camera capture failed");
  }
  // Display success message
  displayText("Image Capture Success");
  delay(300);
  // digitalWrite(flashLight, LOW);
  // Connect to server
  Serial.println("Connecting to server:" + serverName);
  displayText("Connecting to server:\n\n" + serverName);
  client.setInsecure();  // Skip certificate validation for simplicity
  if (client.connect(serverName.c_str(), serverPort)) {
    Serial.println("Connection successful!");
    displayText("Connection successful!");
    delay(300);
    displayText("Data Uploading !");
    // Increment count and prepare file name
    count++;
    Serial.println(count);
    String filename = apiKey + ".jpeg";
    // Prepare HTTP POST request
    String head = "--CircuitDigest\r\nContent-Disposition: form-data; name=\"imageFile\"; filename=\"" + filename + "\"\r\nContent-Type: image/jpeg\r\n\r\n";
    String tail = "\r\n--CircuitDigest--\r\n";
    uint32_t imageLen = fb->len;
    uint32_t extraLen = head.length() + tail.length();
    uint32_t totalLen = imageLen + extraLen;
    client.println("POST " + serverPath + " HTTP/1.1");
    client.println("Host: " + serverName);
    client.println("Content-Length: " + String(totalLen));
    client.println("Content-Type: multipart/form-data; boundary=CircuitDigest");
    client.println("Authorization:" + apiKey);
    client.println();
    client.print(head);
    // Send image data in chunks
    uint8_t* fbBuf = fb->buf;
    size_t fbLen = fb->len;
    for (size_t n = 0; n < fbLen; n += 1024) {
      if (n + 1024 < fbLen) {
        client.write(fbBuf, 1024);
        fbBuf += 1024;
      } else {
        size_t remainder = fbLen % 1024;
        client.write(fbBuf, remainder);
      }
    }
    client.print(tail);
    // Clean up
    esp_camera_fb_return(fb);
    displayText("Waiting For Response!");
    // Wait for server response
    String response;
    long startTime = millis();
    while (client.connected() && millis() - startTime < 5000) { // Modifify the Waiting time as per the response time
      if (client.available()) {
        char c = client.read();
        response += c;
      }
    }
    // Extract and display NPR data from response
    String NPRData = extractJsonStringValue(response, "\"number_plate\"");
    String imageLink1 = extractJsonStringValue(response, "\"view_image\"");
    Serial.print("NPR DATA: ");
    Serial.println(NPRData);
    Serial.print("ImageLink: ");
    Serial.println(imageLink1);
    NoPlateData=NPRData;
    if(NPRData!="NULL"){
      npState = true;
    }
    if(NPRData=="NULL"){
      npState = false;
    }
    imageLink=imageLink1;
    client.stop();
    esp_camera_fb_return(fb);
  } else {
    Serial.println("Connection to server failed");
    esp_camera_fb_return(fb);
  }
}
