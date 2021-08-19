// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "M5StickCPlus.h"

M5StickCPlus::M5StickCPlus():isInited(0) {

}

void M5StickCPlus::begin(bool LCDEnable, bool PowerEnable, bool SerialEnable){
	
	//! Correct init once
	if (isInited) return;
	else isInited = true;


	//! UART
	if (SerialEnable) {
		Serial.begin(115200);
		Serial.flush();
		delay(50);
		Serial.print("M5StickCPlus initializing...");
	}

    // Power
	if (PowerEnable) {
		axp.begin();
	}

	// LCD INIT
	if (LCDEnable) {
		lcd.begin();
	}

	if (SerialEnable) {
		Serial.println("OK");
	}

    beep.begin();
    
	//Rtc.begin();
}

void M5StickCPlus::update() {
	//M5.buttonA.read();
	//M5.buttonB.read();
}

M5StickCPlus M5;