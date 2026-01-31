#include "WiFiS3.h"
#include "arduino_secrets.h"
#include <LiquidCrystal.h>
#include <Arduino_JSON.h>
#include <ArduinoHttpClient.h>

char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PASS;        // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;                 // your network key index number (needed only for WEP)"

int status = WL_IDLE_STATUS;

char serverAddress[] = SECRET_SERVER_IP;  // server address
int port = SECRET_SERVER_PORT;
WiFiClient wifi;
HttpClient client = HttpClient(wifi, serverAddress, port);
char reqPath[] = "/transmission/rpc";
String sessionId = "";


LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

bool getSessionId() {
  String emptyBody = "{}";
  
  client.beginRequest();
  client.post("/transmission/rpc");
  client.sendHeader("Content-Type", "application/json");
  client.sendHeader("Content-Length", String(emptyBody.length()));
  client.beginBody();
  client.print(emptyBody);
  client.endRequest();
  
  int httpCode = client.responseStatusCode();
  Serial.print("HTTP Code: ");
  Serial.println(httpCode);
  
  if (httpCode == 409) {
    // Read response headers manually
    sessionId = readHeader("X-Transmission-Session-Id");
    Serial.print("Session ID: ");
    Serial.println(sessionId);
    return true;
  }
  
  return false;
}

// Function to read a specific header from response
String readHeader(String headerName) {
  String headerValue = "";
  
  // Read response line by line
  while (client.available()) {
    String line = client.readStringUntil('\n');
    line.trim();
    
    // Check if this line contains our header
    if (line.startsWith(headerName + ":")) {
      headerValue = line.substring(line.indexOf(':') + 1);
      headerValue.trim();
      break;
    }
    
    // Empty line means end of headers
    if (line.length() == 0) {
      break;
    }
  }

  // Clear any remaining response body (important!)
  while (client.available()) {
    client.read();
  }
  
  return headerValue;
}

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

  if (getSessionId()) {
    Serial.println("Session ID obtained successfully");
  }

  
  JSONVar request;
  request["jsonrpc"] = "2.0";
  request["method"] = "torrent_get";
  request["id"] = 1;
  
  // Create params object
  JSONVar params;
  JSONVar fields;
  fields[0] = "rate_download";
  fields[1] = "rate_upload";
  fields[2] = "name";
  params["fields"] = fields;
  params["ids"] = "recently_active";
  request["params"] = params;  // Empty params object
  
  String body = JSON.stringify(request);
  Serial.println(body);
  
  // Send request - CORRECT ORDER:
  client.beginRequest();                     // Start request
  client.post(reqPath);                      // POST method with path
  client.sendHeader("Content-Type", "application/json");  // Header
  client.sendHeader("Content-Length", String(body.length()));     // Content length
  client.sendHeader("X-Transmission-Session-Id", sessionId); // CSRF token
  client.beginBody();                        // Start body section
  client.print(body);                        // Send body
  client.endRequest();                       // End request
  
  // Get response
  int httpCode = client.responseStatusCode();
  String response = client.responseBody();
  
  Serial.print("HTTP Code: ");
  Serial.println(httpCode);
  Serial.print("Response: ");
  Serial.println(response);
  
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

