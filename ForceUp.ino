#include <WiFi.h>
#include <time.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h> 
#include <Adafruit_SSD1306.h> 

// wifi settings
const char* ssid = "your-wifi-name";
const char* password = "your-wifi-password";

char date_data[11] = {0};
int time_data[3] = {0};

// OLED settings
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET     -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

bool alarm_state = false;

void NTP_Server_task(void * pvParam)
{
  while(1)
  {
    struct tm timeinfo;
    if(!getLocalTime(&timeinfo))
    {
      Serial.println("can not get time data.");
      return;
    }

    strftime(date_data,11,"%Y/%m/%d",&timeinfo);

    char time_load[5] = {0};
    strftime(time_load,5,"%H",&timeinfo);
    time_data[0] = atof(time_load);
    strftime(time_load,5,"%M",&timeinfo);
    time_data[1] = atof(time_load);
    strftime(time_load,5,"%S",&timeinfo);
    time_data[2] = atof(time_load);

    Serial.println(date_data);
    for (int i=0;i<3;i++)
    {
      Serial.print(time_data[i]);
      if (i!=2)
        Serial.print(":");
      else
        Serial.print("\n");
    }
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void OLED_task(void * pvParam)
{
  // Detect if the OLED is installed correctly
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  display.clearDisplay();
  delay(1000);

  while(1)
  {
    if(alarm_state == false)
    {
      display.clearDisplay();
      display.setTextSize(2);
      display.setTextColor(1);
      display.setCursor(5,10); 
      display.print(date_data); 
      display.setCursor(18,40);
      for (int i=0;i<3;i++)
      {
        if (time_data[i]<10)
            display.print("0");
        display.print(time_data[i]);
        if (i!=2)
          display.print(":");
      }
      display.display();
      vTaskDelay(pdMS_TO_TICKS(1000));
    }
    else
    {
      display.clearDisplay();
      display.setTextSize(2);
      display.setTextColor(1);
      display.setCursor(5,10);
      display.print("C:\\>_");
      display.display();
      vTaskDelay(pdMS_TO_TICKS(500));
      display.clearDisplay();
      display.setTextSize(2);
      display.setTextColor(1);
      display.setCursor(5,10);
      display.print("C:\\>");
      display.display();
      vTaskDelay(pdMS_TO_TICKS(500));
    }
  }
}

void main_task(void * pvParam)
{
  pinMode(13, INPUT); // button set
  pinMode(12, OUTPUT); // buzzer set
  while(1)
  {
    // time setting 6:30:0 ~ 6:30:2 clock will ring
    if (digitalRead(13) == HIGH && time_data[0] == 6 && time_data[1] == 30 && time_data[2] < 3)
      alarm_state = true;
    while(alarm_state == true)
    {
      digitalWrite(12,HIGH);
      delay(100);
      digitalWrite(12,LOW);
      if (digitalRead(13) == LOW)
      {
        digitalWrite(12,LOW);
        alarm_state = false;
        break;
      }
      vTaskDelay(pdMS_TO_TICKS(500));
    }
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void setup()
{
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid,password);
  Serial.println("");
  while(WiFi.status()!=WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }
  Serial.print("IP address:");
  Serial.println(WiFi.localIP());
  
  configTime(28800,0,"pool.ntp.org");
  delay(1000);

  xTaskCreate(NTP_Server_task,"NTP_Server_task",5000,NULL,1,NULL); // time task
  xTaskCreate(OLED_task,"OLED_task",5000,NULL,1,NULL); // oled task
  xTaskCreate(main_task,"main_task",1500,NULL,1,NULL); // main task
}

void loop() {}