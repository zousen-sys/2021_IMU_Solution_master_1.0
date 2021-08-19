#ifndef HARDWARE_H
#define HARDWARE_H
#include <Arduino.h>
#include <pcf8563.h>
#include "M5StickCPlus.h"

class IMU_DATA
{
	public:

	float ax, ay, az;
	float gx, gy, gz;
	float mx, my, mz;
	float qw, qx, qy, qz;
	long temperature;
	unsigned long time;
	float pitch, roll, yaw;
	float heading;
	uint16_t AccelFSR ;
	uint16_t GyroFSR ;
	uint16_t AG_LPF;
	uint16_t All_SampleRate;
} ;

void Hardware_Setup();
bool Hardware_Timer_Refresh(uint8_t channel, uint32_t ms);
void Hardware_Timer_Reset(uint8_t channel, uint32_t ms);

uint8_t Hardware_Get_TFT_Backlit();
void Hardware_Set_TFT_Backlit(uint8_t brightness);
bool Hardware_Get_ChargeStatu();
bool Hardware_Get_ChargeIndication();

bool Hardware_Get_WIFI();
bool Hardware_Get_BLE();

bool Hardware_Toggle_WIFI();
bool Hardware_Toggle_BLE();

String Hardware_Get_Battery_Percent();

String Hardware_Get_VBUS_Voltage();

bool Hardware_RTC_Setup(void);
RTC_Date Hardware_Get_RTC(void);
bool Hardware_UpdateRTC_FromNTP();



IMU_DATA Hardware_Get_IMU(void);

bool Hardware_WiFi_Setup();
uint8_t Hardware_Get_WiFi_Status();
uint8_t Hardware_WiFi_Process();
String Hardware_Get_WiFi_IP();
String Hardware_Get_WiFi_Hostname();

bool Hardware_BLE_Setup();
void Hardware_BLE_Process();
uint8_t Hardware_Get_BLE_Status();
void Hardware_BLE_UART_Print(String data);

uint16_t Hardware_Get_Sensor_RefreshTime(); 
void Hardware_Set_Sensor_RefreshTime(uint16_t delaytime);
uint32_t Hardware_Get_ChipID();
String Hardware_Get_BLE_Hostname();
uint8_t Hardware_BLE_ATCommand_Decode(void);

bool Hardware_IMU_setup(void);
void Hardware_IMU_Set_Enabled(bool enabled);
bool Hardware_IMU_Get_Enabled(void) ;
void Hardware_IMU_Reset(void);
void Hardware_IMU_Process(void);
bool Hardware_Check_IMU(void) ;
bool Hardware_IMU_Pedometer(void);


uint8_t Hardware_Get_HR();
void Hardware_HR_Process();
void Hardware_HR_Set_HR_Enable(bool enable);
bool Hardware_HR_Set_HR_Enable(void);

void Hardware_Set_SystemSleep(void);

//Hardware IO Setup
// #define TP_PIN_PIN       33  by TouchButton.h to define
#define I2C_SDA_PIN         21
#define I2C_SCL_PIN         22

#define TP_PIN_PIN          37

#define LED_PIN             10


//TFT Backlit control
#define TFT_BACKLIT_ON 101
#define TFT_BACKLIT_OFF 0

#define TIMER_ONE           0
#define TIMER_TWO           1
#define TIMER_THREE         2
#define TIMER_FOUR          3
#define TIMER_FIVE          4
#define TIMER_BLE_IMU		8
#define TIMER_HTTPUPDATE    9

#define MESSAGE_DELAY_TIME 2000

#define IMU_STOPMOVE_OVERTIME 5     // 单位分钟   
#define IMU_STOPMOVE_DEFFERENCE 400  // 单位 mg (0.001g)zyz加速传感的移动差值

//WiFi
#ifndef STASSID
#define STASSID "JHT-Innovation-Shanghai"
#define STAPSK  "Lin19740224"
#define HOSTNAME "esp32-"
#define APHOSTNAME "esp32-"
#endif

#define WIFI_STATUS_OFF             0
#define WIFI_STATUS_DISCONNECT      1
#define WIFI_STATUS_DISCONNECTED    2
#define WIFI_STATUS_CONNECTING      3
#define WIFI_STATUS_CONNECTED       4

#define WIFI_FAIL_OVERTIME  1 // WiFi重连次数，每次约2秒

#ifndef NTP_SERVER
#define NTP_SERVER              "ntp.aliyun.com"
#define NTP_SERVER1             "cn.ntp.org.cn"
#define NTP_SERVER2             "pool.ntp.org"
#define NTP_GMT_OFFSET          8*60*60  // China GMT = 8
#define NTP_DAYLIGHT_OFFSET     0
#endif

// BLE
#define BLEHOSTNAME            "ESP-BLE-"
#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UART service UUID
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

#define BLE_STATUS_OFF             0
#define BLE_STATUS_ADVERTISING     1
#define BLE_STATUS_CONNECTING      2
#define BLE_STATUS_CONNECTED       3

#define PDMETER_TIME_MAX		100 //  計步最大間距時間 100x 10ms = 1sec

#endif
