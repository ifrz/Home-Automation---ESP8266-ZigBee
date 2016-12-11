#include <SoftwareSerial.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>

//Not necessary when flash the node with esp-link firmware
//const char* ssid     = null;
//const char* password = null;
//const char* mqtt_server = "192.168.1.10";

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
char msgbuf[50];
float  tempC;

SoftwareSerial mySerial(13, 15); // RXD1 | TXD1

void setup_wifi() 
{  
  delay(10);

  // We start by connecting to a WiFi network

//  Serial.println();
//  Serial.print("Connecting to ");
//  Serial.println(ssid);
  
//  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void setup()
{
  //pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  // Set up both ports at 9600 baud. This value is most important
  // for the XBee. Make sure the baud rate matches the config
  // setting of your XBee.
  mySerial.begin(9600);  //XBee/UART1/pins 0 and 1
  Serial.begin(9600);   //USB
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266Client")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("/casa/sotano/garage/temp", "I'm alive!");
      // ... and resubscribe
      client.subscribe("/casa/sotano/garage/temp");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void parseData()
{
   byte packBytes[22];
   byte dataByte;
   float temp = 0.0;
   
   //Parse the XBee package, obtain the analogic data and device address
   if (mySerial.available()>=21)   //XBee/UART1/pins 0 and 1
   { // If data comes in from XBee, send it out to serial monitor
      Serial.write("Reading package.");  //Serial port
      if (mySerial.read() == 0x7E) 
      {
        Serial.print("7E,");
        packBytes[0] = 0x7E;
        for(int i=0;i<21;i++)
        {
          packBytes[i+1] = mySerial.read();
          Serial.print(packBytes[i+1],HEX); // debug
          Serial.print(",");         //
        }
        Serial.println();
        dataByte = packBytes[19] << 8 | packBytes[20];
        Serial.println(dataByte,HEX);
        temp = (float) dataByte * 5 / 1024;
        temp = temp * 50;
        Serial.println(temp);         //
        tempC = temp;
      }
   }
}

void loop()
{ 
  parseData();

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // publish sensor value every 4 secs
  long now = millis();
  if (now - lastMsg > 4000) {
    lastMsg = now;
    client.publish("/casa/sotano/garage/temp", dtostrf(tempC, 5, 2, msgbuf));
  }
   
}
