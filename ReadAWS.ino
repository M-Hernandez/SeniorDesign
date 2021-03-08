#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <ArduinoJson.h>

extern "C"
{
#include "libb64/cdecode.h"
}
#define LED 


const char *ssid = "";
const char *password = "";

const char *humidity_topic = "humidity";
const char *temperature_topic = "temperature";
const char *gas_topic = "gas";
 
#define DHTTYPE DHT22
#define DHTPIN 4

// Find this awsEndpoint in the AWS Console: Manage - Things, choose your thing
// choose Interact, its the HTTPS Rest endpoint
const char *awsEndpoint = "xxxxxxxx-ats.iot.us-east-1.amazonaws.com";

// For the two certificate strings below paste in the text of your AWS
// device certificate and private key, comment out the BEGIN and END
// lines, add a quote character at the start of each line and a quote
// and backslash at the end of each line:

// xxxxxxxxxx-certificate.pem.crt
const String certificatePemCrt = \
//-----BEGIN CERTIFICATE-----

//-----END CERTIFICATE-----

// xxxxxxxxxx-private.pem.key
const String privatePemKey = \
//-----BEGIN RSA PRIVATE KEY-----

//-----END RSA PRIVATE KEY-----

// This is the AWS IoT CA Certificate from:
// https://docs.aws.amazon.com/iot/latest/developerguide/managing-device-certs.html#server-authentication
// This one in here is the 'RSA 2048 bit key: Amazon Root CA 1' which is valid
// until January 16, 2038 so unless it gets revoked you can leave this as is:
const String caPemCrt = \
//-----BEGIN CERTIFICATE-----

//-----END CERTIFICATE-----

WiFiClientSecure wiFiClient;

void pubSubErr(int8_t MQTTErr);

DHT dht(DHTPIN, DHTTYPE, 11);
PubSubClient pubSubClient(awsEndpoint, 8883, wiFiClient);

void setup()
{ 
  Serial.println(DHTPIN);
  Serial.begin(115200);
  Serial.println();
  Serial.println("Temp Gas Humidity TEST AWS IOT");

  //Connecting to the Wifi
  Serial.print("Connecting to ");
  Serial.print(ssid);
  WiFi.begin(ssid, password);
  WiFi.waitForConnectResult();
  Serial.print(", WiFi connected, IP address: ");
  Serial.println(WiFi.localIP());

  // get current time, otherwise certificates are flagged as expired
  setCurrentTime();

  //Setting the certificates for AWS
  uint8_t binaryCert[certificatePemCrt.length() * 3 / 4];
  int len = b64decode(certificatePemCrt, binaryCert);
  wiFiClient.setCertificate(binaryCert, len);
  uint8_t binaryPrivate[privatePemKey.length() * 3 / 4];
  len = b64decode(privatePemKey, binaryPrivate);
  wiFiClient.setPrivateKey(binaryPrivate, len);
  uint8_t binaryCA[caPemCrt.length() * 3 / 4];
  len = b64decode(caPemCrt, binaryCA);
  wiFiClient.setCACert(binaryCA, len);
  Serial.println("Entered AWS Credentials");

  dht.begin();
}

unsigned long lastPublish;

long lastMsg = 0;
float temp = 0.0;
float hum = 0.0;
float diff = .5;
int gas = 0;

bool checkBound(float newValue, float prevValue, float maxDiff) {
  return !isnan(newValue) && (newValue < prevValue - maxDiff || newValue > prevValue + maxDiff);
}

void loop()
{
  pubSubCheckConnect();

  delay(5000);

  float newTemp = dht.readTemperature(true);
  float newHum = dht.readHumidity();

  if (checkBound(newTemp, temp, diff)) {
    temp = newTemp;
    Serial.print("New temperature:");
    Serial.println(String(temp).c_str());
  }

  if (checkBound(newHum, hum, diff)) {
    hum = newHum;
    Serial.print("New humidity:");
    Serial.println(String(hum).c_str());
  }

  gas = analogRead(A0);

  StaticJsonBuffer<300> JSONbuffer;
  JsonObject& JSONencoder = JSONbuffer.createObject();
 
  JSONencoder[gas_topic] = gas;
  JSONencoder[temperature_topic] = temp;
  JSONencoder[humidity_topic] = hum;
  JSONencoder["deviceid"] = "EnvSystem";

  char JSONmessageBuffer[100];
  JSONencoder.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));

  Serial.println(JSONmessageBuffer);

  pubSubClient.publish("sensor", JSONmessageBuffer);
  pubSubClient.publish(gas_topic, String(gas).c_str());
  pubSubClient.publish(temperature_topic, String(temp).c_str());
  pubSubClient.publish(humidity_topic, String(hum).c_str());

}

void pubSubCheckConnect()
{
  if (!pubSubClient.connected())
  {
    Serial.print("PubSubClient connecting to: ");
    Serial.println(awsEndpoint);

    while (!pubSubClient.connected())
    {
      pubSubClient.connect("TempGasHum");
      pubSubErr(pubSubClient.state());
    }
  }
  pubSubClient.loop();
}

int b64decode(String b64Text, uint8_t *output)
{
  base64_decodestate s;
  base64_init_decodestate(&s);
  int cnt = base64_decode_block(b64Text.c_str(), b64Text.length(), (char *)output, &s);
  return cnt;
}

void setCurrentTime()
{
  configTime(3 * 3600, 0, "pool.ntp.org", "time.nist.gov");

  Serial.print("Waiting for NTP time sync: ");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2)
  {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println("");
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  Serial.print("Current time: ");
  Serial.print(asctime(&timeinfo));
}

void pubSubErr(int8_t MQTTErr)
{
  if (MQTTErr == MQTT_CONNECTION_TIMEOUT)
    Serial.println("Connection timeout");
  else if (MQTTErr == MQTT_CONNECTION_LOST)
    Serial.println("Connection lost");
  else if (MQTTErr == MQTT_CONNECT_FAILED)
    Serial.println("Connect failed");
  else if (MQTTErr == MQTT_DISCONNECTED)
    Serial.println("Disconnected");
  else if (MQTTErr == MQTT_CONNECTED)
    Serial.println("Connected");
  else if (MQTTErr == MQTT_CONNECT_BAD_PROTOCOL)
    Serial.println("Connect bad protocol");
  else if (MQTTErr == MQTT_CONNECT_BAD_CLIENT_ID)
    Serial.println("Connect bad Client-ID");
  else if (MQTTErr == MQTT_CONNECT_UNAVAILABLE)
    Serial.println("Connect unavailable");
  else if (MQTTErr == MQTT_CONNECT_BAD_CREDENTIALS)
    Serial.println("Connect bad credentials");
  else if (MQTTErr == MQTT_CONNECT_UNAUTHORIZED)
    Serial.println("Connect unauthorized");
}
