#include "xport.hpp"
#include "ESP8266TimerInterrupt.h"
#include "ESP8266_ISR_Timer.h"

#include "ESP8266WiFi.h"
#include "ESP8266WebServer.h"
#include <Wire.h>              // i2C 통신을 위한 라이브러리
#include <LiquidCrystal_I2C.h> // LCD 2004 I2C용 라이브러리

#define USING_TIM_DIV1 false
#define USING_TIM_DIV16 false
#define USING_TIM_DIV256 true

ESP8266Timer ITimer;
ESP8266_ISR_Timer ISR_Timer;

static uint8 pins[] = {4, 0, 2, 14, 12, 13}; // pin D1~D7
XBus xbus(pins);
int GCount = 1000000;
LiquidCrystal_I2C lcd(0x3F, 20, 4);

// **************************************************
// **                      ISR
// **************************************************

void ISR()
{
  // xbus.check();
  // xbus.updateMinRemainTime();
  // ISR_Timer.setTimeout(xbus.minRemainTime, ISR);

  // ISR_Timer.setTimeout(10, ISR);
}

void IRAM_ATTR TimerHandler() // fixed basic timer
{
  GCount++;
  // ISR_Timer.run();
}

const char *ssid = "DIV_1_2G";
const char *password = "2-142014";
WiFiServer server(80);

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

  // pinMode(pins[0], OUTPUT);
  // pinMode(pins[1], OUTPUT);
  // pinMode(pins[2], OUTPUT);
  // pinMode(pins[3], OUTPUT);

  // xbus.addXPort_T2(10, 1, 1);
  // xbus.addXPort_T2(10, 1, 1);
  // xbus.addXPort_T2(10, 1, 1);
  // xbus.addXPort_T2(10, 1, 1);

  // xbus.updateMinRemainTime();

  ITimer.attachInterruptInterval(1000, TimerHandler);
  // ISR_Timer.setTimeout(1000,ISR);

  /*
   * WIFI / LCD Setup
   */
  Serial.begin(115200);
  Wire.begin(12, 13);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("TriGen V0.01");
  lcd.setCursor(0, 3);
  lcd.print("WIFI");
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
  {
    lcd.setCursor(0, 0);
    lcd.print("TriGen V0.01");
    lcd.setCursor(0, 3);
    lcd.print("                    ");
    lcd.setCursor(0, 3);
    lcd.print(WiFi.localIP());
  }

  server.begin();
  // pinMode(pins[5], OUTPUT);
  // pinMode(pins[6], OUTPUT);

  //  SR_Timer.setTimeout(100, ISR);
}

void loop()
{
  lcd.setCursor(0, 1);
  lcd.print(GCount);
  delay(100);
  //  Serial.print(GCount);
  //  Serial.print(" ");
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
                        if (value.toInt() == 0)
                        {
                          externTrigger = false;
                        }
                        else
                        {
                          externTrigger = true;
                        }
                        break;
                      case 5:
                        if (value.toInt() == 0)
                        {
                          power = false;
                        }
                        else
                        {
                          power = true;
                        }
                        break;
                      }
                      startIndex = endIndex + 3;
                    }
                  }

                  //  xbus.updateXPort_T2(xportIndex - 1, fpsValue, t2Time, counterValue, externTrigger, power);
                }
              }
            }

            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta charset='UTF-8' name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\"></head>");

            client.println("<body><a href=\"http://" + WiFi.localIP().toString() + "\" style=\"text-decoration: none; color: inherit;\"><h1 class='red'><img src='http://www.dongiltech.co.kr/images/common/logo.png' alt='DIT'>   TRIGGER GENERATOR V0.01</h1></a>");

           

            // h1 redirect
            // client.println("<body><a href=" + WiFi.localIP().toString() + " <style=\"text-decoration: none; color: inherit;\"><h1>DIV FPS Controller</h1></a>");
            const String ss =  "<style>"\
                "table { border-collapse: collapse;   padding: 10px; margin: 15px;}"\
                ".red { color: #E03C32; font-size:40px; font-weight: bold; padding: 10px; margin: 10px;}"\
                ".normal {  background-color: aliceblue; width: 800px; line-height: 1.5em; font-family: 'Nanum Gothic', sans-serif; font-size: 14px; padding: 10px; margin: 10px 40px;}"\
                ".circle::before { content: '\\25CF'; margin-right: 0.5em; margin-left:20px}";

        //    client.println("table { border-collapse: collapse;   padding: 10px; margin: 15px;}");
            client.println(ss);
         //   client.println(".normal {  background-color: aliceblue; width: 800px; font-family: 'Nanum Gothic', sans-serif; font-size: 16px; padding: 10px; margin: 10px;}");
            client.println(".circle::before { content: '\\25CF'; margin-right: 0.5em; margin-left:20px}"); // check 2713 circle 25cf
            client.println("th, td { border: 1px solid #E03C32; text-align: right; padding: 7px; width:150px;}");
            client.println("</style>");

             client.println("<h4 class='circle'>4 Channel Trigger Generator</h4>");
            client.println("<h4 class='circle' > 현재 진행 스템번호 =" + String(GCount) + "</h4>");
            
            const String a = "<div class='normal'>"\
              "FPS 0.1 ~ 200 FPS (signal)<br>"
              "초기설정 : AP모드/시리얼을 이용하여, 초기 AP(ssid,passwd)입력가능<br>"\
              "표준IOT프로토콜 : MQTT 지원, Reset HW Chip <br>"\
              "웹메뉴얼 연결 및 한글설명서 <br> </div>";
           client.println(a);

            client.println("<table>");
            // Camera FPS Duration DownCount ExternTrigger Power
            client.println("<tr><th>Camera</th><th>FPS</th><th>Duration(ms)</th><th>DownCount</th><th>ExternTrigger</th><th>Power</th></tr>");
            for (int i = 0; i < NUM_XPORT; ++i)
            {
              client.println("<tr>");
              client.println("<td>" + String(i + 1) + "</td><td>" + String(xbus.XPorts[i].fps) +
                             "</td><td>" + String(xbus.XPorts[i].T2) +
                             "</td><td>" + String(xbus.XPorts[i].count) + "/" + String(xbus.XPorts[i].countMax) +
                             "</td><td>" + String(xbus.XPorts[i].ExternTrigger ? "True" : "False") +
                             "</td><td>" + String(xbus.XPorts[i].Power ? "True" : "False"));
            }
            client.println("</table>");

            client.println("<br><form action='/set' method='GET'>");

            client.print("Set Camera Data: ");
            client.print("<input type='text' name='data'>");
            client.println("<input type='submit' name = 'submit' value='Set'></form>");

            client.println("<br><img src='http://www.dongiltech.co.kr/uploaded/category/catalog_c54bcaca912987c75e52cfc7b0ef9c780.png' width='600' height='350'/>");

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
    // //  int btnX = btnNum(key_value);
    //   // switch (btnX)
    //   // {
    //   // case btnUP:
    //   //   FPS++;
    //   //   break;
    //   // case btnDOWN:
    //   //   FPS--;
    //   //   break;
    //   // }
    //   if (btnX < 5)
    //   {
    //     lcd.setCursor(0, 1);
    //     lcd.print(key_value + 10000000 * (btnX + 1));
    //     delay(300);
    //   }
  }
}
