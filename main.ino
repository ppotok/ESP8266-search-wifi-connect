/*
*Description in README. I am sorry for mess in code.
*
*/


#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#include <Time.h>


enum {
  APPLICATION_WEBSERVER = 0,
  ACCESS_POINT_WEBSERVER
};

//MDNSResponder mdns;
ESP8266WebServer server(80);

const char* ssid = "Bubbles";  // Use this as the ssid as well
// as the mDNS name
const char* passphrase = ""; // Here you can put your passphrase for wifi in AP mode

int actualtime[5]; //for showing time

String st;
String content;
unsigned long lasttime = 0;
//----HMTL start and end string
String start = "<!DOCTYPE HTML>\n<html>\n<head>\n<title>Potokovo</title>\n<meta charset='UTF-8'></head>\n<body>";
String htmlend = "</body></html>";
//-------------------------
//-------------------Setup for NTP
IPAddress timeServer(132, 163, 4, 101); // time-a.timefreq.bldrdoc.gov
/* Set this to the offset (in seconds) to your local time
   This example is GMT - 6 
   */
//const long timeZoneOffset = -21600L;

//---- +2h for Czech Republic
int timezone = 2;
const long timeZoneOffset = timezone*3600;
/* Syncs to NTP server every 15 seconds for testing,
   set to 1 hour or more to be reasonable */
unsigned int ntpSyncTime = 15;

/* ALTER THESE VARIABLES AT YOUR OWN RISK */
// local port to listen for UDP packets
unsigned int localPort = 8888;
// NTP time stamp is in the first 48 bytes of the message
const int NTP_PACKET_SIZE = 48;
// Buffer to hold incoming and outgoing packets
byte packetBuffer[NTP_PACKET_SIZE];
// A UDP instance to let us send and receive packets over UDP
WiFiUDP Udp;

unsigned long ntpLastUpdate = 0;
// Check last time clock displayed (Not in Production)
time_t prevDisplay = 0;
//--------------------------

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);  // Assume we've already been configured
  //  Serial.setDebugOutput(true);
  // WiFi.printDiag(Serial);
  if (testWifi()) {
    setupApplication(); // WiFi established, setup application
  } else {
    setupAccessPoint(); // No WiFi yet, enter configuration mode
  }
  int trys = 0;
  while (!getTimeAndDate() && trys < 10) {
    trys++;
  }
}

bool testWifi(void) {
  int c = 0;
  Serial.println("\nWaiting for Wifi to connect...");
  while ( c < 20 ) {
    if (WiFi.status() == WL_CONNECTED) {
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
  if ( webservertype == ACCESS_POINT_WEBSERVER ) { // AP mode webpages
    server.on("/", handleDisplayAccessPoints);
    server.on("/setap", handleSetAccessPoint);
    server.onNotFound(handleNotFound);
  }
  else if (webservertype == APPLICATION_WEBSERVER) { // serving pages when ESP is connected to wifi
    server.on("/", handleRoot);
    server.on("/setap", handleAccessPointAlreadySet);

server.on ( "/time", []() { server.send ( 200, "text/plain", getactualtime() );  } );
  server.on ( "/date", []() { server.send ( 200, "text/plain", getactualdate() );  } );
    
    server.onNotFound(handleNotFound);
  }
}
//--------------------------------------------------------------- End of SETUP of Webpages
void handleDisplayAccessPoints() {
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

  content = start + " This is on / of server" + htmlend;
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

void handledate(){
 content=getactualtime();
 content+=" - ";
 content+=getactualdate();
  server.send(200, "text/plain", content);
}
//------------------END of Webpages
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

  // Update the time via NTP server as often as the time you set at the top
  if (now() - ntpLastUpdate > ntpSyncTime) {
    int trys = 0;
    while (!getTimeAndDate() && trys < 10) {
      trys++;
    }
    if (trys < 10) {
      Serial.println("ntp server update success");
    }
    else {
      Serial.println("ntp server update failed");
    }
  }

  if (millis() - lasttime >= 10000) {
    lasttime = millis();
    Serial.print(getactualtime());
    Serial.print(" - ");
    Serial.println(getactualdate());

  }
}

String getactualtime() {
  String actualtime = "";
  if (hour() < 10) {
    actualtime = "0";
  }
  actualtime += hour();
  actualtime += ":";
  if (minute() < 10) {
    actualtime += "0";
  }
  actualtime += minute();
  actualtime += ":";
  if (second() < 10) {
    actualtime += "0";
  }
  actualtime += second();
  return actualtime;
}

String getactualdate() {
  String actualdate = "";
  if (day() < 10) {
    actualdate = "0";
  }
  actualdate += day();
  actualdate += "/";
  if (month() < 10) {
    actualdate += "0";
  }
  actualdate += month();
  actualdate += "/";
  actualdate += year();
  return actualdate;

}

//----------NTP Part start
// Do not alter this function, it is used by the system

int getTimeAndDate() {
  int flag = 0;
  Udp.begin(localPort);
  sendNTPpacket(timeServer);
  delay(1000);
  if (Udp.parsePacket()) {
    Udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer
    unsigned long highWord, lowWord, epoch;
    highWord = word(packetBuffer[40], packetBuffer[41]);
    lowWord = word(packetBuffer[42], packetBuffer[43]);
    epoch = highWord << 16 | lowWord;
    epoch = epoch - 2208988800 + timeZoneOffset;
    flag = 1;
    setTime(epoch);
    ntpLastUpdate = now();
  }
  return flag;
}

// Do not alter this function, it is used by the system
unsigned long sendNTPpacket(IPAddress& address)
{
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  packetBuffer[0] = 0b11100011;
  packetBuffer[1] = 0;
  packetBuffer[2] = 6;
  packetBuffer[3] = 0xEC;
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;
  Udp.beginPacket(address, 123);
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}

// Clock display of the time and date, save it into array and print
void clockDisplay() {
  actualtime[0] = hour();
  actualtime[1] = minute();
  actualtime[2] = second();
  actualtime[3] = day();
  actualtime[4] = month();
  actualtime[5] = year();
  //-----------------

  for (int i = 0; i <= 2; i++) {
    Serial.print(actualtime[i]);
    Serial.print(":");
  }
  Serial.print(" - ");
  for (int i = 3; i <= 5; i++) {
    Serial.print(actualtime[i]);
    Serial.print("/");
  }
  Serial.println();
}

// Utility function for clock display: prints preceding colon and leading 0
int printDigits(int digits) {
  Serial.print(":");
  if (digits < 10)
    Serial.print('0');
  //Serial.print(digits);
  return digits;
}

//----------NTP END

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
