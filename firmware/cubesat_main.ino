#include "esp_camera.h"
#include "esp_http_server.h"
#include "esp_timer.h"
#include "img_converters.h"
#include "fb_gfx.h"
#include "esp32-hal-ledc.h"
#include <WiFi.h>
#include <Wire.h>

// ===========================
// WiFi Credentials
// ===========================
const char *ssid = "<wifi_name>";
const char *password = "<wifi_password>";

// ===========================
// MPU6050 Configuration
// ===========================
const int MPU_ADDR = 0x68;
const int SDA_PIN = 15;
const int SCL_PIN = 14;

// ===========================
// Global MPU6050 Data
// (updated in loop, read by web handlers)
// ===========================
float g_pitch = 0.0, g_roll = 0.0;
float g_axg = 0.0, g_ayg = 0.0, g_azg = 0.0;

// ===========================
// L298N Magnetorquer Pin Configuration
// ===========================
#define IN1_X_AXIS    12
#define IN2_X_AXIS    13
#define IN3_Y_AXIS    2
#define IN4_Y_AXIS    4
#define ENABLE_MAGNETORQUERS false

// ===========================
// Control Parameters
// ===========================
#define TARGET_PITCH   0.0
#define TARGET_ROLL    0.0
#define DEADBAND       5.0

// ===========================
// Camera Pin Configuration (AI-THINKER ESP32-CAM)
// ===========================
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

// ===========================
// Camera Server Variables
// ===========================
#define PART_BOUNDARY "123456789000000000000987654321"
static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

httpd_handle_t stream_httpd = NULL;
httpd_handle_t data_httpd   = NULL;

// ===========================
// MPU6050 Write Helper
// ===========================
void writeMPU(byte reg, byte data) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(reg);
  Wire.write(data);
  Wire.endTransmission(true);
}

// ===========================
// Magnetorquer Functions
// ===========================
void stopMagnetorquer(char axis) {
  if (axis == 'X') { digitalWrite(IN1_X_AXIS, LOW); digitalWrite(IN2_X_AXIS, LOW); }
  else if (axis == 'Y') { digitalWrite(IN3_Y_AXIS, LOW); digitalWrite(IN4_Y_AXIS, LOW); }
}

void setXMagnetorquer(int dir) {
  if (dir > 0)       { digitalWrite(IN1_X_AXIS, HIGH); digitalWrite(IN2_X_AXIS, LOW); }
  else if (dir < 0)  { digitalWrite(IN1_X_AXIS, LOW);  digitalWrite(IN2_X_AXIS, HIGH); }
  else stopMagnetorquer('X');
}

void setYMagnetorquer(int dir) {
  if (dir > 0)       { digitalWrite(IN3_Y_AXIS, HIGH); digitalWrite(IN4_Y_AXIS, LOW); }
  else if (dir < 0)  { digitalWrite(IN3_Y_AXIS, LOW);  digitalWrite(IN4_Y_AXIS, HIGH); }
  else stopMagnetorquer('Y');
}

void initMagnetorquers() {
  if (!ENABLE_MAGNETORQUERS) { Serial.println("Magnetorquers DISABLED"); return; }
  pinMode(IN1_X_AXIS, OUTPUT); pinMode(IN2_X_AXIS, OUTPUT);
  pinMode(IN3_Y_AXIS, OUTPUT); pinMode(IN4_Y_AXIS, OUTPUT);
  stopMagnetorquer('X'); stopMagnetorquer('Y');
  Serial.println("Magnetorquers initialized");
}

int calculateControl(float error) {
  if (abs(error) < DEADBAND) return 0;
  return (error > 0) ? 1 : -1;
}

// ===========================
// Camera Stream Handler ( / )
// ===========================
static esp_err_t stream_handler(httpd_req_t *req) {
  camera_fb_t *fb = NULL;
  esp_err_t res = ESP_OK;
  size_t _jpg_buf_len = 0;
  uint8_t *_jpg_buf = NULL;
  char part_buf[64];

  res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
  if (res != ESP_OK) return res;

  while (true) {
    fb = esp_camera_fb_get();
    if (!fb) {
      res = ESP_FAIL;
    } else {
      if (fb->format != PIXFORMAT_JPEG) {
        bool ok = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
        esp_camera_fb_return(fb); fb = NULL;
        if (!ok) res = ESP_FAIL;
      } else {
        _jpg_buf_len = fb->len;
        _jpg_buf = fb->buf;
      }
    }
    if (res == ESP_OK) {
      size_t hlen = snprintf(part_buf, 64, _STREAM_PART, _jpg_buf_len);
      res = httpd_resp_send_chunk(req, part_buf, hlen);
    }
    if (res == ESP_OK) res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
    if (res == ESP_OK) res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
    if (fb) { esp_camera_fb_return(fb); fb = NULL; _jpg_buf = NULL; }
    else if (_jpg_buf) { free(_jpg_buf); _jpg_buf = NULL; }
    if (res != ESP_OK) break;
  }
  return res;
}

// ===========================
// MPU Data JSON Handler ( /data )
// ===========================
static esp_err_t data_handler(httpd_req_t *req) {
  char json[200];
  snprintf(json, sizeof(json),
    "{\"pitch\":%.2f,\"roll\":%.2f,\"ax\":%.3f,\"ay\":%.3f,\"az\":%.3f}",
    g_pitch, g_roll, g_axg, g_ayg, g_azg);
  httpd_resp_set_type(req, "application/json");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  httpd_resp_sendstr(req, json);
  return ESP_OK;
}

// ===========================
// Dashboard HTML Handler ( /dashboard )
// ===========================
static esp_err_t dashboard_handler(httpd_req_t *req) {
  const char* html = R"rawhtml(
<!DOCTYPE html><html>
<head>
<title>CubeSat Dashboard</title>
<meta name="viewport" content="width=device-width, initial-scale=1">
<style>
  body{font-family:monospace;background:#111;color:#0f0;margin:0;padding:20px;}
  h2{color:#fff;margin-top:0;}
  img{width:320px;height:240px;border:2px solid #0f0;display:block;}
  #data{margin-top:16px;font-size:16px;line-height:2;}
  .label{color:#aaa;}
  .val{color:#0ff;font-weight:bold;}
  .status{font-size:12px;color:#555;margin-top:8px;}
</style>
</head>
<body>
<h2>CubeSat Dashboard</h2>
<img src="/" />
<div id="data">Loading sensor data...</div>
<div class="status" id="status"></div>
<script>
  function update() {
    fetch('http://' + window.location.hostname + ':81/data')
      .then(r => r.json())
      .then(d => {
        document.getElementById('data').innerHTML =
          '<span class="label">Pitch: </span><span class="val">' + d.pitch.toFixed(2) + '&deg;</span><br>' +
          '<span class="label">Roll:  </span><span class="val">' + d.roll.toFixed(2) + '&deg;</span><br>' +
          '<span class="label">Accel X: </span><span class="val">' + d.ax.toFixed(3) + ' g</span><br>' +
          '<span class="label">Accel Y: </span><span class="val">' + d.ay.toFixed(3) + ' g</span><br>' +
          '<span class="label">Accel Z: </span><span class="val">' + d.az.toFixed(3) + ' g</span>';
        document.getElementById('status').innerText = 'Last update: ' + new Date().toLocaleTimeString();
      })
      .catch(() => {
        document.getElementById('status').innerText = 'Sensor read failed - retrying...';
      });
  }
  setInterval(update, 500);
  update();
</script>
</body></html>)rawhtml";

  httpd_resp_set_type(req, "text/html");
  httpd_resp_sendstr(req, html);
  return ESP_OK;
}

// ===========================
// Start Camera Server
// ===========================
void startCameraServer() {
  // --- Port 80: stream + dashboard ---
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = 80;

  httpd_uri_t stream_uri = { .uri="/",          .method=HTTP_GET, .handler=stream_handler,    .user_ctx=NULL };
  httpd_uri_t dash_uri   = { .uri="/dashboard", .method=HTTP_GET, .handler=dashboard_handler, .user_ctx=NULL };

  if (httpd_start(&stream_httpd, &config) == ESP_OK) {
    httpd_register_uri_handler(stream_httpd, &stream_uri);
    httpd_register_uri_handler(stream_httpd, &dash_uri);
  }

  // --- Port 81: sensor data only (separate from stream to fix Safari) ---
  httpd_config_t config2 = HTTPD_DEFAULT_CONFIG();
  config2.server_port = 81;
  config2.ctrl_port   = 32769; // must differ from port 80 server

  httpd_uri_t data_uri = { .uri="/data", .method=HTTP_GET, .handler=data_handler, .user_ctx=NULL };

  if (httpd_start(&data_httpd, &config2) == ESP_OK) {
    httpd_register_uri_handler(data_httpd, &data_uri);
    Serial.println("Camera server started");
  }
}

// ===========================
// Setup
// ===========================
void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(false);
  delay(500);
  Serial.println();
  Serial.println("=== CubeSat Booting ===");

  // I2C + MPU6050
  Wire.begin(SDA_PIN, SCL_PIN);
  delay(250);
  writeMPU(0x6B, 0x80); delay(100); // reset
  writeMPU(0x6B, 0x00); delay(100); // wake
  writeMPU(0x1C, 0x00); delay(10);  // ±2g
  writeMPU(0x19, 0x07); delay(10);  // sample rate
  writeMPU(0x1A, 0x06); delay(10);  // DLPF

  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x75);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, 1, true);
  if (Wire.available()) {
    byte whoami = Wire.read();
    if (whoami == 0x68) Serial.println("✓ MPU6050 ready");
    else { Serial.print("✗ MPU6050 WHO_AM_I error: 0x"); Serial.println(whoami, HEX); }
  } else {
    Serial.println("✗ MPU6050 not responding");
  }

  // Magnetorquers
  initMagnetorquers();

  // Camera
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer   = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM; config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM; config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM; config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM; config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk      = XCLK_GPIO_NUM;
  config.pin_pclk      = PCLK_GPIO_NUM;
  config.pin_vsync     = VSYNC_GPIO_NUM;
  config.pin_href      = HREF_GPIO_NUM;
  config.pin_sccb_sda  = SIOD_GPIO_NUM;
  config.pin_sccb_scl  = SIOC_GPIO_NUM;
  config.pin_pwdn      = PWDN_GPIO_NUM;
  config.pin_reset     = RESET_GPIO_NUM;
  config.xclk_freq_hz  = 20000000;
  config.frame_size    = FRAMESIZE_UXGA;
  config.pixel_format  = PIXFORMAT_JPEG;
  config.grab_mode     = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location   = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality  = 12;
  config.fb_count      = 1;

  if (config.pixel_format == PIXFORMAT_JPEG) {
    if (psramFound()) {
      config.jpeg_quality = 10;
      config.fb_count = 2;
      config.grab_mode = CAMERA_GRAB_LATEST;
    } else {
      config.frame_size = FRAMESIZE_SVGA;
      config.fb_location = CAMERA_FB_IN_DRAM;
    }
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("✗ Camera init failed: 0x%x\n", err);
    return;
  }
  Serial.println("✓ Camera ready");

  sensor_t *s = esp_camera_sensor_get();
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1);
    s->set_brightness(s, 1);
    s->set_saturation(s, -2);
  }
  s->set_framesize(s, FRAMESIZE_QVGA);

  // WiFi
  WiFi.begin(ssid, password);
  WiFi.setSleep(false);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println();
  Serial.println("✓ WiFi connected");

  startCameraServer();

  Serial.println("===========================================");
  Serial.print("Camera stream:  http://"); Serial.println(WiFi.localIP());
  Serial.print("Dashboard:      http://"); Serial.print(WiFi.localIP()); Serial.println("/dashboard");
  Serial.print("Sensor JSON:    http://"); Serial.print(WiFi.localIP()); Serial.println("/data");
  Serial.println("===========================================");
}

// ===========================
// Loop - Read MPU, update globals
// ===========================
void loop() {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);
  byte error = Wire.endTransmission(false);

  if (error != 0) {
    Serial.println("✗ MPU6050 comm error");
    stopMagnetorquer('X'); stopMagnetorquer('Y');
    delay(500);
    return;
  }

  Wire.requestFrom(MPU_ADDR, 6, true);
  if (Wire.available() >= 6) {
    int16_t ax = (Wire.read() << 8) | Wire.read();
    int16_t ay = (Wire.read() << 8) | Wire.read();
    int16_t az = (Wire.read() << 8) | Wire.read();

    g_axg = ax / 16384.0;
    g_ayg = ay / 16384.0;
    g_azg = az / 16384.0;

    g_pitch = atan2(g_axg, sqrt(g_ayg * g_ayg + g_azg * g_azg)) * 180.0 / PI;
    g_roll  = atan2(g_ayg, sqrt(g_axg * g_axg + g_azg * g_azg)) * 180.0 / PI;

    int xDir = calculateControl(TARGET_PITCH - g_pitch);
    int yDir = calculateControl(TARGET_ROLL  - g_roll);

    if (ENABLE_MAGNETORQUERS) { setXMagnetorquer(xDir); setYMagnetorquer(yDir); }

    // Serial monitor still works as before
    Serial.printf("Pitch: %.2f° | Roll: %.2f° | Accel: %.3fg, %.3fg, %.3fg\n",
                  g_pitch, g_roll, g_axg, g_ayg, g_azg);
  } else {
    Serial.println("✗ MPU6050 read error");
    stopMagnetorquer('X'); stopMagnetorquer('Y');
  }

  delay(100);
}