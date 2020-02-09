#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <PubSubClient.h>

IPAddress broker(192, 168, 2, 101); // Casa

const int relayPin = 0;
const int ledPin = LED_BUILTIN;

const char* ssid     = "Papillon"; //Naviter"; //
const char* password = "70445312"; //N4v1t3rWIFI2015"; //

const char* devID = "Outlet1";

const char* devTopic = "PapillonIoT/Outlet1/+";

const unsigned long debounceDelay = 50;    // the debounce time; increase if the output flickers

int status = WL_IDLE_STATUS;   // the Wifi radio's status

// Initialize the Ethernet client object
WiFiClient WIFIclient;

PubSubClient client(WIFIclient);

bool estadoEnchufe = false;
bool viejoEstadoEnchufe = false;

// Declare functions
void controlEnchufe (bool OnOff);

/////////////////////////////////////
//
// MQTT FUNCTIONS
//
/////////////////////////////////////

//print any message received for subscribed topic
void callback(char* topic, byte* payload, unsigned int length) {
  String sTopic = topic;
  String sCommand = sTopic.substring(sTopic.lastIndexOf("/") + 1);

  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  String sPayload = "";
  for (unsigned int i=0; i < length; i++) {
    sPayload += (char)payload[i];
  }
  Serial.println(sPayload);

  Serial.println("Command: " + sCommand);

  if (sCommand == "set") {
      if (sPayload == "ON") {
        controlEnchufe(true);
      } else {
        controlEnchufe(false);
      }
  }

}

bool mqttReconnect() {
  int tries = 0;
  // Loop until we're reconnected
  while (!client.connected() && (tries < 10)) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect, just a name to identify the client
    if (client.connect(devID)) { //}, devUS, devPW)) {
      Serial.println("connected");
      // ... and resubscribe
      client.subscribe(devTopic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 3 seconds");
      // Wait 5 seconds before retrying
      delay(3000);
      tries++;
    }
  }
  if (tries == 10) {
    Serial.println("Too many trials, no MQTT connection was possible");
    return false;
  } else {
    return true;
  }
}

bool connectNetwork(bool outDebug = true) {
  int tries = 0;

  if (outDebug) Serial.println("Trying Main WiFi");
  while ((WiFi.status() != WL_CONNECTED) && (tries < 10)) {
    delay(500);
    if (outDebug) Serial.print(".");
    tries++;
  }
  if (outDebug) Serial.println();
    
  if (tries == 10) {
    Serial.println("Too many trials, no WiFi connection was possible");
    return false;
  } else {
    if (outDebug) Serial.println("WiFi connected");  
    if (outDebug) Serial.println("IP address: ");
    if (outDebug) Serial.println(WiFi.localIP());
  
    return mqttReconnect();
  }
}

void notificarBroker(bool OnOff) {
  String payload = "{\"state\":";
  if (OnOff)
    payload += "\"ON\"";
  else
    payload += "\"OFF\"";
  payload += "}";

  Serial.println(payload);
 
  client.publish("PapillonIoT/Lamp1/status", (char*) payload.c_str());

}

/////////////////////////////////////
//
// CUSTOM HW FUNCITONS
//
/////////////////////////////////////

void controlLED (bool OnOff)
{
  if (OnOff) {
    Serial.println("Encender LED");
    digitalWrite(LED_BUILTIN, LOW);   // turn the LED on (LOW is the voltage level)
  }
  else {
    Serial.println("Apagar LED");
    digitalWrite(LED_BUILTIN, HIGH);   // turn the LED off (HIGH is the voltage level)    
  }
}

void controlRelay (bool OnOff)
{
  if (OnOff) {
    Serial.println("Encender Relay");
    digitalWrite(relayPin, HIGH);   // turn the RELAY on (HIGH is the voltage level)    
  } 
  else {
    Serial.println("Apagar Relay");
    digitalWrite(relayPin, LOW);   // turn the RELAY off (LOW is the voltage level)
  }
}

void controlEnchufe (bool OnOff)
{
  estadoEnchufe = OnOff;

  if (OnOff) {
    Serial.println("Encender Enchufe");
    controlLED(true);
    controlRelay(true);
  }
  else {
    Serial.println("Apagar Enchufe");
    controlLED(false);
    controlRelay(false);
  }

  notificarBroker(estadoEnchufe);
}

/////////////////////////////////////
//
// Setup board
//
/////////////////////////////////////

void setup() {
  // initialize serial for debugging
  Serial.begin(115200);
  Serial.println("<<<<< SETUP START >>>>>");
  // initialize WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  // configure MQTT server
  client.setServer(broker, 1883);
  client.setCallback(callback);

  if (connectNetwork()) {
    Serial.println("Network OK");
  } else {
    Serial.println("Network Problem. We will try again in Loop");
  }

  // Set Relay
  pinMode(relayPin, OUTPUT);

  // initialize digital pin LED_BUILTIN as an output.
  pinMode(ledPin, OUTPUT);

  // Estado inicial
  controlEnchufe(estadoEnchufe);
  Serial.println("<<<<< SETUP END >>>>>");
}

/////////////////////////////////////
//
// Process loop
//
/////////////////////////////////////

void loop() {
  if (connectNetwork(false)) {
    // Serial.println("Loop with Network OK");
    client.loop();
  } else {
    Serial.println("Loop with Network Problem. We will try again in next Loop");
  }
}
