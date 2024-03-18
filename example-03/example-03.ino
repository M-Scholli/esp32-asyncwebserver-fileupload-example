#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "FS.h"
#include "FFat.h"
#include "page.h" // const char index_html[]

const String default_ssid = "somessid";
const String default_wifipassword = "mypassword";
const int default_webserverporthttp = 80;

String listFiles(bool ishtml = false);

// configuration structure
struct Config {
  String ssid;            // wifi ssid
  String wifipassword;    // wifi password
  int webserverporthttp;  // http port number for web admin
};

// variables
Config config;           // configuration
AsyncWebServer *server;  // initialise webserver

volatile uint32_t progressLen = 0;

void setup() {
  Serial.begin(115200);

  delay(1000);

  Serial.println("Booting ...");

  Serial.println("Mounting FatFS ...");

  //   Serial.println("FatFS, formatting");
  // #warning "WARNING ALL DATA WILL BE LOST: FFat.format()"
  //   FFat.format();

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

  // To format all space in FFat
  // FFat.format()

  // Get all information of your FFat
  Serial.print("FatFS Free: ");
  Serial.println(humanReadableSize(FFat.freeBytes()));
  Serial.print("FatFS Used: ");
  Serial.println(humanReadableSize(FFat.usedBytes()));
  Serial.print("FatFS Total: ");
  Serial.println(humanReadableSize(FFat.totalBytes()));

  Serial.println(listFiles());

  Serial.println("\nLoading Configuration ...");

  config.ssid = default_ssid;
  config.wifipassword = default_wifipassword;
  config.webserverporthttp = default_webserverporthttp;

  Serial.print("\nConnecting to Wifi: ");
  WiFi.begin(config.ssid.c_str(), config.wifipassword.c_str());
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\n\nNetwork Configuration:");
  Serial.println("----------------------");
  Serial.print("         SSID: ");
  Serial.println(WiFi.SSID());
  Serial.print("  Wifi Status: ");
  Serial.println(WiFi.status());
  Serial.print("Wifi Strength: ");
  Serial.print(WiFi.RSSI());
  Serial.println(" dBm");
  Serial.print("          MAC: ");
  Serial.println(WiFi.macAddress());
  Serial.print("           IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("       Subnet: ");
  Serial.println(WiFi.subnetMask());
  Serial.print("      Gateway: ");
  Serial.println(WiFi.gatewayIP());
  Serial.print("        DNS 1: ");
  Serial.println(WiFi.dnsIP(0));
  Serial.print("        DNS 2: ");
  Serial.println(WiFi.dnsIP(1));
  Serial.print("        DNS 3: ");
  Serial.println(WiFi.dnsIP(2));

  // configure web server
  Serial.println("\nConfiguring Webserver ...");
  server = new AsyncWebServer(config.webserverporthttp);
  configureWebServer();

  // startup web server
  Serial.println("Starting Webserver ...");
  server->begin();
}

void loop() {
}

void rebootESP(String message) {
  Serial.print("Rebooting ESP32: ");
  Serial.println(message);
  ESP.restart();
}

// list all of the files, if ishtml=true, return html rather than simple text
String listFiles(bool ishtml) {
  String returnText = "";
  Serial.println("Listing files stored on FatFS");
  File root = FFat.open("/");
  File foundfile = root.openNextFile();
  if (ishtml) {
    returnText += "<table><tr><th align='left'>Name</th><th align='left'>Size</th></tr>";
  }
  while (foundfile) {
    if (ishtml) {
      returnText += "<tr align='left'><td>" + String(foundfile.name()) + "</td><td>" + humanReadableSize(foundfile.size()) + "</td></tr>";
    } else {
      returnText += "File: " + String(foundfile.name()) + "\n";
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

void configureWebServer() {
  server->on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    String logmessage = "Client:" + request->client()->remoteIP().toString() + +" " + request->url();
    Serial.println(logmessage);
    request->send_P(200, "text/html", index_html, processor);
  });

  // run handleUpload function when any file is uploaded
  server->on(
    "/upload", HTTP_POST, [](AsyncWebServerRequest *request) {
      request->send(200);
    },
    handleUpload);

  server->on(
    "/readProgress", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send(200, "text/html", String(progressLen));
    });
}

// handles uploads
void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
  String logmessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url();
  Serial.println(logmessage);

  if (!index) {
    logmessage = "Upload Start: " + String(filename);
    // open the file on first call and store the file handle in the request object
    request->_tempFile = FFat.open("/" + filename, "w");
    Serial.println(logmessage);
  }

  if (len) {
    // stream the incoming chunk to the opened file
    progressLen = index;
    request->_tempFile.write(data, len);
    logmessage = "Writing file: " + String(filename) + " index=" + String(index) + " len=" + String(len);
    Serial.println(logmessage);
  }

  if (final) {
    progressLen = index + len;

    logmessage = "Upload Complete: " + String(filename) + ",size: " + String(index + len);
    // close the file handle as the upload is now done
    request->_tempFile.close();
    Serial.println(logmessage);
    request->redirect("/");
  }
}

String processor(const String &var) {
  // Get all information of your FFat

  if (var == "FILELIST") {
    return listFiles(true);
  }
  if (var == "FREEFATFS") {
    return humanReadableSize(FFat.freeBytes());
  }

  if (var == "USEDFATFS") {
    return humanReadableSize(FFat.usedBytes());
  }

  if (var == "TOTALFATFS") {
    return humanReadableSize(FFat.totalBytes());
  }

  return String();
}
