// BLE接続の為の設定
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// においセンサーのピン宣言
#define PIN_HEATER 4  // IO12にすると書き込み時にMD5エラーが出る
#define PIN_SENSOR 15  // D15(A1)
#define PIN_INPUT 39  // Corresponds to Arduino Uno's A3

// LEDのピン宣言
//#define LED 14

// RGB LEDのピン宣言
uint8_t ledR = 25;
uint8_t ledG = 26;
uint8_t ledB = 27;


// BLEServerの設定
#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;
int sendvalue = 0;
int failvalue = -1;


// BLE callback
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
  };

  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
  }
};


// RGB点灯の処理
void rgb(int r, int g, int b) {
  ledcWrite(1, r);
  ledcWrite(2, g);
  ledcWrite(3, b);
}


// BLE通信の処理全般
void ble() {
  // BLE Deviceの生成
  BLEDevice::init("Flesh AI");

  // BLE Serverの生成
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // BLE Serviceの生成
  BLEService* pService = pServer->createService(SERVICE_UUID);

  // BLE Characteristicの生成
  pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_INDICATE);

  // BLE Descriptor(説明文)の生成
  pCharacteristic->addDescriptor(new BLE2902());

  // Serviceの開始
  pService->start();

  // advertisingの開始
  BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  //pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  //Serial.println("Waiting a client connection to notify...");
}


void setup() {
  // シリアルポート宣言
  //Serial.begin(9600);
  Serial.begin(115200);

  // 以下3つのconfigはデフォルト値なので省略しても良い
  // Deep Sleep中にPull Up を保持するために
  // esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
  // // Deep Sleep中にメモリを保持するために
  // esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_ON);
  // esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_ON);

  //deepsleep用ボタンをピンに対応する
  //pinMode(GPIO_NUM_16, INPUT_PULLUP);

  // においセンサーを各ピンに対応する
  pinMode(PIN_HEATER, OUTPUT);
  pinMode(PIN_SENSOR, OUTPUT);
  
  digitalWrite(PIN_HEATER, HIGH);  // Heater Off
  digitalWrite(PIN_SENSOR, LOW);   // Sensor Pullup Off

  //LEDをピンに対応する
  //pinMode(LED, OUTPUT);

  // 12 kHz PWM, 8-bit 宣言
  ledcSetup(1, 12000, 8);
  ledcSetup(2, 12000, 8);
  ledcSetup(3, 12000, 8);

  // RGB LEDを各ピンに対応する
  ledcAttachPin(ledR, 1);
  ledcAttachPin(ledG, 2);
  ledcAttachPin(ledB, 3);

  ble();  //BLE処理全般を任せる
}


void loop() {

  //int sleep = digitalRead( GPIO_NUM_16 );

  /** においセンサー処理 **/
  static bool flag = false;
  static int fcount = 0;
  static int val = 0;
  static int sum = 0;
  static int count = 0;

  //センサ情報を100点満点に換算
  float degital_val = (static_cast<float>(val)/4095)*100;

  //ヒーターの加熱処理
  //においセンサ内の状態をリセットするために加熱処理を施す
  digitalWrite(PIN_HEATER, LOW);
  //ウォームアップ時間短縮
  //起動時に約30分の加熱時間を要する為無理やり短縮させている
  //60点以上が5回続くと推奨値に戻す
  if(degital_val < 60.0 && !flag) delay(100);
  else delay(8);

  digitalWrite(PIN_HEATER, HIGH);   // Heater Off
  delay(237);                       // 次の処理のために間をあける

  //においセンサを起動し値を読み取る
  digitalWrite(PIN_SENSOR, HIGH);  // Sensor Pullup On
  delay(1);
  val = analogRead(PIN_INPUT);  // Get Sensor Voltage
  delay(2);
  digitalWrite(PIN_SENSOR, LOW);  // Sensor Pullup Off

  //値送信用
  sum += val;
  count++;

  //それぞれカウント数によって処理をする
  if (count == 10) {
    // においセンサの取得平均値を100点変換して表示する
    float smell = (static_cast<float>(sum/count)/4095)*100;
    Serial.println(smell);
    sum = 0;
    count = 0;
    if(!flag){
      if(smell >= 60){
        fcount++;
        if (fcount > 5) flag = true;
      } else {
        fcount = 0;
      }
      Serial.println(flag);
      Serial.println(fcount);
    }
  }

  // DeepSleep処理
  //int nowData = digitalRead(GPIO_NUM_16);
  // esp_sleep_enable_ext0_wakeup(GPIO_NUM_16, (nowData==0) ? 1 : 0 );
  // esp_sleep_enable_timer_wakeup(60 * 1000 * 1000);  // wakeup every 1min
  // esp_deep_sleep_start();
  
  /** BLE通信処理 **/
  //通知する値の変更
  if (deviceConnected) {    //接続中
    //digitalWrite(LED, HIGH);
    sendvalue = val;
    pCharacteristic->setValue(sendvalue);   // においデータの送信
    if (!deviceConnected) ble();
  } else {                  //接続待ち
    //digitalWrite(LED, LOW);
    pCharacteristic->setValue(failvalue);
  }

  //通知を送る
  pCharacteristic->notify();

  //通信が切断されたときの処理
  if (!deviceConnected && oldDeviceConnected) {
    delay(500);
    pServer->startAdvertising();
    oldDeviceConnected = deviceConnected;
  }

  if (deviceConnected && !oldDeviceConnected) {
    oldDeviceConnected = deviceConnected;
  }
}