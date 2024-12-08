#include <PS4Controller.h>
#include <Arduino.h>
#include <CAN.h>
#include <ESP32Servo.h>  // ESP32用のサーボライブラリ
#include "tuushin.h"  // tuushin.hをインクルード
#include "PWM.h"//PWM関連は別ファイルにした
#include "souten.h" 
#include "gyoukaku.h"
//#include "functions.h"//運転関連のものはここに入っている。

//使用ボタン設定
  int PS4_Circle=0;
  int PS4_Triangle=0;
  int PS4_R1=0;
  int PS4_L1=0;

// 目標電圧（ここに外部からの値が設定される）
float targetVoltage = 4.2;      // 初期値として3.5Vを設定
// 電圧範囲
const float maxVoltage = 8.0;   // 最大電圧
const float minVoltage = 0.0;   // 最小電圧
/*
const int PIN_SYASYUTU_PWM = 4;  // 射出のPWM
const int PIN_SYASYUTU_1 = 16;
const int PIN_SYASYUTU_2 = 21;
*/
const int PIN_SYASYUTU_PWM = 16;  // 射出のPWM
const int PIN_SYASYUTU_1 = 4;
const int PIN_SYASYUTU_2 = 21;
//21がLOW　　４　HIGH　16 PWM

int syasyutu_condition = 0;
int dutyCycle = calculateDutyCycle(targetVoltage, maxVoltage, minVoltage);
//Max=255とした計算

//追加分
extern Servo soutenServo; // 変数は外部で定義されていると宣言
int souten_servoPin = 13;  // サーボの接続ピンを指定（適宜変更）

Servo gyoukakuServo; // 変数は外部で定義されていると宣言
int gyoukaku_servoPin = 5;  // 仰角用サーボの接続ピンを指定（適宜変更）
//int currentAngle = 0;        // サーボの初期角度

int Ashimawari_Command=0;//コマンド

int value = 0;
  
// setup関数: 初期設定を行う。CANバスの初期化と、送受信の設定を呼び出す
void setup() {
  //ピン設定 
  pinMode(souten_servoPin,OUTPUT);
  pinMode(gyoukaku_servoPin,OUTPUT);
  pinMode(PIN_SYASYUTU_PWM,OUTPUT);
  pinMode(PIN_SYASYUTU_1,OUTPUT);
  pinMode(PIN_SYASYUTU_2,OUTPUT);
  //シリアル通信、PS4準備
  Serial.begin(115200);  // シリアル通信開始
  //PS4.begin("1c:69:20:e6:20:d2");//ここにアドレス
  Serial.println("Ready.");
  while (!Serial);  // シリアル接続待機


//CAN設定
const int CAN_TX_PIN = 27;  // 送信ピン（GPIO27）
const int CAN_RX_PIN = 26;  // 受信ピン（GPIO26）
  Serial.println("CAN Communication");
  CAN.setPins(CAN_RX_PIN, CAN_TX_PIN);
  CAN.begin(500E3);// CANバスの初期化（通信速度500kbps）
  if (!CAN.begin(500E3)) {
    Serial.println("CANの初期化に失敗しました！"); // CAN初期化に失敗した場合、エラーメッセージを表示して停止
    while (1);  // 永久ループで停止
  }
  
  // 受信と送信の初期化関数を呼び出し
  //setupReceiver();
  //サーボピン初期設定
  gyoukakuServo.attach(gyoukaku_servoPin);  // サーボピンを設定
  gyoukakuServo.write(25);  // 初期位置を25度（中央）に設定

  //サーボピン初期設定
  soutenServo.attach(souten_servoPin);  // サーボピンを設定
  soutenServo.write(45);  // 初期位置を-10度（中央）に設定
  /*
//PWM
//ledcSetup PWMピンに使う
//ledcSetup(uint8_t channel, uint32_t freq, uint8_t resolution_bits)
ledcSetup(0, 5000, 8);
// ledcAttachPin(uint8_t pin, uint8_t channel)
ledcAttachPin(PIN_SYASYUTU_PWM, 0);
// ledcWrite(uint8_t channel, uint32_t duty)
ledcWrite(0, 128);   //
  */
  //Serial.println("Ready.");
  setupSender();
  Serial.println("Start");
  Serial.println("Ready to receive commands: W, S, D, R");
}

// loop関数 やること　CAN送信、（前輪Encoder読み、前輪回転）、いろいろやる。



void loop(){
  if (Serial.available() > 0) { // シリアル通信でデータが受信された場合
    char command = Serial.read(); // 1文字を読み込む

    switch (command) {
      case 'W':
        value = 1;
        break;
      case 'S':
        value = 2;
        break;
      case 'D':
        value = 3;
        break;
      case 'R':
        value = 4;
        break;
      default:
        value = 0;
        //Serial.println("Invalid command");
        //return; // 無効なコマンドの場合、何もしない
    }
  }


    if (value == 1){//発射
      Serial.println("Circle Button");      //Serial.print("PWM_syasyutu!");
      if(syasyutu_condition==0){
        syasyutu_condition=1;
      }else{
        syasyutu_condition=0;  
      }
      Serial.printf("%d",syasyutu_condition);
      if(syasyutu_condition==0){
        digitalWrite(PIN_SYASYUTU_PWM, LOW);//回転時間ってどんくらいですか？Dutyサイクルは先に回っています
        //analogWrite(PIN_SYASYUTU_1,dutyCycle);
        digitalWrite(PIN_SYASYUTU_1,HIGH);
        digitalWrite(PIN_SYASYUTU_2,LOW);
      }else{
        Serial.printf("%d",dutyCycle); 
        //analogWrite(PIN_SYASYUTU_PWM, dutyCycle );
        //Dutyサイクルは先に回っています
        //analogWrite(PIN_SYASYUTU_PWM,dutyCycle);
        digitalWrite(PIN_SYASYUTU_PWM,HIGH);
        digitalWrite(PIN_SYASYUTU_1,HIGH);
        //analogWrite(PIN_SYASYUTU_1,dutyCycle);
        digitalWrite(PIN_SYASYUTU_2,LOW);
      }
    }
    if (value == 2) {//装填
      Serial.println("Triangle Button");//Debug  Serial.println("装填開始");
      Souten();
      //digitalWrite(PIN_SYASYUTU,LOW);
      digitalWrite(souten_servoPin,LOW);
      Serial.println("装填終了");
      }
    if (value == 3){
      Serial.println("仰角+1");
      movegyoukakuServoBy(1); // 現在の角度から1度動かす (+1°)
      delay(40);
      }
     if (value == 4){
      Serial.println("仰角-1");
       movegyoukakuServoBy(-1);// 現在の角度から1度動かす (+1°)
      delay(40);
      }

  // 送信処理を実行
/*
  if (PS4.Right()){Ashimawari_Command=3;
      }
      if (PS4.Down()){Ashimawari_Command=2;
      }
      if (PS4.Up()){Ashimawari_Command=1;
      }
      if (PS4.Left()){Ashimawari_Command=4;
      }
      if (PS4.UpRight()){Ashimawari_Command=5;
      }
      if (PS4.DownRight()){Ashimawari_Command=6;
      }
      if (PS4.UpLeft()){Ashimawari_Command=7;
      }
      if (PS4.DownLeft()){Ashimawari_Command=8;
      }
  Serial.printf("%d\n",Ashimawari_Command);//Debug
      
  sendPacket(Ashimawari_Command);
  Ashimawari_Command=0;//初期化
  */

  //ここで動作処理をする。
  //Encoder読み

  //動作

  //delay(150);  // 0.15秒の遅延
  value = 0;
}

/*
 if(data[0]==1){//これでHIGHにする
        analogWrite(PIN_SYASYUTU, dutyCycle );
        Serial.print("PWM");
      }else{
        digitalWrite(PIN_SYASYUTU,LOW);
        Serial.print("LOW");
        }
    if(data[1]==1){//これでHIGHにする
       // analogWrite(PIN_SYASYUTU, dutyCycle );
       Serial.println("装填開始");
      // Souten();
      
      Souten();
      
      data[1]=0; // 動作が完了したらPS4_Triangleを0に戻す
      Serial.println("装填終了");
      }else{
        digitalWrite(PIN_SYASYUTU,LOW);
        Serial.print("LOW");
        }
    if(data[2]==1){//これでHIGHにする
       Serial.println("仰角+1");
      movegyoukakuServoBy(1); // 現在の角度から1度動かす (+1°)
      delay(40);
      }else{
        }
    if(data[3]==1){//これでHIGHにする
       Serial.println("仰角-1");
       movegyoukakuServoBy(-1);// 現在の角度から1度動かす (+1°)
      delay(40);
      
      }else{
        }


        
void loop() {
  // Below has all accessible outputs from the controller
  if (PS4.isConnected()) {

    if (PS4.Square()) {
      Serial.println("Square Button");
      
    }
    if (PS4.Cross()) Serial.println("Cross Button");
    if (PS4.Circle()){
      Serial.println("Circle Button");
      PS4_Circle=1;
      Serial.println("%d",PS4_Circle);
    }
    if (PS4.Triangle()) Serial.println("Triangle Button");

    if (PS4.Charging()) Serial.println("The controller is charging");
    if (PS4.Audio()) Serial.println("The controller has headphones attached");
    if (PS4.Mic()) Serial.println("The controller has a mic attached");

    Serial.printf("Battery Level : %d\n", PS4.Battery());

    
    
    Serial.println();
    // This delay is to make the output more human readable
    // Remove it when you're not trying to see the output
  }
  
//ここで

  
    delay(1000);
  
}*/
