#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiClient.h>


const int ledPin =  2;
int ledState = LOW; 
unsigned long previousMillis = 0; 
int ledtimer=20000;

enum {
  APPLICATION_WEBSERVER = 0,
  ACCESS_POINT_WEBSERVER
};

ESP8266WebServer server(80);
/*for update*/
const char* host = "esp8266-webupdate";
const char* serverIndex = "<form method='POST' action='/update' enctype='multipart/form-data'><input type='file' name='update'><input type='submit' value='Update'></form>";

const char* ssid = "Bubbles";  // Use this as the ssid as well
// as the mDNS name
const char* passphrase = "";

int actualtime[5]; //for showing time

String st;
String content;
unsigned long lasttime = 0;
//----HMTL start and end string
String start = "<!DOCTYPE HTML>\n<html>\n<head>\n<title>ppotok :-P</title>\n<meta charset=\"UTF-8\"></head>\n<body>";
String htmlend = "</body></html>";

//--------------------------

void setup() {
  pinMode(ledPin, OUTPUT);
  digitalWrite ( ledPin, 0 );
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);  // Assume we've already been configured
  Serial.setDebugOutput(true);
  //WiFi.printDiag(Serial);
  Serial.println("Test for wifi");
  if (testWifi()) {
    setupApplication(); // WiFi established, setup application
  } else {
  
    setupAccessPoint(); // No WiFi yet, enter configuration mode
  }

}

bool testWifi(void) {
  int c = 0;
  Serial.println("\nWaiting for Wifi to connect...");
  while ( c < 20 ) {
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("wifi connected");
      return true;
    }
    delay(500);
    Serial.print(WiFi.status());
    c++;
  }
Serial.println("\nConnect timed out, opening AP");
  return false;
}

void setupApplication() {
  launchWeb(APPLICATION_WEBSERVER); // In this example just launch a
  // web server
}

void setupAccessPoint(void) {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  int n = WiFi.scanNetworks();
  Serial.println("scan done");
  if (n == 0)
    Serial.println("no networks found");
  else
  {
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i)
    {
      // Print SSID and RSSI for each network found
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.print(")");
      Serial.println((WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*");
      delay(10);
    }
  }
  Serial.println("");
  st = "<ol>";
  for (int i = 0; i < n; ++i)
  {
    // Print SSID and RSSI for each network found
    st += "<li>";
    st += WiFi.SSID(i);
    st += " (";
    st += WiFi.RSSI(i);
    st += ")";
    st += (WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*";
    st += "</li>";
  }
  st += "</ol>";
  delay(100);
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, passphrase, 6);
  launchWeb(ACCESS_POINT_WEBSERVER);
}

void launchWeb(int webservertype) {
  Serial.println("\nWiFi connected");
  Serial.print("Local IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("SoftAP IP: ");
  Serial.println(WiFi.softAPIP());
  setupWebServerHandlers(webservertype);
  // Start the server
  server.begin();
  Serial.print("Server type ");
  Serial.print(webservertype);
  Serial.println(" started");

  //  WiFi.printDiag(Serial);
}
//---------------------------------------------------------------SETUP of Webpages
void setupWebServerHandlers(int webservertype)
{
  MDNS.begin(host); //--------mdns
  if ( webservertype == ACCESS_POINT_WEBSERVER ) {
    server.on("/", handleDisplayAccessPoints);
    server.on("/setap", handleSetAccessPoint);
    server.onNotFound(handleNotFound);
  }
  else if (webservertype == APPLICATION_WEBSERVER) { // serving pages when ESP is connected
    server.on("/", handleRoot);
    server.on("/upgrade", HTTP_GET, [](){
      server.sendHeader("Connection", "close");
      server.sendHeader("Access-Control-Allow-Origin", "*");
      server.send(200, "text/html", serverIndex);
    });
    server.on("/setap", handleAccessPointAlreadySet);
    /*UPDATE PART*/
server.onFileUpload([](){
      if(server.uri() != "/update") return;
      HTTPUpload& upload = server.upload();
      if(upload.status == UPLOAD_FILE_START){
        Serial.setDebugOutput(true);
        WiFiUDP::stopAll();
        Serial.printf("Update: %s\n", upload.filename.c_str());
        uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
        if(!Update.begin(maxSketchSpace)){//start with max available size
          Update.printError(Serial);
        }
      } else if(upload.status == UPLOAD_FILE_WRITE){
        if(Update.write(upload.buf, upload.currentSize) != upload.currentSize){
          Update.printError(Serial);
        }
      } else if(upload.status == UPLOAD_FILE_END){
        if(Update.end(true)){ //true to set the size to the current progress
          Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
        } else {
          Update.printError(Serial);
        }
        Serial.setDebugOutput(false);
      }
      yield();
    });
    server.on("/update", HTTP_POST, [](){
      server.sendHeader("Connection", "close");
      server.sendHeader("Access-Control-Allow-Origin", "*");
      server.send(200, "text/plain", (Update.hasError())?"FAIL":"OK");
      abort();
    });
    /*UPDATE PART*/
/*server.on ( "/time", []() { server.send ( 200, "text/plain", getactualtime() );  } );
  server.on ( "/date", []() { server.send ( 200, "text/plain", getactualdate() );  } );*/
    
    server.onNotFound(handleNotFound);
  }
}
//---------------------------------------------------------------SETUP of Webpages
void handleDisplayAccessPoints() {
  ledtimer=500;
  IPAddress ip = WiFi.softAPIP();
  String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
  uint8_t mac[6];
  WiFi.macAddress(mac);
  String macStr = macToStr(mac);
  content = start;
  content += ssid;
  content += " at ";
  content += ipStr;
  content += " (";
  content += macStr;
  content += ")";
  content += "<p>";
  content += st;
  content += "<p><form method='get' action='setap'><label>SSID: </label>";
  content += "<input name='ssid' length=32><input name='pass' length=64><input type='submit'></form></p>";
  content += "</br><p>We will attempt to connect to the selected AP and reset if successful.</p>";

  content += htmlend;
  server.send(200, "text/html", content);
}

void handleSetAccessPoint() {
  int httpstatus = 200;
  String qsid = server.arg("ssid");
  String qpass = server.arg("pass");
  if (qsid.length() > 0 && qpass.length() > 0) {
    for (int i = 0; i < qsid.length(); i++)
    {
      // Deal with (potentially) plus-encoded ssid
      qsid[i] = (qsid[i] == '+' ? ' ' : qsid[i]);
    }
    for (int i = 0; i < qpass.length(); i++)
    {
      // Deal with (potentially) plus-encoded password
      qpass[i] = (qpass[i] == '+' ? ' ' : qpass[i]);
    }
    WiFi.mode(WIFI_AP_STA);
    WiFi.begin(qsid.c_str(), qpass.c_str());
    if (testWifi()) {
      Serial.println("\nGreat Success!");
      delay(3000);
      abort();
    }
    content = start;
    content += "Failed to connect to AP ";
    content += qsid;
    content += ", try again.";
    content += htmlend;
  } else {
    content = start;
    content += "Error, no ssid or password set?";
    content += htmlend;
    Serial.println("Sending 404");
    httpstatus = 404;
  }
  server.send(httpstatus, "text/html", content);
}

void handleRoot() {
  ledtimer=1000;
  digitalWrite(ledPin, LOW);
  content = start + " This is on / of server" + htmlend;
  digitalWrite(ledPin, HIGH);
  server.send(200, "text/html", content);
}

void handleAccessPointAlreadySet() {
  IPAddress ip = WiFi.localIP();
  String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
  uint8_t mac[6];
  WiFi.macAddress(mac);
  String macStr = macToStr(mac);
  content = start;
  content += "You already set up the access point and it is working if you got this far.";
  content += "<p><h3>Device is connected to: ";
  //content += qsid;
  content += WiFi.SSID();
  content += "</h3> With IP:  ";
  content += ipStr;
  content += " and MAC address (";
  content += macStr;
  content += ")";
  content += "</p>";
  content += htmlend;
  server.send(200, "text/html", content);
  
}

//-------------------------------------------------END of Webpages
void handleNotFound() {
  content = "File Not Found\n\n";
  content += "URI: ";
  content += server.uri();
  content += "\nMethod: ";
  content += (server.method() == HTTP_GET) ? "GET" : "POST";
  content += "\nArguments: ";
  content += server.args();
  content += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    content += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", content);
}


//------------------------------------LOOP----------------------------------------------------
void loop() {
  server.handleClient();  // In this example we're not doing too much
 /*  
  *   if (WiFi.status() == WL_CONNECTED) {
     ledtimer=2500;
    }
    */
    unsigned long currentMillis = millis();
 
  if(currentMillis - previousMillis >=ledtimer) {
    // save the last time you blinked the LED 
    previousMillis = currentMillis;   

    // if the LED is off turn it on and vice-versa:
    if (ledState == LOW)
      ledState = HIGH;
    else
      ledState = LOW;

    // set the LED with the ledState of the variable:
    digitalWrite(ledPin, ledState);
  }
}

String macToStr(const uint8_t* mac)
{
  String result;
  for (int i = 0; i < 6; ++i) {
    result += String(mac[i], 16);
    if (i < 5)
      result += ':';
  }
  return result;
}
