#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <TridentTD_LineNotify.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <Arduino_JSON.h>
#include "DHT.h"
DHT dht;

ESP8266WebServer    server(80);

String serverName = "http://worldtimeapi.org/api/timezone/Asia/Bangkok";

struct settings {
  char ibhname[20];
  char ssid[30];
  char password[30];
  char LINE_TOKEN[50];
  char temp_min[4];
  char temp_max[4];
  char mois_min[4];
  char mois_max[4];
  char time1[10];
  char time2[10];
  char time3[10];
  
} user_wifi = {};
String timenow;
String msg;
int timemi;

int line_t1;int line_t2;int line_t3;

void setup() {
  Serial.begin(9600);
  //settings myset;
  dht.setup(5); // data pin 1


  
  EEPROM.begin(sizeof(struct settings) );
  EEPROM.get( 0, user_wifi );
   
  WiFi.mode(WIFI_STA);
  WiFi.begin(user_wifi.ssid, user_wifi.password);
  Serial.print("WIFI : ");
  Serial.println(user_wifi.ssid);
     Serial.println(user_wifi.temp_max);
   Serial.println(user_wifi.temp_min);
     Serial.println(user_wifi.mois_min);
   Serial.println(user_wifi.mois_max);
   
   Serial.println(user_wifi.time1);
   Serial.println(user_wifi.time2);
   Serial.println(user_wifi.time3);


         //กำหนดเวลาเตือนเป็นวินาที
        line_t1=(String(user_wifi.time1).substring(0,2).toInt()*60*60)+(String(user_wifi.time1).substring(3,5).toInt()*60);
        line_t2=(String(user_wifi.time2).substring(0,2).toInt()*60*60)+(String(user_wifi.time2).substring(3,5).toInt()*60);
        line_t3=(String(user_wifi.time3).substring(0,2).toInt()*60*60)+(String(user_wifi.time3).substring(3,5).toInt()*60);
        Serial.println(line_t1);

  
  byte tries = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    if (tries++ > 30) {
      WiFi.mode(WIFI_AP);
      WiFi.softAP("IBH IT", "ESP-inburi");
      break;
    }
  }  server.on("/",  handlePortal);
  server.begin();


  LINE.setToken(user_wifi.LINE_TOKEN);
  //LINE.notify("รพ.อินทร์บุรี สวัสดี \n กำหนดค่า อุณหภูมิสูงสุด : ");
  delay(1000);
  LINE.notify(user_wifi.temp_max);

  // ดึงเวลา
  WiFiClient client;
  HTTPClient http;
  String serverPath = serverName + "";
  http.begin(client, serverPath.c_str());
  int httpResponseCode = http.GET();
  if (httpResponseCode>0) {
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
        String payload = http.getString();
        Serial.println(payload);
        JSONVar myObject = JSON.parse(payload);
        Serial.print("myObject[\"datetime\"] = ");
        timenow=String(myObject["datetime"]);
        timenow = timenow.substring(11,16);
        timemi=((timenow.substring(0,2).toInt())*60*60)+(timenow.substring(3,5).toInt()*60);

            delay(dht.getMinimumSamplingPeriod());
            int humidity = dht.getHumidity(); // ดึงค่าความชื้น
            int temperature = dht.getTemperature(); // ดึงค่าอุณหภูมิ
             msg=String(user_wifi.ibhname)+":ตั้งค่าแจ้งเตือน ความชื้นต่ำสุดที่ "+ String(user_wifi.mois_min)+" สูงสุดที่ "+String(user_wifi.mois_max)
             +"อุณหภูมิต่ำสุดที่ "+ String(user_wifi.temp_min)+" สูงสุดที่ "+String(user_wifi.temp_max)
             +"ช่วงเวลาให้รายงาน "+String(user_wifi.time1)+" | "+String(user_wifi.time2)+" | "+String(user_wifi.time3)
             +"\n ค่าความชื้น ขณะนี้ "+String(humidity)+" % \n อุณหภูมิ "+String(temperature)+" °C \n เวลา "+timenow;
             LINE.notify(msg);
      }
      else {
        Serial.print("Error code: ");
        Serial.println(httpResponseCode);
      }


}


void loop() {

  server.handleClient();
  //LINE.notify(user_wifi.temp_max);




  //แสดงความชื้น
    delay(dht.getMinimumSamplingPeriod());
  int humidity = dht.getHumidity(); // ดึงค่าความชื้น
  int temperature = dht.getTemperature(); // ดึงค่าอุณหภูมิ
  Serial.print(dht.getStatusString());
  Serial.print("\t");
  Serial.print(humidity, 1);
  Serial.print("\t\t");
  Serial.print(temperature, 1);
  Serial.print("\t\t");
  Serial.println(dht.toFahrenheit(temperature), 1);

if(dht.getStatusString()=="OK"){
  if(humidity<String(user_wifi.mois_min).toInt() || humidity>String(user_wifi.mois_max).toInt()){
    msg=String(user_wifi.ibhname)+":ความชื้นผิดปกติ โปรดตรวจสอบ \n ค่าความชื้น "+String(humidity)+" % \n อุณหภูมิ "+String(temperature)+" °C";
    LINE.notify(msg);
  }

  if(temperature<String(user_wifi.temp_min).toInt() || temperature>String(user_wifi.temp_max).toInt()){
    msg=String(user_wifi.ibhname)+":อุณหภูมิผิดปกติ โปรดตรวจสอบ \n ค่าความชื้น "+String(humidity)+" % \n อุณหภูมิ "+String(temperature)+" °C";
    LINE.notify(msg);
  }

  if(line_t1==timemi || line_t2==timemi || line_t3==timemi){
    msg=String(user_wifi.ibhname)+":ค่าความชื้น "+String(humidity)+" % \n อุณหภูมิ "+String(temperature)+" °C";
    LINE.notify(msg);
  }
  

}
  if(timemi<86400){
      timemi=timemi+5;
  }else{
    timemi=0;
  }
  delay(5000);
}

void handlePortal() {

  if (server.method() == HTTP_POST) {
    Serial.println(server.arg("time1").c_str());
    Serial.println(server.arg("time2").c_str());
    Serial.println(server.arg("time2").c_str());
    strncpy(user_wifi.ibhname,     server.arg("ibhname").c_str(),     sizeof(user_wifi.ibhname) );
    strncpy(user_wifi.ssid,     server.arg("ssid").c_str(),     sizeof(user_wifi.ssid) );
    strncpy(user_wifi.password, server.arg("password").c_str(), sizeof(user_wifi.password) );
    strncpy(user_wifi.LINE_TOKEN, server.arg("linetoken").c_str(), sizeof(user_wifi.LINE_TOKEN) );
    strncpy(user_wifi.temp_max, server.arg("temp_max").c_str(), sizeof(user_wifi.temp_max) );
    strncpy(user_wifi.temp_min, server.arg("temp_min").c_str(), sizeof(user_wifi.temp_min) );
    strncpy(user_wifi.mois_min, server.arg("mois_min").c_str(), sizeof(user_wifi.mois_min) );
    strncpy(user_wifi.mois_max, server.arg("mois_max").c_str(), sizeof(user_wifi.mois_max) );
    strncpy(user_wifi.time1, server.arg("time1").c_str(), sizeof(user_wifi.time1) );
    strncpy(user_wifi.time2, server.arg("time2").c_str(), sizeof(user_wifi.time2) );
    strncpy(user_wifi.time3, server.arg("time3").c_str(), sizeof(user_wifi.time3) );

    
    user_wifi.ibhname[server.arg("ibhname").length()] = user_wifi.ssid[server.arg("ssid").length()] = user_wifi.password[server.arg("password").length()] = user_wifi.LINE_TOKEN[server.arg("linetoken").length()] = user_wifi.temp_min[server.arg("temp_min").length()] = user_wifi.temp_max[server.arg("temp_max").length()] = user_wifi.mois_min[server.arg("mois_min").length()] = user_wifi.mois_max[server.arg("mois_max").length()] = user_wifi.time1[server.arg("time1").length()]= user_wifi.time2[server.arg("time2").length()] = user_wifi.time3[server.arg("time3").length()] = '\0';
    EEPROM.put(0, user_wifi);
    EEPROM.commit();
    

    server.send(200,   "text/html",  "<!doctype html><html lang='en'><head><meta charset='utf-8'><meta name='viewport' content='width=device-width, initial-scale=1'><title>Wifi Setup</title><style>*,::after,::before{box-sizing:border-box;}body{margin:0;font-family:'Segoe UI',Roboto,'Helvetica Neue',Arial,'Noto Sans','Liberation Sans';font-size:1rem;font-weight:400;line-height:1.5;color:#212529;background-color:#f5f5f5;}.form-control{display:block;width:100%;height:calc(1.5em + .75rem + 2px);border:1px solid #ced4da;}button{border:1px solid transparent;color:#fff;background-color:#007bff;border-color:#007bff;padding:.5rem 1rem;font-size:1.25rem;line-height:1.5;border-radius:.3rem;width:100%}.form-signin{width:100%;max-width:400px;padding:15px;margin:auto;}h1,p{text-align: center}</style> </head> <body><main class='form-signin'> <h1>Wifi Setup</h1> <br/> <p>Your settings have been saved successfully!<br />Please restart the device.</p></main></body></html>" );
  } else {

    server.send(200,   "text/html", "<!doctype html><html lang='en'><head> <meta charset='utf-8'> <meta name='viewport' content='width=device-width, initial-scale=1'> <title>IBHSetup | รพ.อินทร์บุรี</title> <style> *, ::after, ::before { box-sizing: border-box; } body { margin: 0; font-family: 'Segoe UI', Roboto, 'Helvetica Neue', Arial, 'Noto Sans', 'Liberation Sans'; font-size: 1rem; font-weight: 400; line-height: 1.5; color: #212529; background-color: #f5f5f5; } .form-control { display: block; width: 100%; height: calc(1.5em + .75rem + 2px); border: 1px solid #ced4da; } button { cursor: pointer; border: 1px solid transparent; color: #fff; background-color: #007bff; border-color: #007bff; padding: .5rem 1rem; font-size: 1.25rem; line-height: 1.5; border-radius: .3rem; width: 100% } .form-signin { width: 100%; max-width: 400px; padding: 15px; margin-bottom: 90px; } h1 { text-align: center } .footer { position: fixed; left: 0; bottom: 0; width: 100%; background-color: #FFF; text-align: center; z-index: 2; } </style></head><body> <main class='form-signin'> <form action='/' method='post'> <h1 class=''>IBH Wifi Setup</h1><br> <div class='form-floating'><label>ชื่ออุปกรณ์</label><input type='text' class='form-control' name='ibhname'> </div> <div class='form-floating'><label>SSID</label><input type='text' class='form-control' name='ssid'> </div> <div class='form-floating'><br><label>Password</label><input type='password' class='form-control' name='password'></div> <div class='form-floating'><br><label>LINE_TOKEN</label><input type='text' class='form-control' name='linetoken'></div> <div class='form-floating'><br><label>อุณภูมิต่ำสุด (C)</label><input type='number' class='form-control' name='temp_min'></div> <div class='form-floating'><br><label>อุณภูมิสูงสุด (C)</label><input type='number' class='form-control' name='temp_max'></div> <div class='form-floating'><br><label>ความชื้นต่ำสุด</label><input type='number' class='form-control' name='mois_min'></div> <div class='form-floating'><br><label>ความชื้นสูงสุด</label><input type='number' class='form-control' name='mois_max'></div> <hr> <div class='form-floating'><br><label>เวลารายงาน ครั้งที่ 1 </label><input type='time' class='form-control' name='time1'></div> <div class='form-floating'><br><label>เวลารายงาน ครั้งที่ 2 </label><input type='time' class='form-control' name='time2'></div> <div class='form-floating'><br><label>เวลารายงาน ครั้งที่ 3 </label><input type='time' class='form-control' name='time3'></div> <br><br><button type='submit'>บันทึก</button> </form> </main> <footer class='footer'> เครื่องวัดอุณหภูมิแจ้งเตือนไลน์ V1.0 <br>ศูนย์เทคโนโลยีและสารสนเทศ <br><b>โรงพยาบาลอินทร์บุรี</b> </footer></body></html>" );
  }
}
