#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include "StreamUtils.h"
#include "uMQTTBroker.h"

// Electronic
#define CONFIGURATION_MODE_OUTPUT D5 
// #define ledpin D2 //defining the OUTPUT pin for LED
// #define dataDQ D5 // temperature

// Data
String id = "coordinador-07-test";
String name = "";

// Wifi Access Point
String ssid     = id; // Nombre del wifi en modo standalone
String password = "12345678"; //wifi password

// Notify information: Publish when a new user is connected
bool notifyInformation = false;

/// Configuration mode
bool configurationMode = true;
bool configurationModeLightOn = false;

// State JSON
const size_t capacity = 1024; 
DynamicJsonDocument information(capacity); // Information, name, id, ssid...
DynamicJsonDocument memoryJson(capacity); // State, sensors, outputs...

// Convert to json
JsonObject toJson(String str){
  // Serial.println(str);
  DynamicJsonDocument docInput(1024);
  JsonObject json;
  deserializeJson(docInput, str);
  json = docInput.as<JsonObject>();
  return json;
}

/// Returns the JSON converted to String
String jsonToString(DynamicJsonDocument json){
  String buf;
  serializeJson(json, buf);
  return buf;
}

/// Initialize or update the JSON Info of the MQTT Connection (Standalone)
void setInformation(){
  information["id"] = id;
  information["ssid"] = ssid;
  information["standalone"] = false;
  information["configurationMode"] = configurationMode;
}

void setMemoryData(){
  memoryJson["id"] = id;
  memoryJson["ssid"] = ssid;
  memoryJson["password"] = password;
  memoryJson["configurationMode"] = configurationMode;

  EepromStream eepromStream(0, 1024);
  serializeJson(memoryJson, eepromStream);
  EEPROM.commit();

}

/// MQTT Broker ///
class CoordinatorMQTTBroker: public uMQTTBroker
{
public:
    virtual bool onConnect(IPAddress addr, uint16_t client_count) {
      Serial.println(addr.toString()+" connected");
      return true;
    }

    virtual void onDisconnect(IPAddress addr, String client_id) {
      Serial.println(addr.toString()+" ("+client_id+") disconnected");
    }

    virtual bool onAuth(String username, String password, String client_id) {
      Serial.println("Username/Password/ClientId: "+username+"/"+password+"/"+client_id);
      notifyInformation = true;
      return true;
    }
    
    virtual void onData(String topic, const char *data, uint32_t length) {
      char data_str[length+1];
      os_memcpy(data_str, data, length);
      data_str[length] = '\0';
      Serial.println("received topic '"+topic+"' with data '"+(String)data_str+"'");

      // Convert to JSON.
      DynamicJsonDocument docInput(1024); 
      JsonObject json;
      deserializeJson(docInput, (String)data_str);
      json = docInput.as<JsonObject>();
      
      // if (topic == "state/" + id){
      //   Serial.println("Temperature: " + String(json["temperature"]));
      // }

      if(topic == "action/" + id){
        Serial.println("Action: " + String(json["action"]));
        onAction(json);
        // EJECUTAR LAS ACCIONES
        
      }

      
      //printClients();
    }

    // Sample for the usage of the client info methods
    virtual void printClients() {
      for (int i = 0; i < getClientCount(); i++) {
        IPAddress addr;
        String client_id;
         
        getClientAddr(i, addr);
        getClientId(i, client_id);
        Serial.println("Client "+client_id+" on addr: "+addr.toString());
      }
    }
};

CoordinatorMQTTBroker myBroker;

void publishInformation (){
  String informationEncoded = jsonToString(information);
  Serial.println(informationEncoded);
  myBroker.publish("information", informationEncoded);
}

void onAction(JsonObject json){
  String action = json[String("action")];

  if(action.equals("sendState")){
     Serial.println("Recibir y almacenar estado de la nevera");
    //  setData(json, num);
  }
  if(action.equals("confirmConnection")){
    Serial.println("La conexion de la nevera fue verificada y se actualiza los datos");
  }
  if(action.equals("error")){
    Serial.println("Se recibio un error de la nevera");
  }
  if(action.equals("toggleLight")){
    Serial.println("Indicarle a la nevera seleccionada que prenda la luz");
    // toggleLight(json);
  }
  if(action.equals("setMaxTemperature")){
    Serial.println("Indicarle a la nevera seleccionada que cambie su nivel maximo de temperature");
    // setMaxTemperature(json);
  }
  if(action.equals("setMinTemperature")){
    Serial.println("Indicarle a la nevera seleccionada que cambie su nivel minimo de temperature");
    // setMinTemperature(json);
  }
  if(action.equals("setMaxTemperatureForAll")){
    Serial.println("Indicarle a todas las neveras que cambien su nivel maximo de temperature");
  }
  if(action.equals("setMinTemperatureForAll")){
    Serial.println("Indicarle a todas las neveras que cambien su nivel minimo de temperature");
  }
  if(action.equals("delete")){
    Serial.println("Eliminar la nevera indicada");
    // String id = String(json["payload"]["id"]);
    // deleteDevice(id);
  }
  if(action.equals("deleteAll")){
    Serial.println("Eliminar todas las neveras");
  }
}

/// Setup
void startWifiAp(){
  // Init WiFi
  Serial.println("Connecting to wifi");
  IPAddress apIP(192, 168, 0, 1);   //Static IP for wifi gateway
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0)); //set Static IP gateway on NodeMCU
  WiFi.softAP(ssid, password); //turn on WIFI

  Serial.println("AP started");
  Serial.println("IP address: " + WiFi.softAPIP().toString());

}


void setup() {
   // Init Debug Tools
  Serial.begin(9600); //serial start

  // Init WiFi
  startWifiAp();
  // Start the broker
  Serial.println("Starting MQTT Broker");
  myBroker.init();
  myBroker.subscribe("state/" + id);
  myBroker.subscribe("action/" + id);

  setInformation();
  publishInformation();

}

void loop() {
  // Publish info
  if (notifyInformation){
    delay(1000);

    setInformation();
    publishInformation();
    notifyInformation = false;
  }
  // sleep(2);
}
