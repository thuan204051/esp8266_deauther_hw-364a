// ============================================================
//  SingleButton.h
//  Single Boot-Button Navigation for ESP8266 Deauther v2
// ============================================================
//  Nút Boot (GPIO0, active-LOW, pull-up nội) → 3 loại sự kiện:
//
//    CLICK       (< 600 ms)        → Cuộn XUỐNG / item tiếp theo
//    LONG_PRESS  (600 – 1999 ms)   → CHỌN / vào submenu / thực thi
//    XLONG_PRESS (≥ 2000 ms)       → BACK / thoát về menu cha
//
//  Cách dùng (xem PATCH_DisplayUI.cpp để biết chi tiết):
//    1. Copy file này vào thư mục esp8266_deauther/
//    2. Sửa DisplayUI.cpp theo hướng dẫn trong PATCH_DisplayUI.cpp
//    3. Sửa A_config.h theo hướng dẫn trong PATCH_A_config.h
// ============================================================

#pragma once
#include <Arduino.h>

// ---------- Tuning constants ----------
#define SINGLE_BTN_PIN     0       // GPIO0 = Boot / Flash button
#define DEBOUNCE_MS        50      // Bỏ qua nhiễu < 50ms
#define LONG_PRESS_MS      600     // Ngưỡng LONG  (SELECT)
#define XLONG_PRESS_MS     2000    // Ngưỡng XLONG (BACK)

class SingleButton {
public:
    // -------- Event enum --------
    enum Event : uint8_t {
        EVENT_NONE   = 0,
        EVENT_DOWN   = 1,   // Short click  → cuộn xuống
        EVENT_SELECT = 2,   // Long press   → chọn / enter
        EVENT_BACK   = 3    // X-long press → quay lại
    };

    // -------- Khởi tạo --------
    void begin() {
        pinMode(SINGLE_BTN_PIN, INPUT_PULLUP);
        _lastLevel = HIGH;    // Nút chưa bấm → HIGH
        _pressedAt = 0;
        _fired     = false;
    }

    // -------- Đọc sự kiện (gọi mỗi vòng loop) --------
    //  Trả về EVENT_NONE nếu không có gì xảy ra.
    //  Chỉ trả EVENT_SELECT / EVENT_BACK một lần trong suốt thời gian giữ nút.
    //  EVENT_DOWN trả về khi nhả nút (sau short click).
    Event read() {
        bool level = digitalRead(SINGLE_BTN_PIN);
        unsigned long now = millis();
        Event evt = EVENT_NONE;

        // ── Falling edge: bắt đầu bấm ──
        if (level == LOW && _lastLevel == HIGH) {
            _pressedAt = now;
            _fired     = false;
        }

        // ── Đang giữ: phát sự kiện một lần khi đạt ngưỡng ──
        if (level == LOW && !_fired) {
            unsigned long held = now - _pressedAt;
            if (held >= XLONG_PRESS_MS) {
                _fired = true;
                evt = EVENT_BACK;
            } else if (held >= LONG_PRESS_MS) {
                _fired = true;
                evt = EVENT_SELECT;
            }
        }

        // ── Rising edge: nhả nút ──
        if (level == HIGH && _lastLevel == LOW) {
            unsigned long held = now - _pressedAt;
            // Chỉ gửi CLICK nếu chưa kích hoạt long/xlong
            if (!_fired && held >= DEBOUNCE_MS) {
                evt = EVENT_DOWN;
            }
            _fired = false;
        }

        _lastLevel = level;
        return evt;
    }

    // ---- Tiện ích kiểm tra trạng thái nút ----
    bool isHeld() {
        return digitalRead(SINGLE_BTN_PIN) == LOW;
    }

    unsigned long heldDuration() {
        if (isHeld()) return millis() - _pressedAt;
        return 0;
    }

private:
    bool          _lastLevel;
    unsigned long _pressedAt;
    bool          _fired;
};
