#define BUTTON_PIN 14
#define WAKEUP_PIN BUTTON_PIN
#define SLEEP_DURATION 86400 // 1日 (秒単位)

volatile bool deepSleepEnabled = false;

void IRAM_ATTR buttonInterrupt() {
  deepSleepEnabled = !deepSleepEnabled;
}

void setup() {
  Serial.begin(115200);
  //deepSleepEnabled = true;
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonInterrupt, FALLING);
  delay(300);
}

void loop() {
  if (deepSleepEnabled){
    Serial.println("Deepsleep移行...");
    delay(300);
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_14, LOW); // 使用するGPIO, 目覚ましのトリガレベル
    esp_deep_sleep_start();
    delay(1000);
  }else{
    Serial.println("No Deep");
    delay(10);
  }
  
  // ボタンが押されたらディープスリープへ
  // if (digitalRead(BUTTON_PIN) == LOW) {
  //   Serial.println("Deepsleep移行...");
  //   esp_deep_sleep_start();
  // }
}
