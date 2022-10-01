#include <DHT.h>
#include <WiFi.h>
#include <FirebaseESP32.h>
#define DHTPIN 5
#define DHTTYPE DHT11
#define FIREBASE_HOST "My firebase link"
#define FIREBASE_AUTH "My firebasse secret password"

const char* SSID = "Wifi name";
const char* PASSWORD = "Wifi password";

// Humidor ranges
int MIN_IDEAL_TEMPERATURE = 16;
int MAX_IDEAL_TEMPERATURE = 22;
int MIN_IDEAL_HUMIDITY = 68;
int MAX_IDEAL_HUMIDITY = 75;

WiFiServer server(80);
String request;
DHT dht(DHTPIN, DHTTYPE);
FirebaseData fData;
FirebaseJson json;
size_t updateTime; // For updating values in the database

void setup() {
  Serial.begin(115200);
  dht.begin();

  // Connect to Wi-Fi
  Serial.print("Connecting to: ");
  Serial.print(SSID);
  WiFi.begin(SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  // Print local IP address 
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP()); 

  // Start web server
  server.begin(); 

  // Start database connection
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);
  Firebase.setReadTimeout(fData, 1000*60);
  Firebase.setwriteSizeLimit(fData, "tiny");
  updateTime = millis();
}

void loop() {
  //Read values
  float t = dht.readTemperature();
  float h = dht.readHumidity();

  if (millis() - updateTime > 1000) {
    json.set("/temperature", t);
    json.set("/humidity", h);
    Firebase.set(fData, "/values", json);
    updateTime = millis();
  }

  // Listen for incoming clients
  WiFiClient client = server.available();  

  if (client) {
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        request += c;

        // Handle our basic request
        if (c == '\n') {
          String temperature = String(t, 1);
          String humidity = String(h, 1);

          // HTML code
          client.println("<!DOCTYPE html><html>");
          client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
          client.println("<link rel=\"icon\" href=\"data:,\">");
          client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
          client.println(".button {background-color: green; border: none; color: white; padding: 16px 40px;");
          client.println("text-decoration: none; font-size: 30px; margin: 2px;}");
          client.println(".buttonRed {background-color: red;}</style></head>");
          client.println("<body><h1>HUMIDOR WEB SERVER</h1>");
          client.println("<p>TEMPERATURE</p>");

          // Change button color to red if values are not between the target ones
          if (t <= MAX_IDEAL_TEMPERATURE && t >= MIN_IDEAL_TEMPERATURE) {
            client.println("<p><button class=\"button\">"+temperature+"</button></p>");
          } else {
            client.println("<p><button class=\"button buttonRed\">"+temperature+"</button></p>");
          }
          client.println("<p>HUMIDITY</p>");
          if (h <= MAX_IDEAL_HUMIDITY && h >= MIN_IDEAL_HUMIDITY) {
            client.println("<p><button class=\"button\">"+humidity+"</button></p>");
          } else {
            client.println("<p><button class=\"button buttonRed\">"+humidity+"</button></p>");
          }

          client.println("</body></html>");
          break;
        }
      }
    }
    client.stop();
  }
}


