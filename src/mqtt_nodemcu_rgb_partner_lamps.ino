/*
 * NodeMCU RGB Partner Lamps using MQTT
 * https://github.com/CaolanMcKendry/mqtt_nodemcu_rgb_partner_lamps
 */

// https://github.com/bblanchon/ArduinoJson
#include <ArduinoJson.h>

#include <ESP8266WiFi.h>

// http://pubsubclient.knolleary.net/
#include <PubSubClient.h>

const char* ssid = "ssid";
const char* password = "pass";

const char* mqtt_server = "mqtt.domain.cm";
const char* mqtt_username = "";
const char* mqtt_password = "";
//Port not needed if you are using your own, you can change this if you are using a service like cloudmqtt
const int mqtt_port = 1883;

const char* client_id = "CLIENTID"; // Must be unique on the MQTT network

// Topics
const char* light_state_topic = "lamp/firstlamp";
const char* light_partner_topic = "lamp/secondlamp/set";
const char* light_set_topic = "lamp/firstlamp/set";

const int BUFFER_SIZE = JSON_OBJECT_SIZE(10);

//GPIO Pins
const int txPin = 1; //Onboard LED
const int red_light_pin=  14;
const int green_light_pin = 13;
const int blue_light_pin = 12;
const int button_pin_a = 15;

// States
int temp_a = 0; 
int redcolour = 0;
int greencolour = 0;
int bluecolour = 0;

// Timing Variables
unsigned long aliveSentTimerStart;  
unsigned long currentMillis;
unsigned long aliveRecievedTimerStart;
const unsigned long period = 300000;  //5 Mins

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  aliveSentTimerStart = millis();  //initial start time
  aliveRecievedTimerStart = millis();  //initial start time
  pinMode(red_light_pin, OUTPUT);
  pinMode(green_light_pin, OUTPUT);
  pinMode(blue_light_pin, OUTPUT);
  pinMode(button_pin_a, INPUT); 
  Serial.begin(115200);
  Serial.println("Serial Started");
  delay(3000);
  RGB_color(1023, 1023, 1023);
  delay(200);
  RGB_color(0, 0, 0);
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    delay(500);
    statusFlash(1,1);
    Serial.print(WiFi.status());
  }
  
  Serial.println("");
  statusFlash(9,1);
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

// Run when an MQTT message is recieved
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  char message[length + 1];
  for (int i = 0; i < length; i++) {
    message[i] = (char)payload[i];
  }
  message[length] = '\0';
  Serial.println(message);
 
  // Set All LEDs to full brightness
  if(strcmp(message, "full") == 0)
  {
    RGB_color(1023, 1023, 1023);
    Serial.println("Setting White");
  }
  // Set all LEDs off
  if(strcmp(message, "off") == 0)
  {
    RGB_color(0, 0, 0);
    Serial.println("Setting all to 0");
  }
  //Test Green Status Flash
  if(strcmp(message, "testgreenflash") == 0)
  {
    Serial.println("Testing Green Flash...");
    statusFlash(2,10);
    Serial.println("Done");
  }
  //Test Red Flash
  if(strcmp(message, "testredflash") == 0)
  {
    Serial.println("Testing Red Flash...");
    statusFlash(1,10);
    Serial.println("Done");
  }
  // Test Pink Fade
  if(strcmp(message, "testpinkfade") == 0)
  {
    Serial.println("Testing Pink Fade...");
    pinkFade();
    Serial.println("Done");
  }
  // Check Button Status
  if(strcmp(message, "chkbtn") == 0)
  {
    Serial.println("Checking Button Status...");
    checkButtonStatus();
    Serial.println("Done");
  }
  // Register Partner's Alive Message
  if(strcmp(message, "alive") == 0)
  {
    Serial.println("Setting Alive");
    aliveRecievedTimerStart = millis();
    Serial.println("Done");
  }
  sendState(redcolour, greencolour, bluecolour);
}

// Send current state to MQTT broker
void sendState(int red, int green, int blue) {
  StaticJsonBuffer<BUFFER_SIZE> jsonBuffer;

  JsonObject& root = jsonBuffer.createObject();

  JsonObject& color = root.createNestedObject("colour");
  color["r"] = red;
  color["g"] = green;
  color["b"] = blue;

  char buffer[root.measureLength() + 1];
  root.printTo(buffer, sizeof(buffer));

  client.publish(light_state_topic, buffer, true);
}

// Update current state of Partner Lamp
void sendPartnerState(int lightCode) {
  // If partnerAlive() sends 0 (Alive) code
  if (lightCode == 0)
  {
    client.publish(light_partner_topic, "alive");
  }
  // If buttonPush() sends 0 (Fade Pink) code
  if (lightCode == 1)
  {
    client.publish(light_partner_topic, "pink");
  }
  // If some other fuckery sends some wacky code
  else
  {
    client.publish(light_partner_topic, "invalid-light-code");
  }
  
}

// Flash LEDs to indicate system status
void statusFlash(int colour, int flashes)
{
  //red
  if (colour == 1)
  {
    for (size_t i = 0; i < flashes; i++)
    {
    RGB_color(1023, 0, 0);
    delay(750);
    RGB_color(0, 0, 0);
    delay(200);
    }
  }
  //green
  if (colour == 2)
  {
    for (size_t i = 0; i < flashes; i++)
    {
    RGB_color(0, 1023, 0);
    delay(1000);
    RGB_color(0, 0, 0);
    delay(300);
    }
  }
  //amber
  if (colour == 3)
  {
    for (size_t i = 0; i < flashes; i++)
    {
    RGB_color(1023, 600, 0);
    delay(750);
    RGB_color(0, 0, 0);
    delay(200);
    }
  }
  //white
  if (colour == 9)
  {
    for (size_t i = 0; i < flashes; i++)
    {
    RGB_color(1023, 1023, 1023);
    delay(1000);
    RGB_color(0, 0, 0);
    delay(300);
    }
  }
}

// Update partner on current system state & check partner's state
void partnerAlive(){
  currentMillis = millis(); // Update timer's current time
  //Send Alive Message if period > 5
  if (currentMillis - aliveSentTimerStart >= period)  //If the current time - start time is greater than the defined period
  {
    sendPartnerState(0) // Let partner know system is still up
    aliveSentTimerStart = millis();  //get the current no of ms since program start
  }

  // Enter Error Light State if partner has not been seen in 5 mins
  if (currentMillis - aliveRecievedTimerStart >= period)  //If the current time - start time is greater than the defined period
  {
    statusFlash(3, 2) // Flash amber to indicate partner disconnect
    aliveRecievedTimerStart = millis();  //Update start time to start a new timer
  }
}

// Reconnect if connection lost
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(client_id, mqtt_username, mqtt_password)) {
      Serial.println("connected");
      statusFlash(2,1);
      client.subscribe(light_set_topic);
    } else {
      statusFlash(1,1);
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

// Pink LED fade used as primary user light
void pinkFade()
{
  // fade up to pink
  for (size_t i = 0; i < 1020; i=i+5)
  {
    RGB_color(i, 0, i/2);
  }
  delay(200);
  //fade to 500
    for (size_t i = 1020; i > 300; i=i-5)
  {
    RGB_color(i, 0, i/2);
  }
  delay(100);
  // fade up to pink
  for (size_t i = 500; i < 1020; i=i+5)
  {
    RGB_color(i, 0, i/2);
  }
  delay(4000);
  //fade to 0
    for (size_t i = 1020; i > 0; i=i-2)
  {
    RGB_color(i, 0, i/2);
  }
  RGB_color(0, 0, 0);
  Serial.println("Pink Fade complete");
}

// Set LEDs to a defined colour
void RGB_color(int red_light_value, int green_light_value, int blue_light_value)
 {
  Serial.println("Setting LEDs:");
  Serial.print("r: ");
  Serial.print(red_light_value);
  Serial.print(", g: ");
  Serial.print(green_light_value);
  Serial.print(", b: ");
  Serial.println(blue_light_value);
  analogWrite(red_light_pin, red_light_value);
  analogWrite(green_light_pin, green_light_value);
  analogWrite(blue_light_pin, blue_light_value);
  redcolour = red_light_value;
  greencolour = green_light_value;
  bluecolour = blue_light_value;
}

// Output current button status
void checkButtonStatus() {
  temp_a = digitalRead(button_pin_a);
  Serial.println("Button A:");
  Serial.println(temp_a);
}

// Functions to be performed any time button A pressed
void buttonPush() {
  if(digitalRead(button_pin_a) == HIGH) {
    Serial.println("Button A:");
    Serial.println(digitalRead(button_pin_a));
    Serial.println("Button Pushed. Starting Pink Fade...");
    pinkFade();
    Serial.println("Fade Done. Sending Partner State (Code 1)");
    sendPartnerState(1);
    Serial.println("Done.");
    delay(50);
  }
}

// Main program loop
void loop() {
  buttonPush();
  partnerAlive()
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}