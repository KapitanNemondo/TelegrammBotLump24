//#include <Adafruit_NeoPixel.h>
#include <Arduino.h>
#include <FastLED.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <FastBot.h>
//#include <string>
#include <DHT.h>

//using namespace std; 

#define WIFI_SSID       "Asus10"
#define WIFI_PASS       "pin34ok3"
#define BOT_TOKEN       "7351017309:AAFgWCeENL4lWavu56E4wYEWIAFYZPpmkkc"

FastBot bot(BOT_TOKEN);

// Порты
#define DIR         5
#define CLK         4
#define FOTO        A0
#define DHTPIN      D5
#define LED_PIN_1   D7

// ---------------------------------------------- Кодя для дисплея ----------------------------------------------
#include <TM1637Display.h> 
TM1637Display display(CLK, DIR);

unsigned long previousMillis = 0;
unsigned long displayPreviousMillis = 0;
unsigned long rainMillis = 0;
const long displayInterval = 1000;

bool showTemperature = true;
bool autoBrightness = false;


int bri = 0;
int humidity = 0;
int temp = 0;
uint8_t current_bri = 0;

void UpdateFoto();
void UpdateTMP();
void smoothBlinkYellow();
void blinkGreenFourTimes();
void newMsg(FB_msg& msg);
void Raduga();

DHT dht_14(DHTPIN, DHT11);

// Параметры ленты
#define NUM_LEDS_1    20
#define LED_TYPE      WS2812B
#define COLOR_ORDER   GRB

CRGB leds[NUM_LEDS_1];

CRGB selectedColor = CRGB::Black;

enum MODE {
  TEMP_SET,
  RGB_SET,
  RADUGA_MODE
} mood;


// ---------------------------------------------- Кодя для дисплея ----------------------------------------------

const unsigned long BOT_MTBS = 50;

unsigned long bot_lasttime;

int ledStatus = 0;


void setup() {
  Serial.begin(9600);
  
  // Инициализация дисплея TM1637
  display.setBrightness(0x0f);  // Максимальная яркость
  
  
  // Инициализация ленты WS2812B
  //FastLED.addLeds<LED_TYPE, LED_PIN_1, COLOR_ORDER>(leds, NUM_LEDS_1);
  FastLED.addLeds<LED_TYPE, LED_PIN_1, COLOR_ORDER>(leds, NUM_LEDS_1);

  
  Serial.println();
  configTime(0, 0, "pool.ntp.org");

  // Serial.println("[Wi-Fi Name] " + String(WIFI_SSID));
  // Serial.println("[Wi-Fi Pass] " + String(WIFI_PASS));

  // delay(2000);

  WiFi.begin(WIFI_SSID, WIFI_PASS);

  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    smoothBlinkYellow();
  }

  Serial.print("WiFi connected. IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Retrieving time: ");
  time_t now = time(nullptr);
  while (now < 24 * 3600) {
    delay(100);
    now = time(nullptr);
    
  }

  blinkGreenFourTimes();

  bot.attach(newMsg);


  dht_14.begin();


}

void loop() {

  bot.tick();
  // Обновление данных с сенсоров и дисплея
  UpdateTMP();
  UpdateFoto();
}



void UpdateFoto() {
  bri = map(analogRead(FOTO), 0, 1023, 0, 100);

  if (autoBrightness) {
    current_bri = bri;
  }

  unsigned long currentMillis = millis();

  switch (mood) {
    case MODE::TEMP_SET: {
      for (int i = 0; i < NUM_LEDS_1; i++) {
        uint8_t red = map(temp, 15, 30, 0, 255);
        uint8_t blue = map(temp, 15, 30, 255, 0);
        leds[i] = CRGB(red, 0, blue);
        leds[i].nscale8_video(bri);
      }
      break;
    }
    case MODE::RGB_SET: {
      fill_solid(leds, NUM_LEDS_1, selectedColor);
      break;
    }
    case MODE::RADUGA_MODE: {
      fill_rainbow(leds, NUM_LEDS_1, millis() / 10, 7);
      //Raduga();
      rainMillis = currentMillis;
      break;
    }
    default:
      break;
  }
  FastLED.setBrightness(map(current_bri, 0, 100, 0, 255));
  FastLED.show();
}

void UpdateTMP() {
  unsigned long currentMillis = millis();
  humidity = dht_14.readHumidity();
  temp = dht_14.readTemperature();

  // Проверка на ошибки чтения
  // if (isnan(humidity) || isnan(temp)) {
  //   Serial.println("Ошибка чтения с датчика DHT11!");
  //   return;
  // } 

    // Обновление данных на дисплее TM1637
  if (currentMillis - displayPreviousMillis >= displayInterval) {
    displayPreviousMillis = currentMillis;
    display.clear();  // Очистка дисплея перед отображением новых данных

    if (showTemperature) {
      display.showNumberDec((int)temp);
    } else {
      display.showNumberDec((int)humidity);
    }

    showTemperature = !showTemperature;  // Переключение между показом температуры и влажности
  }
};


// Функция плавного мигания желтого цвета
void smoothBlinkYellow() {
  static bool flag = true;
  
  if (flag) {
    fill_solid(leds, NUM_LEDS_1, CRGB::Yellow);
    FastLED.setBrightness(255);
    FastLED.show();
  } else {
    fill_solid(leds, NUM_LEDS_1, CRGB::Black);
    FastLED.show();
  }

  flag = !flag;
}

// Функция мигания зеленым цветом
void blinkGreenFourTimes() {
  for (int i = 0; i < 4; i++) {
    fill_solid(leds, NUM_LEDS_1, CRGB::Green);
    FastLED.setBrightness(255);
    FastLED.show();
    delay(250); // Задержка 250 мс для включенного состояния

    fill_solid(leds, NUM_LEDS_1, CRGB::Black);
    FastLED.show();
    delay(250); // Задержка 250 мс для выключенного состояния
  }
}

void Raduga() {
  static byte counter = 0;
  for (int i = 0; i < NUM_LEDS_1; i++) {
    leds[i].setHue(counter + i * 255 / NUM_LEDS_1);
  }
  counter++;        // counter меняется от 0 до 255 (тип данных byte)
}

void newMsg(FB_msg& msg) {

  if (msg.text == "/gettemperature") {
    bot.sendMessage((String("Температура: ") + String(temp) + String("℃")), msg.chatID);

  } else if (msg.text == "/gethumidity") {
    bot.sendMessage((String("Влажность: ") + String(humidity) + String("%")), msg.chatID);

  }else if (msg.text == "/getbrightness") {
    bot.sendMessage((String("Яркость: ") + String(bri) + String('%')), msg.chatID);

  } else if (msg.text == "/setautobrightness") {
    autoBrightness = !autoBrightness;
    if (autoBrightness) {
      bot.sendMessage("Включен режим автоматического изменения яркости", msg.chatID);
    } else {
      bot.sendMessage("Активировано ручное управление яркостью", msg.chatID);
    }
  } else if (msg.text == "/setcolorwhite") {
    bot.sendMessage("Цвет изменён на белый", msg.chatID);
    mood = MODE::RGB_SET;
    selectedColor = CRGB::White; 

  } else if (msg.text == "/setcolorred") {
    bot.sendMessage("Цвет изменён на красный", msg.chatID);
    mood = MODE::RGB_SET;
    selectedColor = CRGB::Red;

  } else if (msg.text == "/setcolorgreen") {
    bot.sendMessage("Цвет изменён на зелёный", msg.chatID);
    mood = MODE::RGB_SET;
    selectedColor = CRGB::Green;

  } else if (msg.text == "/setcolorblue"){
    bot.sendMessage("Цвет изменён на синий", msg.chatID);
    mood = MODE::RGB_SET;
    selectedColor = CRGB::Blue;

  }else if (msg.text == "/setraindow") {
    bot.sendMessage("Включен режим радуги", msg.chatID);
    mood = MODE::RADUGA_MODE;

  }else if (msg.text == "/upbrightness") {
    current_bri = min(current_bri + 10, 100);  // Ограничение яркости до 100%
    bot.sendMessage("Яркость увеличена на 10%\nТекущий уровень яркости: " + String(current_bri) + "%", msg.chatID);

  }else if (msg.text == "/downbrightness") {
    current_bri = max(current_bri - 10, 0);  // Ограничение яркости до 0%
    bot.sendMessage("Яркость уменьшена на 10%\nТекущий уровень яркости: " + String(current_bri) + "%", msg.chatID);

  } else {
    bot.sendMessage("Введена не существующая команда", msg.chatID);
  }
  // if (text[0] == '#') {
  //   //# 000 000 000
  //   string customR = "";
  //   string customG = "";
  //   string customB = "";
  //   for (int i = 2; i <= 4; i++)
  //   {
  //     customR += text[i];
  //   }    
  //   for (int i = 6; i <= 8; i++)
  //   {
  //     customG += text[i];
  //   }   
  //   for (int i = 10; i <= 12; i++)
  //   {
  //     customB += text[i];
  //   }
  //}
  Serial.print("[Mode] "); Serial.println(mood);
  Serial.print("[Current bri] "); Serial.println(current_bri);
  Serial.print("[Bri data] "); Serial.println(bri);
  Serial.println();
}