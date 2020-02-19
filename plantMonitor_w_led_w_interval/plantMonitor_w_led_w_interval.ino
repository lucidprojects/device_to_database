// IoT Workshop
// Send temperature and humidity data to MQTT
//
// WiFiNINA https://www.arduino.cc/en/Reference/WiFiNINA (MKR WiFi 1010)
// Arduino MKR ENV https://www.arduino.cc/en/Reference/ArduinoMKRENV
// Arduino MQTT Client  https://github.com/arduino-libraries/ArduinoMqttClient

#include <WiFiNINA.h>
#include <Arduino_MKRENV.h>
#include <ArduinoMqttClient.h>

#include "config.h"

WiFiSSLClient net;
MqttClient mqtt(net);

String temperatureTopic = "itp/" + DEVICE_ID + "/temperature";
String humidityTopic = "itp/" + DEVICE_ID + "/humidity";
String pressureTopic = "itp/" + DEVICE_ID + "/pressure";
String illuminanceTopic = "itp/" + DEVICE_ID + "/illuminance";

String uvaTopic = "itp/" + DEVICE_ID + "/uva";
String uvbTopic = "itp/" + DEVICE_ID + "/uvb";
String soilTopic = "itp/" + DEVICE_ID + "/soil";

String ledTopic = "itp/" + DEVICE_ID + "/led";

String configIntervalTopic = "itp/" + DEVICE_ID + "/configInterval";

int configInterval = 10;


// Publish every 10 seconds for the workshop. Real world apps need this data every 5 or 10 minutes.
//unsigned long publishInterval;
unsigned long publishInterval = configInterval * 1000; // realworld wayt to often
unsigned long lastMillis = 0;



const int ledPin = 5;


void setup() {
  Serial.begin(9600);

  // Wait for a serial connection
  // while (!Serial) { } // need to comment this out when not connected to computer

  // initialize the shield
  if (!ENV.begin()) {
    Serial.println("Failed to initialize MKR ENV shield!");
    while (1);
  }


  // initialize ledPin as an output.
  pinMode(ledPin, OUTPUT);

  Serial.println("Connecting WiFi");
  connectWiFi();

  // define function for incoming MQTT messages
  mqtt.onMessage(messageReceived);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();  
  }

  if (!mqtt.connected()) {
    connectMQTT();
  }
  

  // poll for new MQTT messages and send keep alives
  mqtt.poll();  // if we didn't call this we may miss incomming msgs and out going msgs might get stuck

  if (millis() - lastMillis > publishInterval) {
    lastMillis = millis();

    float temperature = ENV.readTemperature(FAHRENHEIT);
    float humidity = ENV.readHumidity();

    float pressure    = ENV.readPressure();                // kPa
    float illuminance = ENV.readIlluminance();             // lux
    float uva         = ENV.readUVA();                     // μW/cm2
    float uvb         = ENV.readUVB();                     // μW/cm2
    //    float uvindex     = ENV.readUVIndex();                 // 0 - 11
    int sensorValue = analogRead(A0);

    Serial.println();
    Serial.print("### New Readings ###");
    Serial.println();

    Serial.print(temperature);
    Serial.println("°F ");
    Serial.print(humidity);
    Serial.println("% RH");

    Serial.print(pressure);
    Serial.println(" kPa");
    Serial.print(illuminance);
    Serial.println(" lux");

    Serial.print(uva);
    Serial.println(" uva μW/cm2");
    Serial.print(uvb);
    Serial.println(" uvb μW/cm2 ");

    Serial.print("Soil Moisture ");
    Serial.println(sensorValue);


    mqtt.beginMessage(temperatureTopic);
    mqtt.print(temperature);
    mqtt.endMessage();

    mqtt.beginMessage(humidityTopic);
    mqtt.print(humidity);
    mqtt.endMessage();

    mqtt.beginMessage(pressureTopic);
    mqtt.print(pressure);
    mqtt.endMessage();

    mqtt.beginMessage(illuminanceTopic);
    mqtt.print(illuminance);
    mqtt.endMessage();

    mqtt.beginMessage(uvaTopic);
    mqtt.print(uva);
    mqtt.endMessage();

    mqtt.beginMessage(uvbTopic);
    mqtt.print(uvb);
    mqtt.endMessage();

    mqtt.beginMessage(soilTopic);
    mqtt.print(sensorValue);
    mqtt.endMessage();



  }
}

void connectWiFi() {
  // Check for the WiFi module
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    while (true);
  }

  Serial.print("WiFi firmware version ");
  Serial.println(WiFi.firmwareVersion());

  Serial.print("Attempting to connect to SSID: ");
  Serial.print(WIFI_SSID);
  Serial.print(" ");

  while (WiFi.begin(WIFI_SSID, WIFI_PASSWORD) != WL_CONNECTED) {
    // failed, retry
    Serial.print(".");
    delay(3000);
  }

  Serial.println("Connected to WiFi");
  printWiFiStatus();

}

void connectMQTT() {
  Serial.print("Connecting MQTT...");
  mqtt.setId(DEVICE_ID);
  mqtt.setUsernamePassword(MQTT_USER, MQTT_PASSWORD);

  while (!mqtt.connect(MQTT_BROKER, MQTT_PORT)) {
    Serial.print(".");
    delay(5000);
  }
  mqtt.subscribe(ledTopic);
  mqtt.subscribe(configIntervalTopic);
  Serial.println("connected.");
}

void printWiFiStatus() {
  // print your WiFi IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
}

void messageReceived(int messageSize) {
  String topic = mqtt.messageTopic();
  String payload = mqtt.readString();
  Serial.println("incoming: " + topic + " - " + messageSize + " bytes ");
  Serial.println(payload);

  if (topic == "itp/device_19/led") {
    if (payload.equalsIgnoreCase("ON")) {
      analogWrite(ledPin, 255);  // need analog write for older firmware
    } else if (payload.equalsIgnoreCase("OFF")) {
      analogWrite(ledPin, LOW);
    } else {

      // see if we have a brightness value
      int percent = payload.toInt();
      // check the range
      if (percent < 0) {
        percent = 0;
      } else if (percent > 100) {
        percent = 100;
      }
      // map brightness of 0 to 100 to 1 byte value 0x00 to 0xFF
      int brightness = map(percent, 0, 100, 0, 255);
      analogWrite(ledPin, brightness);
    }
  }

  if (topic == "itp/device_19/configInterval"){
    Serial.println("receiving configInterval");
    configInterval = payload.toInt();
    publishInterval = configInterval * 1000;
  }
  
}
