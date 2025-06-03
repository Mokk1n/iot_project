//connected libraries
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
//WiFi and MQTT settings
const char* ssid = "";
const char* password = "";
const char* mqttServer = "broker.emqx.io";
const int mqttPort = 1883;
const char* mqttTopic = "traffic_light/control";
const char* mqttResponseTopic = "traffic_light/response";
WiFiClient espClient; //object for connecting to a Wi-Fi network
PubSubClient client(espClient); //object to connect and interact with MQTT broker

class TrafficLightContext; // Forward declaration

//state
class TrafficLightState {
public:
  //method in each specific state
  virtual void update(TrafficLightContext& trafficContext) = 0;
};

//specific states(RegulatedState)
class RegulatedState : public TrafficLightState {
public:
  void update(TrafficLightContext& trafficContext) override;
};

//specific states(UnregulatedState)
class UnregulatedState : public TrafficLightState {
public:
  void update(TrafficLightContext& trafficContext) override;
};

//setting the traffic light
class TrafficLightContext {
public:
  const int redCar; //pin for red light for car
  const int yellowCar; //pin for yellow light for car
  const int greenCar; //pin for green light for car
  const int redPed; //pin for red light for pedestrian 
  const int greenPed; //pin for green light for pedestrian
  const int button; //pin for button
  const int buzzer; //pin for buzzer
  TrafficLightState* currentState; //current traffic light status
  const int crossTime; //road crossing time
  unsigned long changeTime; //last state change time
  bool yellowCarWorking; //yellow light operability
  unsigned long lastChange; //local variable saves last action time

  //constructor
  TrafficLightContext(int pinRedCar, int pinYellowCar, int pinGreenCar, int pinRedPed, int pinGrennPed, int pinButton, int pinBuzzer) : currentState(new RegulatedState()),
  redCar(pinRedCar), yellowCar(pinYellowCar), greenCar(pinGreenCar), redPed(pinRedPed), greenPed(pinGrennPed), button(pinButton), buzzer(pinBuzzer), crossTime(5000), yellowCarWorking(false) {}

  //pin initialization
  void begin_setup() {
    Serial.begin(9600); //start the serial port
    pinMode(redCar, OUTPUT);
    pinMode(yellowCar, OUTPUT);
    pinMode(greenCar, OUTPUT);
    pinMode(redPed, OUTPUT);
    pinMode(greenPed, OUTPUT);
    pinMode(button, INPUT);
    pinMode(buzzer, OUTPUT);
    digitalWrite(buzzer, LOW);
    digitalWrite(greenCar, HIGH); //starting position - green for auto
    digitalWrite(redPed, HIGH); //starting position - red for pedestrians
    changeTime = millis(); //remember the current time
    lastChange = millis(); //local variable saves last action time
  }
  //change the current state
  void setState(TrafficLightState* state) {
    if (currentState) delete currentState; //delete the previous state to avoid memory leakage
    currentState = state;
  }
  //function of calling the current state logic
  void handle() {
    if (currentState) currentState->update(*this);
  }
  //turn off all LEDs
  void allLightsOff() {
    digitalWrite(redCar, LOW); //turn off red for cars
    digitalWrite(yellowCar, LOW); //turn off yellow for cars
    digitalWrite(greenCar, LOW); //turn off green for cars
    digitalWrite(redPed, LOW); //turn off red for pedestrians
    digitalWrite(greenPed, LOW); //turn off green for pedestrians
  }
  //flashing green pedestrian light (end of transition phase)
  void flashGreenPed() {
    for (int x = 0; x < 10; x++) {
      digitalWrite(greenPed, LOW); //turn off green for pedestrians 
      delay(100);
      digitalWrite(greenPed, HIGH); //turn on green for pedestrians
      delay(100);
    }
  }
  //yellow flashing - used in unregulated mode
  void flashYellowLight() {
    allLightsOff();
    for (int i = 0; i < 5; i++) {
      digitalWrite(yellowCar, HIGH); //turn on yellow for cars
      delay(300);
      digitalWrite(yellowCar, LOW); //turn off yellow for cars
      delay(300);
    }
  }
  //normal traffic light change
  void changeLights() {
    digitalWrite(redPed, HIGH); //starting position - red for pedestrians  
    client.publish(mqttResponseTopic, "Changing the traffic signal...");
    digitalWrite(greenCar, LOW); //turn off green for cars
    yellowCarWorking = random(0, 2); //50/50 does yellow work
    if (yellowCarWorking == true) {
      digitalWrite(yellowCar, HIGH); //turn on yellow for cars
      client.publish(mqttResponseTopic, "Yellow light is working");
    } else {
      digitalWrite(yellowCar, LOW); //turn off yellow for cars
      client.publish(mqttResponseTopic, "Yellow light is defective. Skipping yellow...");
    }
    delay(2000);
    digitalWrite(yellowCar, LOW); //turn off yellow for cars
    digitalWrite(redCar, HIGH); //turn on car for cars
    client.publish(mqttResponseTopic, "Red light for cars");
    delay(crossTime); //road crossing time
    digitalWrite(redPed, LOW); //turn off car for pedestrians
    client.publish(mqttResponseTopic, "Green light for pedestrians");
    digitalWrite(greenPed, HIGH); //turn on green for pedestrians
    delay(crossTime); //road crossing time
    flashGreenPed(); //flashing green for pedestrians
    client.publish(mqttResponseTopic, "Green light for cars, red for pedestrians");
    digitalWrite(greenPed, LOW); //turn off green for pedestrians
    digitalWrite(redCar, LOW); //turn off car for cars
    digitalWrite(redPed, HIGH); //turn on car for pedestrians
    digitalWrite(greenCar, HIGH); //turn on green for cars
    delay(crossTime); //road crossing time
    changeTime = millis(); //remember the current time
  }
};

//implement the logic of each state
//RegulatedState
void RegulatedState::update(TrafficLightContext& trafficContext) {
   if (digitalRead(trafficContext.button) == HIGH || (millis() - trafficContext.changeTime) > trafficContext.crossTime) {
    if (digitalRead(trafficContext.button) == HIGH){
      client.publish(mqttResponseTopic, "Pedestrian pressed the button");
    }
    trafficContext.changeLights(); //normal traffic light change (no fault check)
  }
}

//UnregulatedState
void UnregulatedState::update(TrafficLightContext& trafficContext) {
  if (millis() - trafficContext.lastChange > trafficContext.crossTime) {
    trafficContext.lastChange = millis();
    trafficContext.yellowCarWorking = random(0, 2); //50/50 does yellow work
    if (trafficContext.yellowCarWorking == true) {
      client.publish(mqttResponseTopic, "Unregulated intersection - yellow light flashing");
      trafficContext.flashYellowLight(); //yellow flashing - used in unregulated mode
    } else {
      client.publish(mqttResponseTopic, "The yellow light is defective!");
      client.publish(mqttResponseTopic, "Unregulated intersection - all traffic lights are off");
      client.publish(mqttResponseTopic, "ATTENTION!!!!!!!!!!!");
      client.publish(mqttResponseTopic, "Attention to traffic signs");
      trafficContext.allLightsOff(); //turn off all LEDs
      client.publish(mqttResponseTopic, "Attention sound signal");
      //sound hazard signal
      for (int i = 0; i < 3; i++) {
        //internal loop to increase tone:
        for (int f = 400; f <= 1000; f += 10) {
          tone(trafficContext.buzzer, f); 
          delay(10);
        }
        //inner loop for tone reduction
        for (int f = 1000; f >= 400; f -= 10) {
          tone(trafficContext.buzzer, f); 
          delay(10);
        }
        noTone(trafficContext.buzzer); //mute
      }
    }
  }
}


//MQTT class 
class MQTTHandler {
public:
  TrafficLightContext& trafficContext1; //link to a traffic light object (to change its states)
  //constructor
  MQTTHandler(TrafficLightContext& object) : trafficContext1(object) {}
  //client MQTT configuration
  void setup() {
    client.setServer(mqttServer, mqttPort); //set the address and port to connect to the MQTT broker
    //set a callback to process received messages
    client.setCallback([this](char* topic, byte* payload, unsigned int length) {
      String message = ""; //variable to save the received message
      //convert the received message from bytes to string
      for (unsigned int i = 0; i < length; i++) {
        message += (char)payload[i]; //convert bytes to characters and add them to the string
      }
      //change the traffic light mode
      if (message == "0") {
        trafficContext1.setState(new UnregulatedState()); //change the state to unregulated
        client.publish(mqttResponseTopic, "Operating mode - Unregulated intersection");
      } else if (message == "1") {
        trafficContext1.setState(new RegulatedState()); //change the state to adjustable
        client.publish(mqttResponseTopic, "Operation mode - Regulated intersection");
      } 
    });
  }
  //connection to MQTT broker if connection is lost
  void reconnect() {
    //if the client is not connected
    while (!client.connected()) {
      Serial.println("Connecting to MQTT...");
      //connect to the broker with the identifier "ESP8266_Client"
      if (client.connect("ESP8266_Client")) {
        Serial.println("Connected");
        //subscribe to MQTT topic
        client.subscribe(mqttTopic);
      } else {
        delay(5000); //if the connection failed, wait 5 seconds and try again
      }
    }
  }
  //checking connection and receiving messages
  void loop() {
    //if the client is not connected
    if (!client.connected()) {
      //let's try to connect again
      reconnect();
    }
    client.loop(); //call the loop () function to process received messages
  } 
};


//іnitializing an object with output settings and objects
TrafficLightContext trafficContext(D1, D2, D3, D5, D6, D7, D8);
MQTTHandler mqttHandler(trafficContext);


void setup() {
  //connection to Wi-Fi network
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("WiFi connected");
  trafficContext.begin_setup(); //set up a traffic light
  mqttHandler.setup(); //configure the MQTT client
}


void loop() {
  mqttHandler.loop(); //call for check connection to MQTT broker and message processing
  trafficContext.handle(); //call the update method of the current state
}
