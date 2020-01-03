#include <ESP8266WiFi.h>
#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <TridentTD_LineNotify.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 16, 2);
Adafruit_SSD1306 OLED(-1);
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
////////////////////////////////////////////
#define LENG 31   //0x42 + 31 bytes equal to 32 bytes

// firebase
#include <FirebaseArduino.h>

// Config Firebase
#define FIREBASE_HOST "esp8266dust.firebaseio.com"
#define FIREBASE_AUTH "rNErxj1L87sUk4sPqJofH1JyGaI2834RSNZ03LQl"



unsigned char buf[LENG];
int PM01Value = 0;        //define PM1.0 value of the air detector module
int PM2_5Value = 0;       //define PM2.5 value of the air detector module
int PM10Value = 0;       //define PM10 value of the air detector module
const String LINE_TOKEN = "Yxrvua2dy8DHGsKlvEAYgKivJUgEfKBB7sm4nbLuVjQ";

WiFiServer server(80);// Set port to 80
String header; // This storees the HTTP request

///////////////////////////////////////////////
char ssid[] = "bank_2G"; // กรอกชื่อ wifi
char pass[] = "0853644094";  // กรอกรหัสผ่าน wifi
int AQI = 0;
///////////////////////////////////////////////
const long utcOffsetInSeconds = 25200;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "asia.pool.ntp.org", utcOffsetInSeconds);
int  count = 0;
int calAQI(int pm25) {
  if (pm25 <= 12) {
    return pm25 * 50 / 12;
  }
  else if (pm25 <= 35) {
    return 50 + (pm25 - 12) * 50 / 23;
  }
  else if (pm25 <= 55) {
    return 100 + (pm25 - 35) * 5 / 2;
  }
  else if (pm25 <= 150) {
    return 150 + (pm25 - 55) * 50 / 95;
  }
  else if (pm25 <= 350) {
    return 50 + pm25;
  }
  else {
    return 400 + (pm25 - 350) * 2 / 3;
  }
}
char checkValue(unsigned char *thebuf, char leng)
{
  char receiveflag = 0;
  int receiveSum = 0;

  for (int i = 0; i < (leng - 2); i++) {
    receiveSum = receiveSum + thebuf[i];
  }
  receiveSum = receiveSum + 0x42;

  if (receiveSum == ((thebuf[leng - 2] << 8) + thebuf[leng - 1])) //check the serial data
  {
    receiveSum = 0;
    receiveflag = 1;
  }
  return receiveflag;
}

int transmitPM01(unsigned char *thebuf)
{
  int PM01Val;
  PM01Val = ((thebuf[3] << 8) + thebuf[4]); //count PM1.0 value of the air detector module
  return PM01Val;
}

//transmit PM Value to PC
int transmitPM2_5(unsigned char *thebuf)
{
  int PM2_5Val;
  PM2_5Val = ((thebuf[5] << 8) + thebuf[6]); //count PM2.5 value of the air detector module
  return PM2_5Val;
}

//transmit PM Value to PC
int transmitPM10(unsigned char *thebuf)
{
  int PM10Val;
  PM10Val = ((thebuf[7] << 8) + thebuf[8]); //count PM10 value of the air detector module
  return PM10Val;
}

//}

void setup()
{
  Serial.begin(9600);
  Wire.begin(D2, D1);

  lcd.begin();

  WiFi.begin(ssid, pass);
  int count = 0;
  while ( WiFi.status() != WL_CONNECTED ) {
    delay ( 500 );
    Serial.print ( "." );
    //      if (count == 10) {
    //        ESP.restart();
    //      } else {
    //        count ++;
    //      }
  }
  // Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Serial.print (WiFi.localIP());
  timeClient.begin();
  LINE.setToken(LINE_TOKEN);
  server.begin();
}

void loop()
{


  if (Serial.find(0x42)) {  //start to read when detect 0x42
    Serial.readBytes(buf, LENG);

    if (buf[0] == 0x4d) {
      if (checkValue(buf, LENG)) {
        PM01Value = transmitPM01(buf); //count PM1.0 value of the air detector module
        PM2_5Value = transmitPM2_5(buf); //count PM2.5 value of the air detector module
        PM10Value = transmitPM10(buf); //count PM10 value of the air detector module
      }
    }
  }

  if (PM2_5Value <= 12) {
    AQI =  PM2_5Value * 50 / 12;
  }
  else if (PM2_5Value <= 35) {
    AQI =  50 + (PM2_5Value - 12) * 50 / 23;
  }
  else if (PM2_5Value <= 55) {
    AQI =  100 + (PM2_5Value - 35) * 5 / 2;
  }
  else if (PM2_5Value <= 150) {
    AQI =  150 + (PM2_5Value - 55) * 50 / 95;
  }
  else if (PM2_5Value <= 350) {
    AQI =  50 + PM2_5Value;
  }
  else {
    AQI =  400 + (PM2_5Value - 350) * 2 / 3;
  }

  if (WiFi.status() != WL_CONNECTED) {
    ESP.restart();
  }

  // Serial.println(AQI);

  lcd.home();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("PM2.5: " + String(PM2_5Value) + " ug/m3");
  lcd.setCursor(0, 1);
  lcd.print("AQI: " + String(AQI));


  // Firebase.setInt("PM1_0", PM01Value);
  // Firebase.setInt("PM2_5", PM2_5Value);
  // Firebase.setInt("PM10", PM10Value);
  // Firebase.setInt("AQI", calAQI(PM2_5Value));

  //    if (Firebase.failed()) {
  //      Serial.print("setting /message failed:");
  //      Serial.println(Firebase.error());
  //      return;
  //    }

  timeClient.update();

  String getTime = String(timeClient.getHours()) + ":" + String(timeClient.getMinutes()) + ":" + String(timeClient.getSeconds());

  if (getTime.equals("6:0:0") || getTime.equals("12:0:0") || getTime.equals("18:0:0") || getTime.equals("17:0:0") ) {

    if (AQI >= 0 && AQI <= 25) {
      LINE.notify("คุณภาพอากาศดีมาก: " + (String(PM2_5Value) + " ug/m3"));
      LINE.notify("AQI: " + String(AQI));
      //  ESP.restart();
    }
    else if (AQI >= 26 && AQI <= 50) {
      LINE.notify("คุณภาพอากาศดี: " + (String(PM2_5Value) + " ug/m3"));
      LINE.notify("AQI: " + String(AQI));
      // ESP.restart();
    }
    else if (AQI >= 51 && AQI <= 100) {
      LINE.notify("คุณภาพอากาศปานกลาง: " + (String(PM2_5Value) + " ug/m3"));
      LINE.notify("AQI: " + String(AQI));
      // ESP.restart();
    }
    else if (AQI >= 101 && AQI <= 200) {
      LINE.notify("คุณภาพอากาศเริ่มมีผลกระทบต่อสุขภาพ: " + (String(PM2_5Value) + " ug/m3"));
      LINE.notify("AQI: " + String(AQI));
      // ESP.restart();
    } else {
      LINE.notify("คุณภาพอากาศมีผลกระทบต่อสุขภาพ: " + (String(PM2_5Value) + " ug/m3"));
      LINE.notify("AQI: " + String(AQI));
      //  ESP.restart();
    }
  }
  else {
    // Serial.println(count);
    if (AQI >= 101 && AQI <= 200 && count == 0) {
      count = count + 1;
      LINE.notify("คุณภาพอากาศเริ่มมีผลกระทบต่อสุขภาพ: " + (String(PM2_5Value) + " ug/m3"));
      LINE.notify("AQI: " + String(AQI));
    } else if (AQI >= 201) {
      count = count + 1;
      LINE.notify("คุณภาพอากาศมีผลกระทบต่อสุขภาพ: " + (String(PM2_5Value) + " ug/m3"));
      LINE.notify("AQI: " + String(AQI));
    }
    else if (AQI <= 100 && count != 0) {
      count = 0;
    }
  }
  WiFiClient client = server.available();
  if (client) {                             // If a new client connects,
    String currentLine = "";                // make a String to hold incoming data from the client
    // if there's bytes to read from the client,
    char c = client.read();             // read a byte, then
    Serial.write(c);                    // print it out the serial monitor
    header += c;
    
        client.println("HTTP/1.1 200 OK");
        client.println("Content-type:text/html");
        client.println("Connection: close");
        client.println();

        // Display the HTML web page
        client.println("<!DOCTYPE html><html>");
        client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
        client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
        client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}</style></head>");

        // Web Page Heading
        client.println("<body><h1>ESP8266 Web Server</h1>");

        client.println("<p>PM1.0 " + String(PM01Value) + " ug/m3</p>");
        client.println("<p>PM2.5 " + String(PM2_5Value) + " ug/m3</p>");
        client.println("<p>PM10 " + String(PM10Value) + " ug/m3</p>");
        client.println("<p>AQI " + String(AQI) + "</p>");

        client.println("</body></html>");

        client.println();

  }

}
