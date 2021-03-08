#include <ESP8266WiFi.h>
#include <PubSubClient.h>
extern "C"
{
#include "libb64/cdecode.h"
}
#define LED 


const char *ssid = "";
const char *password = "";
const char *topic = "light";
const int LED_PIN = 14;

//USED FOR AWS STARTING HERE
const char *awsEndpoint = "xxxxxxxx-ats.iot.us-east-1.amazonaws.com";

// For the two certificate strings below paste in the text of your AWS
// device certificate and private key, comment out the BEGIN and END
// lines, add a quote character at the start of each line and a quote
// and backslash at the end of each line:

// xxxxxxxxxx-certificate.pem.crt
const String certificatePemCrt = \
//-----BEGIN CERTIFICATE-----
"";
//-----END CERTIFICATE-----

// xxxxxxxxxx-private.pem.key
const String privatePemKey = \
//-----BEGIN RSA PRIVATE KEY-----
"";
//-----END RSA PRIVATE KEY-----


const String caPemCrt = \
//-----BEGIN CERTIFICATE-----
"";
//-----END CERTIFICATE-----

//END OF AWS HERE

WiFiClientSecure wiFiClient;
void msgReceived(char *topic, byte *payload, unsigned int len);
void pubSubErr(int8_t MQTTErr);
PubSubClient pubSubClient(awsEndpoint, 8883, msgReceived, wiFiClient);

void setup()
{
  pinMode(LED_PIN, OUTPUT);
  
  Serial.begin(115200);
  Serial.println();
  Serial.println("LED TEST AWS IOT");

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
}

unsigned long lastPublish;
int msgCount;

void loop()
{
  pubSubCheckConnect();
}

void msgReceived(char *topic, byte *payload, unsigned int length)
{
  String message;
  
  for (int i = 0; i < length; i++)
  {
    message = message + (char)payload[i];
  }
  Serial.print("Message received on topic");
  Serial.print(topic);
  Serial.print(": ");
  Serial.println(message);

  if(message == "1") 
  {
    digitalWrite(LED_PIN,HIGH);
  } 
  else if(message == "0") 
  {
    digitalWrite(LED_PIN,LOW);
  } 
  else 
  {
    Serial.println("Error Invalid Message");
  }
}

void pubSubCheckConnect()
{
  if (!pubSubClient.connected())
  {
    Serial.print("PubSubClient connecting to: ");
    Serial.println(awsEndpoint);

    while (!pubSubClient.connected())
    {
      //CHANGE THIS STRING SO IT WONT DISCONNECT FROM AWS
      pubSubClient.connect("LightClient");
      pubSubErr(pubSubClient.state());
    }
    pubSubClient.subscribe(topic);
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
