////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////               Name: "Developer-RU"                       //////////////////////////////////////||||||||||||//////////////////////////////////////
//////////////////////////////////////////////////////        ok     Email: p.masyukov@gmail.com                //////////////////////////////////////||||||||||||//////////////////////////////////////
//////////////////////////////////////////////////////               DateTime: 03.09.2016                       //////////////////////////////////////||||||||||||//////////////////////////////////////                                                                                                                
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <wiring.c>
#include <LiquidCrystal.h>

#define SENSOR_A    2                    // Пин датчика холла А - анемометра
#define SENSOR_B    3                    // Пин датчика холла В - датчика лопастей
#define INTERVAL 1000                    // Частота обновления информации на дисплее - в миллисекундах

#define LIGHT_PIN 10                     // Пин подсветки дисплея
#define CONTRAST_PIN 13                  // Пин контрастности дисплея

#define SCOUNT_A    3                   // Колличество магнитов на оборот датчика холла А - анемометра
#define SCOUNT_B    4                    // Колличество магнитов на оборот датчика холла В - датчика лопастей

#define DRIVER_A    30                   // Пин привода отк лопастей - реле 1
#define DRIVER_B    32                   // Пин привода закр лопастей - реле 2
#define MOTOR_A     28                   // Пин двигателя лопастей - реле 3
#define MOTOR_B     26                   // Пин двигателя лопастей - реле 4

#define RESET_PIN 40    /// Пин сброса

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////     Начало настройки    //////////////////////////////////////////////////////////////////////////////||||||||||||///////////////////////////////                                                                                                                
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define LIGHT_VALUE 130                  // Величина яркости дисплея
#define CONTRAST_VALUE 50               // Величина контрастности дисплея

#define SPEED_RPM    3                   // Число полных оборотов датчика A для рассчета скорости ветра (тут для примера 5 полных оборота равны 1 м/сек - точность вычислений зависит от колличества магнитов/лопастей SCOUNT_)
#define DRIVER_HIGH_LOW 5000            // Продолжительность включения реле привода отк|закр лопастей(2 минуты) - в миллисекундах
#define MOTOR_HIGH_LOW 5000             // Продолжительность включения реле двигателя лопастей (3 сек) - в миллисекундах

#define SPEED_WIN    1                   // скорость ветра------

#define MIN_RPM 230                      // Минимальные обороты генератора
#define MAX_RPM 270                      // Минимальные обороты генератора

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////     Конец настройки   /////////////////////////////////////////////////////////////////////////||||||||||||//////////////////////////////////////                                                                                                                
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

LiquidCrystal lcd(9, 8, 7, 6, 5, 4);

int rpm_a = 0, rpm_b = 0;                // Текущие обороты
volatile int counter_a = 0, counter_b = 0;       // Счетчики импульсов
volatile int speed_a = 0;                // Тукущая скорость

unsigned long currentTime = 0; 
unsigned long loopTime = 0;

unsigned long driver_on_time = 0;
unsigned long motor_on_time = 0;
unsigned long driver_off_time = 0;
unsigned long motor_off_time = 0;

unsigned long works_time = 0;


void read_frequency_a() { counter_a++; }

void read_frequency_b() { counter_b++; }

void full_reset() { if(works_time >= 86400) digitalWrite(RESET_PIN, LOW ); }

void output() {
    // Отключаем подсчет оборотов
    detachInterrupt(0);
    detachInterrupt(1);

    // Производим все математические рассчеты 
    rpm_a = round(counter_a/SCOUNT_A);
    rpm_b = round( (counter_b *60 )/SCOUNT_B);
    speed_a = round((rpm_a) / SPEED_RPM);
    
    lcd.clear();
    lcd.setCursor(0, 0); lcd.print("RPM");
    lcd.setCursor(4, 0); lcd.print(rpm_a);
    lcd.setCursor(10, 0); lcd.print(rpm_b); 
    lcd.setCursor(0, 1); lcd.print("SPD");
    lcd.setCursor(4, 1); lcd.print(speed_a);
    
    // Сбрасываем данные
    counter_a = counter_b = 0;

    works_time++;
            
    // Включаем подсчет оборотов
    attachInterrupt(0, read_frequency_a, FALLING );
    attachInterrupt(1, read_frequency_b, FALLING );
}

void setup() {
    
    lcd.begin(16, 2);
    
    pinMode(SENSOR_A, INPUT_PULLUP );
    pinMode(SENSOR_B, INPUT_PULLUP );

    pinMode(DRIVER_A, OUTPUT );
    pinMode(DRIVER_B, OUTPUT );
    pinMode(MOTOR_A, OUTPUT );
    pinMode(MOTOR_B, OUTPUT );

    pinMode(RESET_PIN, OUTPUT );
    digitalWrite(RESET_PIN, HIGH );

    digitalWrite(DRIVER_A, HIGH);
    digitalWrite(DRIVER_B, HIGH);
    digitalWrite(MOTOR_A, HIGH);
    digitalWrite(MOTOR_B, HIGH);
    
    pinMode(CONTRAST_PIN, OUTPUT); 
    pinMode(LIGHT_PIN, OUTPUT); 
      
    analogWrite(CONTRAST_PIN, CONTRAST_VALUE);
    analogWrite(LIGHT_PIN, LIGHT_VALUE);

    // Включаем подсчет оборотов
    attachInterrupt(0, read_frequency_a, FALLING );
    attachInterrupt(1, read_frequency_b, FALLING );
}


void loop() {
    while(1) {    
        currentTime = millis(); 
        int pol = 0;
        // Подсчет и вывод на LCD
        if (currentTime >= (loopTime + INTERVAL)) { speed_a = 0; output(); loopTime = currentTime; }
        
         // Если скорсть ветра выше чем 3 м/с
        if( speed_a >= SPEED_WIN ) {      
            driver_off_time = 0; // Сбрасываем счетчик реле DRIVER_B
            digitalWrite(DRIVER_B, HIGH);  // Выключаем выход B привода открытия лопастей на 2 мин.

            if(driver_on_time == 0) {
                driver_on_time = currentTime;
                motor_off_time = 0;
                motor_on_time = 0;

                digitalWrite(DRIVER_A, LOW);  // Включаем выход A привода открытия лопастей на 2 мин.
            } else if (driver_on_time > 0 && driver_on_time + DRIVER_HIGH_LOW < currentTime) {
                digitalWrite(DRIVER_A, HIGH); // Выключаем выход A привода открытия лопастей
               
                // необходимо удержать диапозон 250 об/мин
                int pol = 0;
                
                // Если обороты меньше минимальных
                if(rpm_b < MIN_RPM && pol == 0) {
                    // На всякий случай подаем сигнал на отключение MOTOR_B
                    digitalWrite(MOTOR_B, HIGH);
                    pol = 1;
                                           
                    if(motor_on_time == 0)
                    {
                        motor_off_time = 0;
                        motor_on_time = currentTime;
                        // включаем выход A двигателя лопастей на 3 секунды
                        digitalWrite(MOTOR_A, LOW);
                    } else if (motor_on_time > 0 && motor_on_time + MOTOR_HIGH_LOW < currentTime) {
                        // выключаем выход A двигателя лопастей на 3 секунды
                        digitalWrite(MOTOR_A, HIGH);
                        motor_on_time = 0;
                    }
                }

                //если обороты нормализовались
                if(rpm_b >= MIN_RPM && rpm_b <= MAX_RPM)
                {
                    digitalWrite(MOTOR_A, HIGH);
                    digitalWrite(MOTOR_B, HIGH);
                }
                
                // Если обороты больше максимальных
                if(rpm_b > MAX_RPM && pol == 0) {
                    // На всякий случай подаем сигнал на отключение MOTOR_A
                    digitalWrite(MOTOR_A, HIGH);
                    pol = 1;
                                       
                    if(motor_off_time == 0)
                    {
                        motor_on_time = 0;
                        motor_off_time = currentTime;
                        // включаем выход B двигателя лопастей на 3 секунды
                        digitalWrite(MOTOR_B, LOW);
                    } else if (motor_off_time > 0 && motor_off_time + MOTOR_HIGH_LOW < currentTime) {
                        // вsключаем выход B двигателя лопастей на 3 секунды
                        digitalWrite(MOTOR_B, HIGH);
                        motor_off_time = 0; 
                    }
                }

                
            }
        }
        
        // Если скорсть ветра ниже чем 3 м/с
        if( speed_a < SPEED_WIN ) {    
            driver_on_time = 0; // Сбрасываем счетчик реле DRIVER_A
            digitalWrite(DRIVER_A, HIGH); // Включаем выход A привода открытия лопастей на 2 мин.              

            motor_off_time = 0; motor_on_time = 0;

            digitalWrite(MOTOR_A, HIGH); // если резко упадет 
            digitalWrite(MOTOR_B, HIGH); // если резко упадет 
            
            if(driver_off_time == 0) {
                driver_off_time = currentTime; 
                digitalWrite(DRIVER_B, LOW); // Включаем выход B привода открытия лопастей на 2 мин.              
            } else if (driver_off_time > 0 && driver_off_time + DRIVER_HIGH_LOW < currentTime) {
                digitalWrite(DRIVER_B, HIGH); // Выключаем выход B привода открытия лопастей на 2 мин. 
                                
                full_reset();      
            }
        }

        pol = 0;    

    }
}
