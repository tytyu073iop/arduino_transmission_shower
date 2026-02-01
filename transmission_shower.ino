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

struct Responce {
  JSONVar var;
  int response;
};


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

Responce getActiveTorrents() {
  
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
  Serial.println(response);

  // Parse JSON response
  JSONVar jsonResponse = JSON.parse(response);
  Serial.print("format inside: ");
  Serial.println(jsonResponse["result"]["torrents"]);
  
  // Create Responce object properly
  Responce resp;
  resp.response = httpCode;
  
  if (httpCode == 200 && JSON.typeof(jsonResponse) != "undefined") {
    // Check if we have the expected structure
    if (JSON.typeof(jsonResponse["result"]) != "undefined" &&
        JSON.typeof(jsonResponse["result"]["torrents"]) != "undefined") {
      resp.var = JSONVar(jsonResponse["result"]["torrents"]);
    } else {
      Serial.println("No torrents or wrong structure");
      resp.var = JSONVar();  // Empty JSONVar
    }
  } else {
    Serial.println("Error or invalid response");
    resp.var = JSONVar();  // Empty JSONVar
  }
  
  return resp;
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

  
  
}

JSONVar customMax(JSONVar a, JSONVar b) {
  // Convert to double first, then to long long
  double ard_double = (double)a["rate_download"];
  double brd_double = (double)b["rate_download"];
  long long ard = (long long)ard_double;
  long long brd = (long long)brd_double;
  
  if (ard > 0 || brd > 0) {
    return ard > brd ? a : b;
  } else {
    double aru_double = (double)a["rate_upload"];
    double bru_double = (double)b["rate_upload"];
    long long aru = (long long)aru_double;
    long long bru = (long long)bru_double;
    return aru > bru ? a : b;
  }
}

// Function to format B/s to human readable
String formatBps(long long bps) {
  const char* units[] = {"B/s", "KB/s", "MB/s", "GB/s"};
  
  if (bps == 0) return "0 B/s";
  
  // Handle negative values
  bool isNegative = bps < 0;
  if (isNegative) bps = -bps;
  
  int unitIndex = 0;
  double value = (double)bps;
  
  // Use 1024 for storage/network bytes
  while (value >= 1024.0 && unitIndex < 3) {
    value /= 1024.0;
    unitIndex++;
  }
  
  String result = "";
  if (isNegative) result += "-";
  
  // Smart formatting based on value size
  if (value < 10.0) {
    result += String(value, 2);
  } else if (value < 100.0) {
    result += String(value, 1);
  } else {
    result += String(value, 0);
  }
  
  result += units[unitIndex];
  return result;
}

void formatResult(JSONVar m, String down, String up) {
  Serial.print("formated: ");
  Serial.println(m);
  int spaces = 16 - 2 -  down.length() - up.length();
  String dis1 = "d";
  dis1.concat(down);
  for (int i = 0; i < spaces; i++) {
    dis1.concat(" ");
  }
  dis1.concat("u" + up);


   // If m["rate_download"] contains a JSONVar, cast it properly
  double downloadValue = (double)m["rate_download"];
  double uploadValue = (double)m["rate_upload"];
  long long downloadLL = (long long)downloadValue;
  long long uploadLL = (long long)uploadValue;
  String downt = formatBps(downloadLL);
  String upt = formatBps(uploadLL);

  int spLen = (double) m["rate_download"] > 0 ? downt.length() : upt.length();
  spLen += 2;
  String dis2 = ((String) m["name"]).substring(0, 16 - spLen);
  dis2.concat("|");
  dis2.concat((double) m["rate_download"] > 0 ? "d" : "u");
 
  
  dis2.concat(downloadLL > 0 ? downt : upt);

  lcd.setCursor(0,0);
  lcd.print(dis1);
  Serial.print("dis2: ");
  Serial.println(dis2);
  lcd.setCursor(0,1);
  lcd.print("               "); //16 spaces
  lcd.setCursor(0,1);
  lcd.print(dis2);
}

void showResultOnDisplay(JSONVar var) {
  long long sum_down = 0;
  long long sum_up = 0;
  Serial.print("j: ");
  Serial.println(var);
  JSONVar varMax = var[0];
  
  for(size_t i = 0; i < var.length(); i++) {
    Serial.print("i: ");
    Serial.println(var[i]);
    // Convert JSONVar to long long before adding
    long long download = (long long)(double)var[i]["rate_download"];
    long long upload = (long long)(double)var[i]["rate_upload"];
    
    sum_down += download;
    sum_up += upload;
    
    varMax = customMax(var[i], varMax);
  }
  
  String textDown = formatBps(sum_down);
  String textUp = formatBps(sum_up);
  formatResult(varMax, textDown, textUp);
  
  // Use textDown and textUp as needed...
}

void loop() {
  Responce resp = getActiveTorrents();
  Serial.print("q: ");
  Serial.println(resp.var);
  switch (resp.response) {
    case 409:
      if(!getSessionId()) {
        Serial.println("Cannot obtain session");
      }
      break;
    case 200:
      showResultOnDisplay(resp.var);
      break;
  }
  delay(5000); // 5 sec
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

}

