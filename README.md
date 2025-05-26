# LoRa-Temperature-HomeAssistant
This is my first project repository for LoRa sensor pair for long range temperature measurement and integration to Home Assistant.

First of all, I am not a software developer of any sort. I'm a Home Assistant enthusiastic and I can understand and fiddle around with simple software code. I was able to accomplish a relevantly simple setup to harness Heltec ESP32 LoRa V3 board pair for long range temperature measurement and integrate it to my Home Assistant instance. Both the hardware setup and the code are most likely not optimal in terms of temperature accuracy, battery preservation and cost-efficiency, but they seem to work.

For my use case, I'm measuring the nearby lake water temperature and delivering it 160 meters not fully line of sight:
![image](https://github.com/user-attachments/assets/39448025-b8a3-4917-b1e6-3ceaada8883a)

**Hardware used for this project**

2x Heltec ESP32 LoRa V3 boards

  - You could most likely achieve cost benefits by utilizing cheaper boards and separate LoRa transmission modules

1x DS18B20 waterproof sensors

  - It came to my attention later that onewire sensor like this is not optimal and not officially supported by Mestastic for example

1x 18650 2900mAh battery

  - Remains to be seen what kind of battery life I can expect with this in the transmission end

1x 18650 battery holder with JST connection

1x 4.7 Ohm pull-up resistor

  - I'm not exactly sure if this is actually necessary for this board but soldered it in anyway.

1x suitable waterproof case with holes for antenna and sensor wire

1x receiving end case

**Meshtastic note**

This is not a Meshtastic project. My rather short dig into Meshtastic gave me the understanding that even if getting DS18B20 sensor to work correctly with Meshtastic firmware, I would not be expecting even nearly as good battery life as with this simple Arduino LoRa -setup. I welcome all the comments around this assumption from anyone more involved with Meshtastic.

**Wiring**

Not being electrical engineer either, I ended up with trial and error for the data wiring instead of trying to research the optimal pin for the DS18B20 data line. Soldering it to GPIO07 and defining it in the code seemed to do the trick as the test measurements seemed to be correct. However, bad initial soldering resulted in a lot of false -127 C degree or 85 degree measurements. 

![image](https://github.com/user-attachments/assets/c132bfed-979a-4eda-bce8-236bd121c996)

**Code, compilation challenges and flashing**

Arduino IDE was used to write and upload the code to the boards. There was a major challenge as, in my understanding the OneWire.h and Arduino.h libraries caused some sort of compatibility issue and utilizing both of those libraries caused compilation errors. These were mainly "error: 'GPIO_IS_VALID_GPIO' was not declared in this scope;" or when calling digitalPinIsValid -macro.

I tried to troubleshoot this in multiple occasions as some people in the internet had tweaked the library codes and solved the issue. However, I found out (by luck) that compiling very simple code snippet that includes OneWire.h library makes Arduino IDE to utilize that through precompilation which bypasses the errors when adding other libraries into the code.

So if you cannot come up with a better solution. Just utilize init.ino example, compile and upload it to your board and then use the same sketchbook to modify the code to include other libraries and features. After some idle time or when using new sketch, it apparently needs to be done again this way.

TBC...
