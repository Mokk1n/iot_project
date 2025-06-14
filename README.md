# Iot_project
Implementation of the C++ hardware-software system TLC, which uses the State Pattern design pattern to switch between two operating modes using external control. If an unexpected command is entered, it is ignored and the system status remains unchanged. The basic mode corresponds to the mode of operation of traffic lights at a regulated intersection, which ensures that traffic light signals change in accordance with the traffic light operation algorithm. Due to frequent malfunctions of the yellow signal, the system includes a mechanism to respond to this problem. The appearance of a button for pedestrians to press is due to the acceleration of traffic light signal changes, as well as a reduction in the waiting time for pedestrians for this change. The second mode of the system is activated by external control and corresponds to an unregulated mode, which, instead of the usual traffic light cycle, activates the yellow signal to flash if this signal is working properly. In the event of a yellow signal malfunction, the system responds to the problem by providing an audible signal through a passive buzzer to increase safety.
In RegulatedState and UnregulatedState modes, the yellow light may be faulty, and its status is determined randomly each time it enters the state by generating a random number, namely 0 (yellow light is faulty) or 1 (yellow light is working). In the event of a malfunction in the basic mode, this indication (yellow signal) is ignored, and in the unregulated mode, a signal is sent to the passive buzzer to activate the emergency signal to warn road users of an emergency situation at the intersection.
The disadvantage of this approach is the lack of video surveillance, which makes it difficult for the operator to understand the real situation at the intersection. Without video recording of events, the operator is unable to visually check the status of traffic light signals in real time.
# User instructions
To use the system, you need to have the appropriate equipment, which will be connected as shown. 
Then you need to install Arduino IDE, enable support for the ESP8266 board and connection port, if not already installed. Next, you need to install the libraries used according. In the upper part of the sketch, change the Wi-Fi and MQTT data.
Then you need to install MQTTX, if not already installed. Configure the settings so that you can connect to the MQTT broker (broker.emqx.io), and also configure the topic (traffic_light/control) with which you can control the traffic light mode remotely by sending the following values:
a)    0 - switches from RegulatedState to UnregulatedState;
b)    1 - switches to RegulatedState.
Before testing, you need to download the sketch by connecting the ESP8266 to a PC via USB, selecting the port and board in the Arduino IDE, uploading the sketch to the ESP8266, and then opening the Serial Monitor to view system messages.
The system testing procedure consists of checking the functionality of the system in basic mode. Pressing the pedestrian crossing button initiates a change in traffic light signals. At the same time, the system simulates a yellow LED failure with a probability of 50%. If a malfunction of the yellow signal is detected, the system skips its phase and switches from green to red. If the yellow LED is working properly.
Then test the unregulated mode. To do this, send a command with parameter 0 from MQTTX. In response, the system will start flashing the yellow signal (if it is working properly) or activate the buzzer sound signal (if a malfunction of the yellow LED is detected). If necessary, return to the basic mode by sending a command with parameter 1 to MQTTX.
# Diagram for project
Hardware part of the Trafic Light System project
![image](https://github.com/user-attachments/assets/40bbc5f2-708e-4fdc-8795-90245c896fcb)

Use case Trafic Light System
![image](https://github.com/user-attachments/assets/d25bd977-e0a0-4dda-a615-6c58201c38ec)
 
Class diagram Trafic Light System 
![image](https://github.com/user-attachments/assets/3775d57e-618f-426b-8ae9-d32f15b93527)

Active Diagram проекту Trafic Light System
![image](https://github.com/user-attachments/assets/fa1adc0d-db46-440a-9184-5b5bfcb6280d)
