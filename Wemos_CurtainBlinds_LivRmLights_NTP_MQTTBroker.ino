/*
 Basic ESP8266 MQTT example
 This sketch demonstrates the capabilities of the pubsub library in combination
 with the ESPxxx Wemos board.
 It will reconnect to the server if the connection is lost using a blocking
 reconnect function. See the 'mqtt_reconnect_nonblocking' example for how to
 achieve the same result without blocking the main loop.
*/
//Im using Wemos D1 R1 - set this settings when compiling
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>


#include <dht.h>

// Update these with values suitable for your network.

const char* ssid = "JioFiber-24_EXT";
const char* password = "xxx";  
WiFiUDP ntpUDP;

NTPClient timeClient(ntpUDP);

const char* mqtt_server = "m12.cloudmqtt.com";

// Generally, you should use "unsigned long" for variables that hold time
// The value will quickly become too large for an int to store
unsigned long previousMillis = 0; 
const long interval = 1000;           // interval at which to blink (milliseconds)


WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;
#define CONTROL_BLINDS_IN_1   16 // D0
#define CONTROL_BLINDS_IN_2   14 // D5
#define CONTROL_BLINDS_EN_A   12 // D6

#define CONTROL_LIV_ROOM_LIGHTS  13 // also known as D7 ( thanks to Daksh )

#define DHT11_PIN 12
dht DHT;
int lights_on=1; // global variable 

void setup_wifi() {
  String newHostname = "Curtain_Wemos_D1R1";

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  // don't touch as there is no relay here ! 
  // shashi - Sep 21st 2022 digitalWrite(CONTROL_BLINDS, HIGH); // turn off the Control lights to begin with, HIGH is off !!
  //Serial.println ("Close off ");
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.hostname (newHostname.c_str());

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println(WiFi.hostname().c_str());
  
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  String topicStr = topic;
  
  // Do a string compare of what we got, since we got a character string, and we have to 
  // compare, convert character string to a String class and do the comparision.
  
  if ( topicStr.compareTo( "inBlindsControl") == 0 )  {
      Serial.println (" Shashi : inBlindsControl is the topic"); 
      for (int i = 0; i < length; i++) {
        Serial.print((char)payload[i]);
      }
      Serial.println();
    
      // Switch on the LED if an 1 was received as first character
      if ((char)payload[0] == '1') { //CW - raise
        Serial.println("Setting CONTROL_BLINDS_IN1 Pin 12 to HIGH, 14 to LOW, in function callback, Raise blinds ");
        digitalWrite(CONTROL_BLINDS_EN_A, HIGH);  // enable the motor, 
        digitalWrite(CONTROL_BLINDS_IN_1, HIGH);   // Motor CW
        digitalWrite(CONTROL_BLINDS_IN_2, LOW);    // 
        
        // it is actually HIGH
        // 
      } else if ((char)payload[0] == '0') { // ccw - Lower
        Serial.println("Lower blinds  CONTROL_BLINDS Pin 12 to HIGH, . in function callback, Lower Blinds ");
        digitalWrite(CONTROL_BLINDS_EN_A, HIGH);  // enable the motor, 
        digitalWrite(CONTROL_BLINDS_IN_1, LOW);   // Pin 16 Motor CCW 
        digitalWrite(CONTROL_BLINDS_IN_2, HIGH);  //  Pin 13
        
      } else if ((char)payload[0] == '2') {  // Motor off
        Serial.println("Motor Off CONTROL_BLINDS Pin 12 ENA to LOW, in function callback, Motor Off ");
        digitalWrite(CONTROL_BLINDS_EN_A, LOW);  // Disable the ENABLE PIN, 
      }
  } else if ( topicStr.compareTo( "inLivrmLightsControlTopic") == 0 )  {

      Serial.println ( "the string is inLivrmLightsControlTopic ") ;
      for (int i = 0; i < length; i++) {
        Serial.print((char)payload[i]);
      }
      Serial.println();
    
      // Switch on the LED if an 1 was received as first character
      if ((char)payload[0] == '1') { //ON received
        Serial.println("Turning lights On");
        digitalWrite(CONTROL_LIV_ROOM_LIGHTS, LOW); // its the reverse         
        
        // 
      } else if ((char)payload[0] == '0') { 
        Serial.println("Turning Lights Off");
        digitalWrite(CONTROL_LIV_ROOM_LIGHTS, HIGH);  
      }    
  }
}

void reconnect() {
  // Loop until we're reconnected
  int flag = 0;
  
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str(), "oxefqvkn", "<PASSWORD>" )) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      if ( flag ) {
         flag =0;
         Serial.println("sending a 1");
         client.publish("outTopic", "1");
      } else {
        Serial.println("sending a 0");
        client.publish("outTopic", "0");
      }
      // ... and resubscribe
      client.subscribe("inBlindsControl");
      client.subscribe("inLivrmLightsControlTopic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      digitalWrite(LED_BUILTIN, HIGH);   // Turn the LED OFF to indicate that the connection is not done.

      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  digitalWrite(LED_BUILTIN, HIGH);   // Turn the LED OFF - to indicate connection not done yet.
  
  // Initialize pin 12,14 and 16 for turning lights on and off.
  pinMode (CONTROL_BLINDS_IN_1, OUTPUT);
  pinMode (CONTROL_BLINDS_IN_2, OUTPUT);
  pinMode (CONTROL_BLINDS_EN_A, OUTPUT);
  pinMode (CONTROL_LIV_ROOM_LIGHTS, OUTPUT);  
  
  Serial.begin(115200);
  //Serial.println ("turning CONTROL LIGHTS/Pin 14 off i.e HIGH ");
  setup_wifi();
  client.setServer(mqtt_server, 19757);
  client.setCallback(callback);

  // Code for NTP added from here.
  timeClient.setTimeOffset(19800);

  timeClient.begin();
}

void loop() {

//  char publish_str [20];
  
  if (!client.connected()) {
    digitalWrite(LED_BUILTIN, HIGH);   // Got disconnected, so turn the wifi indicator off.  
    reconnect();
  }
  client.loop();
  digitalWrite(LED_BUILTIN, LOW);   // Got connected, enable wifi indicator on.

  // code migrated, to get the current time - done every second.
  // code - without using delay, nonblocking, gets time.
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    // save the last time you blinked the LED
    previousMillis = currentMillis;
    // Serial.println ( "hello world");
    timeClient.update();

    Serial.println(timeClient.getFormattedTime());
    int curHour = timeClient.getHours();
    //Serial.println(curHour);

    int curMinutes = timeClient.getMinutes();
    //Serial.println(curMinutes);
    if (( curHour >= 18 ) && ( curHour <= 22 ) ) {
    //if ( lights_on == 0 ) {

      Serial.println ("Light is ON ");
      digitalWrite(CONTROL_LIV_ROOM_LIGHTS, LOW);  // Lights On, 
      lights_on=1;
    } else {
      lights_on=0;
      Serial.println ("Light is OFF ");
      digitalWrite(CONTROL_LIV_ROOM_LIGHTS, HIGH);  // Lights On, 
      
    }
    
  }
}
