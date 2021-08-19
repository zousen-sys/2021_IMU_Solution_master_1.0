
#include <Main_RTC.h>

void Main_RTC_Setup(){
    

}

uint8_t Main_RTC_Process(){
    bool initail = true;
    bool Message_Duty = false;
    uint8_t ATCommand ;
    uint8_t Display_Index = INDEX_RTC;
    Display_initial_refresh();
    while(1) {
        // 在线更新 Http update server & Arduini OTA 
        if(Hardware_WiFi_Process() == WIFI_STATUS_DISCONNECT) {
            Display_WiFi_Switch(WIFI_STATUS_DISCONNECTED);
            Hardware_Timer_Reset(TIMER_TWO,MESSAGE_DELAY_TIME);
            TouchButton_DispalyOff_Overtime_Reset();
            Message_Duty = true;
        }

        Hardware_BLE_Process();

        // 九轴传感器数据 解析 
        Hardware_IMU_Process();

        //触控按键处理
        if (TouchButton_DispalyOff_Wakup()) {
           Display_ON();
        }

        if (TouchButton_DispalyOff_Overtime()) {
            Display_OFF();
        }
        switch (TouchButton_Process()) {
            case BUTTON_CLICK:
            Serial.println("BUTTON_CLICK");
            if(Display_Index < INDEX_GYRO) {
                ++Display_Index;
                Hardware_Timer_Reset(TIMER_ONE,10);
            }    
            else  {
                Display_Index = INDEX_RTC;
                Hardware_Timer_Reset(TIMER_ONE,10);
            }    
            Display_initial_refresh();
            initail = true;
            break;

            case BUTTON_CLICK_2:
             Serial.println("BUTTON_CLICK_2");
            break;
        
            case BUTTON_CLICK_3:
             Serial.println("BUTTON_CLICK_3");

            Hardware_Toggle_WIFI();
            if(Hardware_Get_WIFI()) Display_WiFi_Switch(WIFI_STATUS_CONNECTING);
             else Display_WiFi_Switch(WIFI_STATUS_OFF);
            Hardware_Timer_Reset(TIMER_TWO,MESSAGE_DELAY_TIME);
            Message_Duty = true;
            break;

            case BUTTON_HOLD_1S:
            case BUTTON_HOLD_2S:
            case BUTTON_HOLD_3S:
            Serial.println("BUTTON_HOLD");
            if(Hardware_IMU_Get_Enabled()) Hardware_IMU_Set_Enabled(false);
                else Hardware_IMU_Set_Enabled(true);
            Display_IMU_Switch(Hardware_IMU_Get_Enabled());
            Hardware_Timer_Reset(TIMER_TWO,MESSAGE_DELAY_TIME);
            Message_Duty = true;
            break;

            case BUTTON_HOLD_5S:
            break;
        
            default:
            break;
        }
        
      
        if(Hardware_Get_BLE_Status() && Hardware_IMU_Get_Enabled())  TouchButton_Sleep_Overtime_Reset();

        if(TouchButton_Sleep_Overtime()) {

            Display_OFF();
            Hardware_Set_SystemSleep();
        }

        ATCommand = Hardware_BLE_ATCommand_Decode();
        Display_ATCommand_Process(ATCommand);
        switch(ATCommand) {
            case ATCMD_IMU_ON:
            Hardware_IMU_Set_Enabled(true);
            break;

            case ATCMD_IMU_OFF:
            Hardware_IMU_Set_Enabled(false);
            break;

            default:
            break;
        }

        if(ATCommand != ATCMD_NA) {
            Hardware_Timer_Reset(TIMER_TWO,MESSAGE_DELAY_TIME);
            TouchButton_DispalyOff_Overtime_Reset();
            Display_ON();
            Message_Duty = true;
        }


        //首次进入执行
        if(initail) {

        }


        if(Message_Duty) {
            
            if(Hardware_Timer_Refresh(TIMER_TWO, MESSAGE_DELAY_TIME)) { 
                Message_Duty = false;
                Display_initial_refresh();
                initail = 1;
            }    
        }
        else {
            //每秒更新 RTC 日期/时间 及 电池电量..等显示
            switch((Display_Index)) {
                case INDEX_RTC:
                if (Hardware_Timer_Refresh(TIMER_ONE,1000) || initail) {
                    Display_RTC();
                    Display_BLE_Name(Hardware_Get_BLE_Hostname());
                    Display_Battery(Hardware_Get_Battery_Percent());
                }
                break;

                case INDEX_ACCEL:
                case INDEX_GYRO:
                case INDEX_COMPASS:
                case INDEX_EULER:
                if (Hardware_Timer_Refresh(TIMER_ONE,100) || initail) {
                    Display_Sensor(Hardware_Get_IMU(),Display_Index);
                    Display_Battery(Hardware_Get_Battery_Percent());
                }

                break;

                default:
                break;

            }

            //更新蓝芽及WiFi显示图标
        
            Display_BLEStatu(Hardware_Get_BLE_Status());
            Display_WiFiStatu(Hardware_Get_WiFi_Status());
        

            //更新锂电池充电状态显示图标
            if(Hardware_Get_ChargeIndication() || initail) {
            Display_ChargeStatus(Hardware_Get_ChargeStatu());
            }
        }
        initail = false;
    }
}

