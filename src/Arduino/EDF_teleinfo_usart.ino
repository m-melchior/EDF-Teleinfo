/* EDF_teleinfo_usart - An application to read the EDF teleinfo messages
 * Copyright (C) 2016 Michael Melchior <MelchiorMichael@web.de>
 *
 * This program is free software: You can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 */

/*
 *                         EDF_teleinfo_usart.ino  -  description
 *                         ------------------------------------------
 *   begin                : written in 2016 by Michael Melchior
 *   copyright            : (c) 2016 by Michael Melchior
 *   description          : An application to read the EDF teleinfo messages
 *
*/

/*
Labels to be read (for more details, ref. http://www.erdf.fr/sites/default/files/ERDF-NOI-CPT_02E.pdf):
STX
ADCO 123456789012 K
OPTARIF HC.. <
ISOUSC 45 ?
HCHC 123456789 0
HCHP 123456789 +
PTEC HP..  
IINST 002 Y
IMAX 045 H
PAPP 00460 +
HHPHC E 0
MOTDETAT 40CC00 ,
ETX
 */

#include <Wire.h>
#include <LCD.h>
#include <LiquidCrystal_I2C.h>

#define I2C_ADDR 0x27 // address of the I2C talking to the LCD. Might be different for you, ref. http://playground.arduino.cc/Main/I2cScanner
#define BACKLIGHT_PIN 3
#define En_pin  2
#define Rw_pin  1
#define Rs_pin  0
#define D4_pin  4
#define D5_pin  5
#define D6_pin  6
#define D7_pin  7

#define BUTTON_PIN 7 // to switch through the values

#define STX_CHAR 0x02    // start of frame
#define LF_CHAR 0x0a     // start of group
#define SP_CHAR 0x20     // delimiter
#define CR_CHAR 0x0d     // end of group
#define ETX_CHAR 0x03    // end of frame

#define WATERHEATER_PIN 8

#define STATUS_LED_PIN 13

#define ON 1
#define OFF 0

boolean statusLED_Status;

boolean waterheater_Status;

char charIn;
int labelSize;
int valueSize;
int labelCounter = 0;
int selectedLabel = 0;
int maxLabels = 10; // 11 - 1

char labelChars[9];
char valueChars[13];

LiquidCrystal_I2C lcd(I2C_ADDR, En_pin, Rw_pin, Rs_pin, D4_pin, D5_pin, D6_pin, D7_pin);

boolean buttonStatus;

void setup() {
	pinMode(BUTTON_PIN, INPUT);
	buttonStatus = OFF;

	pinMode(STATUS_LED_PIN, OUTPUT);

	pinMode(WATERHEATER_PIN, OUTPUT);

	turnWaterheaterOff();

	Serial.begin(1200);

	toggleStatusLED();

	lcd.begin (16,2);

	lcd.setBacklightPin(BACKLIGHT_PIN, POSITIVE);
	lcd.setBacklight(HIGH);
	lcd.home ();
	lcd.print("OK.");
} // void setup() {


void toggleStatusLED() {
	statusLED_Status = !statusLED_Status;
	digitalWrite(STATUS_LED_PIN, statusLED_Status);
} // void toggleStatusLED() {


void turnWaterheaterOn() {
	digitalWrite(WATERHEATER_PIN, LOW);
	waterheater_Status = ON;
} // void turnWaterHeaterOn()


void turnWaterheaterOff() {
	digitalWrite(WATERHEATER_PIN, HIGH);
	waterheater_Status = OFF;
} // void turnWaterHeaterOff()


void waitForNewFrame() {
	charIn = 0;
	// wait for a new frame to start
	while (charIn != STX_CHAR) {
		// wait for incoming transmission
		while(Serial.available() == 0) {
		} // while(Serial.available() == 0) {

		charIn = Serial.read() & 0x7F;
	} // while (charIn != STX_CHAR)
} // void waitForNewFrame() {


void loop() {

	waitForNewFrame();

	toggleStatusLED();

	while(1) {
		// wait for incoming transmission
		while(Serial.available() == 0) {
		} // while(Serial.available() == 0) {

		charIn = Serial.read() & 0x7F;

		if(charIn == ETX_CHAR) {
			// end of frame
			break;
		} else if (charIn != LF_CHAR) { // if(charIn == ETX_CHAR)
			// there should be no line feed here
			break;
		} //  if(charIn == ETX_CHAR)


		// read label
		// reset old label first
		memset(labelChars, 0, 8);
		labelSize = 0;
		while(1) {
			// wait for incoming transmission
			while(Serial.available() ==0) {
			} // while(Serial.available() ==0) {

			charIn = Serial.read() & 0x7F;

			if (charIn == SP_CHAR) {
				// end of label
				break;
			} else { // if (charIn == SP_CHAR) {
				labelChars[labelSize] = charIn;
				labelSize++;
				// todo: check for size in case we missed a spacer
			} // if (charIn == SP_CHAR) {
		} // while(1) {

		// read value
		// reset old value first
		memset(valueChars, 0, 12);
		valueSize = 0;
		while(1) {
			// wait for incoming transmission
			while(Serial.available() == 0) {
			} // while(Serial.available() == 0) {

			charIn = Serial.read() & 0x7F;

			if (charIn == SP_CHAR) {
				// end of value
				break;
			} else { // if (charIn == SP_CHAR) {
				valueChars[valueSize] = charIn;
				valueSize++;
				// todo: check for size in case we missed a spacer
			} // if (charIn == SP_CHAR) {
		} // while(1) {

		// read checksum
		while(Serial.available() == 0) {
		} // while(Serial.available() ==0) {

		charIn = Serial.read() & 0x7F;
		// todo: check checksum

		// read CR
		while(Serial.available() == 0) {
		} // while(Serial.available() ==0) {

		charIn = Serial.read() & 0x7F;
		
		if (charIn != CR_CHAR) {
			break;
		} // if (charIn != CR_CHAR) {

		// turn on / off the water heater depending on the high price / low price period
		if (waterheater_Status == ON) {
			if (strncmp(labelChars, "PTEC", 4) == 0 && strncmp(valueChars, "HP..", 4) == 0) {
				turnWaterheaterOff();
			} // if (strncmp(labelChars, "PTEC", 4) == 0 && strncmp(valueChars, "HP..", 4) == 0) {
		} else { // if (waterheater_Status == ON) {
			if (strncmp(labelChars, "PTEC", 4) == 0 && strncmp(valueChars, "HC..", 4) == 0) {
				turnWaterheaterOn();
			} // if (strncmp(labelChars, "PTEC", 4) == 0 && strncmp(valueChars, "HC..", 4) == 0) {
		} // if (waterheater_Status == ON) {
		  
		if (buttonStatus == OFF && digitalRead(BUTTON_PIN)) {
			selectedLabel++;
			if (selectedLabel > maxLabels) {
				selectedLabel = 0;
			} // if (selectedLabel > maxLabels) {
			// to debounce the button
			buttonStatus = ON;
		} else if (buttonStatus == ON && !digitalRead(BUTTON_PIN)) { // if (buttonStatus == OFF && digitalRead(BUTTON_PIN)) {
			buttonStatus = OFF;
		} // if (buttonStatus == OFF && digitalRead(BUTTON_PIN)) {

		// update screen with the value of the chosen label
		// todo: update only upon change of value
		if (labelCounter == selectedLabel) {
			lcd.clear();
			lcd.home();
			lcd.print(labelChars);
			lcd.setCursor(0, 1);
			lcd.print(valueChars);
		} // if (labelCounter == selectedLabel) {

		labelCounter++;
		if(labelCounter > maxLabels) {
			labelCounter = 0;
		} // if(labelCounter > maxLabels) {
	} // while(1) {
} // loop()
