#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Update.h>
#include "webpages.h"
#include "FS.h"
#include "FFat.h"

#define FIRMWARE_VERSION "v0.0.1"
const String default_ssid = "somessid";
const String default_wifipassword = "mypassword";
const String default_httpuser = "admin";
const String default_httppassword = "admin";
const int default_webserverporthttp = 80;

// configuration structure
struct Config {
  String ssid;            // wifi ssid
  String wifipassword;    // wifi password
  String httpuser;        // username to access web admin
  String httppassword;    // password to access web admin
  int webserverporthttp;  // http port number for web admin
};

// variables
Config config;              // configuration
bool shouldReboot = false;  // schedule a reboot
bool shouldUpdate = false;  // schedule a firmware update
AsyncWebServer *server;     // initialise webserver

// function defaults
String listFiles(bool ishtml = false);

void setup() {
  Serial.begin(115200);

  delay(1000);

  Serial.print("Firmware: ");
  Serial.println(FIRMWARE_VERSION);

  Serial.println("Booting ...");

  Serial.println("Mounting FatFS ...");

  //   Serial.println("FatFS, formatting");
  // #warning "WARNING ALL DATA WILL BE LOST: FFat.format()"
  //FFat.format();

  if (!FFat.begin()) {
    // Note: An error occurs when using the ESP32 for the first time, it needs to be formatted
    //
    //     Serial.println("ERROR: Cannot mount FatFS, Try formatting");
    // #warning "WARNING ALL DATA WILL BE LOST: FFat.format()"
    //     FFat.format();

    if (!FFat.begin()) {
      Serial.println("ERROR: Cannot mount FatFS, Rebooting");
      rebootESP("ERROR: Cannot mount FatFS, Rebooting");
    }
  }

  Serial.print("FatFS Free: ");
  Serial.println(humanReadableSize(FFat.freeBytes()));
  Serial.print("FatFS Used: ");
  Serial.println(humanReadableSize(FFat.usedBytes()));
  Serial.print("FatFS Total: ");
  Serial.println(humanReadableSize(FFat.totalBytes()));

  Serial.println(listFiles());

  Serial.println("Loading Configuration ...");

  config.ssid = default_ssid;
  config.wifipassword = default_wifipassword;
  config.httpuser = default_httpuser;
  config.httppassword = default_httppassword;
  config.webserverporthttp = default_webserverporthttp;

  Serial.print("\nConnecting to Wifi: ");
  WiFi.softAP(config.ssid.c_str(), config.wifipassword.c_str());
  WiFi.softAPsetHostname(config.ssid.c_str());


  Serial.println("\n\nNetwork Configuration:");
  Serial.println("----------------------");
  Serial.print("         SSID: ");
  Serial.println(WiFi.softAPSSID());
  Serial.print("Wifi Strength: ");
  Serial.print(WiFi.RSSI());
  Serial.println(" dBm");
  Serial.print("          MAC: ");
  Serial.println(WiFi.macAddress());
  Serial.print("           IP: ");
  Serial.println(WiFi.softAPIP());
  Serial.print("       Subnet: ");
  Serial.println(WiFi.softAPSubnetMask());
  Serial.println();

  // configure web server
  Serial.println("Configuring Webserver ...");
  server = new AsyncWebServer(config.webserverporthttp);
  configureWebServer();

  // startup web server
  Serial.println("Starting Webserver ...");
  server->begin();
}

void loop() {
  // reboot if we've told it to reboot
  if (shouldReboot) {
    rebootESP("Web Admin Initiated Reboot");
  }
  if (shouldUpdate) {
    updateESP("Web Admin Initiated Update");
  }
}

void rebootESP(String message) {
  Serial.print("Rebooting ESP32: ");
  Serial.println(message);
  ESP.restart();
}

void updateESP(String message) {
  Serial.print("Rebooting ESP32: ");
  Serial.println(message);
  File firmware = FFat.open("/firmware.bin");
  if (firmware) {
    Serial.println(F("found!"));
    Serial.println(F("Try to update!"));

    //Update.onProgress(progressHandler);
    Update.begin(firmware.size(), U_FLASH);
    Update.writeStream(firmware);
    if (Update.end()) {
      Serial.println(F("Update finished!"));
    } else {
      Serial.println(F("Update error!"));
      Serial.println(Update.getError());
    }

    firmware.close();

    if (FFat.rename("/firmware.bin", "/firmware.bak")) {
      Serial.println(F("Firmware rename succesfully!"));
    } else {
      Serial.println(F("Firmware rename error!"));
    }
    delay(2000);

    ESP.restart();
  }
}

// list all of the files, if ishtml=true, return html rather than simple text
String listFiles(bool ishtml) {
  String returnText = "";
  Serial.println("Listing files stored on FatFS");
  File root = FFat.open("/");
  File foundfile = root.openNextFile();
  if (ishtml) {
    returnText += "<table><tr><th align='left'>Name</th><th align='left'>Size</th><th></th><th></th></tr>";
  }
  while (foundfile) {
    if (ishtml) {
      returnText += "<tr align='left'><td>" + String(foundfile.name()) + "</td><td>" + humanReadableSize(foundfile.size()) + "</td>";
      returnText += "<td><button onclick=\"downloadDeleteButton(\'" + String(foundfile.name()) + "\', \'download\')\">Download</button>";
      returnText += "<td><button onclick=\"downloadDeleteButton(\'" + String(foundfile.name()) + "\', \'delete\')\">Delete</button></tr>";
    } else {
      returnText += "File: " + String(foundfile.name()) + " Size: " + humanReadableSize(foundfile.size()) + "\n";
    }
    foundfile = root.openNextFile();
  }
  if (ishtml) {
    returnText += "</table>";
  }
  root.close();
  foundfile.close();
  return returnText;
}

// Make size of files human readable
// source: https://github.com/CelliesProjects/minimalUploadAuthESP32
String humanReadableSize(const size_t bytes) {
  if (bytes < 1024) return String(bytes) + " B";
  else if (bytes < (1024 * 1024)) return String(bytes / 1024.0) + " kB";
  else if (bytes < (1024 * 1024 * 1024)) return String(bytes / 1024.0 / 1024.0) + " MB";
  else return String(bytes / 1024.0 / 1024.0 / 1024.0) + " GB";
}
