//#include <Adafruit_NeoPixel.h>
#include <Arduino.h>
#include <FastLED.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <string>
#include <DHT.h>

using namespace std; 
#define WIFI_SSID       "Asus10"
#define WIFI_PASSWORD   "pin34ok3"
#define BOT_TOKEN       "7351017309:AAFgWCeENL4lWavu56E4wYEWIAFYZPpmkkc"

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
const long interval = 2000;
const long displayInterval = 1000;

bool showTemperature = true;
bool autoBrightness = false;


int bri = 0;
int humidity = 0;
int temp = 0;
String mode = "static";   // Режим по умолчанию
String color = "#ff0000";  // Цвет по умолчанию
uint8_t current_bri = 0;

void UpdateFoto();
void UpdateTMP();

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

enum DATA {
  HUM,
  TEMP,
  BRIGHT
};


// ---------------------------------------------- Кодя для дисплея ----------------------------------------------

const unsigned long BOT_MTBS = 100;
X509List cert(TELEGRAM_CERTIFICATE_ROOT);
WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);
unsigned long bot_lasttime;

int ledStatus = 0;
void handleNewMessages(int numNewMessages) {
  for (int i = 0; i < numNewMessages; i++) {
    String chat_id = bot.messages[i].chat_id;
    String text = bot.messages[i].text;
    String from_name = bot.messages[i].from_name;
    // if (from_name == "")
    // from_name = "Guest";
    Serial.print("[Message text] "); Serial.println(text);
    Serial.print("[Name text] "); Serial.println(from_name);
    Serial.println();

    if (text == "/gettemperature") {
      bot.sendMessage(chat_id, (String("Температура: ") + String(temp) + String("℃")), "");
    }
    if (text == "/gethumidity") {
      bot.sendMessage(chat_id, (String("Влажность: ") + String(humidity) + String("%")), "");
    }
    if (text == "/getbrightness") {
      bot.sendMessage(chat_id, (String("Яркость: ") + String(bri) + String('%')), "");
    }
    if (text == "/setautobrightness") {
      autoBrightness = !autoBrightness;
      if (autoBrightness) {
        bot.sendMessage(chat_id, "Включен режим автоматического изменения яркости", "");
      } else {
        bot.sendMessage(chat_id, "Активировано ручное управление яркостью", "");
      }
    }
    if (text == "/setcolorwhite") {
      bot.sendMessage(chat_id, "Цвет изменён на белый", "");
      mode = "rgb";
      selectedColor = CRGB::White; 
    }
    if (text == "/setcolorred") {
      bot.sendMessage(chat_id, "Цвет изменён на красный", "");
      mode = "rgb";
      selectedColor = CRGB::Red;
  }
    if (text == "/setcolorgreen") {
      bot.sendMessage(chat_id, "Цвет изменён на зелёный", "");
      mode = "rgb";
      selectedColor = CRGB::Green;
    }
    if (text == "/setcolorblue"){
      bot.sendMessage(chat_id, "Цвет изменён на синий", "");
      mode = "rgb";
      selectedColor = CRGB::Blue;
    }
    if (text == "/setraindow") {
      bot.sendMessage(chat_id, "Включен режим радуги", "");
      mode = "rainbow";
    }
    if (text == "/upbrightness") {
      current_bri += 10;
      bot.sendMessage(chat_id, "Яркость увеличина на 10%\nТеущий уровень яркости: " + String(current_bri) + "%", "");
      if (current_bri >= 100) current_bri = 0;
    }
    if (text == "/downbrightness") {
      current_bri -= 10;
      bot.sendMessage(chat_id, "Яркость уменьшена на 10%\nТеущий уровень яркости: " + String(current_bri) + "%", "");
      if (current_bri <= 0) current_bri = 100;
      
    }
  }
}

void setup() {
  Serial.begin(9600);
  
  // Инициализация дисплея TM1637
  display.setBrightness(0x0f);  // Максимальная яркость
  
  
  // Инициализация ленты WS2812B
  FastLED.addLeds<LED_TYPE, LED_PIN_1, COLOR_ORDER>(leds, NUM_LEDS_1);

  
  Serial.println();
  configTime(0, 0, "pool.ntp.org");
  secured_client.setTrustAnchors(&cert);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  Serial.print("WiFi connected. IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Retrieving time: ");
  time_t now = time(nullptr);
  while (now < 24 * 3600) {
    delay(100);
    now = time(nullptr);
  }

  dht_14.begin();


}

void loop() {
  if (millis() - bot_lasttime > BOT_MTBS) {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    while (numNewMessages) {
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
  bot_lasttime = millis();
  }


  // ---------------------------------------------- Кодя для дисплея ----------------------------------------------
  UpdateTMP();
  // ---------------------------------------------- Кодя для дисплея ----------------------------------------------
  UpdateFoto();

}


void UpdateFoto() {
  bri = map(analogRead(FOTO), 0, 1023, 0, 100);
  

  if (autoBrightness) {
    bri = map(bri, 0, 100, 255, 0);
    current_bri = bri;
  }
  

  unsigned long currentMillis = millis();

  if (mode == "static") {
    mood = MODE::TEMP_SET;
  } else if (mode == "rgb") {
    mood = MODE::RGB_SET;
  } else if (mode == "rainbow") {
    mood = MODE::RADUGA_MODE;
  }

  switch (mood)
  {
  case MODE::TEMP_SET: {
    for (int i = 0; i < NUM_LEDS_1; i++) {
      // Преобразование температуры в цвет (пример, можно изменить по вкусу)
      uint8_t red = map(temp, 15, 30, 0, 255);
      uint8_t green = 0;
      uint8_t blue = map(temp, 15, 30, 255, 0);
      
      leds[i] = CRGB(red, green, blue);

      // Преобразование уровня освещенности в яркость
      
      leds[i].nscale8_video(bri);
    }

    // Обновление состояния светодиодов
    FastLED.setBrightness(current_bri);
    FastLED.show();
    break;
  }

  case MODE::RGB_SET: {
    // CRGB selectedColor = CRGB::Black;
    // long number = strtol(&color[1], NULL, 16);  // Преобразование HEX в long
    // selectedColor = CRGB((number >> 16) & 0xFF, (number >> 8) & 0xFF, number & 0xFF);
    
    for (int i = 0; i < NUM_LEDS_1; i++) {
      leds[i] = selectedColor;
    }
    FastLED.setBrightness(current_bri);
    FastLED.show();
    break;
  }

  case MODE::RADUGA_MODE: {
    fill_rainbow(leds, NUM_LEDS_1, currentMillis / 10, 7);
    FastLED.setBrightness(current_bri);
    FastLED.show();
    break;
  }
  default:
    break;
  }
  
}

void UpdateTMP() {
  unsigned long currentMillis = millis();
  humidity = dht_14.readHumidity();
  temp = dht_14.readTemperature();

  // Проверка на ошибки чтения
  if (isnan(humidity) || isnan(temp)) {
    Serial.println("Ошибка чтения с датчика DHT11!");
    return;
  } 

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
