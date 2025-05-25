# ESP32-Nightscout-TFT
ESP32-C3 based low cost desktop display for Nightscout Glucose

Displays supported:
1.54" - https://www.aliexpress.com/item/1005008180192989.html
1.3" - https://www.aliexpress.com/item/1005006824077787.html
Any 1.54" or 1.3" displays with a resolution of 240x240 pixels and ST7789 driver should work. Make sure you select the correct model if the seller has several!
Note that if your display has additional pins, i.e. a CS pin, you can just ignore that in the wiring.

ESP32-C3:
https://www.aliexpress.com/item/1005006599448997.html

![wiring-C3](https://github.com/user-attachments/assets/b0e42f97-5557-42ae-8414-6258b47b1f5d)

Connect the backlight pin (usually BLK or BK) together with the 3.3V pin.

It's critical that you install/downgrade the ESP32 Board version to 2.0.14! Versions 3+ will not fit the available memory, while versions >2.0.14 can cause a reset loop on the C3.
As board type, select the LOLIN C3 Mini if you are using the above ESP32-C3 SuperMini.
STL files are provided for both display types, use hotglue to position the ESP32 on the back and to keep the lid attached after.
If you need to get back in there for any reason, isopropyl alcohol will instantly release hotglue.

Once the device starts for the first time, connect your wifi to the access point "Nightscout-TFT" that becomes available. Enter the password shown on the screen, then visit 192.168.4.1 in your webbrowser. You can set up your nightscout instance and your home wifi there. Once everything is connected, you should get a full display. If it hangs, try unplugging, waiting for a moment, and plugging it back in.

Some status info is available on the serial monitor via USB (9600 baud)
