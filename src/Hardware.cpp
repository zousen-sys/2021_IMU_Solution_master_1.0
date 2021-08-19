#include <Arduino.h>
#include <Hardward.h>
#include <sensor.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <HTTPUpdateServer.h>
#include <ArduinoOTA.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <ATCommand.h>
#include <TouchButton.h>

void drawProgressBar(uint16_t x0, uint16_t y0, uint16_t w, uint16_t h, uint8_t percentage, uint16_t frameColor, uint16_t barColor);

//TFT显示相关变量

uint8_t BL_brightness = 30;
uint16_t Sensor_Refresh = 200;

// Wifi 相关变量
bool otaStart = false;
String WiFi_SSID = STASSID;
String WiFi_Password = STAPSK;
String AP_Hostname;
WebServer httpServer(80);
HTTPUpdateServer httpUpdater;
bool WIFI = false;
uint8_t WIFI_Status = WIFI_OFF;


//蓝芽BLE相关变量
BLEServer *pServer = NULL;
BLECharacteristic * pTxCharacteristic;
String BT_Hostname;
bool deviceConnected = false;
bool oldDeviceConnected = false;
bool BLE_UART_RxCallback =false;
uint8_t txValue = 0;
bool BLE = true;
uint8_t BLE_Status = BLE_STATUS_ADVERTISING;
String BLE_UART_RxData;


//RTC实时时钟相关变量
PCF8563_Class rtc;

//充电电路相关变量
bool charge_indication = false;
bool VBAT_indication = false;

uint16_t Battery_Voltage;
uint8_t Battery_Percent;


// 9D Sensor 九轴传感器 相关变量
bool IMU_Enable;
bool IMU_Mode_Accel = false;
bool IMU_Mode_Calulate = false;
bool IMU_Mode_Pedometer = true;

uint8_t Pedometer_DBtime = 30;  //debounce time unit 10ms
float Pedometer_Trigger = 0.55;
uint32_t Pedometer = 0;
uint32_t Pedometer_Time = 0;
float Pedometer_Accel_Sum_Differ; // 加速Z轴 前后差值累积值
float Pedometer_Accel_Avg_Differ; // 加速Z轴 前后差值平均值
bool Pedometer_Rise = false;
bool Pedometer_Fall = true;
bool Pedometer_Nobody = false;
bool Pedometer_MachineStop = true;

// CHIP ID
uint32_t chipId = 0;


void Hardware_Setup() {
    
    //! Must be set to pull-up output mode in order to wake up in deep sleep mode

    pinMode(LED_PIN, INPUT);

    Hardware_Set_TFT_Backlit(BL_brightness);

    Hardware_RTC_Setup();

    Hardware_IMU_setup();

    Hardware_IMU_Set_Enabled(1);

    Hardware_Get_ChipID();

    Hardware_BLE_Setup();

    Hardware_WiFi_Setup();


    if (Hardware_UpdateRTC_FromNTP() == false) {
        rtc.syncToSystem();
    }

}

bool Hardware_RTC_Setup(void) {
    bool output = false;
    
    Wire1.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    Wire1.setClock(400000);

    rtc.begin(Wire1);
    rtc.check();
    return output;
}

RTC_Date Hardware_Get_RTC(void) {
    
    return (rtc.getDateTime());
}

uint8_t Hardware_Get_TFT_Backlit() {
    return BL_brightness;
}


void Hardware_Set_TFT_Backlit(uint8_t brightness) {
    
    if(brightness) M5.axp.SetLDO2(1);
     else M5.axp.SetLDO2(0);
}

uint16_t Hardware_Get_Sensor_RefreshTime() {

    return Sensor_Refresh;
}

void Hardware_Set_Sensor_RefreshTime(uint16_t delaytime) {

    Sensor_Refresh = delaytime;
}

bool Hardware_Get_ChargeStatu() {

    if (M5.axp.GetBatChargeCurrent() > 0.1) { 
        return true;
    } else {
        return false;
    }    
}

bool Hardware_Get_ChargeIndication() {

    bool output;
    output = charge_indication;
    charge_indication = false;
    return output;
}


bool Hardware_Get_WIFI() {
    return WIFI;
}

bool Hardware_Get_BLE() {
    return BLE;
}

bool Hardware_Toggle_WIFI() {
    
    if(WIFI)  {
        WIFI = false;
        WIFI_Status = WIFI_STATUS_OFF;
    }    
    else  {
        WIFI = true;
    }
    return WIFI;
}

static uint32_t targetTime[10];
bool Hardware_Timer_Refresh(uint8_t channel, uint32_t ms) {
    if (targetTime[channel] < millis()) {
        targetTime[channel] = millis() + ms;
        return true;
    }
    return false;
}

void Hardware_Timer_Reset(uint8_t channel, uint32_t ms)  {
  
  targetTime[channel] = millis() + ms;

}

int vref = 1100;
String Hardware_Get_Battery_Percent()
{
    String output;
    Battery_Percent = M5.axp.GetBatPower();

    if(Battery_Percent < 10) output = "  " + String(Battery_Percent);
    if(Battery_Percent >= 10 && Battery_Percent < 100) output = " " + String(Battery_Percent);
    if(Battery_Percent >= 100) output = String(Battery_Percent);
    return output = output + "%";
}

const char* ntpServer0 = NTP_SERVER;
const char* ntpServer1 = NTP_SERVER1;
const char* ntpServer2 = NTP_SERVER2;
const long  gmtOffset_sec = NTP_GMT_OFFSET; 
const int   daylightOffset_sec = NTP_DAYLIGHT_OFFSET; 

bool Hardware_UpdateRTC_FromNTP(){

    struct tm timeinfo;

    if(WiFi.waitForConnectResult() != WL_CONNECTED) {
        Serial.println("WiFi disconnected");
        return false;
    }
    
    configTime(gmtOffset_sec, daylightOffset_sec,ntpServer0,ntpServer1,ntpServer2);
  
    if(!getLocalTime(&timeinfo)){
        Serial.println("Failed to obtain time");
        return false;
    }
    Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
    rtc.syncToRtc();
    
    return true;
}

IMU_DATA IMU_Data;
#define AZ_SUM_LENGHT 10
#define AZ_AVG_LENGHT 10
float az_buffer_sum[AZ_SUM_LENGHT];
float az_buffer_avg[AZ_AVG_LENGHT];
float az_old;

void Hardware_IMU_Calculate(void)
{  
    uint8_t loop;

    M5.imu.getAccelData(&IMU_Data.ax,&IMU_Data.ay,&IMU_Data.az);  
    M5.imu.getGyroData(&IMU_Data.gx,&IMU_Data.gy,&IMU_Data.gz);  

    // 针对加速传感器Z轴，取前后次数差值 进行 一定数量（AZ_AVG_LENGHT） 的平均值
    Pedometer_Accel_Avg_Differ = 0;
    for(loop = 0 ; loop <  AZ_AVG_LENGHT - 1 ; ++loop)  {
        
        az_buffer_avg[loop+1] = az_buffer_avg[loop];
        Pedometer_Accel_Avg_Differ += az_buffer_avg[loop+1];      
    }
    az_buffer_avg[0] = abs(az_old - IMU_Data.ax);
    Pedometer_Accel_Avg_Differ += az_buffer_avg[loop];  

    // 针对加速传感器Z轴，取前后次数差值 进行 一定数量（AZ_SUM_LENGHT） 的累加
    Pedometer_Accel_Sum_Differ = 0;
    for(loop = 0 ; loop <  AZ_SUM_LENGHT - 1 ; ++loop)  {
        
        az_buffer_sum[loop+1] = az_buffer_sum[loop];
        Pedometer_Accel_Sum_Differ += az_buffer_sum[loop+1];      
    }
    az_buffer_sum[0] = abs(az_old - IMU_Data.ax);
    Pedometer_Accel_Sum_Differ += az_buffer_sum[loop];

    az_old = IMU_Data.ax;        
}

void Hardware_IMU_Data_Print(void) {

    String output;

    output += String(millis()) + "," ;   

    if(IMU_Mode_Accel) {
        output += String(IMU_Data.ax) + "," + String(IMU_Data.ay) + "," +String(IMU_Data.az);  
    }

    if(IMU_Mode_Calulate) {
    output += String(Pedometer_Accel_Avg_Differ) + "," +String(Pedometer_Accel_Sum_Differ);
    }

    output += "\n";

    Hardware_BLE_UART_Print(output);

}

uint32_t  StopMove_Count = 0;
uint32_t Pedometer_Count = 0;
uint32_t old_Pedometer_Time;
uint32_t Pedometer_Time_Count = 0;
uint32_t Timer_Count_Nobody = 0;
uint32_t Timer_Count_MachineStop = 0;
bool Pedometer_Nobody_old;
bool Pedometer_MachineStop_old;
bool old_Pedometer_Rise;

bool Hardware_IMU_Pedometer(void) {
    bool output = false;
    bool refresh = false;
    String buffer;
    if(Pedometer_Count >= Pedometer_DBtime) {
        
        if(Pedometer_Accel_Sum_Differ > Pedometer_Trigger && Pedometer_Fall) {
            Pedometer_Rise = true;
            Pedometer_Fall = false;
            Pedometer_Count = 0;
            
        }
    }
    if(Pedometer_Accel_Sum_Differ < Pedometer_Trigger && Pedometer_Rise) {
        Pedometer_Fall = true;
        Pedometer_Rise = false;
        
    }    

    if(old_Pedometer_Rise != Pedometer_Rise && !Pedometer_Fall) {
        
        
        output = true;
        Timer_Count_Nobody = 0;
        Timer_Count_MachineStop = 0;
        
        if(Pedometer_MachineStop_old == Pedometer_MachineStop) {
            
            if(!Pedometer_MachineStop) {
                ++Pedometer;
                buffer = "Pedometer Count:" + String(Pedometer) 
                   + " Time:" + String(Pedometer_Time*10) + " ms";
                refresh = true;
            }
            else {
                buffer = "User just on Treadmill....";
                Pedometer = 0;
                refresh = true;
            }
        }
        
        if(Pedometer_Time_Count!=0 && Pedometer_Time_Count < PDMETER_TIME_MAX) {
            Pedometer_Time = Pedometer_Time_Count;
        }    
        Pedometer_Time_Count = 0;

        Pedometer_Nobody = false;
        Pedometer_MachineStop = false;
    }

    if(Pedometer_Time_Count < PDMETER_TIME_MAX)  ++Pedometer_Time_Count;
    

    if(Timer_Count_Nobody >= 500) {
        Pedometer_Nobody = true;
        if(Pedometer_Nobody_old != Pedometer_Nobody) {
            Timer_Count_MachineStop = 0;
            buffer = "No User on Treadmill....";
            refresh = true;
        }        
    }
    else ++Timer_Count_Nobody;
    
    if(Pedometer_Nobody) {
        
        if(Timer_Count_MachineStop >= 1000) {
            if(Timer_Count_MachineStop == 1000) {

                buffer = "Treadmill has been stop ---";
                Pedometer_MachineStop = true;
                refresh = true;
            }    
            ++Timer_Count_MachineStop; 
        } 
        else { 
            ++Timer_Count_MachineStop;
            if(Timer_Count_MachineStop % 100 == 0) {
                buffer = "Treadmill will be stop --- countdown：" + String(10-(Timer_Count_MachineStop/100)) + " Sec";
                refresh = true;
            }
        }    
    }

    if(Pedometer_Count <= Pedometer_DBtime) ++Pedometer_Count;
     
    old_Pedometer_Rise = Pedometer_Rise;
    Pedometer_Nobody_old = Pedometer_Nobody;
    Pedometer_MachineStop_old = Pedometer_MachineStop;

    if(IMU_Mode_Pedometer && refresh) Hardware_BLE_UART_Print(buffer);
    
    if(refresh) Serial.println(buffer);
    
    return output;
}

bool old_IMU_Enable;
void Hardware_IMU_Process(void) {

    if(old_IMU_Enable != IMU_Enable) Hardware_Timer_Reset(TIMER_BLE_IMU,IMU_Data.All_SampleRate);
    old_IMU_Enable = IMU_Enable;
     
    if(Hardware_Timer_Refresh(TIMER_BLE_IMU,IMU_Data.All_SampleRate)) {
                
        Hardware_IMU_Calculate();
        Hardware_IMU_Pedometer();  
        if(!IMU_Mode_Pedometer) Hardware_IMU_Data_Print();  
    }    

}

bool Hardware_IMU_setup(void) {
    bool output = true; 

    M5.imu.Init();
    
    M5.imu.SetAccelFsr(M5.imu.AFS_2G);
    IMU_Data.AccelFSR = 2;
    
    M5.imu.SetGyroFsr(M5.imu.GFS_250DPS);
    IMU_Data.AccelFSR = 250;

    IMU_Data.All_SampleRate = 10;

    return output;
}


IMU_DATA Hardware_Get_IMU(void) {

    return (IMU_Data);
}

IMU_DATA Hardware_Get_Sensor() {
    
    IMU_DATA imu_buf;

    M5.imu.getAccelData(&imu_buf.ax,&imu_buf.ay,&imu_buf.az);
    M5.imu.getGyroData(&imu_buf.gx,&imu_buf.gy,&imu_buf.gz);
    
    return imu_buf;
}

uint8_t Hardware_Get_WiFi_Status() {
    return WIFI_Status;
}

uint32_t Hardware_Get_ChipID() {
    for(int i=0; i<17; i=i+8) {
	  chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
	}
    return chipId;
}

bool Hardware_WiFi_Setup() {
    bool output = true;
    
    
    AP_Hostname = APHOSTNAME + String(chipId%1000);
    Serial.print("AP HOSTNAME: "); Serial.println(AP_Hostname);
    
    
    /* 
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(AP_Hostname.c_str());
    Serial.print("AP IP address: ");
    Serial.println(WiFi.softAPIP());
    WiFi.begin(ssid, password);


    以下为Http Update sever 代码
    if (MDNS.begin(AP_Hostname.c_str())) {
        Serial.println("mDNS responder started");
    }
    
    

    httpUpdater.setup(&httpServer);
    httpServer.begin();

    MDNS.addService("http", "tcp", 80);
    Serial.printf("HTTPUpdateServer ready! Open http://%s.local/update in your browser\n", AP_Hostname.c_str());
    */

    // 以下为 Arduino OTA 代码
    
    ArduinoOTA.setHostname(AP_Hostname.c_str());

    Serial.print("OTA HOSTNAME: "); Serial.println(AP_Hostname);

    ArduinoOTA.onStart([]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH)
            type = "sketch";
        else // U_SPIFFS
            type = "filesystem";

        // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
        Serial.println("Start updating " + type);
        otaStart = true;
        M5.lcd.fillScreen(TFT_BLACK);
        M5.lcd.drawString("Updating...", M5.lcd.width() / 2 - 20, 55 );
    })
    .onEnd([]() {
        Serial.println("\nEnd");
        delay(500);
    })
    .onProgress([](unsigned int progress, unsigned int total) {
        // Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
        int percentage = (progress / (total / 100));
        M5.lcd.setTextDatum(TC_DATUM);
        M5.lcd.setTextPadding(M5.lcd.textWidth(" 888% "));
        M5.lcd.drawString(String(percentage) + "%", 145, 35);
        drawProgressBar(10, 30, 120, 15, percentage, TFT_WHITE, TFT_BLUE);
    })
    .onError([](ota_error_t error) {
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
        else if (error == OTA_END_ERROR) Serial.println("End Failed");

        M5.lcd.fillScreen(TFT_BLACK);
        M5.lcd.drawString("Update Failed", M5.lcd.width() / 2 - 20, 55 );
        delay(3000);
        otaStart = false;
        M5.lcd.fillScreen(TFT_BLACK);
        M5.lcd.setTextDatum(TL_DATUM);
    });

    // ArduinoOTA.begin();
    
    return output;
}

void drawProgressBar(uint16_t x0, uint16_t y0, uint16_t w, uint16_t h, uint8_t percentage, uint16_t frameColor, uint16_t barColor)
{
    if (percentage == 0) {
        M5.lcd.fillRoundRect(x0, y0, w, h, 3, TFT_BLACK);
    }
    uint8_t margin = 2;
    uint16_t barHeight = h - 2 * margin;
    uint16_t barWidth = w - 2 * margin;
    M5.lcd.drawRoundRect(x0, y0, w, h, 3, frameColor);
    M5.lcd.fillRect(x0 + margin, y0 + margin, barWidth * percentage / 100.0, barHeight, barColor);
}

bool WIFI_old;

uint8_t Hardware_WiFi_Process() {
    
    if(WIFI) ArduinoOTA.handle();

    if(otaStart) return WIFI_Status;
    
    if(WIFI) httpServer.handleClient();

    if(WIFI_Status == WIFI_STATUS_DISCONNECT)  WIFI_Status = WIFI_STATUS_DISCONNECTED;

    if(WIFI != WIFI_old) {
        
        if(WIFI == false)  {             
            WiFi.mode(WIFI_OFF); 
            WIFI_Status = WIFI_STATUS_OFF;
            Serial.println("WiFi OFF");
        }  
        else {                           
            //WiFi 开启 AP＋STA模式开始进行STA连接
            WIFI_Status = WIFI_STATUS_CONNECTING;
            WiFi.mode(WIFI_AP_STA);
            WiFi.softAP(AP_Hostname.c_str());
            Serial.print("AP HOSTNAME: "); Serial.println(AP_Hostname);
            Serial.print("AP IP address: ");
            Serial.println(WiFi.softAPIP());
            WiFi.begin(WiFi_SSID.c_str(), WiFi_Password.c_str());    
            
            //WiFi 开启 mDNS域名
            if (MDNS.begin(AP_Hostname.c_str())) {
                Serial.println("WiFi ON  & mDNS started");
            }
            
            httpUpdater.setup(&httpServer);

            httpServer.begin();

            MDNS.addService("http", "tcp", 80);
            Serial.printf("HTTPUpdateServer ready! Open http://%s.local/update in your browser\n", AP_Hostname.c_str());
            

            //WiFi STA模式连接尝试，若失败会尝试3次共3秒
            Serial.println("STA WiFi Connecting...by SSID:" + WiFi_SSID + "; Password:" + WiFi_Password);
            uint8_t count = 0;
            while (WiFi.waitForConnectResult() != WL_CONNECTED) {
                Serial.println("STA WiFi failed, retrying - " + String(count) + " times");
                if(count >= WIFI_FAIL_OVERTIME)  {
                    WiFi.disconnect(false,false);
                    Serial.println("STA WiFi disconnected and WiFi OFF");
                    WIFI_Status = WIFI_STATUS_DISCONNECT;
                    break;
                }
                else count++;    
            }   
            // WiFi STA模式连接成功，并串口打印IP地址
            if(WIFI_Status != WIFI_STATUS_DISCONNECT) {
                Serial.println("STA connected");
                Serial.print("STA IP address: ");
                Serial.println(WiFi.localIP());
                WIFI_Status = WIFI_STATUS_CONNECTED;
            }    
            ArduinoOTA.begin();
        }
    } 
    WIFI_old = WIFI;
    return  WIFI_Status;
}

String Hardware_Get_WiFi_IP() {

    IPAddress local = WiFi.localIP();
    return String(local.toString());
}

String Hardware_Get_WiFi_Hostname() {

 return AP_Hostname;
}
String Hardware_Get_BLE_Hostname() {

 return BT_Hostname;
}

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
        std::string rxValue = pCharacteristic->getValue();
        
      
        if (rxValue.length() > 0) {
            Serial.print("BLE UART Received: ");
            Serial.println(rxValue.c_str());
            BLE_UART_RxData = rxValue.c_str();
            BLE_UART_RxCallback = true;
        }

    }
};


bool Hardware_BLE_Setup() {

    bool output = true;

    BT_Hostname = BLEHOSTNAME + String(chipId%1000);
     
    // Create the BLE Device
    BLEDevice::init(BT_Hostname.c_str());

    // Create the BLE Server
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    // Create the BLE Service
    BLEService *pService = pServer->createService(SERVICE_UUID);

    // Create a BLE Characteristic
    pTxCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID_TX,
    BLECharacteristic::PROPERTY_NOTIFY
    );
                      
    pTxCharacteristic->addDescriptor(new BLE2902());

    BLECharacteristic * pRxCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID_RX,
	BLECharacteristic::PROPERTY_WRITE
	);

    pRxCharacteristic->setCallbacks(new MyCallbacks());
    
    // Start the service
    pService->start();

    // Start advertising
    pServer->getAdvertising()->start();


    Serial.println("Waiting a client connection to notify...");

    return output;
}

bool BLE_old;

void Hardware_BLE_Process() {
    
    if(BLE != BLE_old) {
        
        if(BLE)  {   
            Serial.println("BLE ON");         
        }  
        else {                           
            BLE_Status = BLE_STATUS_OFF;
            Serial.println("BLE OFF");
        }   
    }

    if(BLE) {
        if (deviceConnected) {

	    }
        // disconnecting
        if (!deviceConnected && oldDeviceConnected) {
            pServer->startAdvertising(); // restart advertising
            Serial.println("BLE start advertising");
            oldDeviceConnected = deviceConnected;
            BLE_Status = BLE_STATUS_ADVERTISING;
        }

        // connecting
        if (deviceConnected && !oldDeviceConnected) {
		    oldDeviceConnected = deviceConnected;
            Serial.println("BLE Connected");
            BLE_Status = BLE_STATUS_CONNECTED;
        }
            
    }
    BLE_old = BLE;
}


void Hardware_BLE_UART_Print(String data) {

    data += "\n";
    if (deviceConnected) {
        pTxCharacteristic->setValue(data.c_str());
        pTxCharacteristic->notify();
    }

}

uint8_t Hardware_Get_BLE_Status() {
    
    return BLE_Status;
}

bool Hardware_Toggle_BLE() {
    if(BLE)  {
        BLE = false;
        BLE_Status = BLE_STATUS_OFF;
        //btStop();
    } else {
        BLE = true;
        //btStart();
    }    
    return BLE;
}


bool Hardware_IMU_Get_Enabled(void) {

    return IMU_Enable;
}

void Hardware_IMU_Set_Enabled(bool enabled)
{

    M5.imu.SetEnable(enabled);
    IMU_Enable = enabled;

}

void Hardware_IMU_Reset(void) {

    M5.imu.Init();

}


void Hardware_Set_SystemSleep(void) {

    M5.axp.PowerOff();
}

uint8_t Hardware_BLE_ATCommand_Decode(void) {
    
    uint8_t output = ATCMD_NA; 
    String RX_Data,RX_Index;
    uint16_t Year;
    uint8_t Month, Day, Hour, Minute, Second;
    int Data_Length;
    
    if(!BLE_UART_RxCallback) return output;

    BLE_UART_RxCallback = false;
    RX_Data = BLE_UART_RxData;

    // RX_Data.toUpperCase();
    if(RX_Data.startsWith("AT+"))  {
        
        RX_Data = RX_Data.substring(3);
        RX_Index = RX_Data.substring(0,RX_Data.indexOf("#"));

        if(RX_Index.equals(ATCMD_STR_RESET)) {

            Serial.println("System to Reset...");
            esp_restart();        
        }

        if(RX_Index.equals(ATCMD_STR_SLEEP)) {

            Hardware_Set_SystemSleep();   
        }

        if(RX_Index.equals(ATCMD_STR_IMU_ON)) {
            Hardware_IMU_Set_Enabled(true);
            output = ATCMD_IMU_ON;
            Serial.println("AT+IMU-ON#");
        }

        if(RX_Index.equals(ATCMD_STR_IMU_OFF)) {
            Hardware_IMU_Set_Enabled(false);
            output = ATCMD_IMU_OFF;
            Serial.println("AT+IMU-OFF#");
        }

        if(RX_Index.equals(ATCMD_STR_IMU_MODE_ALL)) {
            output = ATCMD_IMU_MODE_ALL;
            IMU_Mode_Accel = true;
            IMU_Mode_Calulate = true;
            IMU_Mode_Pedometer = false;
            Serial.println("AT+IMU-MODE-ALL#");
        }
        
        if(RX_Index.equals(ATCMD_STR_IMU_MODE_3D)) {
            output = ATCMD_IMU_MODE_3D;
            IMU_Mode_Accel = true;
            IMU_Mode_Calulate = false;
            IMU_Mode_Pedometer = false;
            Serial.println("AT+IMU-MODE-ACCEL#");
        }
        
        if(RX_Index.equals(ATCMD_STR_IMU_MODE_CALU)) {
            output = ATCMD_IMU_MODE_CALU;
            IMU_Mode_Accel = false;
            IMU_Mode_Calulate = true;
            IMU_Mode_Pedometer = false;
            Serial.println("AT+IMU-MODE-CALU#");
        }

        if(RX_Index.equals(ATCMD_STR_IMU_MODE_PDMETER)) {
            output = ATCMD_IMU_MODE_PDMETER;
            IMU_Mode_Accel = false;
            IMU_Mode_Calulate = false;
            IMU_Mode_Pedometer = true;
            Serial.println("AT+IMU-MODE-PDMETER#");
        }

        if(RX_Index.equals(ATCMD_STR_SYS_DISPLAY_OFF_TIME)) {
            //清除 命令 字符在 RX_Data里，便于解析 数值
            RX_Data = RX_Data.substring(RX_Index.length()+1);

            Data_Length = RX_Data.indexOf("#");
            if(Data_Length == -1)  return ATCMD_ERROR;
            RX_Index = RX_Data.substring(0,Data_Length);
            
            TouchButton_DisplayOff_Overtime_Set(RX_Index.toInt());
            Serial.println("AT+DISPLAY-OFF-TIME#" + RX_Index + "#(Seconds)");
            output = ATCMD_SYS_DISPLAY_OFF_TIME;
        }    

        if(RX_Index.equals(ATCMD_STR_SYS_SLEEP_TIME)) {
            //清除 命令 字符在 RX_Data里，便于解析 数值
            RX_Data = RX_Data.substring(RX_Index.length()+1);

            Data_Length = RX_Data.indexOf("#");
            if(Data_Length == -1)  return ATCMD_ERROR;
            RX_Index = RX_Data.substring(0,Data_Length);
            
            TouchButton_Sleep_Overtime_Set(RX_Index.toInt());
            Serial.println("AT+SLEEP-TIME#" + RX_Index + "#(Minutes)");
            output = ATCMD_SYS_SLEEP_TIME;
        }   

        if(RX_Index.equals(ATCMD_STR_WIFI_ON)) {
            WIFI = true;
            output = ATCMD_WIFI_ON;
            Serial.println("AT+WIFI-ON#");
        }

        if(RX_Index.equals(ATCMD_STR_WIFI_OFF)) {
            WIFI = false;
            WIFI_Status = WIFI_STATUS_OFF;
            output = ATCMD_WIFI_OFF;
            Serial.println("AT+WIFI-OFF#");
        }

        if(RX_Index.equals(ATCMD_STR_WIFI_ID)) {
            
            //清除 "DATE" 字符在 RX_Data里，便于解析 数值
            RX_Data = RX_Data.substring(RX_Index.length()+1);

            // WiFi SSID
            Data_Length = RX_Data.indexOf("#");
            if(Data_Length == -1)  return ATCMD_ERROR;
            RX_Index = RX_Data.substring(0,Data_Length);
            WiFi_SSID = RX_Index;
            Serial.println("AT+WIFI-ID#" + RX_Index + "#");
            if(WIFI) {
                WIFI = false;
                WIFI_Status = WIFI_STATUS_OFF;
            }    
            output = ATCMD_WIFI_ID;
        }    

        if(RX_Index.equals(ATCMD_STR_WIFI_PW)) {
            
            //清除 "DATE" 字符在 RX_Data里，便于解析 数值
            RX_Data = RX_Data.substring(RX_Index.length()+1);

            // WiFi Password
            Data_Length = RX_Data.indexOf("#");
            if(Data_Length == -1)  return ATCMD_ERROR;
            RX_Index = RX_Data.substring(0,Data_Length);
            WiFi_Password = RX_Index;
            Serial.println("AT+WIFI-PW#" + RX_Index + "#");
            if(WIFI) {
                WIFI = false;
                WIFI_Status = WIFI_STATUS_OFF;
            }    
            output = ATCMD_WIFI_PD;
        }   

        if(RX_Index.equals(ATCMD_STR_DATE)) {
            
            //清除 "DATE" 字符在 RX_Data里，便于解析 数值
            RX_Data = RX_Data.substring(RX_Index.length()+1);

            // Year data
            Data_Length = RX_Data.indexOf(".");
            if(Data_Length == -1)  return ATCMD_ERROR;
            RX_Index = RX_Data.substring(0,Data_Length);
            Year = RX_Index.toInt();
            RX_Data = RX_Data.substring(RX_Index.length()+1);

            // Month data
            Data_Length = RX_Data.indexOf(".");
            if(Data_Length == -1)  return ATCMD_ERROR;
            RX_Index = RX_Data.substring(0,Data_Length);
            Month = RX_Index.toInt();
            RX_Data = RX_Data.substring(RX_Index.length()+1);

            // Day data
            Data_Length = RX_Data.indexOf(".");
            if(Data_Length == -1)  return ATCMD_ERROR;
            RX_Index = RX_Data.substring(0,Data_Length);
            Day = RX_Index.toInt();
            RX_Data = RX_Data.substring(RX_Index.length()+1);

            // Hour data
            Data_Length = RX_Data.indexOf(".");
            if(Data_Length == -1)  return ATCMD_ERROR;
            RX_Index = RX_Data.substring(0,Data_Length);
            Hour = RX_Index.toInt();
            RX_Data = RX_Data.substring(RX_Index.length()+1);

            // Minute data
            Data_Length = RX_Data.indexOf(".");
            if(Data_Length == -1)  return ATCMD_ERROR;
            RX_Index = RX_Data.substring(0,Data_Length);
            Minute = RX_Index.toInt();
            RX_Data = RX_Data.substring(RX_Index.length()+1);

            // Second data
            Data_Length = RX_Data.indexOf("#");
            if(Data_Length == -1)  return ATCMD_ERROR;
            RX_Index = RX_Data.substring(0,Data_Length);
            Second = RX_Index.toInt();

            rtc.setDateTime(Year, Month, Day, Hour, Minute, Second);
            
            Serial.println("AT+DATE#" + String(Year) + "."
                                                    + String(Month) + "."
                                                    + String(Day) + "."
                                                    + String(Hour) + "."
                                                    + String(Minute) + "."
                                                    + String(Second) + "#");
            output = ATCMD_DATE;

        }

    }      

    return output;
}








