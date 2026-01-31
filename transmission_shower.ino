#include "WiFiS3.h"
#include "arduino_secrets.h"
#include <LiquidCrystal.h>
#include <Arduino_JSON.h>
#include <ArduinoHttpClient.h>

char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PASS;        // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;                 // your network key index number (needed only for WEP)"

int status = WL_IDLE_STATUS;
WiFiClient client;


LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

void connectToWifiWithStop() {
  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    while (true);
  }
  
  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Please upgrade the firmware");
  }
  
  // attempt to connect to WiFi network:
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);
     
    // wait 10 seconds for connection:
    delay(10000);
  }
}

void setup() {
  delay(2000);
  Serial.begin(9600);
  Serial.println("Started");  // Debugging
  lcd.begin(16, 2); // Set up display
  lcd.print("Connecting");

  connectToWifiWithStop();

  // you're connected now, so print out the status
  printWiFiStatus();

  if (client.connected()) {
    Serial.println("connected to somebody");
  } else {
    Serial.println("not connected to somebody");
  }

  byte ip[] = {192,168,1,81};
  if (client.connect(ip, 9091)) { // connect to transmission
    lcd.print("y");
  } else {
    lcd.print("x");
  }
  
}

void loop() {

}

void printWiFiStatus() {
  lcd.setCursor(0, 0);
  lcd.print(WiFi.SSID());
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP());
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print where to go in a browser:
  Serial.print("To see this page in action, open a browser to http://");
  Serial.println(ip);

}

