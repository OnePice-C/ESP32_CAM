#include "esp_camera.h"
#include <WiFi.h>
#include "BlynkSimpleEsp32.h"
#include "esp_camera.h"
#include "fd_forward.h"
#include "fr_forward.h"
#include "fr_flash.h"

#define detection_on      '1'
#define recognition_on    'b'
#define eroll_on          'c'
#define delete_faceID     'd'

#define detection_off     'x'
#define recognition_off   'y'
#define eroll_off         'z'

#define CAMERA_MODEL_AI_THINKER // Has PSRAM
#define ENROLL_CONFIRM_TIMES 5
#define FACE_ID_SAVE_NUMBER 7

#include "camera_pins.h"

const char* ssid = "CIA";
const char* password = "12345678";
const char* auth = "SK3JUawyFvE2JOW_4qk1Ydv1LDv-Tt51";

void startCameraServer();
void delete_face_ID();

boolean matchFace = false;
int8_t Blynk_Pin_V1 = 0;
int8_t Blynk_Pin_V2 = 0;
int8_t Blynk_Pin_V3 = 0;
int8_t Blynk_Pin_V4 = 0;
extern int8_t detection_enabled;
extern int8_t is_enrolling;
extern int8_t recognition_enabled;
static face_id_list id_list = {0};

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();
  pinMode(4, OUTPUT);

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
  
  // if PSRAM IC present, init with UXGA resolution and higher JPEG quality
  //                      for larger pre-allocated frame buffer.
  if(psramFound()){
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

#if defined(CAMERA_MODEL_ESP_EYE)
  pinMode(13, INPUT_PULLUP);
  pinMode(14, INPUT_PULLUP);
#endif

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  sensor_t * s = esp_camera_sensor_get();
  // initial sensors are flipped vertically and colors are a bit saturated
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1); // flip it back
    s->set_brightness(s, 1); // up the brightness just a bit
    s->set_saturation(s, -2); // lower the saturation
  }
  // drop down frame size for higher initial frame rate
  s->set_framesize(s, FRAMESIZE_QVGA);

#if defined(CAMERA_MODEL_M5STACK_WIDE) || defined(CAMERA_MODEL_M5STACK_ESP32CAM)
  s->set_vflip(s, 1);
  s->set_hmirror(s, 1);
#endif

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  startCameraServer();

  Serial.print("Camera Ready! Use 'http://");
  Serial.print(WiFi.localIP());
  Serial.println("' to connect");

  Blynk.begin(auth, ssid, password, "blynk-server.com", 8080);
}

BLYNK_CONNECTED() {
  Blynk.syncAll();
}
BLYNK_WRITE(V1) {
  Blynk_Pin_V1 = param.asInt();
}
BLYNK_WRITE(V2) {
  Blynk_Pin_V2 = param.asInt();
}
BLYNK_WRITE(V3) {
  Blynk_Pin_V3 = param.asInt();
}
BLYNK_WRITE(V4) {
  Blynk_Pin_V4 = param.asInt();
}

void loop() {
  
  Blynk.run();
  
  if(Blynk_Pin_V1 == 1) {
    detection_enabled = 1;
    Serial.write(detection_on);
    delay(100); 
  }
  if(Blynk_Pin_V2 == 1){
    recognition_enabled = 1;
    Serial.write(recognition_on);
    delay(100); 
  }
  if(Blynk_Pin_V3 == 1 && Blynk_Pin_V2 == 1 && Blynk_Pin_V1 == 1){
    is_enrolling = 1;
    Serial.write(eroll_on);
    delay(100); 
  }
  if(Blynk_Pin_V4 == 1){
    delete_face_ID();
  }
  
  if(Blynk_Pin_V1 == 0){
    detection_enabled = 0;
    Serial.write(detection_off);
    delay(100);
  }
  if(Blynk_Pin_V2 == 0){
    recognition_enabled = 0;
    Serial.write(recognition_off);
    delay(100);
  }
   if(Blynk_Pin_V3 == 0){
    is_enrolling = 0;
    Serial.write(eroll_off);
    delay(100);
   }

  if(matchFace == true){
                Serial.write("pass");
                delay(100);
                digitalWrite(4, HIGH);
                delay(100);
                digitalWrite(4, LOW);
                matchFace = false;
  }
}

void delete_face_ID(){
  face_id_init(&id_list, FACE_ID_SAVE_NUMBER, ENROLL_CONFIRM_TIMES);
  read_face_id_from_flash(&id_list);// Read current face data from on-board flash
  Serial.println("Faces Read"); 
  while ( delete_face_id_in_flash(&id_list) > -1 ){
        Serial.println("Deleting Face");
  }
        Serial.println("All Deleted");
        Serial.write(delete_faceID);
        delay(100);
      
}
