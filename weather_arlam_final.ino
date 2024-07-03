#include "RTClib.h"
#include<Wire.h>
#include <WiFiEsp.h>
#include <WiFiEspClient.h>
#include <WiFiEspServer.h>
#include <WiFiEspUdp.h>
#include <SoftwareSerial.h> 
#define rxPin 4 //와이파이모듈 rx핀 선언
#define txPin 5 //와이파이모듈 tx핀 선언
SoftwareSerial esp01(txPin, rxPin); //rx, tx 지정

char ssid[] = "jindo";    // 와이파이 이름
char pass[] = "hyunuk0927";  // 와이파이 비밀번호 


int status = WL_IDLE_STATUS;        // the Wifi radio's status
RTC_DS1307 RTC;                     // RTC객체선언


const char* host = "apis.data.go.kr"; //사이트를 문자열 상수로 선언
const char KOR_HEAD[] PROGMEM       = "http://apis.data.go.kr/1360000/VilageFcstInfoService_2.0/";  //단기예보 조회를 활용하기 위한 서비스 URL
const char KOR_GUSN[] PROGMEM       = "getUltraSrtNcst"; // 초단기실황조회
const char KOR_GUSF[] PROGMEM       = "getUltraSrtFcst"; // 초단기예보조회
const char KOR_GVLF[] PROGMEM       = "getVilageFcst";   // 단기예보조회
const char KOR_ADD0[] PROGMEM       = "?serviceKey="; 
const char KOR_KEY[] PROGMEM        = "WCEuUQwLYVBVcJC2GvSdWu7EZcHqJ9NEEJtyDxptZ6rvNgB92di8%2FZ3VJU3VoJsk8s6j83lxEfza1xaRH17pcg%3D%3D"; //인증키
const char KOR_ADD1[] PROGMEM       = "&numOfRows=";
const char KOR_ADD2[] PROGMEM       = "&pageNo=";
const char KOR_ADD3[] PROGMEM       = "&dataType=JSON&base_date=";
const char KOR_ADD4[] PROGMEM       = "&base_time=";
const char KOR_ADD5[] PROGMEM       = "&nx="; //위도
const char KOR_ADD6[] PROGMEM       = "&ny="; //경도

int sun = 0;  //맑음 변수 선언
int rain = 0; //비 변수 선언
int rns = 0;  //눈비 변수 선언
int snow = 0; //눈 변수 선언

/* 데이터 전송 형식: http://apis.data.go.kr/1360000/VilageFcstInfoService_2.0/{ 요청 서비스 }?serviceKey={ 인증키 }&numOfRows={ 행 개수 }&pageNo={ 페이지 번호 }
({ 데이터 타입 })&base_date={ 날짜 }&base_time={ 예보 시간 }&nx={ 위도 격자값 }&ny={ 경도 격자값 }*/
int yellow = 9; //맑음 led 9번 핀 지정
int blue = 6; //비 led 6번 핀 지정
int white = 8;  //눈 led 8번 핀 지정
int pir = 7;  //근접센서 7번 핀 지정
int sw = 10;  //스위치 10번 핀 지정
int sensor = 0; //근접센서 데이터 저장 변수 선언

WiFiEspClient client; // WiFiEspClient 객체 선언

void setup() {
  Serial.begin(9600);  //시리얼모니터
  Wire.begin(); 
  RTC.begin();  //RTC 세팅
  esp01.begin(9600);  //와이파이 시리얼
  delay(1000);  //딜레이 1초
  WiFi.init(&esp01);  // initialize ESP module
  pinMode(yellow,OUTPUT); //맑음 led 출력설정
  pinMode(blue,OUTPUT); //비 led 출력 설정
  pinMode(white,OUTPUT);  //눈 led 출력설정
  pinMode(pir,INPUT); //근접센서 입력설정
  pinMode(sw, INPUT_PULLUP);

  while ( status != WL_CONNECTED) { // 와이파이에 연결되지 않았을 시
    Serial.print(F("해당 WPA SSID에 연결 시도중: "));
    Serial.println(ssid); //와이파이명 출력
    status = WiFi.begin(ssid, pass);  // 와이파이에 연결시도
  }

  if (status == WL_CONNECTED) Serial.println(F("네트워크에 연결됨")); //와이파이 연결성공 시 문구 프린트
  Serial.println();
}

#define PI 3.1415926535897932384626433832795  //원주율 선언

int nx, ny; //격자값 저장 변수

//위도 , 경도를 입력받아 격자 값으로 변환하는 함수
void getGride(float lat, float lon ) {
  float ra;
  float theta;
  float RE = 6371.00877;   // 지구 반경(km)
  float GRID = 5.0;        // 격자 간격(km)
  float SLAT1 = 30.0;      // 투영 위도1(degree)
  float SLAT2 = 60.0;      // 투영 위도2(degree)
  float OLON = 126.0;      // 기준점 경도(degree)
  float OLAT = 38.0;       // 기준점 위도(degree)
  float XO = 43;           // 기준점 X좌표(GRID)
  float YO = 136;          // 기1준점 Y좌표(GRID)
  float DEGRAD = PI / 180.0;
  float RADDEG = 180.0 / PI;
  float re = RE / GRID;
  float slat1 = SLAT1 * DEGRAD;
  float slat2 = SLAT2 * DEGRAD;
  float olon = OLON  * DEGRAD;
  float olat = OLAT  * DEGRAD;
  float sn = tan( PI * 0.25f + slat2 * 0.5f ) / tan( PI * 0.25f + slat1 * 0.5f );
  sn = log(cos(slat1) / cos(slat2)) / log(sn);
  float sf = tan(PI * 0.25f + slat1 * 0.5f);
  sf = pow(sf, sn) * cos(slat1) / sn;
  float ro = tan(PI * 0.25f + olat * 0.5f);
  ro = re * sf / pow(ro, sn);
  ra = tan(PI * 0.25f + (lat) * DEGRAD * 0.5f);
  ra = re * sf / pow(ra, sn);
  theta = lon * DEGRAD - olon;
  if(theta > PI) theta -= 2.0f * PI;
  if(theta < -PI) theta += 2.0f * PI;
  theta *= sn;
  nx = int(floor(ra * sin(theta) + XO + 0.5f));
  ny = int(floor(ro - ra * cos(theta) + YO + 0.5f));
  Serial.print(F("nx: ")); Serial.println(nx);
  Serial.print(F("ny: ")); Serial.println(ny);
}

// 예보 업데이트 시간: 2, 5, 8, 11, 14, 17, 20, 23 
float lat = 37.48;  // 위도 
float lon = 126.94; // 경도
uint8_t checkTime = 02;  //예보 기준시간
uint32_t year;  //년도 저장 변수
uint32_t month; //달 저장 변수
uint32_t day; // 일 저장 변수

//dateTime에 yyyymmdd형식으로 넣기위한 변수들
uint32_t year_a; 
uint32_t month_a;
uint32_t time;
uint32_t minute;


uint32_t dateTime; // 날짜 입력
uint8_t Rows = 12;  //페이지 당 행 개수
uint8_t pageNum = 1;  //받고자 하는 페이지 번호
uint8_t totalPage = 23; //전체 페이지 수
bool getNext = true;  //페이지 컨트롤 변수

// 기온, 강수 형태 배열 선언
float  Temp[24] = {}; //0시~23시까지의 기온 데이터를 배열에 저장
uint8_t Pty[24] = {}; //0시~23시까지의 강수 데이터를 배열에 저장

String convertTime(uint8_t checkTime) {
  String temp;
  if (checkTime < 10) { temp += '0'; temp += checkTime; }
  else temp += checkTime;
  temp += F("00");
  return temp;
}

bool finished = true;
uint8_t count = 0;
uint8_t failCount = 0;

//기상청 기상정보 요청 함수
void get_weather(uint32_t date, String bsTime, uint8_t Rows, uint8_t pageNum)
{
  Serial.println(F("Starting connection to server..."));
  if (client.connect(host, 80)) {
    Serial.println(F("Connected to server"));
    client.print(F("GET "));
    client.print((const __FlashStringHelper *)KOR_HEAD);
    client.print((const __FlashStringHelper *)KOR_GVLF);
    client.print((const __FlashStringHelper *)KOR_ADD0);
    client.print((const __FlashStringHelper *)KOR_KEY);
    client.print((const __FlashStringHelper *)KOR_ADD1);
    client.print(Rows);
    client.print((const __FlashStringHelper *)KOR_ADD2);
    client.print(pageNum);
    client.print((const __FlashStringHelper *)KOR_ADD3);
    client.print(date);
    client.print((const __FlashStringHelper *)KOR_ADD4);
    client.print(bsTime);
    client.print((const __FlashStringHelper *)KOR_ADD5);
    client.print(nx);
    client.print((const __FlashStringHelper *)KOR_ADD6);
    client.print(ny);
    client.print(F(" HTTP/1.1\r\nHost: "));
    client.print(host);
    client.print(F("\r\nConnection: close\r\n\r\n"));
    getNext = false;
  } else {
    failCount++; delay(500); 
    Serial.println(failCount);
    if (failCount > 5) {
      while (client.available()) char c = client.read();
      Serial.println(F("Failed & Finished the task")); 
      failCount = 0; finished = true; count = 0; pageNum = 1; getNext = true; 
    }
  }
}

bool checkRain = false;

void loop() {
  sensor = digitalRead(pir);  //pir(근접센서)에서 입력받은 데이터를 sensor 변수에 저장
  DateTime now = RTC.now();
  year = now.year();
  month = now.month();
  day = now.day();
  time = now.hour();
  minute = now.minute();

  //날씨 정보 요청 및 데이터 수신 시
  if (!finished) 
  { //데이터 수신이 완료 안됐을때
    DateTime now = RTC.now();
    year = now.year();
    month = now.month();
    day = now.day();
    year_a = year * 10000;
    month_a = month * 100;
    dateTime = year_a + month_a + day;
    while (count < totalPage) { // 받고자하는 페이지만큼 받지 않은 동안에
      if (getNext) {  // 다음 페이지가 필요하다면
        if (count == 0) getGride(lat, lon); // 위도, 경도
        String cTime = convertTime(checkTime);  // uint8_t 자료형의 예보 기준 시간을 문자열로 변환
        Serial.println(F("--------------------"));  // 페이지 구분선
        Serial.print(dateTime); Serial.print('/'); Serial.println(cTime); // 날짜, 시간 정보
        Serial.print(Rows);Serial.print('/');Serial.println(pageNum); // 페이지당 행개수, 요청 페이지 번호
        get_weather(dateTime, cTime, Rows, pageNum);  // 요청 메세지 전송
      }
      if (client.available()) { // apis.data.go.kr에 연결이 되어 있다면
        bool check = false; 
        while (client.available()) { check = client.find("["); if (check) break; }
        Serial.println(F("OK")); 
        //기상정보 파싱코드
        if (check) {  //대괄호 '['가 있으면
          float value = 0; uint8_t timeVal = 255; uint8_t rowC = 0; char cc;
          String line0, cat; 
          while (client.available()) {
            while (rowC < Rows) { // 현재 행이 전체행 개수 보다 작은 동안
              for (uint8_t i = 0; i < 3; i++) check = client.find(":"); // 알거나 필요없는 데이터 버리기
              cc = client.read(); cc = 'a'; cat = ""; //'"' 제거, cc, cat 초기화
              while (cc != '"') { cc = client.read(); if (cc != '"') cat += cc; } // 날씨 코드값 저장
              for (uint8_t i = 0; i < 2; i++) check = client.find(":"); // 예보 시간 앞으로 이동
              cc = client.read(); line0 = ""; // 문자 '"' 제거, line0 변수 초기화
              line0 += char(client.read()); line0 += char(client.read()); // 예보 시간 문자 두개 저장
              timeVal = line0.toInt(); line0 = "";  // timeVal에 정수값 저장, line0 변수 초기화
              bool save = false;  // 특정 시간대만 저장하도록 하는 플래그
              if (timeVal == 0 || timeVal == 1 || timeVal == 2 || timeVal == 3 
                          || timeVal == 4 || timeVal == 5 || timeVal == 6 || timeVal == 7 || timeVal == 8 || timeVal == 9
                           || timeVal == 10 || timeVal == 11 || timeVal == 12 || timeVal == 13 || timeVal == 14
                            || timeVal == 15 || timeVal == 16 || timeVal == 17 || timeVal == 18 || timeVal == 19
                             || timeVal == 20 || timeVal ==21 || timeVal == 22 || timeVal == 23)save = true;  //0~23시 사이면
              if (save) { // 저장해야 한다면
                check = client.find(":"); cc = client.read(); cc = 'a'; // 값 앞으로 이동, 문자 '"'제거, cc 변수 초기화
                while (cc != '"') { cc = client.read(); if (cc != '"') line0 += cc; } // 날씨 값 저장
                value = line0.toFloat(); line0 = "";  // 변수 value에 소수로 변환 저장, line0 변수 초기화
              }
              check = client.find("}"); // 행 마지막 이동
              if (save) {
                if (cat == F("TMP")) Temp[timeVal/1] = value; // 카테고리가 TMP이면 Temp배열에 저장        
                else if (cat == F("PTY")) Pty[timeVal/1] = uint8_t(value);  // 카테고리가 PTY이면 Pty배열에 저장
              }
              rowC++; // 다음 행 이동
            }
            while (client.available()) cc = client.read();  // 전체행 실행 종료후 나머지 데이터 버리기
          }
          count++; getNext = true; pageNum++; failCount = 0;  // 다음 페이지 연결을 실행하기위한 플래그 또는 값 설정
        } else {  // 문자 '[' 없으면
          Serial.println(F("No Data, check Date & time"));  // 요청 날짜 및 시간 확인, 3일이전 날짜는 NO DATA
          getNext = true; failCount++;
          Serial.println(failCount);
          if (failCount > 3) {
            while (client.available()) char c = client.read();
            Serial.println(F("Failed & Finished the task")); 
            failCount = 0; finished = true; count = 0; pageNum = 1; getNext = true; // 초기화
            break;  // while (count < totalPage) 루프 나가기
          }
        }
        if (count == totalPage) // 모든 페이지의 값을 수신 했으면
        { 
          finished = true; checkRain = false; // 다음번 날씨 정보 수신을 위해 초기화
          for (int i=0; i<24; i++)  
          {
            if(Pty[i]== 1 || Pty[i]==4) //강수정보 배열 안에 비코드를 뜻하는 1 / 소나기코드를 뜻하는 4 가 저장되있으면
            {
              rain = 4; //비 변수에 값 2를 저장
              Serial.print("비옴"); //시리얼 모니터에 비옴 단어 출력
            }
            else if(Pty[i]==0)
            {
                sun = 1;
            }
            else if(Pty[i]==2)
            {
                rns = 3;
            }
            else if(Pty[i]==3)
            {
                snow = 2;
            }            
          }
         
        }
      }
      if (finished) break;  // 연결 실패 카운터가 만족된 경우
    }// 받고자 하는 페이지를 모두 받았으면
    count = 0; pageNum = 1; // 날씨정보 수신 종료 및 변수 초기화
  }
  ledcon(sensor, sun, rain, rns, snow); //led 동작 함수 실행
  if(time == 4 && minute == 30) //오전 4시 30분이 되면 날씨정보 받아오기
  {
    String temp = "1";
    if (temp == "1") {
      finished = !finished;
    }  
  }
  if(digitalRead(sw) == LOW)  //스위치를 누르면 날씨정보 받아오기
  {
    String temp = "1";
    if (temp == "1") {  
      finished = !finished;
    }  
  }
}

//날씨에 따른 led동작 함수
void ledcon(int sensor,int sun,int rain, int rns, int snow) 
{
  int a=0;  // if문 컨트롤 변수
  if(sensor == 1) //근접센서 입력 값이 1이면
  {
    if(rain == 4) //비 변수 값이 4이면
    {
      a = rain;
      digitalWrite(blue, HIGH); //비 led 출력
      digitalWrite(white, LOW);
      digitalWrite(yellow, LOW);
    }

    if(rns > a) //눈비 변수 값이 비 변수 값보다 크면
    {
      a = rns;
      digitalWrite(blue, HIGH); //비 led 출력
      digitalWrite(white, HIGH); //눈 led 출력
      digitalWrite(yellow, LOW);
    }

    if(snow > a) //눈 변수 값이 비 변수 값보다 크면
    {
      a = snow;
      digitalWrite(blue, LOW); 
      digitalWrite(white, HIGH); //눈 led 출력
      digitalWrite(yellow, LOW);
    } 

    
    if(sun >= a)  //맑음변수 값이 비 변수 값보다 높을 시
    {
      digitalWrite(yellow, HIGH); //맑음 led 출력
      digitalWrite(white, LOW);
      digitalWrite(blue, LOW);
    }
  }
  else  //근접센서 입력 값이 1 이외면 전부 다 끈다.
  {
    digitalWrite(white, LOW);
    digitalWrite(blue, LOW);
    digitalWrite(yellow, LOW);
  }
  a = 0;  //초기화
}