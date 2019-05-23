/*
    скетч метеостанции с выводом на LCD дисплей и сигнализацией RGB диодом
    датчик температуры и влажности DHT11
    модуль времени ds1302
    дисплей подключен через I2C конвертер
*/
#include <iarduino_RTC.h>//библиотека часов
#include <Wire.h>//библиотека конвертера i2c
#include <DHT.h>//библиотека датчика температуры и влажности
#include <LiquidCrystal_I2C.h> //библиотека дисплея через I2C
#define DHTPIN 2 //номер пина DHT11

DHT dht(DHTPIN, DHT11); //Инициация датчика
iarduino_RTC time(RTC_DS1302, 8, 9, 10); //пины часов RST, CLK, DAT
LiquidCrystal_I2C lcd(0x27, 16, 2); // Указываем I2C адрес (наиболее распространенное значение), а также параметры экрана (в случае LCD 1602 - 2 строки по 16 символов в каждой

//----Задаём номера пинов----
//
byte rPIN = 4;//пин красного диода
byte gPIN = 7;//пин зеленого диода
byte lcdbackligthPin = 11;//пин подсветки, ШИМ
byte photoresistorPin = A0;//пин фоторезистора
unsigned long prevMillis1s = 0;//интервал 1сек
unsigned long prevMillis60s = 0;//интервал 60сек
unsigned long prevMillis300s = 0;//интервал 300сек

void setup()
{
  //Serial.begin(9600);//инициализация COM-порта, для отладки
  lcd.init();                      // Инициализация дисплея
  lcd.backlight();                 // Подключение подсветки
  lcd.clear();//очистка дисплея
  dht.begin();//инициализация датчика температуры
  time.begin();//инициализация CRT модуля
  //----Настраиваем режимы работы пинов----
  pinMode(rPIN, OUTPUT);
  pinMode(gPIN, OUTPUT);
  pinMode(lcdbackligthPin, OUTPUT);
  pinMode(photoresistorPin, INPUT);
}

void loop()
{
  backligth();
  timeOutput();
  weatherOutput();
}

//----//функция плавной регулировки подсветки по фоторезистору//----//
void backligth()
{

  // проверяем – настало ли время изменить состояние
  // для этого берём разность между текущим временем и
  // временем последнего изменения состояния,
  // а затем сверяем полученный интервал с нужным интервалом
  if (millis() - prevMillis1s > 1000) {//чем меньше значение там плавнее происходит изменение интенсивности подсветки
    prevMillis1s = millis(); //запоминаем время срабатываия

    int photoRes = 1023 - analogRead(photoresistorPin);//переменная для инвертирования показаний с фотодатчика
    //в дальнейшем инвертированые данные проще конвертировать в PWM-сигнал(ШИМ)

    //Преобразуем полученное значение в уровень PWM-сигнала.
    //Чем меньше значение освещенности, тем меньше мощности мы должны подавать на светодиод через ШИМ.
    int ledPower = map(photoRes, 0, 1023, 0, 255);
    //Serial.println(ledPower);//для отладки, вывод в монитор порта
    if (ledPower <= 10) {
      //добавим немного подсветки при низких показаниях датчика, чтобы в полной темноте подсветка совсем не отключалась
      ledPower = ledPower + 5;
      analogWrite(lcdbackligthPin, ledPower);
    } else {
      analogWrite(lcdbackligthPin, ledPower);
    }
  }
}

//-----------------//функция вывода времени//---------------------------//
void timeOutput() {
  /*
       сначала выводим время циклом do, после чего начинаем проверку интервала обновления
       контроль времени аналогичен функции backligth() с поправкой на цикл do
  */
  do {
    prevMillis60s = millis(); //запоминаем время срабатываия
    //Вывод времени на LCD
    lcd.setCursor(0, 0);//установка курсора. в скобках первая цифра номер символа, вторая - номер строки
    lcd.print("TIME:");
    lcd.setCursor(5, 0);
    lcd.print(time.gettime("H:i")); // выводим время
  } while (millis() - prevMillis60s > 60000);
}

//-------------------//функция вывода погоды//------------------------------//
void weatherOutput() {
  /*
         сначала данные датчика циклом do, после чего начинаем проверку интервала обновления
         контроль времени аналогичен функции backligth() с поправкой на цикл do
  */
  do {
    prevMillis300s = millis(); //запоминаем время срабатываия

    byte h = dht.readHumidity(); //Измеряем влажность
    byte t = dht.readTemperature(); //Измеряем температуру

    if (isnan(h) || isnan(t))
    { // Проверка. Если не удается считать показания, выводится «Ошибка считывания», и программа завершает работу
      lcd.setCursor(0, 0);
      lcd.print("ERRoR");
    }
    else
    {

      lcd.setCursor(0, 1);
      lcd.print("TEMP:");
      lcd.setCursor(5, 1);
      lcd.print(t);
      lcd.setCursor(7, 1);
      lcd.print("c");
      lcd.setCursor(9, 1);
      lcd.print("WET:");
      lcd.setCursor(13, 1);
      lcd.print(h);
      lcd.setCursor(15, 1);
      lcd.print("%");

      if ((t > 18 && t < 25) && (h > 40 && h < 65)) //проверяем диапазоны температуры и влажности 18<Т<25, 40<Вл<60
      {
        digitalWrite(rPIN, LOW);
        digitalWrite(gPIN, HIGH);
        lcd.setCursor(12, 0);
        lcd.print("GOOD");
      } else {
        digitalWrite(gPIN, LOW);
        digitalWrite(rPIN, HIGH);
        lcd.setCursor(12, 0);
        lcd.print("BAD");
      }
    }
  } while (millis() - prevMillis300s > 300000);
}
