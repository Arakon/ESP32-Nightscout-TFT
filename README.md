# ESP32-Nightscout-TFT
![image](https://github.com/user-attachments/assets/3c873dd6-202d-4296-86c1-1da339f6a62e)


ESP32-C3 based low cost desktop display for Nightscout Glucose

Displays supported:

1.54" - https://www.aliexpress.com/item/1005008180192989.html

1.3" - https://www.aliexpress.com/item/1005006824077787.html

Any 1.54" or 1.3" displays with a resolution of 240x240 pixels and ST7789 driver should work. Make sure you select the correct model if the seller has several!
Note that if your display has additional pins, i.e. a CS pin, you can just ignore that in the wiring.

ESP32-C3:
https://www.aliexpress.com/item/1005006599448997.html

![wiring-C3](https://github.com/user-attachments/assets/eaa8e22f-1cd9-4673-a234-814b33b1991a)

**Required libraries to install: TFT_eSPI, ArduinoJSON, WiFiManger**

**It's critical that you install/downgrade the ESP32 Board version to _2.0.14_!**

Versions 3+ will not fit the available memory, while versions >2.0.14 can cause a reset loop on the C3 in combination with TFT_eSPI.

As board type, select the **LOLIN C3 Mini** if you are using the above ESP32-C3 SuperMini.

**After downloading, copy the User_Setup.h to C:\Users\<yourusername>\\Documents\Arduino\libraries\TFT_eSPI.**

STL files are provided for both display types, use hotglue to position the ESP32 on the back and to keep the lid attached after.
If you need to get back in there for any reason, isopropyl alcohol will instantly release hotglue.

Once the device starts for the first time, connect your WiFi (cellphone or PC) to the access point "Nightscout-TFT" that becomes available. Enter the password shown on the screen, then visit 192.168.4.1 in your webbrowser. You can set up your nightscout instance and your home wifi there. Once everything is connected, you should get a full display. If it hangs, try unplugging, waiting for a moment, and plugging it back in.

**First** run the Setup tab, with your Nightscout data, High/Low/Critical values and background light strength.

**Afterwards** you will have to reconnect to the access point again and this time, set up the WiFi connection.

If you ever want to change any of the options, quickly **doubleclick** the Reset button on the ESP32 to force config mode. This will require you to connect to the Nightscout-TFT access point again with the password displayed on the screen.

Some status info is available on the serial monitor via USB (9600 baud).
