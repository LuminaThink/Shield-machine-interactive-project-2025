#include <TFT_eSPI.h>
#include <PubSubClient.h>
#include <WiFi.h> // 👈 添加此行

// === WiFi 设置 ===
#define WIFI_SSID "lixunWLAN"        // 替换为您的WiFi名称
#define WIFI_PASSWORD "mqna8256" // 替换为您的WiFi密码

// === MQTT 设置 ===
#define MQTT_SERVER "sensecraft-mqtt-broker.sieeed.cc"
#define MQTT_PORT 1893
#define MQTT_TOPIC "sscm/a/o/device-9-17661244867/tx"

// === 屏幕与蜂鸣器 ===
TFT_eSPI tft;
#define BUZZER_PIN WIO_BUZZER

// === MQTT客户端 ===
WiFiClient wifiClient; // 👈 正确使用 WiFiClient
PubSubClient mqttClient(wifiClient);

void setup() {
  // 初始化屏幕
  tft.init();
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);
  
  // 初始化蜂鸣器
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  // 连接WiFi
  connectToWiFi();

  // 配置MQTT
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  mqttClient.setCallback(mqttCallback);
}

void loop() {
  if (!mqttClient.connected()) {
    reconnectMQTT();
  }
  
  mqttClient.loop();
}

// === WiFi连接函数 ===
void connectToWiFi() {
  Serial.println("Connecting to WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\nWiFi connected!");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

// === MQTT重连函数 ===
void reconnectMQTT() {
  while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    
    if (mqttClient.connect("WioTerminal")) {
      Serial.println("connected");
      mqttClient.subscribe(MQTT_TOPIC);
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

// === MQTT回调函数 ===
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String msg = "";
  for (int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }

  Serial.println("Received: " + msg);
  
  if (msg.indexOf("open") != -1) {
    triggerAlarm();
  }
}

// === 触发警报函数 ===
void triggerAlarm() {
  // 显示红色警报界面
  tft.fillScreen(TFT_RED);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(4);
  tft.setCursor(110, 80);
  tft.print("!");
  
  tft.setTextSize(2);
  tft.setCursor(40, 200);
  tft.println("DOOR OPEN!");
  
  // 播放专业警报音效】
  analogWrite(WIO_BUZZER, 128); delay(100);
  analogWrite(WIO_BUZZER, 0);   delay(50);
  
  analogWrite(WIO_BUZZER, 128); delay(100);
  analogWrite(WIO_BUZZER, 0);   delay(50);
  
  analogWrite(WIO_BUZZER, 200); delay(300);
  analogWrite(WIO_BUZZER, 0);   delay(200);
}