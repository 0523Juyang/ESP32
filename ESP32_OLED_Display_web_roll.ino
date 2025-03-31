//OLED 左滾動顯示文字 是透過手動「清除 → 重繪 → 遞減位置」
//網頁控制OLED顯示文字 是透過「POST → 網頁輸入文字 → 發送到 OLED」

//ESP32                OLED
//3.3V    --------→   VCC
//GND     --------→   GND
//GPIO22  --------→   SCL
//GPIO21  --------→   SDA

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <WebServer.h>

#define SCREEN_WIDTH 128 // OLED顯示器寬度，單位像素
#define SCREEN_HEIGHT 32 // OLED顯示器高度，單位像素（0.96吋為32或64）
#define OLED_RESET    -1 // Reset pin
#define SCREEN_ADDRESS 0x3C // OLED的I2C地址

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
WebServer server(80);

// 滾動文字相關變數
String scrollText = "";  // 要顯示的文字
int16_t textX = 120;  // 文字的起始X座標
int16_t textY = 16;  // 文字的Y座標
bool isScrolling = false;  // 是否正在滾動
unsigned long lastScrollTime = 0;  // 上次滾動時間
int SCROLL_SPEED = 60;  // 滾動速度（毫秒）
const int SCROLL_PIXELS = 1;  // 每次滾動的像素數
const int CHAR_WIDTH = 6;  // 每個字元的寬度
const int CHAR_SPACING = 8;  // 字元間距
int currentCharIndex = 0;  // 當前顯示到第幾個字元
int currentPixel = 0;  // 當前像素位置

// WiFi設定
const char* ssid = "ros";     // 請更改為您的WiFi名稱
const char* password = "bottle0616"; // 請更改為您的WiFi密碼

// 網頁HTML
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
    <meta charset="UTF-8">
    <title>ESP32 OLED 控制</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            margin: 0;
            padding: 20px;
            background-color: #f0f0f0;
        }
        .container {
            max-width: 600px;
            margin: 0 auto;
            background-color: white;
            padding: 20px;
            border-radius: 10px;
            box-shadow: 0 2px 5px rgba(0,0,0,0.1);
        }
        h1 {
            color: #333;
            text-align: center;
        }
        .input-group {
            margin: 20px 0;
        }
        input[type="text"] {
            width: 100%;
            padding: 10px;
            margin: 10px 0;
            border: 1px solid #ddd;
            border-radius: 5px;
            box-sizing: border-box;
        }
        button {
            width: 100%;
            padding: 10px;
            background-color: #4CAF50;
            color: white;
            border: none;
            border-radius: 5px;
            cursor: pointer;
        }
        button:hover {
            background-color: #45a049;
        }
        #status {
            margin-top: 20px;
            padding: 10px;
            border-radius: 5px;
            text-align: center;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>ESP32 OLED 顯示控制</h1>
        <div class="input-group">
            <input type="text" id="text" placeholder="請輸入要顯示的文字">
            <select id="speed" style="width: 100%; padding: 10px; margin: 10px 0; border: 1px solid #ddd; border-radius: 5px; box-sizing: border-box;">
                <option value="100">慢速 (100ms)</option>
                <option value="60" selected>中速 (60ms)</option>
                <option value="30">快速 (30ms)</option>
            </select>
            <button onclick="sendText()">發送到 OLED</button>
        </div>
        <div id="status"></div>
    </div>
    <script>
        function sendText() {
            var text = document.getElementById('text').value;
            var speed = document.getElementById('speed').value;
            var xhr = new XMLHttpRequest();
            xhr.open('POST', '/display', true);
            xhr.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded');
            xhr.onreadystatechange = function() {
                if (xhr.readyState == 4) {
                    if (xhr.status == 200) {
                        document.getElementById('status').innerHTML = '發送成功！';
                        document.getElementById('status').style.backgroundColor = '#dff0d8';
                        document.getElementById('status').style.color = '#3c763d';
                    } else {
                        document.getElementById('status').innerHTML = '發送失敗！';
                        document.getElementById('status').style.backgroundColor = '#f2dede';
                        document.getElementById('status').style.color = '#a94442';
                    }
                }
            };
            xhr.send('text=' + encodeURIComponent(text) + '&speed=' + speed);
        }
    </script>
</body>
</html>
)rawliteral";

void setup() {
  Serial.begin(115200);
  
  // 初始化OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }

  // 清除顯示緩衝
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.display();
  
  // 連接WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi已連接");
  Serial.println("IP位址: ");
  Serial.println(WiFi.localIP());

  // 在OLED上顯示IP位址
  display.clearDisplay();
  display.setCursor(0,0);
  display.println("WiFi Connected!");
  display.setCursor(0,20);
  display.println(WiFi.localIP().toString());
  display.display();

  // 設置網頁路由
  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", index_html);
  });

  server.on("/display", HTTP_POST, []() {
    String text = server.arg("text");
    String speedStr = server.arg("speed");
    
    // 更新滾動速度
    if (speedStr.length() > 0) {
      int speed = speedStr.toInt();
      if (speed > 0) {
        SCROLL_SPEED = speed;
      }
    }
    
    scrollText = text;  // 更新要滾動的文字
    currentCharIndex = 0;  // 重置字元索引
    textX = 120;  // 重置文字位置
    isScrolling = true;  // 開始滾動
    
    // 清除螢幕並重置顯示狀態
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.display();
    
    // 等待一下確保螢幕更新完成
    delay(10);
    
    server.send(200, "text/plain", "OK");
  });

  server.begin();
}

void loop() {
  server.handleClient();
  
  // 處理文字滾動
  if (isScrolling && scrollText.length() > 0) {
    unsigned long currentTime = millis();
    if (currentTime - lastScrollTime >= SCROLL_SPEED) {
      display.clearDisplay();
      display.setCursor(0,0);
      delay(100);
      // 計算要顯示的字元數量
      int numChars = min((int)scrollText.length(), (currentPixel / CHAR_SPACING) + 1);
      
      // 計算要顯示的文字部分
      String displayText = scrollText.substring(0, numChars);
      
      // 計算顯示位置
      int16_t displayX = textX;
      
      // 從起始位置(120,16)開始顯示每個字元
      for(int i = 0; i < numChars; i++) {
        display.setCursor(displayX + (i * CHAR_WIDTH), textY);
        display.print(displayText[i]);
      }
      
      display.display();
      
      // 更新像素位置
      currentPixel++;
      
      // 如果文字已經移動到最左側
      if (textX <= -(numChars * CHAR_WIDTH)) {
        textX = 120;  // 重置到起始位置
        currentPixel = 0;  // 重置像素計數
        
        // 如果所有字元都顯示完畢
        if (numChars >= scrollText.length()) {
          currentPixel = 0;  // 重置像素計數
          textX = 120;  // 重置到起始位置
          // 不需要停止滾動，讓它繼續循環
        }
      } else {
        // 更新X座標位置
        textX -= SCROLL_PIXELS;
      }
      
      lastScrollTime = currentTime;
    }
  }
} 
