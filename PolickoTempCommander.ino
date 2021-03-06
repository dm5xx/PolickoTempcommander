// OL7M PolickoTempCommander v1.5 20181103 for 2 thermostatic sensors + 5 normal ds18bs20
// cc by DM5XX @ CC License: BY-NC-ND
// LLAP! 

#include <OneWire.h>
#include <DallasTemperature.h>
#include <Ethernet.h> // Used for Ethernet

//#define SERIALDEBUG;
//#define SHOWTEMP;

OneWire oneWire(2); //Pin for ONE-WIRE-BUS
DallasTemperature sensors(&oneWire);

//DeviceAdressen der einzelnen ds1820 Temperatursensoren angeben. (loop anpassen)
DeviceAddress s1 = { 0x28, 0xFF, 0x19, 0x20, 0x01, 0x16, 0x03, 0xB8 }; // this will be thermostatic sensor #1     KD uvnitr
DeviceAddress s2 = { 0x28, 0x62, 0xAD, 0xC4, 0x04, 0x00, 0x00, 0xC5 }; // this will be thermostatic sensor #2     Koupelna
DeviceAddress s3 = { 0x28, 0xFF, 0x13, 0x3E, 0x01, 0x16, 0x03, 0x28 }; // normal temp sensor                      KD drez
DeviceAddress s4 = { 0x28, 0xFF, 0x4E, 0x07, 0x01, 0x16, 0x03, 0x8A }; // normal temp sensor                      Venku
DeviceAddress s5 = { 0x28, 0xFF, 0x8D, 0xF3, 0x87, 0x16, 0x03, 0x2A }; // normal temp sensor                      Loznice 
DeviceAddress s6 = { 0x28, 0xFF, 0x99, 0xE2, 0x00, 0x16, 0x03, 0xE6 }; // normal temp sensor                      Studna
DeviceAddress s7 = { 0x28, 0xFF, 0xF9, 0xB5, 0x87, 0x16, 0x03, 0xB1 }; // high temp sensor                      Koupelna podlaha

//////////////// Values for automatic mode aka thermostatic mode ////////////////////////////////////////////////
const float temp1Min = 0.00; // if temperature is below this level (s1), Relay 1 will be switched on, until... 
const float temp1Max = 2.00; // ...temperature of s1 will reach this level, so relay 1 will be switched off!
							 // Relay 1 will be remain switched off, until temperature measured from s1 will fall below temp1Min again

const float temp2Min = 0.00; // if temperature is below this level (s2), Relay 2 will be switched on, until...
const float temp2Max = 2.00; // ...temperature at s2 will reach this level, so relay 2 will be switched off!
							 // Relay 2 will be remain switched off, until temperature measured from s2 will fall below temp2Min again

int readSensorsEvery = 30000; // read sensors every 30 seconds...
boolean isInManualMode = false; // define: what mode should it be after a reset or restart...


const byte relais1 = 6; // Pin for Relay #1 controlled by Sensor #1 s1
const byte relais2 = 7; // Pin for Relay #1 controlled by Sensor #2 s2
const byte manualLed = 3; // Pin for manualmode indicator led
const byte relayOneLed = 4; // Pin for relay1 status led
const byte relayTwoLed = 5; // Pin for relay2 status led

const byte pinButton1 = A4; // Pin button 1
const byte pinButton2 = 8; // Pin button 2
const byte pinButton3 = 9; // Pin button 3

const byte mac[] = { 0xDE, 0x00, 0xBA, 0xEF, 0x88, 0x73 };
const byte ip[] = { 192, 168, 11, 208 };
const byte gateway[] = { 192, 168, 11, 1 };
const byte subnet[] = { 255, 255, 255, 0 };
const EthernetServer server(80);

float temp1;
float temp2;
float temp3;
float temp4;
float temp5;
float temp6;
float temp7;

boolean relayOneIsOn = false;
boolean relayTwoIsOn = false;
String requestString;

unsigned long lastSensorReading = 0;  // the last time the sensors are called...
unsigned long lastButtonPressed = 0;  // the last time a button was pressed...

long lastDebounceTime = 0;  // the last time the output pin1 was toggled
long lastDebounceTime2 = 0;  // the last time the output pin2 was toggled
long lastDebounceTime3 = 0;  // the last time the output pin3 was toggled
long debounceDelay = 50;    // the debounce time; increase if the output flickers

void setup(void)
{
//#ifdef SHOWTEMP
	Serial.begin(115200);
//#endif

	sensors.begin();
	sensors.setResolution(s1, 10);
	sensors.setResolution(s2, 10);
	sensors.setResolution(s3, 10);
	sensors.setResolution(s4, 10);
	sensors.setResolution(s5, 10);
	sensors.setResolution(s6, 10);
	sensors.setResolution(s7, 10);

	pinMode(relais1, OUTPUT);
	pinMode(relais2, OUTPUT);

	Ethernet.begin(mac, ip); // Client starten

#ifdef SHOWTEMP
	Serial.print("server is at ");
	Serial.println(Ethernet.localIP());
#endif

	pinMode(manualLed, OUTPUT);
	pinMode(relayOneLed, OUTPUT);
	pinMode(relayTwoLed, OUTPUT);

	pinMode(pinButton1, INPUT_PULLUP);
	pinMode(pinButton2, INPUT_PULLUP);
	pinMode(pinButton3, INPUT_PULLUP);

	digitalWrite(relais1, LOW);
	digitalWrite(relais2, LOW);

	sensors.requestTemperatures();
	delay(200);
	fillTempVariables();

#ifdef SERIALDEBUG
    Serial.println("Values requersted from the Sensors...");
#endif

	reloadLeds();
}

void fillTempVariables()
{
	temp1 = getTemperature(s1, false);
	temp2 = getTemperature(s2, false);
	temp3 = getTemperature(s3, false);
	temp4 = getTemperature(s4, false);
	temp5 = getTemperature(s5, false);
	temp6 = getTemperature(s6, false);
	temp7 = getTemperature(s7, true);
}

float getTemperature(DeviceAddress deviceAddress, bool isHighTempSensor)
{
	byte counter = 0;
	float upperTempLevel = 50.00;
	float tempC = sensors.getTempC(deviceAddress);
	delay(50);

	if(isHighTempSensor)
		upperTempLevel = 90.00;

	while(tempC < -30.00 || tempC > upperTempLevel) {

		if(counter == 5)
			return upperTempLevel;

		#ifdef SHOWTEMP
			Serial.print("Temperature Error ");
			Serial.print(counter);
			Serial.print(" ");
			Serial.println(tempC);
		#endif
		tempC = sensors.getTempC(deviceAddress);
		delay(100);

		counter++;
	}
	return tempC;
}

void loop(void)
{
	unsigned long currentMillis = millis();
	boolean isNotLocked = (lastButtonPressed + 500 < currentMillis); // locktime is set to 500ms...

	if ((lastSensorReading + readSensorsEvery) < currentMillis)
	{
		sensors.requestTemperatures();
		delay(200);
		lastSensorReading = millis();
#ifdef SHOWTEMP
		Serial.println("Values are requester from the Sensors...");
#endif
		fillTempVariables();
#ifdef SERIALDEBUG
	Serial.print("S1 ");
	Serial.print(temp1);
	Serial.println(" Celsius");

	Serial.print("S2 ");
	Serial.print(temp2);
	Serial.println(" Celsius");

	Serial.print("S3 ");
	Serial.print(temp3);
	Serial.println(" Celsius");

	Serial.print("S4 ");
	Serial.print(temp4);
	Serial.println(" Celsius");

	Serial.print("S5 ");
	Serial.print(temp5);
	Serial.println(" Celsius");

	Serial.print("S6 ");
	Serial.print(temp6);
	Serial.println(" Celsius");

	Serial.print("S7 ");
	Serial.print(temp7);
	Serial.println(" Celsius");
#endif // SERIALDEBUG

	}

	byte currentButton = getButtonPushed();
	
#ifdef SERIALDEBUG
	Serial.print("Current Button is...");
	Serial.println(currentButton);
	if (isInManualMode)
	{
		Serial.println("I am in manual Mode");
	}
    delay(2000);
#endif

	if (currentButton == 1 && isNotLocked) {
		if (isInManualMode)
		{
#ifdef SERIALDEBUG
 			Serial.println("Switching from MANUAL mode to AUTOMATIC mode...");
#endif
			digitalWrite(manualLed, LOW);
			isInManualMode = false;
		}
		else
		{
#ifdef SERIALDEBUG
 			Serial.println("Switching from AUTOMATIC mode to MANUAL mode...");
#endif
			digitalWrite(manualLed, HIGH);
			isInManualMode = true;
		}
		lastButtonPressed = millis();
		reloadLeds();
	}

	if (isInManualMode) // so somebody pressed the button...
	{
		// for the secondbutton aka relay1
		if (currentButton == 2 && isNotLocked) {
			if (relayOneIsOn)
			{
#ifdef SERIALDEBUG
 				Serial.println("Switching OFF Relay1 in manual mode");
#endif
				digitalWrite(relayOneLed, LOW);
				digitalWrite(relais1, LOW);
				relayOneIsOn = false;
			}
			else
			{
#ifdef SERIALDEBUG
 				Serial.println("Switching ON Relay1 in manual mode");
#endif
				digitalWrite(relayOneLed, HIGH);
				digitalWrite(relais1, HIGH);
				relayOneIsOn = true;
			}
			lastButtonPressed = millis();
			reloadLeds();
		}

		//For 3rd button aka relay2
		if (currentButton == 3 && isNotLocked) {
			if (relayTwoIsOn)
			{
#ifdef SERIALDEBUG
 				Serial.println("Switching OFF Relay2 in manual mode");
#endif
				digitalWrite(relayTwoLed, LOW);
				digitalWrite(relais2, LOW);
				relayTwoIsOn = false;
			}
			else
			{
#ifdef SERIALDEBUG
 				Serial.println("Switching ON Relay2 in manual mode");
#endif
				digitalWrite(relayTwoLed, HIGH);
				digitalWrite(relais2, HIGH);
				relayTwoIsOn = true;
			}
			lastButtonPressed = millis();
			reloadLeds();
		}
	}
	else
	{
		if(currentButton == 2 || currentButton == 3)
			isInManualMode = true;
		termostaticControl();
	}
/// end
	WebserverStart();
}

byte getButtonPushed()
{

	int reading = digitalRead(pinButton1);
	if ( (millis() - lastDebounceTime) > debounceDelay) 
	{
		if(reading == 0)
		{
			lastDebounceTime = millis();
			return 1;  
	  	}
	}

	int reading2 = digitalRead(pinButton2);
	if ( (millis() - lastDebounceTime2) > debounceDelay) 
	{
		if(reading2 == 0)
		{
			lastDebounceTime2 = millis();
			return 2;  
	  	}
  	}

	int reading3 = digitalRead(pinButton3);
	if ( (millis() - lastDebounceTime3) > debounceDelay) 
	{
		if(reading3 == 0)
		{
			lastDebounceTime3 = millis();
			return 3;
		}
  	}
	return 0; //nothing pushed :P
}

///////////////////////////////////////// WEBSERVER //////////////////////////////////////////////////////////

void WebserverStart()
{
	// Create a client connection
	EthernetClient client = server.available();
	if (client) {
		while (client.connected()) {
			if (client.available()) {
				char c = client.read();

				if (requestString.length() < 100) {
					requestString += c;
				}

				//if HTTP request has ended
				if (c == '\n') {
					int cmdSet = requestString.indexOf("Set/"); // see if its a set request
					int cmdGet = requestString.indexOf("Get/"); // see if its a get request

					if (cmdSet >= 0)
					{
					     byte command = getStringPartByNr(requestString, '/', 2).toInt(); // the 2nd part is the bank-number
						 byte requestValue = getStringPartByNr(requestString, '/', 3).toInt(); // the 2nd part is the bank-number

#ifdef SERIALDEBUG
						 Serial.println(command);
						 Serial.println(requestValue);
#endif

						 if (command == 9)
						 {
							 if (requestValue == 1) // manual mode on
							 {
#ifdef SERIALDEBUG
								 Serial.println("Switching to manual mode");
#endif
								digitalWrite(manualLed, HIGH);
								isInManualMode = true;
							 }
							 if (requestValue == 0) // automatic mode on
							 {
#ifdef SERIALDEBUG
								 Serial.println("Switching to automatic mode");
#endif
								digitalWrite(manualLed, LOW);
								digitalWrite(relayOneLed, LOW);
								digitalWrite(relais1, LOW);
								relayOneIsOn = false;
								digitalWrite(relayTwoLed, LOW);
								digitalWrite(relais2, LOW);
								relayTwoIsOn = false;
								isInManualMode = false;
							 }
						 }
						 else if (command == 1)
						 {
							 digitalWrite(manualLed, HIGH);
							 isInManualMode = true;

							 if (requestValue == 1)
							 {
#ifdef SERIALDEBUG
								 Serial.println("Switching ON Relay1 in manual mode");
#endif
								digitalWrite(relayOneLed, HIGH);
								digitalWrite(relais1, HIGH);
								relayOneIsOn = true;
							 }
							 else if (requestValue == 0)
							 {
#ifdef SERIALDEBUG
								 Serial.println("Switching OFF Relay1 in manual mode");
#endif
								digitalWrite(relayOneLed, LOW);
								digitalWrite(relais1, LOW);
								relayOneIsOn = false;
							 }
						 }
						 else if (command == 2)
						 {
							 digitalWrite(manualLed, HIGH);
							 isInManualMode = true;

							 if (requestValue == 1)
							 {
#ifdef SERIALDEBUG
								 Serial.println("Switching ON Relay2 in manual mode");
#endif
								digitalWrite(relayTwoLed, HIGH);
								digitalWrite(relais2, HIGH);
								relayTwoIsOn = true;
							 }
							 else if (requestValue == 0)
							 {
#ifdef SERIALDEBUG
								 Serial.println("Switching OFF Relay2 in manual mode");
#endif
								digitalWrite(relayTwoLed, LOW);
								digitalWrite(relais2, LOW);
								relayTwoIsOn = false;
							 }
						 }
						 reloadLeds();
						 GetPage(client);
					}
					else if (cmdGet >= 0)
						GetPage(client);
					else
						GetPage(client);

					requestString = "";

					delay(1);
					client.stop();
				}

			}
		}
	}
}

/*-------------------------------------------------- Fancy Helpers --------------------------------------------------------------*/
// my little string splitting method
String getStringPartByNr(String data, char separator, int index)
{
	int stringData = 0;
	String dataPart = "";
	for (int i = 0; i<data.length(); i++)
	{
		if (data[i] == separator)
			stringData++;
		else if (stringData == index)
			dataPart.concat(data[i]);
		else if (stringData>index)
		{
			return dataPart;
			break;
		}
	}
	return dataPart;
}


// Speed | Direction | Rain | maxSpeedHour | maxSpeed24h | rain1h | rain24h | minTemp | maxTemp
void GetPage(EthernetClient client)
{
	client.println("HTTP/1.1 200 OK"); //send new page
	client.println("Content-Type: text/html");
	client.println("Access-Control-Allow-Origin: *");
	client.println("Access-Control-Allow-Methods: POST, GET, OPTIONS");
	client.println("Access-Control-Allow-Headers: Authorization");
	client.println();
	client.print("myCB({'v':'");
	client.print(temp1);
	client.print("|");
	client.print(temp2);
	client.print("|");
	client.print(temp3);
	client.print("|");
	client.print(temp4);
	client.print("|");
	client.print(temp5);
	client.print("|");
	client.print(temp6);
	client.print("|");
	client.print(temp7);
	client.print("|");
	client.print(isInManualMode);
	client.print("|");
	client.print(relayOneIsOn);
	client.print("|");
	client.print(relayTwoIsOn);
	client.print("'})");
}

void termostaticControl()
{
#ifdef SERIALDEBUG
		Serial.println("I am in automatic Mode");
		delay(300);
#endif
	if (temp1 < temp1Min && !relayOneIsOn)
	{
#ifdef SERIALDEBUG
		Serial.println("Switching on Relay1 its too cold...");
#endif
		digitalWrite(relais1, HIGH);
		relayOneIsOn = true;
		reloadLeds();
	}
	else if (temp1 > temp1Max && relayOneIsOn)
	{
#ifdef SERIALDEBUG
		Serial.println("Switching off Relay1 its too hot...");
#endif
		digitalWrite(relais1, LOW);
		relayOneIsOn = false;
		reloadLeds();
	}

	if (temp2 < temp2Min && !relayTwoIsOn)
	{
#ifdef SERIALDEBUG
		Serial.println("Switching on Relay2 its too cold...");
#endif
		digitalWrite(relais2, HIGH);
		relayTwoIsOn = true;
		reloadLeds();
	}
	else if (temp2 > temp2Max && relayTwoIsOn)
	{
#ifdef SERIALDEBUG
		Serial.println("Switching off Relay2 its too hot...");
#endif
		digitalWrite(relais2, LOW);
		relayTwoIsOn = false;
		reloadLeds();
	}
}

void reloadLeds()
{
#ifdef SERIALDEBUG
	Serial.println("Reload Leds is called...");
#endif

	if (isInManualMode)
		digitalWrite(manualLed, HIGH);
	else
		digitalWrite(manualLed, LOW);

	if (relayOneIsOn)
		digitalWrite(relayOneLed, LOW);
	else
		digitalWrite(relayOneLed, HIGH);

	if (relayTwoIsOn)
		digitalWrite(relayTwoLed, LOW);
	else
		digitalWrite(relayTwoLed, HIGH);
}
