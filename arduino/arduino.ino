
#include "RCSwitch.h"
#include <SPI.h>
#include <Ethernet.h>
#include "PubSubClient.h"
#include <math.h>

#define WIFIPIN 9
#define AMBILIGHTBLUEEPIN 6
#define AMBILIGHTREDPIN 5
#define AMBILIGHTGREENPIN 3
#define THERMISTORPIN A0       
#define THERMISTORNOMINAL 2200  // resistance at 25 degrees C
#define TEMPERATURENOMINAL 25   // temp. for nominal resistance (almost always 25 C)
#define BCOEFFICIENT 4791       // The beta coefficient of the thermistor (usually 3000-4000)
#define SERIESRESISTOR 2200     // the value of the 'other' resistor

#define DEVICE_1_SUBSCRIBE"/devices/321995-ambilight/controls/#"
#define DEVICE_1_POWER "/devices/321995-ambilight/controls/power"
#define DEVICE_1_POWER_TYPE "/devices/321995-ambilight/controls/power/type"
#define DEVICE_1_FADING "/devices/321995-ambilight/controls/fading"
#define DEVICE_1_FADING_TYPE "/devices/321995-ambilight/controls/fading/type"
#define DEVICE_1_VALUE "/devices/321995-ambilight/controls/intensity"
#define DEVICE_1_VALUE_TYPE "/devices/321995-ambilight/controls/intensity/type"

#define DEVICE_1_HUE "/devices/321995-ambilight/controls/color/type"
#define DEVICE_1_HUE_TYPE "/devices/321995-ambilight/controls/color"

#define DEVICE_2_SUBSCRIBE "/devices/862671-wirelessSwitch/controls/#"
#define DEVICE_2_POWER "/devices/862671-wirelessSwitch/controls/power"
#define DEVICE_2_POWER_TYPE "/devices/862671-wirelessSwitch/controls/power/type"
#define DEVICE_3_SUBSCRIBE "/devices/558426-wirelessSwitch/controls/#"
#define DEVICE_3_POWER "/devices/558426-wirelessSwitch/controls/power"
#define DEVICE_3_POWER_TYPE "/devices/558426-wirelessSwitch/controls/power/type"
#define SENSORS_1_TEMPERATURE "/devices/482031-sensors/controls/temp"
#define SENSORS_1_TEMPERATURE_TYPE "/devices/482031-sensors/controls/temp/type"



// Prototypes
void setWifi(char* state, char* group, int switchNumber);
void setLedColor();
float getTemp();
void mqttReceive(char* topic, byte* payload, unsigned int length);
void publishRetained(char* topic, char* payload);
void subscribe();
void publishDeviceMetaInformation();
void fadeStepToTargetColor();

void wakeup();



// Settings 
byte mac[]    = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFF, 0xFF };
byte ip[]     = { 192, 168, 8, 4 };
byte mqttServer[] = { 192, 168, 8, 2 };
char* mqttClientId = "homeduino";
char* wifiSwitchHomeGroup = "11011";



// Global variables
RCSwitch wifiTransmitter = RCSwitch();
EthernetClient ethClient;
PubSubClient client(mqttServer, 1883, mqttReceive, ethClient);



// Global status variabels
boolean fadeStepFlag = false;
boolean publishSensorsFlag = false;
boolean fadeStepToTargetColorFlag = false;

int fadecounter = 0;
int fadetToTargetCounter = 0;
int sensorcounter = 0;

int ambilightR = 0;
int ambilightG = 255;
int ambilightB = 0;

double ambilightValue = 1.0;
double ambilightValueExpectedEcho;
int ambilightHue = 360;
int ambilightHueExpectedEcho;

void setWifi(char* state, char* group, int switchNumber) {
  if (strcmp(state, "1") == 0) {
    wifiTransmitter.switchOn(group, switchNumber);
    delay(500);
    wifiTransmitter.switchOn(group, switchNumber);  
  } else if (strcmp(state, "0") == 0)  {  
    wifiTransmitter.switchOff(group, switchNumber); 
    delay(500);
    wifiTransmitter.switchOff(group, switchNumber); 
  } 
}

void setLedColor(int red, int green, int blue) {    
  analogWrite(AMBILIGHTREDPIN, red);
  analogWrite(AMBILIGHTGREENPIN, green);
  analogWrite(AMBILIGHTBLUEEPIN, blue);
}



float getTemp() {
  float temp;
  temp = analogRead(THERMISTORPIN); 
  temp = 1023 / temp - 1;
  temp = SERIESRESISTOR / temp;  
  temp = temp / THERMISTORNOMINAL;                 // (R/Ro)
  temp = log(temp);                                // ln(R/Ro)
  temp /= BCOEFFICIENT;                            // 1/B * ln(R/Ro)
  temp += 1.0 / (TEMPERATURENOMINAL + 273.15);     // + (1/To)
  temp = 1.0 / temp;                             // Invert
  temp -= 273.15;                                  // convert to C
  return temp;
}






//
//
// MQTT 
//
//

void setAmbilightValue(double value) {
 if (value == ambilightValueExpectedEcho)
   return
   
   
  ambilightValue = value;
  setLedColorHSV(ambilightHue,1,ambilightValue); //Staturation constant at 1

}     


void setAmbilightHue(int hue) {
  if (value == ambilightHueExpectedEcho)
   return
  
  ambilightHue = hue;
  setLedColorHSV(ambilightHue,1,ambilightValue); //Staturation constant at 1

}


void mqttReceive(char* topic, byte* rawPayload, unsigned int length) {  
  char  payload[length+1];
  memcpy(payload, rawPayload, length);
  payload[length] = '\0';
  Serial.println("there");
    


  if (strcmp(topic, DEVICE_1_VALUE) == 0) {
    double value;
    sscanf(payload, "%f", &value);
    setAmbilightValue(value);     
  } else if (strcmp(topic, DEVICE_1_HUE) == 0) {
    int hue;
    sscanf(payload, "%f", &hue);
    setAmbilightHue(hue); 
  } else if(strcmp(topic, DEVICE_1_POWER) == 0) {
    setWifi((char*)payload, wifiSwitchHomeGroup, 1);
  } else if(strcmp(topic, DEVICE_1_FADING)  == 0 ) {
    fadeStepFlag  = !(*payload == '0');
    fadeStepToTargetColorFlag = false;
  } else if(strcmp(topic, DEVICE_2_POWER)  == 0) {
    setWifi((char*)payload, wifiSwitchHomeGroup, 2);
  }else if(strcmp(topic,DEVICE_3_POWER)  == 0) {
    setWifi((char*)payload, wifiSwitchHomeGroup, 3);
  } else if (strcmp(topic, "/devices/321995-ambilight/actions/wakeup") == 0) {
    Serial.println("wakeup");
  }
}

void publishRetained(char* topic, char* payload) {
  client.publish(topic, (uint8_t*)payload, strlen((const char*)payload), true);
}



//
//
// Init
//
//



void setup() {
  Serial.begin(9600);
  Serial.println("Starting");

  pinMode(AMBILIGHTREDPIN, OUTPUT);
  pinMode(AMBILIGHTGREENPIN, OUTPUT);
  pinMode(AMBILIGHTBLUEEPIN, OUTPUT);
  pinMode(WIFIPIN, OUTPUT);
  
  wifiTransmitter.enableTransmit(WIFIPIN);

  Ethernet.begin(mac, ip);
  if (client.connect(mqttClientId)) {
    subscribe();
    publishDeviceMetaInformation();
  }
}


void setLedColorHSV(int h, double s, double v) {
  //this is the algorithm to convert from RGB to HSV
  double r=0; 
  double g=0; 
  double b=0;

  double hf=h/60.0;

  int i=(int)floor(h/60.0);
  double f = h/60.0 - i;
  double pv = v * (1 - s);
  double qv = v * (1 - s*f);
  double tv = v * (1 - s * (1 - f));

  switch (i)
  {
  case 0: //rojo dominante
    r = v;
    g = tv;
    b = pv;
    break;
  case 1: //verde
    r = qv;
    g = v;
    b = pv;
    break;
  case 2: 
    r = pv;
    g = v;
    b = tv;
    break;
  case 3: //azul
    r = pv;
    g = qv;
    b = v;
    break;
  case 4:
    r = tv;
    g = pv;
    b = v;
    break;
  case 5: //rojo
    r = v;
    g = pv;
    b = qv;
    break;
  }

  //set each component to a integer value between 0 and 255
  int red=constrain((int)255*r,0,255);
  int green=constrain((int)255*g,0,255);
  int blue=constrain((int)255*b,0,255);

  setLedColor(red,green,blue);
}


void publishAmbilightValue(double value) {
  
}

void publishAmbilightHue(int hue) {

  
}

// Fades brightness of single HSV value to maximum
void wakeupLoop() {
  if (ambilightValue < 1.0) {
     ambilightValue += 0.01;
     
     char buffer[4];
     sprintf(buffer, "%s", ambilightValue);
     publishRetained(DEVICE_1_VALUE,  buffer);
     
     ambilightHueExpectedEcho = ambilightValue;
     setLedColorHSV(ambilightHue,1,ambilightValue);   
  }
}

// Continuously Fades values on HSV spectrum
void fadeLoop() {
  ambilightHue = ambilightHue < 360 ? ambilightHue+1 : 0;
   
  char buffer[3];
  sprintf(buffer, "%s", ambilightHue);
  publishRetained(DEVICE_1_HUE,  buffer);
  
  ambilightValueExpectedEcho = ambilightHue;
  setLedColorHSV(ambilightHue,1,ambilightValue);
  
  
}

void sensorLoop(){
  char tempStr[5];
  dtostrf(getTemp(),2,2,tempStr);
  publishRetained(SENSORS_1_TEMPERATURE, tempStr);
}


void loop() {
  // Mqtt loop
  client.loop();

  if (sensorcounter % 65000 == 0) {
    sensorLoop();
    sensorcounter= 0;
  }
    
  if (fadeStepFlag && ((fadecounter % 2000) == 0)) {
    fadeLoop();
    fadecounter = 0;
  }
  
  fadecounter++;
  sensorcounter++;
}




void publishDeviceMetaInformation() {
 publishRetained(DEVICE_1_POWER_TYPE, "switch");
 publishRetained(DEVICE_1_FADING_TYPE, "switch");
 publishRetained(DEVICE_1_HUE_TYPE, "range");
 publishRetained(DEVICE_1_VALUE_TYPE, "range"); 
 publishRetained(DEVICE_2_POWER_TYPE, "switch");
 publishRetained(DEVICE_3_POWER_TYPE, "switch");
 publishRetained(SENSORS_1_TEMPERATURE_TYPE, "text");
}

void subscribe() {
 client.subscribe("/devices/321995-ambilight/actions/#");
 client.subscribe(DEVICE_1_SUBSCRIBE);
 client.subscribe(DEVICE_2_SUBSCRIBE);
 client.subscribe(DEVICE_3_SUBSCRIBE);  
 
 
}

void publishSensors () {
}
