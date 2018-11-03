// OL7M PolickoTempCommander v1.0 20170212 for 2 thermostatic sensors + 5 normal ds18bs20
// cc by DM5XX @ GPL
// LLAP! 

#include <OneWire.h>
#include <DallasTemperature.h>
#include <Ethernet.h> // Used for Ethernet

//#define SERIALDEBUG;

OneWire oneWire(2); //Pin for ONE-WIRE-BUS
DallasTemperature sensors(&oneWire);

//DeviceAdressen der einzelnen ds1820 Temperatursensoren angeben. (loop anpassen)
DeviceAddress s1 = { 0x28, 0xFF, 0x19, 0x20, 0x01, 0x16, 0x03, 0xB8 }; // this will be thermostatic sensor #1   KD uvnitr
DeviceAddress s2 = { 0x28, 0xFF, 0x16, 0x60, 0x87, 0x16, 0x03, 0x84 }; // this will be thermostatic sensor #2      Koupelna
DeviceAddress s3 = { 0x28, 0xFF, 0x13, 0x3E, 0x01, 0x16, 0x03, 0x28 }; // normal temp sensor                      KD drez
DeviceAddress s4 = { 0x28, 0xFF, 0x4E, 0x07, 0x01, 0x16, 0x03, 0x8A }; // normal temp sensor                      Venku
DeviceAddress s5 = { 0x28, 0xFF, 0x8D, 0xF3, 0x87, 0x16, 0x03, 0x2A }; // normal temp sensor                      Loznice 
DeviceAddress s6 = { 0x28, 0xFF, 0x99, 0xE2, 0x00, 0x16, 0x03, 0xE6 }; // normal temp sensor                      Studna
DeviceAddress s7 = { 0x28, 0xFF, 0xF9, 0xB5, 0x87, 0x16, 0x03, 0xB1 }; // normal temp sensor                         Koupelna podlaha
/*
Use Example sketch at Onewire library example dir called DS18x20_Temperature.pde to get the adress values
ROM = 28 A8 20 B6 6 0 0 52
ROM = 28 A4 98 B4 6 0 0 59
ROM = 28 3A 78 B5 6 0 0 AE
ROM = 28 1E CE B5 6 0 0 17
ROM = 28 9 8A B4 6 0 0 B0
ROM = 28 F9 57 B6 6 0 0 77
ROM = 28 63 8 B7 6 0 0 7C





ROM = 28 FF 13 3E 1 16 3 28
  Chip = DS18B20
  Data = 1 4D 1 4B 46 7F FF C 10 C0  CRC=C0
  Temperature = 20.81 Celsius, 69.46 Fahrenheit
No more addresses.



*/

//////////////// Values for automatic mode aka thermostatic mode ////////////////////////////////////////////////
const float temp1Min = 21.0; // if temperature is below this level (s1), Relay 1 will be switched on, until... 
const float temp1Max = 25.0; // ...temperature of s1 will reach this level, so relay 1 will be switched off!
							 // Relay 1 will be remain switched off, until temperature measured from s1 will fall below temp1Min again

const float temp2Min = 22.0; // if temperature is below this level (s2), Relay 2 will be switched on, until...
const float temp2Max = 25.0; // ...temperature at s2 will reach this level, so relay 2 will be switched off!
							 // Relay 2 will be remain switched off, until temperature measured from s2 will fall below temp2Min again

int readSensorsEvery = 30000; // read sensors every 30 seconds...
boolean isInManualMode = false; // define: what mode should it be after a reset or restart...


const byte relais1 = 6; // Pin for Relay #1 controlled by Sensor #1 s1
const byte relais2 = 7; // Pin for Relay #1 controlled by Sensor #2 s2
const byte manualLed = 3; // Pin for manualmode indicator led
const byte relayOneLed = 4; // Pin for relay1 status led
const byte relayTwoLed = 5; // Pin for relay2 status led
const byte analogSwitchPin = A5; // Use A5 as analog input for switches

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

int buttonValue; //Stores analog value when button is pressed
unsigned long lastSensorReading = 0;  // the last time the sensors are called...
unsigned long lastButtonPressed = 0;  // the last time a button was pressed...

void setup(void)
{
//#ifdef SERIALDEBUG
	Serial.begin(57600);
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

//#ifdef SERIALDEBUG
	Serial.print("server is at ");
	Serial.println(Ethernet.localIP());
//#endif

	digitalWrite(relais1, HIGH);
	digitalWrite(relais2, HIGH);

	pinMode(manualLed, OUTPUT);
	pinMode(relayOneLed, OUTPUT);
	pinMode(relayTwoLed, OUTPUT);

	sensors.requestTemperatures();
  //#ifdef SERIALDEBUG
    Serial.println("Now requesting values from the Sensors...");
//#endif

	reloadLeds();
}

float getTemperature(DeviceAddress deviceAddress)
{
	float tempC = sensors.getTempC(deviceAddress);
	if (tempC == -127.00) {
#ifdef SERIALDEBUG
		Serial.print("Error getting temperature");
#endif
	}
	else {
		return tempC;
	}
}

void loop(void)
{
  unsigned long currentMillis = millis();
	boolean isNotLocked = (lastButtonPressed + 500 < currentMillis); // locktime is set to 500ms...

	if ((lastSensorReading + readSensorsEvery) < currentMillis)
	{
		sensors.requestTemperatures();
		lastSensorReading = millis();
//#ifdef SERIALDEBUG
		Serial.println("Now requesting values from the Sensors...");
//#endif
	}

	buttonValue = analogRead(analogSwitchPin); //Read analog value from A5 pin

#ifdef SERIALDEBUG
	Serial.println(buttonValue);
  delay(2000);
#endif

	if (buttonValue >= 24 && buttonValue <= 32 && isNotLocked) {
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

	/// termostat sensors s1 and s2
	temp1 = getTemperature(s1);
 delay(20);
	temp2 = getTemperature(s2);
delay(20);

	if (isInManualMode) // so somebody pressed the button...
	{
		// for the secondbutton aka relay1
		if (buttonValue >= 16 && buttonValue <= 22 && isNotLocked) {
			if (relayOneIsOn)
			{
#ifdef SERIALDEBUG
				Serial.println("Switching OFF Relay1 in manual mode");
#endif
				digitalWrite(relayOneLed, LOW);
				digitalWrite(relais1, HIGH);
				relayOneIsOn = false;
			}
			else
			{
#ifdef SERIALDEBUG
				Serial.println("Switching ON Relay1 in manual mode");
#endif
				digitalWrite(relayOneLed, HIGH);
				digitalWrite(relais1, LOW);
				relayOneIsOn = true;
			}
			lastButtonPressed = millis();
			reloadLeds();
		}

		//For 3rd button aka relay2
		if (buttonValue >= 4 && buttonValue <= 10 && isNotLocked) {
			if (relayTwoIsOn)
			{
#ifdef SERIALDEBUG
				Serial.println("Switching OFF Relay2 in manual mode");
#endif
				digitalWrite(relayTwoLed, LOW);
				digitalWrite(relais2, HIGH);
				relayTwoIsOn = false;
			}
			else
			{
#ifdef SERIALDEBUG
				Serial.println("Switching ON Relay2 in manual mode");
#endif
				digitalWrite(relayTwoLed, HIGH);
				digitalWrite(relais2, LOW);
				relayTwoIsOn = true;
			}
			lastButtonPressed = millis();
			reloadLeds();
		}
	}
	else
	{
		termostaticControl();
	}
/// end

	temp3 = getTemperature(s3);
 delay(20);
	temp4 = getTemperature(s4);
 delay(20);
	temp5 = getTemperature(s5);
 delay(20);
	temp6 = getTemperature(s6);
 delay(20);
	temp7 = getTemperature(s7);
 delay(20);

#ifdef SERIALDEBUG
	Serial.print("S1 ");
	Serial.print(temp1);
	Serial.println(" Celsius");
	Serial.println();

	Serial.print("S2 ");
	Serial.print(temp2);
	Serial.println(" Celsius");
	Serial.println();

	Serial.print("S3 ");
	Serial.print(temp3);
	Serial.println(" Celsius");
	Serial.println();

	Serial.print("S4 ");
	Serial.print(temp4);
	Serial.println(" Celsius");
	Serial.println();

	Serial.print("S5 ");
	Serial.print(temp5);
	Serial.println(" Celsius");
	Serial.println();

	Serial.print("S6 ");
	Serial.print(temp6);
	Serial.println(" Celsius");
	Serial.println();

	Serial.print("S7 ");
	Serial.print(temp7);
	Serial.println(" Celsius");
	Serial.println();
#endif // SERIALDEBUG

	WebserverStart();
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
					//Serial.print(c);
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
							 if (requestValue == 1)
							 {
								 isInManualMode = true;
							 }
							 if (requestValue == 0)
							 {
								 isInManualMode = false;
							 }
						 }
						 else if (command == 1)
						 {
							 isInManualMode = true;

							 if (requestValue == 1)
							 {
#ifdef SERIALDEBUG
								 Serial.println("Switching ON Relay1 in manual mode");
#endif
								 digitalWrite(relayOneLed, HIGH);
								 digitalWrite(relais1, LOW);
								 relayOneIsOn = true;
							 }
							 else if (requestValue == 0)
							 {
#ifdef SERIALDEBUG
								 Serial.println("Switching OFF Relay1 in manual mode");
#endif
								 digitalWrite(relayOneLed, LOW);
								 digitalWrite(relais1, HIGH);
								 relayOneIsOn = false;
							 }
						 }
						 else if (command == 2)
						 {
							 isInManualMode = true;

							 if (requestValue == 1)
							 {
#ifdef SERIALDEBUG
								 Serial.println("Switching ON Relay2 in manual mode");
#endif
								 digitalWrite(relayTwoLed, HIGH);
								 digitalWrite(relais2, LOW);
								 relayTwoIsOn = true;
							 }
							 else if (requestValue == 0)
							 {
#ifdef SERIALDEBUG
								 Serial.println("Switching OFF Relay2 in manual mode");
#endif
								 digitalWrite(relayTwoLed, LOW);
								 digitalWrite(relais2, HIGH);
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
	if (temp1 < temp1Min && !relayOneIsOn)
	{
#ifdef SERIALDEBUG
		Serial.println("Switching on Relay1 its too cold...");
#endif
		digitalWrite(relais1, LOW);
		relayOneIsOn = true;
		reloadLeds();
	}
	else if (temp1 > temp1Max && relayOneIsOn)
	{
#ifdef SERIALDEBUG
		Serial.println("Switching off Relay1 its too hot...");
#endif
		digitalWrite(relais1, HIGH);
		relayOneIsOn = false;
		reloadLeds();
	}

	if (temp2 < temp2Min && !relayTwoIsOn)
	{
#ifdef SERIALDEBUG
		Serial.println("Switching on Relay2 its too cold...");
#endif
		digitalWrite(relais2, LOW);
		relayTwoIsOn = true;
		reloadLeds();
	}
	else if (temp2 > temp2Max && relayTwoIsOn)
	{
#ifdef SERIALDEBUG
		Serial.println("Switching off Relay2 its too hot...");
#endif
		digitalWrite(relais2, HIGH);
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
		digitalWrite(relayOneLed, HIGH);
	else
		digitalWrite(relayOneLed, LOW);

	if (relayTwoIsOn)
		digitalWrite(relayTwoLed, HIGH);
	else
		digitalWrite(relayTwoLed, LOW);
}
