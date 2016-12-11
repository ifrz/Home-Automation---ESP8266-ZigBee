#include <PubSubClient.h>
#include <ESP8266WiFi.h>

//const char* ssid     = "";
//const char* password = "";
//const char* mqtt_server = "192.168.1.10";

WiFiClient espClient;
PubSubClient client(espClient);

char state[10];

void setup_wifi() 
{  
  delay(10);

  // We start by connecting to a WiFi network

  //Serial.println();
  //Serial.print("Connecting to ");
  //Serial.println(ssid);
  
  //WiFi.begin(ssid, password);
  
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
  pinMode(5, OUTPUT);
  digitalWrite(5, LOW);
  Serial.begin(9600);   //USB
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  memset(state,0,strlen(state));
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    state[i] = (char)payload[i];
  }
  Serial.println(state);
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
      client.publish("/casa/sotano/garage/light", "I'm alive!");
      // ... and resubscribe
      client.subscribe("/casa/sotano/garage/light");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void loop() 
{
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  if (strcmp(state, "ON") == 0) //this mqtt message is send by the client
  {
    Serial.println("Encender");
    digitalWrite(5, HIGH);
  }
  else digitalWrite(5, LOW);  
}
