#include<Arduino.h>
#include<DHT.h>
#include <SPI.h>
#include <WiFiNINA.h>
#include <BlynkSimpleWiFiNINA.h>
#include <ThingSpeak.h>

char auth[] = "a1iSeRo5HOu_Vxwzi6duypH30ZkwP8fN";
char ssid[] = "NomanMalik";
char pass[] = "NomanMalik";

unsigned long myChannelNumber = 1246831;	//ThingSpeak channel number
const char * myWriteAPIKey = "7WJ9UQUMK1ULDFVT";		//ThingSpeak write API key

char email[] = "nomanciit3@gmail.com";

int current_quality =-1;
int _lastVoltage, _currentVoltage, _voltageSum, _volSumCount, _standardVoltage;

int pin = 8;
unsigned long duration;
unsigned long starttime;
unsigned long sampletime_ms = 3000;
unsigned long lowpulseoccupancy = 0;
float ratio = 0;
float concentration = 0;
int ctr = 0;

#define DHTPIN  3
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);
float h = 0.0, pt = 0.0, t = 0.0, hic = 0.0;

WiFiClient client;

void setup()
{
    Serial.begin(115200);

    pinMode(A0, INPUT);
    pinMode(pin,INPUT);
    dht.begin();
    Blynk.begin(auth, ssid, pass);
    ThingSpeak.begin(client);
    WiFi.begin(ssid, pass);

    starttime = millis();
}
void loop()
{
  Blynk.run();
	readData();

    if((millis()-starttime) > sampletime_ms){

        processData();

        testSensors();
        
        if(concentration > 6000 && current_quality > 1 && (t-pt > 0.08)){
        	Serial.println("Fire!");
			Blynk.email(email, "Fire Alert", "Fire has been detected");
        	Blynk.notify("Fire!");
          delay(30000);
        }

        if(ctr++ > 7){
        	logData();
          ctr = 0;
        }
    }
}

void readData() {
	duration = pulseIn(pin, LOW);
    lowpulseoccupancy = lowpulseoccupancy+duration;
    h = dht.readHumidity();
    pt = t;
    t = dht.readTemperature();
    hic = dht.computeHeatIndex(t, h, false);
}

void processData(){
	current_quality=slope();
    ratio = lowpulseoccupancy/(sampletime_ms*10.0);
    concentration = 1.1*pow(ratio,3)-3.8*pow(ratio,2)+520*ratio+0.62; 
    ratio = lowpulseoccupancy/(sampletime_ms*10.0);
    lowpulseoccupancy = 0;
    starttime = millis();
}

int slope() {
    _lastVoltage = _currentVoltage;
    _currentVoltage = analogRead(A0);
 
    _voltageSum += _currentVoltage;
    _volSumCount += 1;
 
    updateStandardVoltage();
    if (_currentVoltage - _lastVoltage > 400 || _currentVoltage > 700) {
        return 0;
    }
    else if ((_currentVoltage - _lastVoltage > 400 && _currentVoltage < 700)
             || _currentVoltage - _standardVoltage > 150) {
        return 1;
    }
    else if ((_currentVoltage - _lastVoltage > 200 && _currentVoltage < 700)
             || _currentVoltage - _standardVoltage > 50) {
        return 2;
    }
    else {
        return 3;
    }
 
    return -1;
}

void testSensors(){
	if (current_quality >= 0){
        if (current_quality==0)
            Serial.print("High pollution! Force signal active; ");
        else if (current_quality==1)
            Serial.print("High pollution; ");
        else if (current_quality==2)
            Serial.print("Low pollution; ");
        else if (current_quality ==3)
            Serial.print("Fresh air; ");
    }
    Serial.print(F("Humidity: "));
    Serial.print(h);
    Serial.print(F("%  Temperature: "));
    Serial.print(t);
    Serial.print(F("°C "));
    Serial.print(F(" Heat index: "));
    Serial.print(hic);
    Serial.print(F("°C; "));
    Serial.print("Concentration: ");
    Serial.println(concentration);
}

void updateStandardVoltage(){
   _standardVoltage = (_currentVoltage + _lastVoltage)/2;
}

void logData(){
	ThingSpeak.setField(1, h);
  	ThingSpeak.setField(2, t);
  	ThingSpeak.setField(3, hic);
	ThingSpeak.setField(4, concentration);

	int x = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
	if(x == 200){
	  Serial.println("Channel update successful.");
	}
	else{
	  Serial.println("Problem updating channel. HTTP error code " + String(x));
	}
}
