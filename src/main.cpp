#include <Arduino.h>
#include <BleKeyboard.h>
#include <NimBLEDevice.h>

#ifdef ESP32C3_SUPERMINI
    #define TOGGLE_BUTTON 9
    #define LED 8
    #define LED_ON LOW
    #define LED_OFF HIGH
#endif

// 状态定义
enum SystemState {
    STATE_WAITING_CONNECT,  // 等待连接
    STATE_CONNECTED,        // 已连接
    STATE_AUTO_PAGING       // 自动翻页
};

// 全局变量
SystemState currentState = STATE_WAITING_CONNECT;
volatile bool buttonPressed = false;
BleKeyboard bleKeyboard("AutoPager", "ESP32", 100);

// LED状态指示任务
void ledIndicatorTask(void *pvParameters) {
    // 配置LED引脚为输出
    pinMode(LED, OUTPUT);
    digitalWrite(LED, LED_OFF);

    while (1) {
        switch (currentState) {
            case STATE_WAITING_CONNECT:
                // 等待连接时，LED闪烁2Hz
                digitalWrite(LED, LED_ON);
                vTaskDelay(pdMS_TO_TICKS(250));
                digitalWrite(LED, LED_OFF);
                vTaskDelay(pdMS_TO_TICKS(250));
                break;

            case STATE_CONNECTED:
                // 连接上之后，LED常亮
                digitalWrite(LED, LED_ON);
                vTaskDelay(pdMS_TO_TICKS(100));  // 短延时，避免占用CPU
                break;

            case STATE_AUTO_PAGING:
                // 自动翻页状态时，LED 0.5Hz的频率闪烁
                digitalWrite(LED, LED_ON);
                vTaskDelay(pdMS_TO_TICKS(1000));
                digitalWrite(LED, LED_OFF);
                vTaskDelay(pdMS_TO_TICKS(1000));
                break;
        }
    }
}

// 自动翻页任务
void autoPagingTask(void *pvParameters) {
    while (1) {
        // 只有在自动翻页状态下才执行
        if (currentState == STATE_AUTO_PAGING && bleKeyboard.isConnected()) {
            // 生成30到45秒之间的随机间隔（毫秒）
            uint32_t randomDelay = random(30000, 45000);
            Serial.printf("Next page in %lu ms\n", randomDelay);

            vTaskDelay(pdMS_TO_TICKS(randomDelay));

            // 检查状态是否改变
            if (currentState == STATE_AUTO_PAGING && bleKeyboard.isConnected()) {
                // 发送右方向键
                Serial.println("Sending RIGHT_ARROW key");
                bleKeyboard.write(KEY_RIGHT_ARROW);
            }
        } else {
            // 不在自动翻页状态，延时等待
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}

// 按钮中断处理函数
void IRAM_ATTR buttonISR() {
    buttonPressed = true;
}

// 按钮处理任务
void buttonTask(void *pvParameters) {
    // 配置按钮引脚为输入，启用上拉电阻
    pinMode(TOGGLE_BUTTON, INPUT_PULLUP);

    // 配置中断
    attachInterrupt(digitalPinToInterrupt(TOGGLE_BUTTON), buttonISR, FALLING);

    while (1) {
        if (buttonPressed) {
            // 去抖动延时
            vTaskDelay(pdMS_TO_TICKS(50));

            // 再次检查按钮状态
            if (digitalRead(TOGGLE_BUTTON) == LOW) {
                Serial.println("Button pressed");

                // 状态转换
                switch (currentState) {
                    case STATE_WAITING_CONNECT:
                        // 等待连接状态下，按钮无效
                        Serial.println("Waiting for connection, button ignored");
                        break;

                    case STATE_CONNECTED:
                        // 已连接状态下，进入自动翻页状态
                        Serial.println("Entering auto paging mode");
                        currentState = STATE_AUTO_PAGING;
                        // 立即翻页一次
                        if (bleKeyboard.isConnected()) {
                            Serial.println("Immediate page turn");
                            bleKeyboard.write(KEY_RIGHT_ARROW);
                        }
                        break;

                    case STATE_AUTO_PAGING:
                        // 自动翻页状态下，停止自动翻页，回到已连接状态
                        Serial.println("Exiting auto paging mode");
                        currentState = STATE_CONNECTED;
                        break;
                }
            }

            buttonPressed = false;

            // 等待按钮释放
            while (digitalRead(TOGGLE_BUTTON) == LOW) {
                vTaskDelay(pdMS_TO_TICKS(10));
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println("AutoPager BLE Keyboard Starting...");

    // 初始化随机数生成器
    randomSeed(analogRead(0));

    // 启动BLE键盘
    bleKeyboard.begin();
    Serial.println("BLE Keyboard started, waiting for connection...");

    // 创建任务
    xTaskCreate(ledIndicatorTask, "LEDIndicator", 2048, NULL, 2, NULL);
    xTaskCreate(buttonTask, "ButtonHandler", 2048, NULL, 3, NULL);
    xTaskCreate(autoPagingTask, "AutoPaging", 2048, NULL, 1, NULL);
}

void loop() {
    // 在主循环中检查BLE连接状态
    static bool lastConnected = false;
    bool currentConnected = bleKeyboard.isConnected();

    if (currentConnected != lastConnected) {
        if (currentConnected) {
            Serial.println("BLE Connected!");
            if (currentState == STATE_WAITING_CONNECT) {
                currentState = STATE_CONNECTED;
            }
        } else {
            Serial.println("BLE Disconnected!");
            currentState = STATE_WAITING_CONNECT;
        }
        lastConnected = currentConnected;
    }

    vTaskDelay(pdMS_TO_TICKS(100));
}
