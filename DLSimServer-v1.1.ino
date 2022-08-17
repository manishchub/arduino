#include <WiFi.h>                                     // needed to connect to WiFi
#include <WebServer.h>                                // needed to create a simple webserver
#include <WebSocketsServer.h>                         // needed for instant communication between client and server through Websockets
#include <ArduinoJson.h>          // needed for JSON encapsulation (send multiple variables with one string)
#include <WebSocketsClient.h>
#include <FS.h>
#include "SPIFFS.h"


#define FORMAT_SPIFFS_IF_FAILED true


// SSID and password of Wifi connection:
const char* ssid = "ESP32";
const char* password = "1212121212";

// The String below "webpage" contains the complete HTML code that is sent to the client whenever someone connects to the webserver
String webpage = "<!DOCTYPE html><html><head><title>Page Title</title></head><body style='background-color: #EEEEEE;'><span style='color: #003366;'><h1>Lets generate a random number</h1><p>The first random number is: <span id='rand1'>-</span></p><p>The second random number is: <span id='rand2'>-</span></p><p><button type='button' id='BTN_SEND_BACK'>Send info to ESP32</button></p></span></body><script> var Socket; document.getElementById('BTN_SEND_BACK').addEventListener('click', button_send_back); function init() { Socket = new WebSocket('ws://' + window.location.hostname + ':81/'); Socket.onmessage = function(event) { processCommand(event); }; } function button_send_back() { var msg = {brand: 'Gibson',type: 'Les Paul Studio',year: 2010,color: 'white'};Socket.send(JSON.stringify(msg)); } function processCommand(event) {var obj = JSON.parse(event.data);document.getElementById('rand1').innerHTML = obj.rand1;document.getElementById('rand2').innerHTML = obj.rand2; console.log(obj.rand1);console.log(obj.rand2); } window.onload = function(event) { init(); }</script></html>";

// The JSON library uses static memory, so this will need to be allocated:
StaticJsonDocument<1024> doc_tx;                       // provision memory for about 300 characters
StaticJsonDocument<512> doc_rx;


//List of DLSims
IPAddress ipAddr;

// We want to periodically send values to the clients, so we need to define an "interval" and remember the last time we sent data to the client (with "previousMillis")
int interval = 1000;                                  // send data to the client every 1000ms -> 1s
unsigned long previousMillis = 0;                     // we use the "millis()" command for time reference and this will output an unsigned long

// Initialization of webserver and websocket
WebServer server(80);                                 // the server uses port 80 (standard port for websites
WebSocketsServer webSocket = WebSocketsServer(81);    // the websocket uses port 81 (standard port for websockets

void setup() {
  Serial.begin(115200);                               // init serial port for debugging

  // Connect to Wi-Fi network with SSID and password
  Serial.print("Setting AP (Access Point)…");
  // Remove the password parameter, if you want the AP (Access Point) to be open


    
  WiFi.mode(WIFI_MODE_APSTA);
   // Set static IP
  IPAddress AP_LOCAL_IP(192, 168, 0, 69);
  IPAddress AP_GATEWAY_IP(192, 168, 0, 69);
  IPAddress AP_NETWORK_MASK(255, 255, 255, 0);
  if (!WiFi.softAPConfig(AP_LOCAL_IP, AP_GATEWAY_IP, AP_NETWORK_MASK)) {
    Serial.println("AP Config Failed");
    return;
  }
  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.println(IP);

  server.on("/", []() {                               // define here wat the webserver needs to do
    server.send(200, "text\html", webpage);           //    -> it needs to send out the HTML string "webpage" to the client
    // NOTE: if you use Edge or IE, then use:
    //server.send(200, "text/html", webpage);
  });
  server.begin();                                     // start server
  
  webSocket.begin();                                  // start websocket
  webSocket.onEvent(webSocketEvent);                  // define a callback function -> what does the ESP32 need to do when an event from the websocket is received? -> run function "webSocketEvent()"

  
  //allows serving of files from SPIFFS
  if(!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)){
        Serial.println("SPIFFS Mount Failed");
        return;
    }
    
 
}


void loop() {
  server.handleClient();                              // Needed for the webserver to handle all clients
  webSocket.loop();                                   // Update function for the webSockets 
  
  unsigned long now = millis();                       // read out the current "time" ("millis()" gives the time in ms since the Arduino started)
  if ((unsigned long)(now - previousMillis) > interval) { // check if "interval" ms has passed since last time the clients were updated
    
    String jsonString = "";                           // create a JSON string for sending data to the client
    JsonObject object = doc_tx.to<JsonObject>();      // create a JSON Object
    object["rand1"] = random(100);                    // write data into the JSON object -> I used "rand1" and "rand2" here, but you can use anything else
    object["rand2"] = random(100);
    serializeJson(doc_tx, jsonString);                // convert JSON object to string
   // Serial.println(jsonString);                       // print JSON string to console for debug purposes (you can comment this out)
   // webSocket.broadcastTXT(jsonString);               // send JSON string to clients
    
    previousMillis = now;                             // reset previousMillis
  }

  
}

void webSocketEvent(byte num, WStype_t type, uint8_t * payload, size_t length) {      // the parameters of this callback function are always the same -> num: id of the client who send the event, type: type of message, payload: actual data sent and length: length of payload
  switch (type) {                                     // switch on the type of information sent
    case WStype_DISCONNECTED:                         // if a client is disconnected, then type == WStype_DISCONNECTED
    //  Serial.println("Client " + String(num) + " disconnected");
      break;
    case WStype_CONNECTED:                            // if a client is connected, then type == WStype_CONNECTED
      Serial.println("Client " + String(num) + " connected");
      Serial.println("Got from Client: " + String((char *)payload));

      char * _payload;
      
      _payload=(char *)payload;

      // if client invoked connect api with /?tsn=9090

      if((String(_payload).compareTo("/?tsn=9090"))==0){
        String _send_spi_response=read_json_file(SPIFFS,"/api_connect_response.json");
        //serializeJson(doc_tx,_send_spi_response);
        // send the api response to client
        webSocket.sendTXT(num,_send_spi_response);
       // Serial.println("Read from file... "+_send_spi_response);
        
      }

             
      break;
    case WStype_TEXT:                                 // if a client has sent data, then type == WStype_TEXT
      
      
      char * command=(char*)payload;
      Serial.println("Got from Client: " + String(command));    
            
      // try to decipher the JSON string received
      DeserializationError error = deserializeJson(doc_rx, payload);
      const char * _action=doc_rx["action"];
      if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        return;
      }
      else {

        // got from client open request

        if((String(_action).compareTo("openLockers"))==0){
        String _send_spi_response=read_json_file(SPIFFS,"/api_open_response.json");
        //serializeJson(doc_tx,_send_spi_response);
        // send the api response to client

        //open the WDLSim

        OpenLockWDLSim(0);

        
        webSocket.sendTXT(num,_send_spi_response);
       // Serial.println("Read from file... "+_send_spi_response);
        
      }
       
      }
      Serial.println("");
      break;
  }
}

String read_json_file(fs::FS &fs, const char * path)
{
     
   Serial.printf("Reading file... ");

   String _json_data="";
   String line_buff="";
    File file = fs.open(path);
    if(!file || file.isDirectory()){
        Serial.println("- failed to open file for reading");
        return "Error";
    }

    Serial.println("- read from file:");
    while(file.available()){
     // Serial.write(file.read());
     //file.read(line_buff,1024);
     //file.readBytesUntil('\n', line_buff, 1024);
      //char* str=(char*)line_buff;
     // String str=String(line_buff);

     line_buff=file.readStringUntil('\n');
     _json_data.concat(line_buff);
     
     //Serial.println(line_buff);
    }
    file.close();
    //Serial.println("_json_data "+_json_data);
    return (String)_json_data;
}


// WDLSim Open Lock

void OpenLockWDLSim(byte num){



  String jsonString = "";                           // create a JSON string for sending data to the client
    JsonObject _object1 = doc_tx.to<JsonObject>();      // create a JSON Object
     String _jsonString = "";  
     _object1["data_key"]="DLSIM_CTRL_OPEN";
     _object1["data_val"]="OPEN";
    serializeJson(doc_tx, _jsonString);                // convert JSON object to string
    Serial.println(_jsonString);                       // print JSON string to console for debug purposes (you can comment this out)
    webSocket.sendTXT(num,_jsonString);               // send JSON string to clients


}
