#define Gerkon 1 // 1 вкл. геркон 0 выкл
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

//LiquidCrystal_I2C lcd(0x3F, 16, 2); //Адрес экрана
LiquidCrystal_I2C lcd(0x27, 16, 2);

int pinSolderOut = 5;  // Выход для паяльника
int pinSolderIn = A0;  // Потенциометр паяльника
int pinSolderTCouple = A7;  // Термопара паяльника
int pinSolderButton = 13;  // Кнопка вкл. и выкл. паяльника

int pinHotAirOut = 6;  // Выход для фена
int pinHotAirIn = A1;  // Потенциометр фена
int pinHotAirTCouple = A6;  // Термопара фена
int pinHotAirCoolerOut = 9;  // Выход для вентилятора фена ( PWM )
int pinHotAirCoolerIn = A2;  // Потенциометр вентилятора фена
int pinHotAirButton = 2;  // Кнопка вкл.и выкл. фена
int pinGerkon = A3; // Геркон фена 28ая нога
int pinRele = 3; //На управление реле(13я нога меги)
int work_fen = 0; //правление феном
int SolderInAverage[10] = {0,0,0,0,0,0,0,0,0,0};
int HotAirInAverage[10] = {0,0,0,0,0,0,0,0,0,0};
int SolderTCoupleAverage[10] = {0,0,0,0,0,0,0,0,0,0};
int HotAirTCoupleAverage[10] = {0,0,0,0,0,0,0,0,0,0};
int count_aver=10; //Количество значений для среднего
int Cooler = 1;
int Cooler_ON = 2;

byte f0[8] = { 0b00000, 0b00000, 0b11001, 0b01011, 0b00100, 0b11010, 0b10011, 0b00000 }; //вентилятор
byte f1[8] = { 0b00000, 0b00000, 0b11000, 0b01000, 0b00100, 0b00010, 0b00011, 0b00000 };
byte f2[8] = { 0b00000, 0b00000, 0b00100, 0b00110, 0b00100, 0b01100, 0b00100, 0b00000 };
byte f3[8] = { 0b00000, 0b00000, 0b00001, 0b00011, 0b00100, 0b11000, 0b10000, 0b00000 };
byte f4[8] = { 0b00000, 0b00000, 0b00000, 0b01000, 0b11111, 0b00010, 0b00000, 0b00000 };
byte fy[8] = { 0b10001, 0b10001, 0b11001, 0b10101, 0b10101, 0b10101, 0b11001, 0b00000 }; //ы
byte fd[8] = { 0b00001, 0b00010, 0b00100, 0b01111, 0b00010, 0b00100, 0b01000, 0b10000 }; // молния

void setup()
{
  TCCR1B = TCCR1B & 0b11111000 | 0x02; //кулер фена 24v. Частота ШИМ 11 и 3
  pinMode(pinSolderOut, OUTPUT);
  pinMode(pinSolderButton, INPUT);
  pinMode(pinHotAirOut, OUTPUT);
  pinMode(pinHotAirButton, INPUT);
  pinMode(pinGerkon, INPUT);
  pinMode(pinRele, OUTPUT);

  lcd.begin(16, 2);
  lcd.init();
  lcd.backlight();
  lcd.createChar(0, f0);
  lcd.createChar(1, f1);
  lcd.createChar(2, f2);
  lcd.createChar(3, f3);
  lcd.createChar(4, f4);
  lcd.createChar(5, fd);
  lcd.createChar(6, fy);
}

void loop()
{

  if (Gerkon == 1) //проверка геркона
  {
    digitalRead(pinGerkon);
  }
  else {
    pinGerkon = HIGH; //откл геркон
  }

  // Преобразовываем значения
  int setSolderTemp = map(analogRead(pinSolderIn), 0, 1023, 0, 480);             //регулировка паяльника
  int solderTCouple = map(analogRead(pinSolderTCouple), 0, 750, 0, 480);         //показания
  int setHotAirTemp = map(analogRead(pinHotAirIn), 0, 1023, 0, 480);             //регулировка фена
  int hotAirTCouple = map(analogRead(pinHotAirTCouple), 0, 750, 0, 480);         //показания
  int setHotAirCooler = map(analogRead(pinHotAirCoolerIn), 0, 1023, 130, 255);
  int displayHotAirCooler = map(analogRead(pinHotAirCoolerIn), 0, 1023, 0, 99);

  // Защита, если не работает термопара
  if (solderTCouple > 481) {
    setSolderTemp = 0;
  }
  if (hotAirTCouple > 481) {
    setHotAirTemp = 0;
  }

// расчет десяти последних средних показателей дя сглаживания показателей
// если разница в показанияъ меньше 10 
// определение вращение потенциометра 
  for (int i=0; i < count_aver-1; i++){
    if (abs(SolderInAverage[i]-setSolderTemp) >10) {SolderInAverage[i]=setSolderTemp;}
        else {SolderInAverage[i] = SolderInAverage[i+1];}
    if (abs(HotAirInAverage[i]-setHotAirTemp) >10) {HotAirInAverage[i]=setHotAirTemp;}
        else {HotAirInAverage[i] = HotAirInAverage[i+1];}
    SolderTCoupleAverage[i] = SolderTCoupleAverage[i+1];
    HotAirTCoupleAverage[i] = HotAirTCoupleAverage[i+1];
    }

  SolderInAverage[count_aver-1] = setSolderTemp; 
  HotAirInAverage[count_aver-1] = setHotAirTemp; 
  SolderTCoupleAverage[count_aver-1] = solderTCouple;
  HotAirTCoupleAverage[count_aver-1] = hotAirTCouple;

  int SolderInWork = 0;
  int HotAirInWork = 0;
  int SolderTCoupleWork = 0;
  int HotAirTCoupleWork = 0;

  for (int i=0; i < count_aver; i++){
    SolderInWork = SolderInWork + SolderInAverage[i];
    HotAirInWork = HotAirInWork + HotAirInAverage[i];
    SolderTCoupleWork = SolderTCoupleWork + SolderTCoupleAverage[i];
    HotAirTCoupleWork = HotAirTCoupleWork + HotAirTCoupleAverage[i];
   }
    SolderInWork = SolderInWork / count_aver;
    HotAirInWork = HotAirInWork / count_aver; 
    SolderTCoupleWork = SolderTCoupleWork / count_aver;
    HotAirTCoupleWork = HotAirTCoupleWork / count_aver;

  // Поддержка установленной температуры паяльника (средние показатели)
  if (SolderInWork >= solderTCouple && digitalRead(pinSolderButton) == HIGH)
  {
    digitalWrite(pinSolderOut, HIGH);
  }
  else {
    digitalWrite(pinSolderOut, LOW);
  }

  //Защита от пробоя симистора/замыкания термопары (замыкаем реле)
  if (digitalRead(pinHotAirButton) == HIGH && setHotAirTemp + 100 > pinHotAirTCouple && pinHotAirTCouple < 480) {
    digitalWrite(pinRele, HIGH);
  }
  else {
    digitalWrite(pinRele, LOW);
  }

  //проверить Фен выключен или на подставке 0, в работе 1
  if (digitalRead(pinHotAirButton) == LOW || digitalRead(pinGerkon) == LOW) {
   work_fen = 0; 
   }
   else {
   work_fen = 1;
   }

  //Установка оборотов вентилятора фена с изображением вентилятора (средние показатели)
  if (HotAirTCoupleWork <= 50 && work_fen==0) {  //При температуре < 50 отключить
    analogWrite(pinHotAirCoolerOut, 0);
    Cooler_ON=0;
  } else 
    if (HotAirTCoupleWork > 75 || work_fen ==1) { //При температуре от 75 или включеном фене 
    analogWrite(pinHotAirCoolerOut, setHotAirCooler);
    if (HotAirTCoupleWork < 481) {
       Cooler_ON=1;
      } else {   //Если фен не подключен, вывести молнию
       Cooler_ON=2;
       lcd.setCursor(14, 1);
       lcd.print(char(5));
       lcd.print(char(5));
      }
    } else
  if (HotAirTCoupleWork >60  && work_fen==0) { //При температуре от 50, подождать до 60 и продуть на максимуме
    analogWrite(pinHotAirCoolerOut, 255);
    Cooler_ON=1;
  }

  //Вывод изображения кулера
  lcd.setCursor(14, 0);
  if (Cooler_ON == 0) {
    lcd.print(char(0));
    lcd.print(char(0));
  } else if (Cooler_ON == 1) {
       lcd.print(char(Cooler));
       lcd.print(char(Cooler));
       Cooler++; if (Cooler == 5) {Cooler=1;}
   } else {
       lcd.print("  ");
   }

 
  // Поддержка установленной температуры фена (реальные показатели)
  if (setHotAirTemp >= hotAirTCouple && work_fen == 1)
  {
    digitalWrite(pinHotAirOut, HIGH);
    delay(setHotAirTemp-hotAirTCouple+60);  //Изменяемое время включение термопары
    digitalWrite(pinHotAirOut, LOW);
  }
  else {
    digitalWrite(pinHotAirOut, LOW);
    delay(90);
  }


//Расположение значений на экране
//0123456789012345
//Sol: 300 200  **
//Fen: 300  60  99

  // Данные паяльника на дисплей
  lcd.setCursor(0, 0);
  lcd.print("Sol: ");
  if (solderTCouple > 500) {
      lcd.print("Not found"); //Паяльник не подключен в разъем. 
  } else
  if (solderTCouple > 480) {
      lcd.print("Error   "); //Паяльник перегрет. 
  } else
  if (digitalRead(pinSolderButton) == HIGH) { //Паяльник подключен в разъем и включен. 
      lcd.print(myprint(SolderInWork));
      lcd.print(" ");
      lcd.print(myprint(SolderTCoupleWork));
      lcd.print("  ");
    } else {            //Паяльник подключен в разъем и выключен. 
      lcd.print("Off ");
      if (SolderTCoupleWork > 50) {
        lcd.print(myprint(SolderTCoupleWork));
        lcd.print("  ");
      } else {
       lcd.print("    ");
      }
    }

  // Данные фена на дисплей
  lcd.setCursor(0, 1);
  lcd.print("Fen: ");
  if (hotAirTCouple > 500) {
      lcd.print("Not found  "); //Фен не подключен в разъем. 
   } else
  if (hotAirTCouple > 480) {
      lcd.print("Error      "); //Фен перегрет 
   } else
  if (digitalRead(pinHotAirButton) == HIGH) { //Фен подключен в разъем, и включен
      lcd.print(myprint(HotAirInWork));
      lcd.print(" ");
      lcd.print(myprint(HotAirTCoupleWork));
      lcd.print("  ");
      lcd.setCursor(14, 1);
      if (displayHotAirCooler<10) {lcd.print("0");}
      lcd.print(displayHotAirCooler);
      if (digitalRead(pinGerkon) == LOW ) //геркон замкнут, мигаем уст. температурой (ловим по земле)
      {
        delay(150);
        lcd.setCursor(5, 1);
        lcd.print("    ");
       }
    } else { //Фен подключен в разъем, и выключен
      lcd.print("Off ");
      lcd.setCursor(9, 1);
      if (HotAirTCoupleWork > 50) {  //Вывод температуры фена >50
        lcd.print(myprint(HotAirTCoupleWork));
      } else {
        lcd.print("    ");
      }
      lcd.print("     ");
    }

  delay(60);
}

//Функция вывода форматированного значения
String myprint(int arg){
 String result;
  result = String(arg); 
  if (arg<10) {result=String(" "+result);}
  if (arg<100) {result=String(" "+result);}
 return result;
}
