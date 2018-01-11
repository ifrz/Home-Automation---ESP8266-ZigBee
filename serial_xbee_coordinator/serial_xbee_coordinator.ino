#include <SoftwareSerial.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>

const char* mqtt_server = "10.10.66.200";

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
char msgbuf[50];
float tempC, Humidity, Lux = 0;
char state[10];
int stateAlarm;
byte pir = 0x0;
bool buzzState = LOW;

SoftwareSerial mySerial(13, 15); // RXD1 | TXD1

void setup_wifi()
{
   delay(10);

  // Only for debug purpouse, the WiFi connection is handled by esp-link firmware 

    Serial.println();
    Serial.print("Connecting to ");

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
  pinMode(D0, OUTPUT); //D0
  digitalWrite(D0, LOW);
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
  memset(state, 0, strlen(state));
  Serial.print(topic);
  for (int i = 0; i < length; i++) {
    state[i] = (char)payload[i];
  }
 // client.publish("/debug/node2", "Alarm state");
 // client.publish("/debug/node2", state);
  if (strcmp(state, "ON") == 0)
    stateAlarm = 1;
  else
    if (strcmp(state, "OFF") == 0)
      stateAlarm = 0;
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266Client")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("/casa/P0/temp", "I'm alive!");
      client.publish("/casa/P0/LuxDbg", "I'm alive!");
      client.publish("/casa/P0/humidity", "I'm alive!");
      client.publish("/casa/P0/alarm", "I'm alive!");
      // ... and resubscribe
      client.subscribe("/casa/P0/temp");
      client.subscribe("/casa/P0/LuxDbg");
      client.subscribe("/casa/P0/humidity");
      client.subscribe("/casa/P0/alarm");
	  // client.subscribe("/debug/node2");
    } 
    else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      client.publish("/debug/node2", "try again in 5 seconds");
      delay(5000);
    }
  }
}

void parseData()
{
  byte packBytes[22];
  int dataByteTmp, dataByteHum, dataByteLux;
  float temp, humidity, lux = 0.0;

  //Parse the XBee package, obtain the analogic data and device address
  if (mySerial.available() == 28) //XBee/UART1/pins 0 and 1
  {
    Serial.println("Reading package.");  //Serial port
    if (mySerial.read() == 0x7E)
    {
      packBytes[0] = 0x7E;
      for (int i = 0; i < 28; i++)
      {
        packBytes[i + 1] = mySerial.read();
      }
      pir = packBytes[20];
      Serial.println("Pir byte");
      Serial.println(pir, HEX);
      dataByteHum = ((int)packBytes[21]) * 256 + int(packBytes[22]); //AD0 -> HIH
      dataByteLux = ((int)packBytes[23]) * 256 + int(packBytes[24]); //AD1 -> Ldr
      dataByteTmp = ((int)packBytes[25]) * 256 + int(packBytes[26]); //AD3 -> Tmp

      humidity = dataByteHum * 100 / 1024.0;
      temp = (float) dataByteTmp * 1200 / 1024.0; // Vref=1,2 convert adc to miliV
      temp = (temp - 500) / 10; // miliV to degrees
      tempC = temp;
      Humidity = humidity;
      lux = (1023.0 - dataByteLux)*1.5;
      Lux = lux;
      Serial.println(lux);
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
  if (stateAlarm == 1) //this mqtt message is send by the client
  {
    if (pir == 0x10)
    {
      Serial.println("Detectado movimiento");
      digitalWrite(D0, HIGH);
      buzzState = HIGH;
    }
  }
  if ((stateAlarm == 0) && (buzzState == HIGH)) //this mqtt message is send by the client
  {
    digitalWrite(D0, LOW);
    buzzState = LOW;
    //pir = 0x0; 
  }
  // publish sensors values every 5 secs
  long now = millis();
  if (now - lastMsg > 29000) {
    lastMsg = now;
    client.publish("/casa/P0/temp", dtostrf(tempC, 0, 2, msgbuf));
    client.publish("/casa/P0/humidity", dtostrf(Humidity, 0, 2, msgbuf));
    dtostrf(Lux, 0, 2, msgbuf);
    client.publish("/casa/P0/LuxDbg", msgbuf);
  }
}
