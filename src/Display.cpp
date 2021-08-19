#include <Arduino.h>
#include <pcf8563.h>
#include "M5StickCPlus.h"
#include <Display.h>
#include <Display_Picture.h>

void Display_Setup() {

    M5.lcd.init();
    M5.lcd.setRotation(1);
    M5.lcd.setSwapBytes(true);
    M5.lcd.pushImage(0, 0, 160, 80, ICON_Johnson_logo);
}

void Display_OFF() {
    Serial.println("Display OFF");
    M5.lcd.writecommand(TFT_SLPIN);
    M5.lcd.writecommand(TFT_DISPOFF);
    Hardware_Set_TFT_Backlit(TFT_BACKLIT_OFF);
}

void Display_ON() {
    Serial.println("Display ON");
    M5.lcd.writecommand(TFT_SLPOUT);
    M5.lcd.writecommand(TFT_DISPON);
    Hardware_Set_TFT_Backlit(TFT_BACKLIT_ON);
}

bool BLE_Name_initial = 1;
void Display_BLE_Name(String ble_name) {

    if(BLE_Name_initial)  {
        M5.lcd.setTextSize(1);
        M5.lcd.setTextColor(TFT_WHITE, TFT_BLACK);
        M5.lcd.setCursor (10, 0);
        M5.lcd.print("BLE Name: " + ble_name);
    }
    BLE_Name_initial = 0;
}

bool date_initial = 1;
uint8_t omm = 99;
uint8_t xcolon = 0;

void Display_RTC() {

    struct tm rtcTime;

    if(!getLocalTime(&rtcTime)) {
    Serial.println("Failed to obtain time");
    return ;
    }
    
    if (date_initial) {
        Serial.println("Display RTC");
        M5.lcd.fillScreen(TFT_BLACK);
    }
    if (rtcTime.tm_sec == 0 || date_initial) {
        M5.lcd.setTextColor(TFT_GREEN, TFT_BLACK);
        M5.lcd.setCursor (STR_DATE_LOCATION_X, STR_DATE_LOCATION_Y);
        M5.lcd.print(__DATE__); // This uses the standard ADAFruit small font
    }
    uint8_t xpos = STR_TIME_LOCATION_X;
    uint8_t ypos = STR_TIME_LOCATION_Y;
    if (omm != rtcTime.tm_min || date_initial) { // Only redraw every minute to minimise flicker
        // Uncomment ONE of the next 2 lines, using the ghost image demonstrates text overlay as time is drawn over it
        omm = rtcTime.tm_min;
        M5.lcd.setTextColor(0x39C4, TFT_BLACK);  // Leave a 7 segment ghost image, comment out next line!
        //M5.lcd.setTextColor(TFT_BLACK, TFT_BLACK); // Set font colour to black to wipe image
        // Font 7 is to show a pseudo 7 segment display.
        // Font 7 only contains characters [space] 0 1 2 3 4 5 6 7 8 9 0 : .
        M5.lcd.drawString("88:88", xpos, ypos, FONT_RTC_SIZE); // Overwrite the text to clear it
        M5.lcd.setTextColor(0xFBE0, TFT_BLACK); // Orange
        

        if (rtcTime.tm_hour < 10) xpos += M5.lcd.drawChar('0', xpos, ypos, FONT_RTC_SIZE);
        xpos += M5.lcd.drawNumber(rtcTime.tm_hour, xpos, ypos, FONT_RTC_SIZE);
        xcolon = xpos;
        xpos += M5.lcd.drawChar(':', xpos, ypos, FONT_RTC_SIZE);
        
        if (rtcTime.tm_min < 10) xpos += M5.lcd.drawChar('0', xpos, ypos, FONT_RTC_SIZE);
        M5.lcd.drawNumber(rtcTime.tm_min, xpos, ypos, FONT_RTC_SIZE);
    }

    if (rtcTime.tm_sec % 2) { // Flash the colon
        M5.lcd.setTextColor(0x39C4, TFT_BLACK);
        xpos += M5.lcd.drawChar(':', xcolon, ypos, FONT_RTC_SIZE);
        
    } else {
            M5.lcd.setTextColor(0xFBE0, TFT_BLACK);
            M5.lcd.drawChar(':', xcolon, ypos, FONT_RTC_SIZE);
        }
    date_initial = 0;
}

bool ChargeStatus_initial = 1;
bool chager_statu_old;
void Display_ChargeStatus(bool status) {
    
    if(status != chager_statu_old|| ChargeStatus_initial) {
        if (status) {
            Serial.println("Battery Charge ON");
            M5.lcd.pushImage(ICON_CHARGE_LOCATION_X, ICON_CHARGE_LOCATION_Y, ICON_CHARGE_SIZE_W, ICON_CHARGE_SIZE_H, ICON_Charge);
        } 
        else {
            Serial.println("Battery Charge OFF");
            M5.lcd.fillRect(ICON_CHARGE_LOCATION_X, ICON_CHARGE_LOCATION_Y, ICON_CHARGE_SIZE_W, ICON_CHARGE_SIZE_H, TFT_BLACK);
        }    
    }
    chager_statu_old = status;
    ChargeStatus_initial = 0;
}

bool Battery_initial = 1;
String old_percent = "  ";
void Display_Battery(String percent) {
    if(Battery_initial) {
        M5.lcd.pushImage(ICON_BAT_LOCATION_X, ICON_BAT_LOCATION_Y, ICON_BAT_SIZE_W, ICON_BAT_SIZE_H, ICON_Battery);
    }
    if(Battery_initial || old_percent != percent) {
        Serial.println("Display Battery percent");
        M5.lcd.setTextColor(TFT_BLACK, TFT_WHITE);
        M5.lcd.drawString(percent, STR_BAT_LOCATION_X, STR_BAT_LOCATION_Y, 1); // Next size up font 2
        old_percent = percent;
    }
    Battery_initial = 0; 
}

bool BLEStatu_inital = 1;
uint8_t BLEStatu_old;
void Display_BLEStatu(uint8_t status) {
    
    if(status != BLEStatu_old|| BLEStatu_inital) {
        if (status == BLE_STATUS_CONNECTED) {
            Serial.println("BLE Connected");
            M5.lcd.pushImage(ICON_BLESTATU_LOCATION_X, ICON_BLESTATU_LOCATION_Y, ICON_BLESTATU_SIZE_W, ICON_BLESTATU_SIZE_H, ICON_BLE_Statu);
        } 
        else {
            Serial.println("BLE Disconnect");
            M5.lcd.fillRect(ICON_BLESTATU_LOCATION_X, ICON_BLESTATU_LOCATION_Y, ICON_BLESTATU_SIZE_W, ICON_BLESTATU_SIZE_H, TFT_BLACK);
        }
    }      
    BLEStatu_old = status;
    BLEStatu_inital = 0;
}
bool WiFiStatu_inital = 1;
uint8_t WiFiStatu_old;
void Display_WiFiStatu(uint8_t status) {
    String buff;
    if(status != WiFiStatu_old || WiFiStatu_inital) {
        if (status == WIFI_STATUS_CONNECTED) {
            Serial.println("WiFi Connected");
            M5.lcd.pushImage(ICON_WIFISTATU_LOCATION_X, ICON_WIFISTATU_LOCATION_Y, ICON_WIFISTATU_SIZE_W, ICON_WIFISTATU_SIZE_H, ICON_WiFi_Statu);
        } 
        else {
            Serial.println("WiFi OFF");
            M5.lcd.fillRect(ICON_WIFISTATU_LOCATION_X, ICON_WIFISTATU_LOCATION_Y, ICON_WIFISTATU_SIZE_W, ICON_WIFISTATU_SIZE_H, TFT_BLACK);
        }    
    }   
    WiFiStatu_old = status;
    WiFiStatu_inital = 0; 
}

void Display_WiFi_Switch(uint8_t status) {

    M5.lcd.fillScreen(TFT_BLACK);
    M5.lcd.pushImage(5, 10, ICON_WIFI_SIZE_W, ICON_WIFI_SIZE_H, ICON_WiFi);
    
    if(status == WIFI_STATUS_OFF) M5.lcd.pushImage(80, 30, ICON_SETTING_WIFI_SWITCH_SIZE_W, ICON_SETTING_WIFI_SWITCH_SIZE_H, ICON_Switch_OFF);
     else M5.lcd.pushImage(80, 30, ICON_SETTING_WIFI_SWITCH_SIZE_W, ICON_SETTING_WIFI_SWITCH_SIZE_H, ICON_Switch_ON);

    M5.lcd.setTextColor(TFT_WHITE, TFT_BLACK);
    M5.lcd.setCursor(70, 15);
    M5.lcd.setTextSize(1);
    switch(status) {
        
        case WIFI_STATUS_CONNECTING:
        M5.lcd.print("Connecting");
        Serial.println("WiFi Connecting...");  
        break;

        case WIFI_STATUS_CONNECTED:
        M5.lcd.print("Connecting");
        Serial.println("WiFi Connected");  
        break;

        case WIFI_STATUS_DISCONNECTED:
        M5.lcd.print("Disconnected");
        Serial.println("WiFi Disconnected");  
        break;

        case WIFI_STATUS_OFF:
        M5.lcd.pushImage(5, 10, ICON_WIFI_SIZE_W, ICON_WIFI_SIZE_H, ICON_WiFi);
        M5.lcd.print("WiFi Off");
        Serial.println("WiFi Off");  
        break;

        default:
        M5.lcd.print("default");
        Serial.println("default");  
        break;


    }

}


void Display_IMU_Switch(bool status) {

    M5.lcd.fillScreen(TFT_BLACK);
    if(status) {
        M5.lcd.pushImage(80-25, 30 , 50, 20, ICON_Switch_ON);
        M5.lcd.setTextColor(TFT_WHITE, TFT_BLACK);
        M5.lcd.setCursor(20, 15);
        M5.lcd.setTextSize(1);
        M5.lcd.print("Motion Capture On..");
        Serial.println("Motion Capture On..");  

    }
    else {
        M5.lcd.pushImage(80-25, 30 , 50, 20, ICON_Switch_OFF);
        M5.lcd.setTextColor(TFT_WHITE, TFT_BLACK);
        M5.lcd.setCursor(20, 15);
        M5.lcd.setTextSize(1);
        M5.lcd.print("Motion Capture Off..");
        Serial.println("Motion Capture Off..");  
    }

}

#define IUM_TITLE_Y  0
#define IUM_TITLE_X  0
#define IUM_DIS_GAP_Y  12
bool IMU_inital = 1;

void Display_Sensor(IMU_DATA imu,uint8_t Index)
{
    if (IMU_inital) {
        
        M5.lcd.fillScreen(TFT_BLACK);
        M5.lcd.setTextColor(TFT_WHITE, TFT_BLACK);
        M5.lcd.setTextSize(1);
        M5.lcd.setCursor(IUM_TITLE_Y, IUM_TITLE_X);
        
        switch(Index) {
            case INDEX_ACCEL:
            M5.lcd.print("Accelerometer(g)");  M5.lcd.print(" - FSR:");  M5.lcd.print(imu.AccelFSR);
            M5.lcd.setCursor(IUM_TITLE_X , IUM_TITLE_Y + IUM_DIS_GAP_Y);
            M5.lcd.setTextColor(TFT_BLUE, TFT_BLACK);
            M5.lcd.print("SampleRate:");  M5.lcd.print(imu.All_SampleRate);  M5.lcd.print("Hz ");
            M5.lcd.print("LPF:");  M5.lcd.print(imu.AG_LPF);   M5.lcd.print("Hz");
            break;

            case INDEX_GYRO:
            M5.lcd.print("Gyroscope(dps)");   M5.lcd.print(" - FSR:");  M5.lcd.print(imu.GyroFSR); 
            M5.lcd.setCursor(IUM_TITLE_X  , IUM_TITLE_Y + IUM_DIS_GAP_Y);
            M5.lcd.setTextColor(TFT_BLUE, TFT_BLACK);
            M5.lcd.print("SampleRate:");  M5.lcd.print(imu.All_SampleRate);  M5.lcd.print("Hz ");
            M5.lcd.print("LPF:");  M5.lcd.print(imu.AG_LPF);   M5.lcd.print("Hz");
            break;

            case INDEX_COMPASS:
            M5.lcd.print("Magnetometer(uT)");  
            M5.lcd.setCursor(IUM_TITLE_X  , IUM_TITLE_Y + IUM_DIS_GAP_Y);
            M5.lcd.setTextColor(TFT_BLUE, TFT_BLACK);
            M5.lcd.print("SampleRate:");  M5.lcd.print(imu.All_SampleRate);  M5.lcd.print("Hz ");
            break;

            case INDEX_EULER:
            M5.lcd.println("Roll/Pitch/Yaw(o)");  
            M5.lcd.setCursor(IUM_TITLE_X  , IUM_TITLE_Y + IUM_DIS_GAP_Y);
            M5.lcd.setTextColor(TFT_BLUE, TFT_BLACK);
            M5.lcd.print("SampleRate:");  M5.lcd.print(imu.All_SampleRate);  M5.lcd.print("Hz ");
            break;

            default:
            break;
        }

    }
    
    M5.lcd.setTextColor(TFT_GREEN, TFT_BLACK);
    M5.lcd.setTextSize(1);
    M5.lcd.setCursor(IUM_TITLE_X , IUM_TITLE_Y + (IUM_DIS_GAP_Y*2));

    switch(Index) {

        case INDEX_ACCEL:
        M5.lcd.print("Accel X: "); M5.lcd.println(String(imu.ax,4));  
        M5.lcd.setCursor(IUM_TITLE_X , IUM_TITLE_Y + (IUM_DIS_GAP_Y*3));
        M5.lcd.print("Accel Y: "); M5.lcd.println(String(imu.ay,4)); 
        M5.lcd.setCursor(IUM_TITLE_X , IUM_TITLE_Y + (IUM_DIS_GAP_Y*4));
        M5.lcd.print("Accel Z: "); M5.lcd.println(String(imu.az,4));
        break;

        case INDEX_GYRO:
        M5.lcd.print("Gyro X: "); M5.lcd.println(String(imu.gx,4));   
        M5.lcd.setCursor(IUM_TITLE_X , IUM_TITLE_Y + (IUM_DIS_GAP_Y*3));
        M5.lcd.print("Gyro Y: "); M5.lcd.println(String(imu.gy,4));  
        M5.lcd.setCursor(IUM_TITLE_X , IUM_TITLE_Y + (IUM_DIS_GAP_Y*4));
        M5.lcd.print("Gyro Z: "); M5.lcd.println(String(imu.gz,4));
        break;

        case INDEX_COMPASS:
        M5.lcd.print("Magneto X: "); M5.lcd.println(String(imu.mx,2));   
        M5.lcd.setCursor(IUM_TITLE_X , IUM_TITLE_Y + (IUM_DIS_GAP_Y*3));
        M5.lcd.print("Magneto Y: "); M5.lcd.println(String(imu.my,2));  
        M5.lcd.setCursor(IUM_TITLE_X , IUM_TITLE_Y + (IUM_DIS_GAP_Y*4));
        M5.lcd.print("Magneto Z: "); M5.lcd.println(String(imu.mz,2)); 
        break;

        case INDEX_EULER:
        M5.lcd.print("Roll:  ");  M5.lcd.println(String(imu.roll,2));   
        M5.lcd.setCursor(IUM_TITLE_X , IUM_TITLE_Y + (IUM_DIS_GAP_Y*3));
        M5.lcd.print("Pitch: "); M5.lcd.println(String(imu.pitch,2));  
        M5.lcd.setCursor(IUM_TITLE_X , IUM_TITLE_Y + (IUM_DIS_GAP_Y*4));
        M5.lcd.print("Yaw:   ");   M5.lcd.println(String(imu.yaw,2));
        break;

        default:
        break;
    }
    IMU_inital = 0;
}

void Display_BLE_ATCommand(String display) {
    
    M5.lcd.fillScreen(TFT_BLACK);
    M5.lcd.pushImage(80-30, 20, ICON_SETTING_SIZE_W, ICON_SETTING_SIZE_H, ICON_Setting);
    M5.lcd.setTextColor(TFT_WHITE,TFT_BLACK);
    M5.lcd.setTextColor(TFT_WHITE, TFT_BLACK);
    M5.lcd.setCursor(0, 5);
    M5.lcd.setTextSize(1);
    M5.lcd.print(display);
    Serial.println(display);  

}

uint8_t Display_ATCommand_Process(uint8_t Index) {
    //BLE UART AT Command 处理

    switch (Index) {

        case ATCMD_RESET:
        break;

        case ATCMD_DATE:
        Display_BLE_ATCommand("RTC Date/Time modified");
        break;

        case ATCMD_WIFI_ON:
        Display_BLE_ATCommand("WiFi Turn On");
        break;

        case ATCMD_WIFI_OFF:
        Display_BLE_ATCommand("WiFi Turn Off");
        break;

        case ATCMD_WIFI_ID:
        Display_BLE_ATCommand("WiFi SSID modified");
        break;

        case ATCMD_WIFI_PD:
        Display_BLE_ATCommand("WiFi Password modified");
        break;

        case ATCMD_SYS_DISPLAY_OFF_TIME:
        Display_BLE_ATCommand("Display Off Time modified");
        break;

        case ATCMD_SYS_SLEEP_TIME:
        Display_BLE_ATCommand("System Sleep Time modified");
        break;

        case ATCMD_IMU_ACCEL_FSR:
        Display_BLE_ATCommand("Accel FSR modified");
        break;

        case ATCMD_IMU_GYRO_FSR:
        Display_BLE_ATCommand("Gyro FSR modified");
        break;

        case ATCMD_IMU_AG_LPF:
        Display_BLE_ATCommand("Accel/Gyro LPF modified");

        break;

        case ATCMD_IMU_SAMPLERATE:
        Display_BLE_ATCommand("IMU SampleRate modified");
        break;

        case ATCMD_IMU_MODE_3D:
        Display_BLE_ATCommand("IMU Mode switch to Accel");
        break;

        case ATCMD_IMU_MODE_GYRO:
        Display_BLE_ATCommand("IMU Mode switch to Gyro");
        break;
        
        case ATCMD_IMU_MODE_ALL:
        Display_BLE_ATCommand("IMU Mode switch to Accel&Calulate");
        break;

        case ATCMD_IMU_MODE_PDMETER:
        Display_BLE_ATCommand("IMU Mode switch to Pademeter");
        break;
            
        case ATCMD_IMU_ON:
        Display_BLE_ATCommand("IMU Mode switch ON");
        break;

        case ATCMD_IMU_OFF:
        Display_BLE_ATCommand("IMU Mode switch OFF");
        break;

        default:
        break;
    }
    return Index;
}

void Display_initial_refresh() {

    date_initial = 1;
    IMU_inital = 1;

    Battery_initial = 1;

    ChargeStatus_initial = 1;
    BLEStatu_inital = 1;
    WiFiStatu_inital = 1;
    BLE_Name_initial = 1;
}