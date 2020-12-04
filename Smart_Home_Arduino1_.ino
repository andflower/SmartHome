#include <SoftwareSerial.h>                                 //LCD Sensor library
#include <DHT.h>                                            //tinyDHT sensor library
#include <Adafruit_Sensor.h>                                //tinyDHT sensor library
#include <DHT_U.h>                                          //tinyDHT sensor library
#include <Wire.h>                                           //I2C Communication(DHT)

#define led_Y               A0                              //평상시(Safe)
#define led_R               A1                              //위험경고시(Warning)
#define buzzer1             A2                              //부저1(경고)_거실 ACTIVE형식
#define buzzer2             A3                              //부저2(경고)_침실 ACTIVE형식
#define pir_Sensor1         12                              //인체감지센서1_베란다
#define pir_Sensor2         13                              //인체감지센서2_현관
#define ir1                 2                               //적외선 인체감지센서1_베란다
#define ir2                 3                               //적외선 인체감지센서2_베란다
#define ir3                 4                               //적외선 인체감지센서3_베란다
#define ir4                 5                               //적외선 인체감지센서4_베란다
#define blueRx              6                               //RX(HC-06), TX(Arduino)
#define blueTx              7                               //TX(HC-06), RX(Arduino)
#define DHTPIN              8                               //온도, 습도 센서
#define DHTTYPE             DHT11                           //DHT타입 DHT11로 정의

SoftwareSerial BT(blueTx, blueRx);                          //시리얼 통신을 위한 객체선언

String ReciveBTString;                                      //블루투스 통신 문자열 변수
String ReciveSRString;                                      //시리얼 통신 문자열 변수
char ReciveBT;                                              //블루투스 통신 수신데이터 문자 변수
char ReciveSR;                                              //시리얼 통신 수신데이터 문자 변수
String inString1;                                           //안드로이드 시간 값 변수

DHT_Unified dht(DHTPIN, DHTTYPE);                           //DHT설정 - dht (디지털8, dht11)
sensors_event_t event;                                      //DHT 이벤트주소 변수 선언

boolean pir_val1 = 0;                                       //인체감지센서1 변수
boolean pir_val2 = 0;                                       //인체감지센서2 변수
boolean ir_val1 = 0;                                        //적외선 인체감지센서1 변수
boolean ir_val2 = 0;                                        //적외선 인체감지센서2 변수
boolean ir_val3 = 0;                                        //적외선 인체감지센서3 변수
boolean ir_val4 = 0;                                        //적외선 인체감지센서4 변수
String state = "방범제어(기본모드)";

volatile int hours;                                         //시간 변수
int startHourTime = 19;                                     //방범모드 시간 시간
int endHourTime = 7;                                        //방범모드 종료 시간
volatile boolean outingMode = false;                        //외출모드 활성화 여부
unsigned long previousTime = 0;                             //멀티태스킹 전체 이전시간
unsigned long currentTime = 0;                              //멀티태스킹 전체 현재시간
const long delayTime = 1000;                                //멀티태스킹 전체 딜레이
unsigned long buzzerPreviousTime = 0;                       //멀티태스킹 부저 이전시간
unsigned long buzzerCurrentTime = 0;                        //멀티태스킹 부저 현재시간
unsigned long buzzerdelayTime = 300;                        //멀티태스킹 부저 딜레이
const byte buzzerCount = 3;                                 //부저활성화 카운터
volatile boolean buzzerState = false;                       //부저활성화 여부
float temp = 0.0;                                           //온도 값 저장
float humi = 0.0;                                           //습도 값 저장

void setup()
{
    Wire.begin();                                           //I2C통신 활성화
    dht.begin();                                            //DHT센서 활성화
    sensor_t sensor;                                        //DHT센서 sensor Structure 활성화

    Serial.begin(9600);                                     //시리얼 통신 활성화
    BT.begin(9600);                                         //블루투스 통신 활성화

    //온도센서 정보 Serial Print
    dht.temperature().getSensor(&sensor);

    //습도센서 정보 Serial Print
    dht.humidity().getSensor(&sensor);

    //센서 Setup
    pinMode(pir_Sensor1, INPUT);                            //PIR1 Sensor Setup
    pinMode(pir_Sensor2, INPUT);                            //PIR2 Sensor Setup
    pinMode(ir1, INPUT);                                    //ir1 Sensor Setup
    pinMode(ir2, INPUT);                                    //ir2 Sensor Setup
    pinMode(ir3, INPUT);                                    //ir3 Sensor Setup
    pinMode(ir4, INPUT);                                    //ir4 Sensor Setup

    //알림 Setup
    pinMode(led_Y, OUTPUT);                                 //LED YELLOW Setup
    pinMode(led_R, OUTPUT);                                 //LED RED Setup
    pinMode(buzzer1, OUTPUT);                               //Buzzer1 Setup 
    pinMode(buzzer2, OUTPUT);                               //buzeer2 Setup
}

void loop()
{
    dhtRead();

    warning_Sensor(buzzerCount, buzzerdelayTime);

    bluetooth_Send_Recive();
}

//현재상태 블루투스 통신
void stateBTPrint()
{
    BT.print(state);
    BT.print(",");
    dht_BTPrint();
}

//Assign to variable temp & humi
void dhtRead()
{
    dht.temperature().getEvent(&event);                     //DHT 이벤트주소 온도 변수에 할당
    temp = event.temperature;
    dht.humidity().getEvent(&event);                        //DHT 이벤트주소 습도 변수에 할당
    humi = event.relative_humidity;
}

//DHT Sensor Bluetooth Print
void dht_BTPrint()
{
    BT.print(temp);                                         //이벤트 구조체 온도값을 블루투스로 전달
    BT.print(",");
    BT.println(humi);                                       //이벤트 구조체 습도값을 블루투스로 전달
}

//방법상태 센서 및 경고음
void warning_Sensor(int count, int warning_Intalval)
{
    pir_val1 = digitalRead(pir_Sensor1);
    pir_val2 = digitalRead(pir_Sensor2);
    ir_val1 = digitalRead(ir1);
    ir_val2 = digitalRead(ir2);
    ir_val3 = digitalRead(ir3);
    ir_val4 = digitalRead(ir4);

    currentTime = millis();

    if ((hours < endHourTime && hours >= startHourTime || outingMode == true) && currentTime - previousTime >= delayTime)
    {
        if (pir_val1 == 1 || pir_val2 == 1 || ir_val1 == 1 || ir_val2 == 1 || ir_val3 == 1 || ir_val4 == 1)
        {
            digitalWrite(led_R, HIGH);
            digitalWrite(led_Y, LOW);
            state = "Warning";
            stateBTPrint();
            Serial.println(state);
            buzzerState = true;
            buzzerActive(count, warning_Intalval);          //위험경고상태 알림음(부저) 횟수
        }

        else
        {
            digitalWrite(led_R, LOW);
            digitalWrite(led_Y, HIGH);
            digitalWrite(buzzer1, LOW);
            digitalWrite(buzzer2, LOW);
            state = "Safe";
            stateBTPrint();
        }
        previousTime = currentTime;
    }

    else if (currentTime - previousTime >= delayTime)
    {
        stateBTPrint();
        previousTime = currentTime;
    }
}

//At Warning Buzzer Active
void buzzerActive(int count, int warning_Intalval)
{
    buzzerCurrentTime = millis();

    if ((buzzerState == true) && (buzzerCurrentTime - buzzerPreviousTime >= buzzerdelayTime))
    {
        digitalWrite(buzzer1, HIGH);
        digitalWrite(buzzer2, HIGH);
        buzzerState = false;
        buzzerPreviousTime = buzzerCurrentTime;
    }

    else if ((buzzerState == false) && buzzerCurrentTime - buzzerPreviousTime >= buzzerdelayTime)
    {
        digitalWrite(buzzer1, LOW);
        digitalWrite(buzzer2, LOW);
        buzzerState = true;
        buzzerPreviousTime = buzzerCurrentTime;
    }
}

//블루투스, 시리얼 통신(수신, 송신)
void bluetooth_Send_Recive()
{
    while (BT.available())                                  //블루투스 수신데이터가 있는동안
    {
        ReciveBTString = BT.readStringUntil('\n');
        delay(5);                                           //수신 문자열 끊김 방지
    }
    while (Serial.available()) {                            //시리얼 수신데이터가 있는동안
        ReciveSRString = Serial.readStringUntil('\n');
        delay(5);                                           //수신 문자열 끊김 방지 
    }
    if (!ReciveBTString.equals(""))                         //ReciveBTString 값이 있다면
    {
        int index1 = ReciveBTString.indexOf(')');
        int index = ReciveBTString.length();
        inString1 = ReciveBTString.substring(0, index1 + 1);
        String inString2 = ReciveBTString.substring(index1 + 1, index - 1);
        String inString3 = ReciveBTString.substring(12, 14);

        char inString4[] = { 0 };

        hours = inString3.toInt();

        inString2.toCharArray(inString4, index);

        if (strcmp(inString4, "cpca") == 0)
        {
            outingMode = false;
            digitalWrite(led_Y, LOW);
            digitalWrite(led_R, LOW);
            digitalWrite(buzzer1, LOW);
            digitalWrite(buzzer2, LOW);
            state = "방범제어(기본모드)";
            Serial.print(inString1);
            Serial.println("방범제어 - 기본모드");
        }

        if (strcmp(inString4, "cpcm") == 0)
        {
            outingMode = true;
            Serial.print(inString1);
            Serial.println("방범제어 - 외출모드");
        }

        if (strcmp(inString4, "Time") == 0)
        {
            ReciveBTString = "";                            //초기화
        }

        else
        {
            if (strcmp(inString4, "ledhigh") == 0) { state = "LED ON"; }
            if (strcmp(inString4, "ledlow") == 0) { state = "LED OFF"; }
            if (strcmp(inString4, "opencurtains") == 0) { state = "커튼 OPEN"; }
            if (strcmp(inString4, "closecurtains") == 0) { state = "커튼 CLOSE"; }
            if (strcmp(inString4, "curtainsauto") == 0) { state = "커튼자동화"; }
            Serial.print(inString1);
            Serial.println(inString4);
        }
        ReciveBTString = "";                                //초기화
    }
    if (!ReciveSRString.equals(""))                         //ReciveSRString 값이 있다면
    {
        Serial.print(ReciveSRString);
        BT.print(ReciveSRString);

        ReciveSRString = "";                                //초기화
    }
}