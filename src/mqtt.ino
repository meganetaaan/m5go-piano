// https://www.1ft-seabass.jp/memo/2018/05/10/m5stack-meets-nodered-with-mqtt/

#include <WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <M5Stack.h>
#include <ArduinoJson.h>
 
// Wi-FiのSSID
char *ssid = "elecom2g-a4632d";
// Wi-Fiのパスワード
char *password = "9puchd99i9aw";
// MQTTの接続先のIP
const char *endpoint = "192.168.2.109";
// MQTTのポート
const int port = 1883;
// デバイスID
char *deviceID = "M5Stack";  // デバイスIDは機器ごとにユニークにします
// メッセージを知らせるトピック
char *pubTopic = "/pub/M5Stack";
// メッセージを待つトピック
char *subTopic = "/sub/M5Stack";
 
////////////////////////////////////////////////////////////////////////////////
   
WiFiClient httpsClient;
PubSubClient mqttClient(httpsClient);
   
void setup() {
    Serial.begin(115200);
     
    // Initialize the M5Stack object
    M5.begin();
 
    // START
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setCursor(10, 10);
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.setTextSize(3);
    M5.Lcd.printf("START");
     
    // Start WiFi
    Serial.println("Connecting to ");
    Serial.print(ssid);
    WiFi.begin(ssid, password);
   
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
 
    // WiFi Connected
    Serial.println("\nWiFi Connected.");
    M5.Lcd.setCursor(10, 40);
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.setTextSize(3);
    M5.Lcd.printf("WiFi Connected.");
     
    mqttClient.setServer(endpoint, port);
    mqttClient.setCallback(mqttCallback);
   
    connectMQTT();
}
   
void connectMQTT() {
    while (!mqttClient.connected()) {
        if (mqttClient.connect(deviceID)) {
            Serial.println("Connected.");
            int qos = 0;
            mqttClient.subscribe(subTopic, qos);
            Serial.println("Subscribed.");
        } else {
            Serial.print("Failed. Error state=");
            Serial.print(mqttClient.state());
            // Wait 5 seconds before retrying
            delay(5000);
        }
    }
}
   
long messageSentAt = 0;
int count = 0;
char pubMessage[128];
int led,red,green,blue;
   
void mqttCallback (char* topic, byte* payload, unsigned int length) {
 
    String str = "";
    Serial.print("Received. topic=");
    Serial.println(topic);
    for (int i = 0; i < length; i++) {
        Serial.print((char)payload[i]);
        str += (char)payload[i];
    }
    Serial.print("\n");
 
    StaticJsonBuffer<200> jsonBuffer;
     
    JsonObject& root = jsonBuffer.parseObject(str);
   
    // パースが成功したか確認。できなきゃ終了
    if (!root.success()) {
      Serial.println("parseObject() failed");
      return;
    }
    // JSONデータを割りあて
    const char* message = root["message"];
    led = root["led"];
    red = root["r"];
    green = root["g"];
    blue = root["b"];
 
    Serial.print("red = ");
    Serial.print(red);
    Serial.print(" green = ");
    Serial.println(green);
    Serial.print(" blue = ");
    Serial.println(blue);
 
    if( led == 1 ){
      // RGBカラー uint16_t に変換
      uint16_t RGB = red << 11 | green << 5 | blue;
      // 背景カラー反映
      M5.Lcd.fillRect(0, 0, 320, 240, RGB);
      // テキスト反映
      M5.Lcd.setCursor(10, 120);
      M5.Lcd.setTextColor(WHITE);
      M5.Lcd.setTextSize(5);
      M5.Lcd.printf(message);
    } else {
      // 消灯
      M5.Lcd.fillScreen(BLACK);
    }
 
    delay(300);
     
}
  
void mqttLoop() {
    if (!mqttClient.connected()) {
        connectMQTT();
    }
    mqttClient.loop();
}
 
void loop() {
 
  // 常にチェックして切断されたら復帰できるように
  mqttLoop();
 
  // 5秒ごとにメッセージを飛ばす
  long now = millis();
  if (now - messageSentAt > 33) {
      messageSentAt = now;
      sprintf(pubMessage, "{\"count\": %d}", count++);
      Serial.print("Publishing message to topic ");
      Serial.println(pubTopic);
      Serial.println(pubMessage);
      mqttClient.publish(pubTopic, pubMessage);
      Serial.println("Published.");
  }
}