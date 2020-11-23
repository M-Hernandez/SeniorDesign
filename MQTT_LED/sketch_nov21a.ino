#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#define LED 2
 
//Enter your wifi credentials
const char* ssid = "Wifi Name";  
const char* password =  "Wifi Password";

//Enter your mqtt server configurations
const char* mqttServer = "192.168.68.118";    //Enter Your mqttServer address
const int mqttPort = 1883;       //Port number
const char* mqttUser = "HA"; //User
const char* mqttPassword = "12345"; //Password

const int LED_PIN = 14;

WiFiClient espClient;
PubSubClient client(espClient);
 
void setup() {
  pinMode(LED_PIN,OUTPUT);
  
  Serial.begin(115200);
 
  WiFi.begin(ssid, password);
 
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi..");
  }
  
  Serial.print("Connected to WiFi :");
  Serial.println(WiFi.SSID());
 
  client.setServer(mqttServer, mqttPort);
  client.setCallback(MQTTcallback);
 
  while (!client.connected()) {
    Serial.println("Connecting to MQTT... ");

    if (client.connect("ESP8266", mqttUser, mqttPassword )) {
 
      Serial.print("connected: ");
      Serial.println(mqttServer);  
 
    } else {
 
      Serial.print("failed with state ");
      Serial.println(client.state());  //If you get state 5: mismatch in configuration
      delay(2000);
 
    }
  }
 
  client.publish("esp/test", "Hello from ESP8266");
  client.subscribe("esp/test");
}
 
void MQTTcallback(char* topic, byte* payload, unsigned int length) {
 
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
 
  Serial.print("Message:");

  String message;
  for (int i = 0; i < length; i++) {
    message = message + (char)payload[i];  //Conver *byte to String
  }
   Serial.println(message);
  if(message == "#on") {
   Serial.println("Turning ON");
   digitalWrite(LED_PIN,HIGH);
  } 
  if(message == "#off") {
    Serial.println("Turning OFF");
   digitalWrite(LED_PIN,LOW);
  }

  Serial.println();
  Serial.println("-----------------------");  
}
 
void loop() {
  client.loop();
}
