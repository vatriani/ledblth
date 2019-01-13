/*
 * ledstripe bluetooth - ledstripe.ino
 * Copyright © 2019 - Niels Neumann  <vatriani.nn@googlemail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file ledstripe.ino
 * @author Niels Neumann
 * @date Jan 2019
 * @brief File containing the programm for the Arduino.
 *
 * 
 * @see http://www.stack.nl/~dimitri/doxygen/commands.html
 */


// config:
#define DHTTYPE DHT22	/// comment out for disable dht support or change
#define DEBUG			/// comment out for no debug information over usb

#define TX_BL		1	/// tx for bluetooth module
#define RX_BL 		0	/// rx for bluetooth module
#define RED_LED		5	/// define pin for red led
#define BLUE_LED	3	/// define pin for blue led
#define GREEN_LED	6	/// define pin for green led

#ifdef DHTTYPE
	#include <DHT.h>
	#define DHTPIN 19	/// change to the desired DHT-data pin
#endif



// !! do not edit beyond this point - here starts the program !!
#include <SoftwareSerial.h>
#include <EEPROM.h>

#define ADDRESS_EEPROM 0
#define INTERVAL_UPDATE 30000



/**
 * define all different LED modes
 * @brief
 * 0 = normal		- simple on
 * 1 = color Flash	- 
 * 2 = strobe		- 
 * 3 = fade			- 
 * 4 = smooth		- 
 */
enum modes {
	NORMAL = 0,
	COLOR_FLASH,
	STROBE,
	FADE,
	SMOOTH,
};
typedef enum modes Modes;



enum state {
	OFF = 0,
	ON,
};
typedef enum state State;



enum action {
	LED_COMMAND = 1,
	GET_STATUS,
};
typedef enum action Action;


/**
 * holds the state of the led strip
 */
struct data {
	/// 0..31 red value
	byte red;
	/// 0..31 green value
	byte green;
	/// 0..31 blue value
	byte blue;
	/// 0..8  brightness
	byte brightness;
	/// @see enum modes
	byte mode;
	/// 0..1 simple on/off
	byte state;
};
typedef struct data Data;



// global variables
/// DHT handler
DHT dht (DHTPIN, DHTTYPE);
/// Software Serial Handler connects to bluetooth
SoftwareSerial mySerial (TX_BL, RX_BL);
/// for handling all time relevant things
long previousMillis = 0;
/// store of settings
Data tmpData;



/**
 * calculate the values and set pins for the coresponding leds
 * 
 * example: red value * brightness = pin value
 */
void outputRGB () {
	if (tmpData.state != OFF) {
		analogWrite (RED_LED,   tmpData.red   * tmpData.brightness);
		analogWrite (GREEN_LED, tmpData.green * tmpData.brightness);
		analogWrite (BLUE_LED,  tmpData.blue  * tmpData.brightness);
	} else {
		analogWrite (RED_LED,   OFF);
		analogWrite (GREEN_LED, OFF);
		analogWrite (BLUE_LED,  OFF);
	}
}



/**
 * writing global dataset to EEPROM
 * @see loadFromEEPROM
 */
inline void storeToEEPROM () {
	byte counter;
	byte *ptrData;

	ptrData = &tmpData.red;
	counter = 0;

	for (; counter < sizeof (tmpData); ++counter) {
		EEPROM.write (ADDRESS_EEPROM + counter, *ptrData);
		ptrData += sizeof (byte);
	}
}



/**
 * reading global dataset from EEPROM
 * @see storeToEEPROM
 */
inline void loadFromEEPROM () {
	byte counter;
	byte *ptrData;

	ptrData = &tmpData.red;
	counter = 0;
	
	for (; counter < sizeof (tmpData); ++counter) {
		*ptrData = EEPROM.read (ADDRESS_EEPROM + counter);
		ptrData += sizeof (byte);
	}
}


#ifdef DEBUG
void debugSensor () {
#ifdef DHTTYPE
	float humidity = dht.readHumidity();                           
	float temperature = dht.readTemperature(); 

	Serial.print ("\nTemperature: ");
	Serial.print (temperature);
	Serial.print ("  Humidity: ");
	Serial.print (humidity);
#endif

	Serial.print ("\nred ");
	Serial.print (tmpData.red);
	Serial.print ("  blue ");
	Serial.print (tmpData.blue);
	Serial.print ("  green ");
	Serial.print (tmpData.green);
	Serial.print ("  brightness ");
	Serial.print (tmpData.brightness);
	Serial.print ("  mode ");
	Serial.print (tmpData.mode);
	Serial.print ("  state ");
	Serial.print (tmpData.state);
}
#endif



void sendToBluetooth () {
	
}



void setup () {
	loadFromEEPROM ();				// load last used values from EEPROM

	pinMode (GREEN_LED, OUTPUT);
	pinMode (RED_LED,   OUTPUT);
	pinMode (BLUE_LED,  OUTPUT);

	mySerial.begin (9600);			// sets Bluetooth baudrate

#ifdef DHTTYPE
	dht.begin();
#endif

#ifdef DEBUG
	Serial.begin (9600);
	Serial.print ("serial works\n");
#endif

	outputRGB ();
}



void loop () {
	unsigned long currentMillis = millis();
	Action action;

	while (Serial.available () > 0) {
		action = (Action)Serial.parseInt();
		
		switch(action) {
			case LED_COMMAND:
				tmpData.red = Serial.parseInt() % 32;
				tmpData.blue = Serial.parseInt() % 32;
				tmpData.green = Serial.parseInt() % 32;
				tmpData.brightness = Serial.parseInt() % 9;
				tmpData.mode = Serial.parseInt() % 3;
				tmpData.state = Serial.parseInt() % 2;
				if (Serial.read() == '\n') {
					outputRGB ();
					storeToEEPROM ();
				}
				break;;

			case GET_STATUS:
				if (Serial.read () == '\n') {
#ifdef DEBUG
					debugSensor ();
#endif
				}
				break;;
		}
	}

#ifdef DEBUG
	if (currentMillis - previousMillis > INTERVAL_UPDATE) {
		previousMillis = currentMillis;
		debugSensor ();
	}
#endif
}
