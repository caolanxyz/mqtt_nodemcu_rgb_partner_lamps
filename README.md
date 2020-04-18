# NodeMCU RGB Partner Lamps using MQTT 

This project is primarliy a fork of [babakhani](https://github.com/babakhani/nodemcu-mqtt-rgb-led)'s fork of [JamesMcClelland](https://github.com/JamesMcClelland/nodemcu-mqtt-rgb-led)'s fork of [corbanmailloux](https://home-assistant.io/components/light.mqtt_json/)'s repository [esp-mqtt-rgb-led](https://github.com/corbanmailloux/esp-mqtt-rgb-led), originally designed to build a RGB lamp that could work with HomeAssistant.

I have since gutted the code and created the spaghetti garbage heap that lays before you. This project is inspired by [Long Distance Friendship Lamps](https://www.uncommongoods.com/product/long-distance-friendship-lamp) from [Uncommon Goods](https://www.uncommongoods.com/). I've stripped out most of the neopixel and homeassistant code and instead the software is now designed for use with standard common cathode 4mm RGB LEDs. MQTT allows for 2 lamps to (very jankily) be 'bonded' together, making both light up when one is activated.

I first tried using Python on a Raspberry Pi Zero W but the Pi's GPIO wasn't compatible with the LEDs so I switched to the NodeMCU instead.

## Installation/Configuration

For this I used 2x NodeMCU v3's which used a ESP8266-01 microcontroller. The NodeMCU is bigger than a raw ESP8266 but it has a voltage regulator and is breadboard friendly.
This should in theory work with PlatformIO as-is but I havent tested it so on your own head be it.

To set up the code, update the following on both units: ssid, password, mqtt_server, mqtt_username (if appliciable), mqtt_password (if appliciable) mqtt_port (default 1883), client_id (Must be unique on the MQTT network).

For each lamp, set the light_state_topic to the topic you'd like to update with the LED's state. Set the light_partner_topic to the topic you'd like the partner lamp to update with (should match partner's light_set_topic). Set the light_set_topic to the topic the partner lamp will send messages to.

#### Wiring
To-Do