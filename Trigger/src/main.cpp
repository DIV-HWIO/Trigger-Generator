#include "ESP8266TimerInterrupt.h"
#include "ESP8266_ISR_Timer.h"

#include "ESP8266WiFi.h"
#include "ESP8266WebServer.h"
#include <Wire.h>              // i2C 통신을 위한 라이브러리
#include <LiquidCrystal_I2C.h> // LCD 2004 I2C용 라이브러리

#pragma region ISR

#define USING_TIM_DIV1 false
#define USING_TIM_DIV16 false
#define USING_TIM_DIV256 true

ESP8266Timer ITimer;
ESP8266_ISR_Timer ISR_Timer;
// ESP8266_ISR_Timerac ISR_timer;

static uint8 pins[] = {5, 4, 0, 2, 14, 12, 13}; // pin D1~D7

class Xport
{
public:
  Xport(float FPS, float DutyRatio, long count)
  {
    int timePeriodMs = 1000 / FPS;
    this->T1 = round(timePeriodMs * (1 - DutyRatio));
    this->T2 = timePeriodMs - T1;
    this->RemainMs = T1;
    this->isOn = false;
    this->count = count;
    this->countMax = count;
    this->fps = FPS;
  }
  long T1;
  long T2;
  float fps;

  long RemainMs;
  bool isOn;
  long count;
  long countMax;

  bool ExternTrigger;
  bool Power;
};

class XBus
{
public:
  long minRemainTime;
  std::vector<Xport> Xports;

  void addXport(float FPS, float DutyRatio, long count = -1)
  {
    Xports.push_back(Xport(FPS, DutyRatio, count));
  }

  void addXport_T2(float FPS, long T2Time, long count = -1)
  {
    long timePeriodMs = round(1000.0 / FPS);
    long T1 = timePeriodMs - T2Time;
    float DutyRatio = 1 - (float)T1 / timePeriodMs;
    Xports.push_back(Xport(FPS, DutyRatio, count));
  }

  void updateXport(int index, float FPS, float DutyRatio, long count = -1)
  {
    Xports[index] = Xport(FPS, DutyRatio, count);
  }

  void updateXport_T2(int index, float FPS, long T2Time, long count = -1, bool externTrigger = false, bool power = false)
  {
    long timePeriodMs = round(1000.0 / FPS);
    long T1 = timePeriodMs - T2Time;
    float DutyRatio = 1 - (float)T1 / timePeriodMs;
    Xports[index] = Xport(FPS, DutyRatio, count);
    Xports[index].ExternTrigger = externTrigger;
    Xports[index].Power = power;
  }

  void updateMinRemainTime()
  {
    long temp = __LONG_MAX__;
    for (const Xport &xport : Xports)
    {
      if (xport.RemainMs < temp)
      {
        temp = xport.RemainMs;
      }
    }
    minRemainTime = temp;
  }

  void check()
  {
    for (int i = 0; i < Xports.size(); i++)
    {
      Xport &xport = Xports[i];
      if (xport.count == 0)
      { // 카운트가 0이면 무시
        continue;
      }
      if (xport.RemainMs - minRemainTime <= 0)
      {
        if (xport.isOn)
        {
          xport.isOn = false;
          xport.RemainMs = xport.T1;
          digitalWrite(pins[i], LOW);
        }
        else
        {
          xport.isOn = true;
          xport.RemainMs = xport.T2;
          digitalWrite(pins[i], HIGH);

          if (xport.count > 0)
          { //-1이면 무한반복, 0이상이면 카운트 감소
            xport.count--;
            /* 0번 채널의 끝나는 시간을 millis로 출력
            if(i==0 && xport.count==0){
              Serial.println("");
              Serial.print(F("Time: ")); Serial.print(millis());

            }
            */
          }
        }
      }
      else
      {
        xport.RemainMs -= minRemainTime;
      }
    }
  }
};
XBus xbus;

void ISR()
{
  xbus.check();
  xbus.updateMinRemainTime();
  ISR_Timer.setTimeout(xbus.minRemainTime, ISR);
}

void IRAM_ATTR TimerHandler()
{
  ISR_Timer.run();
}

#pragma endregion

#pragma region WEB
LiquidCrystal_I2C lcd(0x3F, 20, 4);
const char *ssid = "DIV_1_2G";
const char *password = "2-142014";
WiFiServer server(80);

#define btnRIGHT 0
#define btnUP 1
#define btnDOWN 2
#define btnLEFT 3
#define btnSELECT 4
#define btnNONE 5

int btnNum(int value)
{
  if (value > 1400)
    return btnNONE;
  if (value < 100)
    return btnRIGHT;
  if (value < 400)
    return btnUP;
  if (value < 800)
    return btnDOWN;
  if (value < 1100)
    return btnLEFT;
  if (value < 1300)
    return btnSELECT;
  return btnNONE;
}

String header;

const int NUM_XPORT = 4;

String output26State = "off";
String output27State = "off";
// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0;
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;
#pragma endregion

void setup()
{
#pragma region ISR
  Serial.begin(115200);

  pinMode(pins[0], OUTPUT);
  pinMode(pins[1], OUTPUT);
  pinMode(pins[2], OUTPUT);
  pinMode(pins[3], OUTPUT);

  xbus.addXport_T2(10, 1, 1);
  xbus.addXport_T2(10, 1, 1);
  xbus.addXport_T2(10, 1, 1);
  xbus.addXport_T2(10, 1, 1);

  xbus.updateMinRemainTime();

  ITimer.attachInterruptInterval(1000, TimerHandler);
  ISR_Timer.setTimeout(xbus.minRemainTime, ISR);

#pragma endregion

#pragma region WEB
  // ESP UNO Bug 로 반드시 해주어야 한다.
  // pinMode(2, OUTPUT);
  // digitalWrite(2, LOW);
  // lcd.begin(20, 4); // 16자 2행
  Wire.begin(pins[5], pins[6]);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("WAIT WIFI2- ");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    lcd.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  lcd.setCursor(0, 3);
  lcd.print(WiFi.localIP());
  server.begin();
  for (int i = 0; i < 10; i++)
  {
    lcd.setCursor(0, 0);
    lcd.print(i);
  }

  pinMode(pins[5], OUTPUT);
  pinMode(pins[6], OUTPUT);

#pragma endregion
}

void loop()
{
  WiFiClient client = server.available(); // Listen for incoming clients
  if (client)
  { // If a new client connects,
    currentTime = millis();
    previousTime = currentTime;
    Serial.println("New Client."); // print a message out in the serial port
    String currentLine = "";       // make a String to hold incoming data from the client
    while (client.connected() && currentTime - previousTime <= timeoutTime)
    { // loop while the client's connected
      currentTime = millis();
      if (client.available())
      {                         // if there's bytes to read from the client,
        char c = client.read(); // read a byte, then
        header += c;
        if (c == '\n')
        { // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0)
          {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Connection: close");
            client.println();

            // "GET set?data=1%2C1%2C1%2C1%2C1%2C1&submit=Set"와 같은 형태로 데이터를 받아와서 처리할 수 있어
            if (header.indexOf("GET /set") >= 0)
            {
              Serial.println("GET /set");
              if (header.indexOf("data=") >= 0)
              {
              // 6개의 column을 가진 데이터를 받아와서 처리
              int startIndex = header.indexOf("data=") + 5;

              int xportIndex = 0;
              int fpsValue = 0;
              int t2Time = 0;
              int counterValue = 0;
              bool externTrigger = false;
              bool power = false;

              if (startIndex > 0)
              {
                for (int i = 0; i < 6; i++)
                {
                  int endIndex = header.indexOf("%2C", startIndex);
                  if (endIndex > 0)
                  {
                    String value = header.substring(startIndex, endIndex);
                    switch (i)
                    {
                    case 0:
                      xportIndex = value.toInt();
                      break;
                    case 1:
                      fpsValue = value.toInt();
                      break;
                    case 2:
                      t2Time = value.toInt();
                      break;
                    case 3:
                      counterValue = value.toInt();
                      break;
                    case 4:
                      if(value.toInt() == 0){
                        externTrigger = false;
                      }
                      else{
                        externTrigger = true;
                      }
                      break;
                    case 5:
                      if(value.toInt() == 0){
                        power = false;
                      }
                      else{
                        power = true;
                      }
                      break;
                    }
                    startIndex = endIndex + 3;
                  }
                }

                xbus.updateXport_T2(xportIndex - 1, fpsValue, t2Time, counterValue, externTrigger, power);
            }
              }
            }

      

            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\"></head>");
            
            client.println("<body><a href=\"http://" + WiFi.localIP().toString() + "\" style=\"text-decoration: none; color: inherit;\"><h1>DIV FPS Controller</h1></a>");


            //h1 redirect
            //client.println("<body><a href=" + WiFi.localIP().toString() + " <style=\"text-decoration: none; color: inherit;\"><h1>DIV FPS Controller</h1></a>");

            client.println("<style>");
            client.println("table { border-collapse: collapse; width: 100%; }");
            client.println("th, td { border: 1px solid #dddddd; text-align: left; padding: 8px; }");
            client.println("</style>");

            client.println("<table>");
            // Camera FPS Duration DownCount ExternTrigger Power
            client.println("<tr><th>Camera</th><th>FPS</th><th>Duration(ms)</th><th>DownCount</th><th>ExternTrigger</th><th>Power</th></tr>");
            for (int i = 0; i < NUM_XPORT; ++i)
            {
              client.println("<tr>");
              client.println("<td>" + String(i + 1) + "</td><td>" + String(xbus.Xports[i].fps) +
                             "</td><td>" + String(xbus.Xports[i].T2) +
                             "</td><td>" + String(xbus.Xports[i].count) + "/" + String(xbus.Xports[i].countMax) +
                             "</td><td>" + String(xbus.Xports[i].ExternTrigger ? "True" : "False") +
                             "</td><td>" + String(xbus.Xports[i].Power ? "True" : "False"));
            }
            client.println("</table>");

            client.println("<form action='/set' method='GET'>");

            client.print("Set Camera Data: ");
            client.print("<input type='text' name='data'>");
            client.println("<input type='submit' name = 'submit' value='Set'></form>");

            client.println("</body></html>");
            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            break;
          }
          else
          { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        }
        else if (c != '\r')
        {                   // if you got anything else but a carriage return character,
          currentLine += c; // add it to the end of the currentLine
        }
      }
    }

    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
  }
  int key_value = 0;
  int FPS = 10;
  return;
  for (int i = 1000; i < 90000000; i += 2)
  {
    lcd.setCursor(0, 0);
    delay(1000 / FPS);
    lcd.print(i);
    lcd.print(" ");
    lcd.print(FPS);
    lcd.print(" Fps");
    key_value = analogRead(2);
    int btnX = btnNum(key_value);
    switch (btnX)
    {
    case btnUP:
      FPS++;
      break;
    case btnDOWN:
      FPS--;
      break;
    }
    if (btnX < 5)
    {
      lcd.setCursor(0, 1);
      lcd.print(key_value + 10000000 * (btnX + 1));
      delay(300);
    }
  }
}
