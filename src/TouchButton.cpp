#include <TouchButton.h>
#include <Hardward.h>

bool pressed = false;
bool relased = true;
uint32_t pressedTime = 0;
uint32_t pressedCounter = 0;
uint32_t releaseTime = 0;
uint32_t ButtonOptput = BUTTON_NA;
uint32_t DisplayOffOverTimer;
uint32_t SleepOverTimer;

bool DisplayOffOverTimeFlag = false;
bool DisplayOffOverTimeFlag_old = true;
bool TouchPassOnce = false;
uint32_t Sleep_Over_Time_Target;
uint32_t DisplayOff_Over_Time_Target;

void TouchButton_Setup() {

    pinMode(TP_PIN_PIN, INPUT);
    Sleep_Over_Time_Target = (SLEEP_OVER_TIME*1000*60); // 变量 X 1000ms x 60 = 分钟
    DisplayOff_Over_Time_Target = (DISLAY_OVER_TIME*1000); // 变量 X 1000ms = 秒
    SleepOverTimer = millis() + Sleep_Over_Time_Target;
    DisplayOffOverTimer = millis() + DisplayOff_Over_Time_Target;
}

uint8_t TouchButton_Process() {
    if (digitalRead(TP_PIN_PIN) == LOW) {
        relased = false;
        if (!pressed) {
            pressed = true;
            pressedTime = millis();
            ++pressedCounter;
        } else {
            if (millis() - pressedTime > BUTTON_HOLD_1S_Time) ButtonOptput = BUTTON_HOLD_1S;
            if (millis() - pressedTime > BUTTON_HOLD_2S_Time) ButtonOptput = BUTTON_HOLD_2S;
            if (millis() - pressedTime > BUTTON_HOLD_3S_Time) ButtonOptput = BUTTON_HOLD_3S;
            if (millis() - pressedTime > BUTTON_HOLD_5S_Time) ButtonOptput = BUTTON_HOLD_5S;    
            }
    } else {
        pressed = false;
        if(!relased) {
            relased = true;    
            releaseTime =  millis();
        } else {
            if (millis() - releaseTime > BUTTON_RELASE_TIME) {
                
                if (ButtonOptput == BUTTON_HOLD_5S) {
                    ButtonOptput = BUTTON_NA;
                    pressedCounter = 0;
                    ESP.restart();
                    return(BUTTON_HOLD_5S);
                }
                if (ButtonOptput == BUTTON_HOLD_3S) {
                    ButtonOptput = BUTTON_NA;
                    pressedCounter = 0;
                    return(BUTTON_HOLD_3S);
                }
                if (ButtonOptput == BUTTON_HOLD_2S) {
                    ButtonOptput = BUTTON_NA;
                    pressedCounter = 0;
                    return(BUTTON_HOLD_2S);
                }
                if (ButtonOptput == BUTTON_HOLD_1S) {
                    ButtonOptput = BUTTON_NA;
                    pressedCounter = 0;
                    return(BUTTON_HOLD_1S);
                }
                if(pressedCounter) {
                    DisplayOffOverTimer = millis() + DisplayOff_Over_Time_Target;
                    SleepOverTimer = millis() + Sleep_Over_Time_Target;
                    DisplayOffOverTimeFlag = false;
                    ButtonOptput = pressedCounter;
                    pressedCounter = 0;
                    if (TouchPassOnce) {
                        TouchPassOnce = false;
                        ButtonOptput = 0;
                        pressedCounter = 0;
                    } 
                    return(ButtonOptput);
                }
                
            }
        } 
        
    }
    return(BUTTON_NA);
}

bool TouchButton_DispalyOff_Overtime() {
    if (DisplayOffOverTimer < millis()) {
        DisplayOffOverTimer = millis() + DisplayOff_Over_Time_Target;
        if(!DisplayOffOverTimeFlag) {
            DisplayOffOverTimeFlag = true;
            TouchPassOnce = true;
            return true;
        }
    }
    return false;
}

bool TouchButton_DispalyOff_Wakup() {
    bool output = false;
    if (DisplayOffOverTimeFlag == false) {
        if (DisplayOffOverTimeFlag  != DisplayOffOverTimeFlag_old) { 
            output = true;
        }
    }
    DisplayOffOverTimeFlag_old = DisplayOffOverTimeFlag;
    return output;
}

void TouchButton_DispalyOff_Overtime_Reset() {
    SleepOverTimer = millis() + Sleep_Over_Time_Target;
    DisplayOffOverTimer = millis() + DisplayOff_Over_Time_Target;
    DisplayOffOverTimeFlag = false;
}

void TouchButton_Sleep_Overtime_Set(uint32_t overtime) {
    Sleep_Over_Time_Target = overtime*1000*60; // 变量 X 1000ms x 60 = 分钟
    SleepOverTimer = millis() + Sleep_Over_Time_Target;

}

void TouchButton_DisplayOff_Overtime_Set(uint32_t overtime) {
    DisplayOff_Over_Time_Target = overtime*1000; // 变量 X 1000ms  = 秒
    DisplayOffOverTimer = millis() + DisplayOff_Over_Time_Target;
    DisplayOffOverTimeFlag = false;
}

bool TouchButton_Sleep_Overtime() {
    bool output = false;
    if (SleepOverTimer < millis()) {
        SleepOverTimer = millis() + Sleep_Over_Time_Target;
        output = true;
    }
    return output;
}

void TouchButton_Sleep_Overtime_Reset() {
    SleepOverTimer = millis() + Sleep_Over_Time_Target;
} 

