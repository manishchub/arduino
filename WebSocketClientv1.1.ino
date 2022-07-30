// ---------------------------------------------------------------------------------------
//
// Code for a simple webserver on the ESP32 (device used for tests: ESP32-WROOM-32D).
// The code generates two random numbers on the ESP32 and uses Websockets to continuously
// update the web-clients. For data transfer JSON encapsulation is used.
//
// For installation, the following libraries need to be installed:
// * Websockets by Markus Sattler (can be tricky to find -> search for "Arduino Websockets"
// * ArduinoJson by Benoit Blanchon
//
// NOTE: in principle this code is universal and can be used on Arduino AVR as well. However, AVR is only supported with version 1.3 of the webSocketsServer. Also, the Websocket
// library will require quite a bit of memory, so wont load on Arduino UNO for instance. The ESP32 and ESP8266 are cheap and powerful, so use of this platform is recommended. 
//
// Refer to https://youtu.be/15X0WvGaVg8
//
// Written by mo thunderz (last update: 11.09.2021)
//
// ---------------------------------------------------------------------------------------

#include <WiFi.h>                                     // needed to connect to WiFi

#include <WebSocketsServer.h>                         // needed for instant communication between client and server through Websockets
#include <ArduinoJson.h>                              // needed for JSON encapsulation (send multiple variables with one string)
#include <WebSocketsClient.h>
#include <StreamUtils.h>

// SSID and password of Wifi connection:
const char* ssid = "netter";
const char* password = "1212121212";
WebSocketsClient webSocketCl;


// The JSON library uses static memory, so this will need to be allocated:
StaticJsonDocument<200> doc_tx;                       // provision memory for about 200 characters
StaticJsonDocument<200> doc_rx;
StaticJsonDocument<200> doc;

// We want to periodically send values to the clients, so we need to define an "interval" and remember the last time we sent data to the client (with "previousMillis")
int interval = 1000;                                  // send data to the client every 1000ms -> 1s
unsigned long previousMillis = 0;                     // we use the "millis()" command for time reference and this will output an unsigned long

// Initialization of webserver and websocket
//WebServer server(80);                                 // the server uses port 80 (standard port for websites
//WebSocketsServer webSocket = WebSocketsServer(81);    // the websocket uses port 81 (standard port for websockets

void setup() {
  Serial.begin(115200);                               // init serial port for debugging

   // Connect to local WiFi
  WiFi.begin("netter","1212121212");
 
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP()); // Print local IP address


  
  webSocketCl.begin("192.168.0.69", 81, "/");
  // WebSocket event handler
  webSocketCl.onEvent(webSocketEvent);
  webSocketCl.setReconnectInterval(5000);
}

void loop() {
  //server.handleClient();                              // Needed for the webserver to handle all clients
  webSocketCl.loop();                                   // Update function for the webSockets 

 // send heartbeats to Server from Node MCU 19
   // send json object to server
        JsonObject object1 = doc_tx.to<JsonObject>();
        String jsonString = "";  
        object1["brand"]="DLSim20";
        object1["type"]="DLSim20";
        object1["year"]="DLSim20";
        object1["color"]="DLSim20";
        serializeJson(doc_tx, jsonString);
        Serial.println(jsonString);
        webSocketCl.sendTXT(jsonString); 
  

}
 
void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {      // the parameters of this callback function are always the same -> num: id of the client who send the event, type: type of message, payload: actual data sent and length: length of payload
 
      
    switch (type) {                                     // switch on the type of information sent
    case WStype_DISCONNECTED:                         // if a client is disconnected, then type == WStype_DISCONNECTED
      Serial.println("Server disconnected");
      break;
    case WStype_CONNECTED:                            // if a client is connected, then type == WStype_CONNECTED
      Serial.println("Server connected");
      // optionally you can add code here what to do when connected
      break;
    case WStype_TEXT:                                 // if a client has sent data, then type == WStype_TEXT
      // try to decipher the JSON string received
      DeserializationError error = deserializeJson(doc_rx, payload);
      Serial.println("Got TEXT from Server...");
      if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        return;
      }
      else {
        // JSON string was received correctly, so information can be retrieved:
       
        const String& rnd1=doc_rx["rand1"];        
        const String& rnd2=doc_rx["rand2"];
        Serial.println("rnd1:" + String(rnd1));
        Serial.println("rnd1:" + String(rnd2));
       

             
        
      }
      Serial.println(length);
      break;
  }










      
     
  
}
