#define ___DEBUG___

#ifdef ___DEBUG___
#define DPRINT(X) Serial.print(X)
#define DPRINTLN(X) Serial.println(X)
#else
#define DPRINT(X)
#define DPRINTLN(X)
#endif

// библиотеки для датчика ориентации
#include <MPU9250_asukiaaa.h>
#include <Adafruit_BMP280.h>

// библиотеки для лоры
#include <SPI.h>
#include <LoRa.h>


// константы для лоры
#define BAND 866E6

#define SCK 5
#define MISO 19
#define MOSI 27
#define SS 18
#define RST 14
#define DIO0 26


// строки для отправки и приёма сообщений
String LoRaData;
char msg[32];

// настройки сенсора
Adafruit_BMP280 bme;
MPU9250_asukiaaa mySensor;
float aX, aY, aZ, gX, gY, gZ, mX, mY, mZ, aX_last, aY_last, aZ_last, deltaX, deltaY, deltaZ;

String help_data = "0";

// установка смещения нуля   ОБНУЛИТЬ ВСЕ ДАТЧИКИ!
void set_zero()
{
  int result;
  result = mySensor.accelUpdate();
  if (result == 0)
  {
    deltaX = mySensor.accelX();
    deltaY = mySensor.accelY();
    deltaZ = mySensor.accelZ();
    DPRINTLN("Null position. Gyro:");
    DPRINTLN(String(deltaX) + "\t" + 
             String(deltaY) + "\t" + 
             String(deltaZ));
  }
}


// определение текущей позиции
void now_pos()
{
  int result;
  result = mySensor.accelUpdate();
  if (result == 0)
  {
    aX = (mySensor.accelX() - deltaX) * 90;
    aY = (mySensor.accelY() - deltaY) * 90;
    aZ = (mySensor.accelZ() - deltaZ) * 90;
  }
}


// сохранение текущей позиции в прошлую позицию
void last_pos()
{
  aX_last = aX;
  aY_last = aY;
  aZ_last = aZ;
}


// получение текущих данных с датчика
void get_value()
{
  aX = mySensor.accelX();
  aY = mySensor.accelY();
  aZ = mySensor.accelZ();
  gX = mySensor.gyroX();
  gY = mySensor.gyroY();
  gZ = mySensor.gyroZ();
  mX = mySensor.magX();
  mY = mySensor.magY();
  mZ = mySensor.magZ();
}


// инициализация датчика
void sensor_setup()
{
  Wire.begin();
  mySensor.setWire(&Wire);
  bme.begin();
  mySensor.beginAccel();
  mySensor.beginGyro();
  mySensor.beginMag();
}


// инициализация лоры 
void LoRa_setup()
{
  SPI.begin(SCK, MISO, MOSI, SS);
  LoRa.setPins(SS, RST, DIO0);
  if (!LoRa.begin(BAND)) 
  {
    DPRINTLN("ERROR: Starting LoRa failed!");
    while (1);
  }
}


#define LAUNCH_CODE   '1'
#define STOP_CODE     '0'

void waiting_message_from_base()
{
  char cmd = STOP_CODE;
  
  DPRINTLN("RECEIVED MSG:");
  
  while (cmd != LAUNCH_CODE)
  {
      if (LoRa.available())
      {
        cmd = char(LoRa.read()); 
        DPRINT(msg);
      } 
  }
  DPRINTLN();
}


#define MSG_LEN 32
#define TEXT_LEN 10

// сообщение с показателями гироскопа
String convert_msg_to_str_to_send(float X, float Y, float Z)
{
  char msg[MSG_LEN+1] = "00000000000000000000000000000000";
  snprintf(msg, TEXT_LEN, "v%.3d%.3d%.3d", abs(int(X*100)%101), abs(int(Y*100)%101), abs(int(Z*100)%101));
  return String(msg);
}


void build_msg_and_send(float X, float Y, float Z)
{
  DPRINTLN("float xyz to message \t" + String(X) + "\t" + String(Y) + "\t" + String(Z));
  String msg = convert_msg_to_str_to_send(X, Y, Z);
  DPRINTLN(msg);
  DPRINT("msg length = " + String(msg.length()));
  DPRINTLN();
  
  LoRa.beginPacket();
  LoRa.print(msg);
  LoRa.endPacket();

  delay(100);
}


// отправка сообщения на станцию
void send_to_base()
{
  //get_value();
  build_msg_and_send(aX, aY, aZ);
  build_msg_and_send(gX, gY, gZ);
  build_msg_and_send(mX, mY, mZ);
}


// единоразовая настройка
void setup()
{
  Serial.begin(115200);
  sensor_setup();
  LoRa_setup();  
  set_zero();
  now_pos();
}


#define ANGLE_MAX_DELTA 80

void loop()
{
  last_pos();
  now_pos();
  DPRINTLN();  
  DPRINTLN("Last Gyro (Last_Pos):");
  DPRINTLN(String(aX_last) + "\t" + 
           String(aY_last) + "\t" + 
           String(aZ_last));
  DPRINTLN("New Gyro (Now_Pos):");
  DPRINTLN(String(aX) + "\t" + 
           String(aY) + "\t" + 
           String(aZ));
  DPRINTLN();
  waiting_message_from_base();

  DPRINTLN("abs(aX - aX_last) = " + String(abs(aX - aX_last)));
  DPRINTLN("abs(aY - aY_last) = " + String(abs(aY - aY_last)));
  DPRINTLN("abs(aZ - aZ_last) = " + String(abs(aZ - aZ_last)));
  if ((abs(aX - aX_last) > ANGLE_MAX_DELTA) or 
      (abs(aY - aY_last) > ANGLE_MAX_DELTA) or 
      (abs(aZ - aZ_last) > ANGLE_MAX_DELTA))
  {
    send_to_base();   
  }
  delay(2000);  
}