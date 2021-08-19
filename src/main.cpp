#include <M5StickCPlus.h>
#include <WiFi.h>

//Local function"
#include "Hardward.h"
#include "TouchButton.h"
#include "Display.h"
#include "Main_RTC.h"
#include "Main_Process_Mode.h"


// Process Mode definition
uint8_t Process_Mode=0;

void setup() {
  // put your setup code here, to run once:
    M5.begin();
    Display_Setup();
    Hardware_Setup();
    TouchButton_Setup();
    Main_RTC_Setup();
    Process_Mode = PM_RTC;
  
}


void loop() {
    
    switch (Process_Mode) {
        case PM_RTC:
        Process_Mode = Main_RTC_Process();
        break;
    }
}

