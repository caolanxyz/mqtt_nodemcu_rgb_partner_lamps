/*
 * ESP8266 MQTT Lights for Home Assistant.
 * See https://github.com/corbanmailloux/esp-mqtt-rgb-led
 */

// https://github.com/bblanchon/ArduinoJson
#include <ArduinoJson.h>

#include <ESP8266WiFi.h>

// http://pubsubclient.knolleary.net/
#include <PubSubClient.h>

const char* ssid = "ssid";
const char* password = "pass";

const char* mqtt_server = "url";
const char* mqtt_username = "";
const char* mqtt_password = "";
//Port not needed if you are using your own, you can change this if you are using a service like cloudmqtt
const int mqtt_port = 1883;

const char* client_id = "id"; // Must be unique on the MQTT network

/*

IMPORTANT
-
REMEMBER TO SET UP YOUR TOPICS TO MATCH THE PARTNER DEVICE

*/

// Topics
const char* light_state_topic = "bob/lamp1";
const char* light_partner_topic = "bob/lamp2/set";
const char* light_set_topic = "bob/lamp1/set";

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

// Timer Variables
unsigned long timerStartTime;  
unsigned long currentTimeInMs;
bool awaitingConfirmation = false;

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
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
 
  //////////////////////////////////// TEST COMMANDS
  // Set All LEDs to full brightness
  if(strcmp(message, "full") == 0) {
    RGB_color(1023, 1023, 1023);
    Serial.println("Setting White");
  }
  // Set all LEDs off
  if(strcmp(message, "off") == 0) {
    RGB_color(0, 0, 0);
    Serial.println("Setting all to 0");
  }
  //Test Green Status Flash
  if(strcmp(message, "testgreenflash") == 0) {
    Serial.println("Testing Green Flash...");
    statusFlash(2,10);
    Serial.println("Done");
  }
  //Test Red Flash
  if(strcmp(message, "testredflash") == 0) {
    Serial.println("Testing Red Flash...");
    statusFlash(1,10);
    Serial.println("Done");
  }
  // Check Button Status
  if(strcmp(message, "chkbtn") == 0) {
    Serial.println("Checking Button Status...");
    checkButtonStatus();
    Serial.println("Done");
  }
  //////////////////////////////////// TEST COMMANDS

  // Acknowledge Partner's Online check
  if(strcmp(message, "checkifpartneractive") == 0)
  {
    Serial.println("Online Check Recieved - Acknowledging");
    sendPartnerState(11);
    Serial.println("Rodger Dodger");
  }

  // If Partner requests that the light flash pink
  if(strcmp(message, "beginpartnerlight") == 0) 
  {
    Serial.println("Partner Button Triggered. Starting Pink Fade...");
    pinkFade(); // Fade LEDs
    Serial.println("Pink Fade Done - Updating Partner");
    sendPartnerState(12); // On successful fade, notify partner the lights have worked ("lightsuccess")
    Serial.println("Done");
  }

  // Let partner know you are online after it's button is pressed
  if(strcmp(message, "partnerready") == 0)
  {
    Serial.println("Rodger Recieved - Partner Online");
    awaitingConfirmation = false;
    sendPartnerState(1);
  }

  // If Partner has successfully displayed pink
  if(strcmp(message, "lightsuccess") == 0) 
  {
    Serial.println("Partner Lights Successful. Starting Pink Fade...");
    awaitingConfirmation = false;
    pinkFade(); // Fade LEDs
    Serial.println("Pink Fade Done");
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
  // Send Code 0 to check if partner is online
  if (lightCode == 0)
  {
    client.publish(light_partner_topic, "checkifpartneractive");
    timerStartTime = millis();
    awaitingConfirmation = true;
  }
  // If partner confirmed online, send request to fade pink
  if (lightCode == 1)
  {
    client.publish(light_partner_topic, "beginpartnerlight");
    timerStartTime = millis();
    awaitingConfirmation = true;
  }
  // If partner requests online status reply rodger
  if (lightCode == 11)
  {
    client.publish(light_partner_topic, "partnerready");
  }
  // If lights have successfully lit, update partner
  if (lightCode == 12)
  {
    client.publish(light_partner_topic, "lightsuccess");
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

// Reconnect if connection lost
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(client_id, mqtt_username, mqtt_password)) {
      Serial.println("connected");
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

// Blue LED fade used if partner offline
void blueFade()
{
  // fade up to pink
  for (size_t i = 0; i < 1020; i=i+5)
  {
    RGB_color(0, i/4, i);
  }
  delay(200);
  //fade to 500
    for (size_t i = 1020; i > 300; i=i-5)
  {
    RGB_color(0, i/4, i);
  }
  delay(100);
  // fade up to pink
  for (size_t i = 500; i < 1020; i=i+5)
  {
    RGB_color(0, i/4, i);
  }
  delay(4000);
  //fade to 0
    for (size_t i = 1020; i > 0; i=i-2)
  {
    RGB_color(0, i/4, i);
  }
  RGB_color(0, 0, 0);
  Serial.println("Blue Fade complete");
}


// Check if response recieved 
void fadeConfirmation(){
  if (awaitingConfirmation == true)
  {
    currentTimeInMs = millis(); // Update timer's current time
    //Send Alive Message if period > 10
    if (currentTimeInMs - timerStartTime >= 10000)  //If the current time - start time is greater than the defined period
    {
      blueFade(); // Let partner know system is still up
      awaitingConfirmation = false;
    }
  }
}

// Set LEDs to a defined colour
void RGB_color(int red_light_value, int green_light_value, int blue_light_value)
 {
  /*
  Serial.println("Setting LEDs:");
  Serial.print("r: ");
  Serial.print(red_light_value);
  Serial.print(", g: ");
  Serial.print(green_light_value);
  Serial.print(", b: ");
  Serial.println(blue_light_value);
  */
  analogWrite(red_light_pin, red_light_value);
  analogWrite(green_light_pin, green_light_value);
  analogWrite(blue_light_pin, blue_light_value);
  redcolour = red_light_value;
  greencolour = green_light_value;
  bluecolour = blue_light_value;
  delay(10);
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

    Serial.println("Sending Partner State Code 0 - Check if Partner is Online");
    sendPartnerState(0);
    Serial.println("Done.");
    delay(50);
  }
}

// Main program loop
void loop() {
  buttonPush();
  fadeConfirmation();
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}