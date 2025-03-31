//ESP32                OLED
//3.3V    --------→   VCC
//GND     --------→   GND
//GPIO22  --------→   SCL
//GPIO21  --------→   SDA
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED顯示器寬度，單位像素
#define SCREEN_HEIGHT 32 // OLED顯示器高度，單位像素（0.96吋為32或64）
#define OLED_RESET    -1 // Reset pin
#define SCREEN_ADDRESS 0x3C // OLED的I2C地址

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

String inputString = ""; // 儲存接收到的字串
boolean stringComplete = false; // 字串是否完整的標誌

void setup() {
  Serial.begin(115200);
  
  // 初始化OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306初始化失敗"));
    for(;;); // 無限循環
  }

  // 清除顯示緩衝
  display.clearDisplay();
  display.display();
  
  // 設置文字顯示參數
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  
  Serial.println("請輸入要顯示的文字：");
}

void loop() {
  // 當收到完整的字串時
  if (stringComplete) {
    // 打印接收到的字串
    Serial.println("OLED showing:"+inputString);  

    
    // 清除顯示
    display.clearDisplay();
    display.setCursor(0,0);
    
    // 顯示文字
    display.println(inputString);
    display.display();
    
    // 清除字串並重置標誌
    inputString = "";
    stringComplete = false;
    
    Serial.println("請輸入要顯示的文字：");
  }
  
  // 檢查是否有新的串口數據
  while (Serial.available()) {
    char inChar = (char)Serial.read();
    
    // 如果收到換行符號，表示輸入完成
    if (inChar == '\n'|| inChar == '\r') {
      stringComplete = true;
    } else {
      inputString += inChar;
    }
  }
} 