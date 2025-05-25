/**
Nightscout glucose display for use with ESPS32-C3 and 1.3" or 1.54" SPI screen.
Based partially on my original version of this for ESP8266: https://github.com/Arakon/Nightscout-TFT
and gluci-clock by Frederic1000: https://github.com/Frederic1000/gluci-clock
Also compatible with other ESP32 devices, but may require pin reassignments in User_Setup.h and PIN_LED below

Copy User_Setup.h to C:\Users\<yourusername>\Documents\Arduino\libraries\TFT_eSPI before compiling!

*/

// install libraries: WifiManager, arduinoJson, ESP_DoubleResetDetector, TFT_eSPI

// In the Arduino IDE, select Board Manager and downgrade to ESP32 2.0.14, ESP32 3.x based will NOT fit most smaller ESP32 boards anymore.

// ----------------------------
// Library Defines - Need to be defined before library import
// ----------------------------

#define ESP_DRD_USE_SPIFFS true

#include <Arduino.h>

#include <WiFi.h>
#include <FS.h>
#include <SPIFFS.h>
#include <TFT_eSPI.h>
#include <HTTPClient.h>
#include <time.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include "gill28pt7b.h"  //Font file, keep in same folder as the .ino
#include <WiFiManager.h>
//including the arrow images
#include "Flat.h"
#include "DoubleDown.h"
#include "DoubleUp.h"
#include "FortyFiveUp.h"
#include "FortyFiveDown.h"
#include "Up.h"
#include "Down.h"

#define FONT &gill28pt7b
#define GFXFF 1

TFT_eSPI tft = TFT_eSPI();


// Captive portal for configuring the WiFi

// Can be installed from the library manager (Search for "WifiManager", install the Alhpa version)
// https://github.com/tzapu/WiFiManager

#include <ESP_DoubleResetDetector.h>
// A library for checking if the reset button has been pressed twice
// Can be used to enable config mode
// Can be installed from the library manager (Search for "ESP_DoubleResetDetector")
//https://github.com/khoih-prog/ESP_DoubleResetDetector

int hour_c;
int min_c;
int sec_c;

// These determine when the color of the glucose value changes, edit if you like.
int HighBG = 180;
int LowBG = 80;
int CritBG = 60;

// -------------------------------------
// -------   Other Config   ------
// -------------------------------------

const int PIN_LED = 8;  // if not using ESP32-C3, edit to match the onboard LED pin

#define JSON_CONFIG_FILE "/sample_config.json"

// Number of seconds after reset during which a
// subseqent reset will be considered a double reset.
#define DRD_TIMEOUT 1
// RTC Memory Address for the DoubleResetDetector to use
#define DRD_ADDRESS 0


// -----------------------------

// -----------------------------

DoubleResetDetector* drd;

//flag for saving data
bool shouldSaveConfig = false;


// url and API key for Nightscout - You can also set these later in the web interface

// We use the /pebble API since it contains the delta value. Don't remove the /pebble at the end!
char NS_API_URL[150] = "http://yournightscoutwebsite/pebble";

// Create a token with read access in NS > Hamburger menu > Admin tools
// Enter the token here
char NS_API_SECRET[50] = "view-123456790";


// Parameters for time NTP server
char ntpServer1[50] = "pool.ntp.org";
char ntpServer2[50] = "de.pool.ntp.org";

// Time zone for local time and daylight saving
// list here:
// https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
char local_time_zone[50] = "CET-1CEST,M3.5.0,M10.5.0/3";  // set for Europe/Paris
char utc_time_zone[] = "GMT0";
int gmtOffset_sec = 3600;
long daylightOffset_sec = 3600;  // long

// time zone offset in minutes, initialized to UTC
int tzOffset = 0;

void serialPrintParams() {
  Serial.println("\tNS_API_URL : " + String(NS_API_URL));
  Serial.println("\tNS_API_SECRET: " + String(NS_API_SECRET));
  Serial.println("\tntpServer1 : " + String(ntpServer1));
  Serial.println("\tntpServer2 : " + String(ntpServer2));
  Serial.println("\tlocal_time_zone : " + String(local_time_zone));
  Serial.println("\tgmtOffset_sec : " + String(gmtOffset_sec));
  Serial.println("\tdaylightOffset_sec : " + String(daylightOffset_sec));
}

void saveConfigFile() {
  Serial.println(F("Saving config"));
  StaticJsonDocument<512> json;

  json["NS_API_URL"] = NS_API_URL;
  json["NS_API_SECRET"] = NS_API_SECRET;
  json["ntpServer1"] = ntpServer1;
  json["ntpServer2"] = ntpServer2;
  json["local_time_zone"] = local_time_zone;
  json["gmtOffset_sec"] = gmtOffset_sec;
  json["daylightOffset_sec"] = daylightOffset_sec;


  File configFile = SPIFFS.open(JSON_CONFIG_FILE, "w");
  if (!configFile) {
    Serial.println("failed to open config file for writing");
  }

  serializeJsonPretty(json, Serial);
  if (serializeJson(json, configFile) == 0) {
    Serial.println(F("Failed to write to file"));
  }
  configFile.close();

  delay(3000);
  ESP.restart();
  delay(5000);
}

bool loadConfigFile() {

  //read configuration from FS json
  Serial.println("mounting FS...");

  // May need to make it begin(true) first time you are using SPIFFS
  // NOTE: This might not be a good way to do this! begin(true) reformats the spiffs
  // it will only get called if it fails to mount, which probably means it needs to be
  // formatted, but maybe dont use this if you have something important saved on spiffs
  // that can't be replaced.
  if (SPIFFS.begin(false) || SPIFFS.begin(true)) {
    Serial.println("mounted file system");
    if (SPIFFS.exists(JSON_CONFIG_FILE)) {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open(JSON_CONFIG_FILE, "r");
      if (configFile) {
        Serial.println("opened config file");
        StaticJsonDocument<512> json;
        DeserializationError error = deserializeJson(json, configFile);
        serializeJsonPretty(json, Serial);
        if (!error) {
          strcpy(NS_API_URL, json["NS_API_URL"]);
          strcpy(NS_API_SECRET, json["NS_API_SECRET"]);
          strcpy(ntpServer1, json["ntpServer1"]);
          strcpy(ntpServer2, json["ntpServer2"]);
          strcpy(local_time_zone, json["local_time_zone"]);
          gmtOffset_sec = json["gmtOffset_sec"];
          daylightOffset_sec = json["daylightOffset_sec"];


          Serial.println("\nThe loaded values are: ");
          serialPrintParams();

          return true;
        } else {
          Serial.println("failed to load json config");
        }
      }
    }
  } else {
    Serial.println("failed to mount FS");
    // SPIFFS.format();
  }
  //end read
  return false;
}




//callback notifying us of the need to save config
void saveConfigCallback() {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

WiFiManager wm;



// default password for Access Point, made from macID
char* getDefaultPassword() {
  // example:
  // const char * password = getDefaultPassword();

  // source for chipId: Espressif library example ChipId
  uint32_t chipId = 0;
  for (int i = 0; i < 17; i = i + 8) {
    chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
  }
  chipId = chipId % 100000000;
  static char pw[9];  // with +1 char for end of chain
  sprintf(pw, "%08d", chipId);
  return pw;
}
const char* apPassword = getDefaultPassword();

//callback notifying us of the need to save parameters
void saveParamsCallback() {
  Serial.println("Should save params");
  shouldSaveConfig = true;
  wm.stopConfigPortal();  // will abort config portal after page is sent
}

// This gets called when the config mode is launced, might
// be useful to update a display with this info.
void configModeCallback(WiFiManager* myWiFiManager) {
  Serial.println("Entered Conf Mode");


  Serial.print("Config SSID: ");
  Serial.println(myWiFiManager->getConfigPortalSSID());
  Serial.print("Config password: ");
  Serial.println(apPassword);
  Serial.print("Config IP Address: ");
  Serial.println(WiFi.softAPIP());

  // Show IP and password for initial setup
  tft.fillRect(10, 40, 190, 55, TFT_BLACK);
  tft.setCursor(10, 120);
  tft.print("Please connect to: ");
  tft.println(WiFi.softAPIP());
  tft.setCursor(10, 140);
  tft.print("Password: ");
  tft.println(apPassword);
}


/**
 * Set the timezone
 */
void setTimezone(char* timezone) {
  Serial.printf("  Setting Timezone to %s\n", timezone);
  configTzTime(timezone, ntpServer1, ntpServer2);
}

/**
 * Get time from internal clock
 * returns time for the timezone defined by setTimezone(char* timezone)
 */
struct tm getActualTzTime() {
  struct tm timeinfo = { 0 };
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time!");
    return (timeinfo);  // return {0} if error
  }
  Serial.print("System time: ");
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  return (timeinfo);
}



/**
 * Returns the offset in seconds between local time and UTC
 */
int getTzOffset(char* timezone) {

  // set timezone to UTC
  setTimezone(utc_time_zone);

  // and get tm struct
  struct tm tm_utc_now = getActualTzTime();

  // convert to time_t
  time_t t_utc_now = mktime(&tm_utc_now);

  // set timezone to local
  setTimezone(local_time_zone);

  // convert time_t to tm struct
  struct tm tm_local_now = *localtime(&t_utc_now);

  // set timezone back to UTC
  setTimezone(utc_time_zone);

  // convert tm to time_t
  time_t t_local_now = mktime(&tm_local_now);

  // calculate difference between the two time_t, in seconds
  int tzOffset = round(difftime(t_local_now, t_utc_now));
  Serial.printf("\nTzOffset : %d\n", tzOffset);

  return (tzOffset);
}

/**
 * Setup : 
 * - connect to wifi, 
 * - evaluate offset between local time and UTC
 */
void setup() {


  Serial.begin(115200);

  Serial.println();
  Serial.println();
  Serial.println("ESP start");

  pinMode(PIN_LED, OUTPUT);

  // Initialize display
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  //Print something during boot so you know it's doing something

  tft.setTextColor(TFT_YELLOW, TFT_BLACK);  // Note: the new fonts do not draw the background colour
  tft.setCursor(10, 80);
  tft.println("Starting up... Wait 60 seconds.");  // Due to the way the updates are delayed, it can take up to 60 seconds to get glucose data
  Serial.begin(115200);
  Serial.setTimeout(2000);
  Serial.println();
  Serial.println("Starting up...");



  bool forceConfig = false;

  drd = new DoubleResetDetector(DRD_TIMEOUT, DRD_ADDRESS);
  if (drd->detectDoubleReset()) {
    Serial.println(F("Forcing config mode as there was a Double reset detected"));
    forceConfig = true;
  }

  bool spiffsSetup = loadConfigFile();
  if (!spiffsSetup) {
    Serial.println(F("Forcing config mode as there is no saved config"));
    forceConfig = true;
  }

  WiFi.mode(WIFI_STA);  // explicitly set mode, esp defaults to STA+AP
  Serial.begin(115200);
  delay(10);

  //sets timeout until configuration portal gets turned off
  //useful to make it all retry or go to sleep
  //in seconds
  wm.setTimeout(10 * 60);

  //wm.resetSettings(); // wipe settings

  //set callbacks
  wm.setSaveConfigCallback(saveConfigCallback);
  wm.setSaveParamsCallback(saveParamsCallback);
  //set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wm.setAPCallback(configModeCallback);

  // Custom parameters here
  wm.setTitle("Nightscout-TFT");

  // Set cutom menu via menu[] or vector
  // const char* menu[] = {"wifi", "wifinoscan", "info", "param", "close", "sep", "erase", "restart", "exit", "erase", "update", "sep"};
  const char* wmMenu[] = { "param", "wifi", "close", "sep", "info", "restart", "exit" };
  wm.setMenu(wmMenu, 7);  // custom menu array must provide length


  //--- additional Configs params ---

  // url and API key for Nightscout

  WiFiManagerParameter custom_NS_API_URL("NS_API_URL", "NightScout API URL", NS_API_URL, 150);

  WiFiManagerParameter custom_NS_API_SECRET("NS_API_SECRET", "NightScout API secret", NS_API_SECRET, 50);

  // Parameters for time NTP server
  // char ntpServer1[50] = "pool.ntp.org";
  WiFiManagerParameter custom_ntpServer1("ntpServer1", "NTP server 1", ntpServer1, 50);
  // char ntpServer2[50] = "time.nist.gov";
  WiFiManagerParameter custom_ntpServer2("ntpServer2", "NTP server 2", ntpServer2, 50);
  ;


  // Time zone for local time and daylight saving
  // list here:
  // https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
  // char[50] local_time_zone = "CET-1CEST,M3.5.0,M10.5.0/3"; // set for Europe/Paris
  WiFiManagerParameter custom_local_time_zone("local_time_zone", "local_time_zone", local_time_zone, 50);

  //int  gmtOffset_sec = 3600;
  char str_gmtOffset_sec[5];
  sprintf(str_gmtOffset_sec, "%d", gmtOffset_sec);
  WiFiManagerParameter custom_gmtOffset_sec("gmtOffset_sec", "gmtOffset sec", str_gmtOffset_sec, 5);


  // int   daylightOffset_sec = 3600;
  char str_daylightOffset_sec[5];
  sprintf(str_daylightOffset_sec, "%d", daylightOffset_sec);
  WiFiManagerParameter custom_daylightOffset_sec("daylightOffset_sec", "daylightOffset sec", str_daylightOffset_sec, 5);


  // add app parameters to web interface
  wm.addParameter(&custom_NS_API_URL);
  wm.addParameter(&custom_NS_API_SECRET);
  wm.addParameter(&custom_ntpServer1);
  wm.addParameter(&custom_ntpServer2);
  wm.addParameter(&custom_local_time_zone);
  wm.addParameter(&custom_gmtOffset_sec);
  wm.addParameter(&custom_daylightOffset_sec);


  //--- End additional parameters

  digitalWrite(PIN_LED, LOW);
  if (forceConfig) {
    Serial.println("forceconfig = True");

    if (!wm.startConfigPortal("Nightscout-TFT", apPassword)) {
      Serial.print("shouldSaveConfig: ");
      Serial.println(shouldSaveConfig);
      if (!shouldSaveConfig) {
        Serial.println("failed to connect CP and hit timeout");
        delay(3000);
        //reset and try again, or maybe put it to deep sleep
        ESP.restart();
        delay(5000);
      }
    }
  } else {
    Serial.println("Running wm.autoconnect");

    if (!wm.autoConnect("Nightscout-TFT", apPassword)) {
      Serial.println("failed to connect AC and hit timeout");
      delay(3000);
      // if we still have not connected restart and try all over again
      ESP.restart();
      delay(5000);
    }
  }

  // If we get here, we are connected to the WiFi or should save params
  digitalWrite(PIN_LED, HIGH);

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // Lets deal with the user config values

  strcpy(NS_API_URL, custom_NS_API_URL.getValue());
  strcpy(NS_API_SECRET, custom_NS_API_SECRET.getValue());
  strcpy(ntpServer1, custom_ntpServer1.getValue());
  strcpy(ntpServer2, custom_ntpServer2.getValue());
  strcpy(local_time_zone, custom_local_time_zone.getValue());
  gmtOffset_sec = atoi(custom_gmtOffset_sec.getValue());
  daylightOffset_sec = atoi(custom_daylightOffset_sec.getValue());


  Serial.println("\nThe values returned are: ");
  serialPrintParams();

  //save the custom parameters to FS
  if (shouldSaveConfig) {
    saveConfigFile();
  }

  // offset in seconds between local time and UTC
  tzOffset = getTzOffset(local_time_zone);

  // set timezone to local
  setTimezone(local_time_zone);
}

/**
 * Connect to wifi
 * Retrieve glucose data from NightScout server and parse it to Json

 */
void loop() {
  tm tm;
  char buf[20];
  constexpr uint32_t INTERVAL{ 1000 };
  static uint32_t previousMillis;
  if (millis() - previousMillis >= INTERVAL) {
    previousMillis += INTERVAL;
    time_t now = time(nullptr);
    localtime_r(&now, &tm);
    strftime(buf, sizeof(buf), "%d.%m.%Y %T", &tm);
    sec_c = (tm.tm_sec);
  }
  if ((sec_c) == 0) {  //check only on the full minute. Side effect is that it can take up to a minute for the "Starting up..." screen to disappear.
    // Retrieve glucose data from NightScout Server
    HTTPClient http;
    Serial.println("\n[HTTP] begin...");
    http.begin(NS_API_URL);  // fetch latest value
    http.addHeader("API-SECRET", NS_API_SECRET);
    Serial.print("[HTTP] GET...\n");

    // start connection and send HTTP header
    int httpCode = http.GET();
    // httpCode will be negative on error
    if (httpCode > 0) {
      // HTTP header has been send and Server response header has been handled
      Serial.printf("[HTTP] GET... code: %d\n", httpCode);

      // NightScout data received from server
      if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        Serial.println(payload);

        // parse NightScour data Json response
        // buffer for Json parser
        StaticJsonDocument<500> httpResponseBody;
        DeserializationError error = deserializeJson(httpResponseBody, payload);

        // Test if parsing NightScout data succeeded
        if (error) {
          Serial.println();
          Serial.print(F("deserializeJson() failed: "));
          Serial.println(error.f_str());
        } else {

          // Get glucose values, trend, delta and timestamps
          long sgv = long(httpResponseBody["bgs"][0]["sgv"]);
          int trend = httpResponseBody["bgs"][0]["trend"];
          int bg_delta = httpResponseBody["bgs"][0]["bgdelta"];
          String status0_now = httpResponseBody["status"][0]["now"];
          status0_now = status0_now.substring(0, status0_now.length() - 3);
          time_t status0_now1 = status0_now.toInt();
          time_t bgs0_datetime = httpResponseBody["bgs"][0]["datetime"];
          String bgs0_datetime2 = httpResponseBody["bgs"][0]["datetime"];
          bgs0_datetime2 = bgs0_datetime2.substring(0, bgs0_datetime2.length() - 3);
          time_t bgs0_datetime3 = bgs0_datetime2.toInt();
          int elapsed_mn = (status0_now1 - bgs0_datetime3) / 60;
          //add "+" before delta value only if increasing
          String delta = "0";
          String positive = "+";
          if ((bg_delta) >= 0) {
            delta = positive + bg_delta;
          } else {
            delta = (bg_delta);
          }

          int bgcol = TFT_WHITE;
          int agecol = TFT_WHITE;
          int agepos = 38;
          if (sgv > 0) {  //only display if glucose value is valid
            // configure display positions depending on whether glucose is two or three digits
            int digits;
            if ((sgv) < 100) {
              digits = 70;
            } else {
              digits = 30;
            }
            // set color depending on levels
            if (sgv >= HighBG) {
              bgcol = TFT_ORANGE;
            } else if ((sgv <= LowBG) && (sgv > CritBG)) {
              bgcol = TFT_YELLOW;
            } else if (sgv <= CritBG) {
              bgcol = TFT_RED;
            } else {
              bgcol = TFT_WHITE;
            }

            // actually display glucose value
            tft.fillRect(30, 110, 148, 62, TFT_BLACK);  //clearing BG string for less flicker on refresh, still flickers some
            tft.setCursor(digits, 159);
            tft.setFreeFont(FONT);
            tft.setTextColor((bgcol), TFT_BLACK);
            tft.println(sgv);

            // set color and position of last data depending on amount of digits and time since last value
            if (elapsed_mn >= 15) {
              agecol = TFT_RED;
            } else {
              agecol = TFT_WHITE;
            }
            if (elapsed_mn >= 10) {
              agepos = 30;
            }
            if (elapsed_mn >= 100) {
              agepos = 23;
            }
            // actually display Last Data
            tft.fillRect(0, 0, 240, 48, TFT_BLACK);  //clearing date string for less flicker on refresh
            tft.setFreeFont(&FreeSerifBold9pt7b);
            tft.setTextColor((agecol), TFT_BLACK);
            tft.setCursor(agepos, 20);
            tft.print("Last Data: ");
            tft.print(elapsed_mn);
            if (elapsed_mn == 1) {
              tft.println(" min ago");
            } else tft.println(" mins ago");
          }

          // prepare and display clock, add leading 0 if single digits
          hour_c = (tm.tm_hour);
          min_c = (tm.tm_min);
          tft.fillRect(10, 40, 190, 55, TFT_BLACK);  //clearing time string for less flicker on refresh
          tft.setFreeFont(&FreeSerifBold24pt7b);
          tft.setTextColor(TFT_BLUE, TFT_BLACK);
          tft.setCursor(65, 80);
          if ((hour_c) < 10) tft.print("0");
          tft.print(hour_c);
          tft.print(":");
          if ((min_c) < 10) tft.print("0");
          tft.print(min_c);

          //Check trend number from json and show the matching arrow
          if ((trend) == 1) {
            tft.setSwapBytes(true);
            tft.pushImage(180, 116, 50, 50, DoubleUp);
          }
          if ((trend) == 2) {
            tft.setSwapBytes(true);
            tft.pushImage(180, 116, 50, 50, Up);
          }
          if ((trend) == 3) {
            tft.pushImage(180, 116, 50, 50, FortyFiveUp);
          }
          if ((trend) == 4) {
            tft.setSwapBytes(true);
            tft.pushImage(180, 116, 50, 50, Flat);
          }
          if ((trend) == 5) {
            tft.setSwapBytes(true);
            tft.pushImage(180, 116, 50, 50, FortyFiveDown);
          }
          if ((trend) == 6) {
            tft.setSwapBytes(true);
            tft.pushImage(180, 116, 50, 50, Down);
          }
          if ((trend) == 7) {
            tft.setSwapBytes(true);
            tft.pushImage(180, 116, 50, 50, DoubleDown);
          }
          if ((trend) == 8) { //show blank if no valid trend arrow
            tft.fillRect(175, 114, 60, 50, TFT_BLACK);
          }

          // adjust delta position depending on digit count and display
          int deltapos = 54;
          if (bg_delta >= 10) {
            deltapos = 42;
          }
          tft.fillRect(35, 180, 200, 60, TFT_BLACK);  //clearing Delta string for less flicker on refresh
          tft.setFreeFont(&FreeSerifBold18pt7b);
          tft.setCursor(deltapos, 220);
          tft.setTextColor(TFT_BLUE, TFT_BLACK);
          tft.print("Delta: ");
          tft.println(delta);
        }
      }
    }


    else {
      Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }

    http.end();
    delay(500);  //added delay to prevent accidental running twice within the same second, resulting in data corruption
  }
}
