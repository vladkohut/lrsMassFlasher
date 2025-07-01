#include <M5Unified.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "xbm.h"
#include "squareBitmap.h"
#include "images.h"

// ---- Wi-Fi credentials ----
const char* ssid     = "ExpressLRS RX";
const char* password = "expresslrs";

// ---- ELRS target IP ----
const char* FWIP     = "10.0.0.1";

// ---- Helper: print a line on the display ----
static void showLine(int row, const String &txt) {
  M5.Display.setCursor(10, row * 20);
  M5.Display.print(txt);
}

// ---- Scan in a tight loop until SSID appears, then connect ----
bool connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  M5.Display.clear();
  showLine(0, "Scanning for WiFi");

  // keep scanning until we see our SSID
  while (true) {
    int n = WiFi.scanNetworks(false, false);
    bool found = false;
    for (int i = 0; i < n; ++i) {
      if (WiFi.SSID(i) == ssid) {
        found = true;
        break;
      }
    }
    if (found) break;

    // show a dot every half-second
    M5.Display.print(".");
    delay(500);
  }

  // now our SSID is present → connect
  M5.Display.clear();
  showLine(0, "SSID found!");
  delay(500);

  M5.Display.clear();
  showLine(0, "Connecting WiFi");
  WiFi.begin(ssid, password);

  uint32_t start = millis();
  const uint32_t timeout = 15000;  // 15 seconds
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - start > timeout) {
      M5.Display.clear();
      showLine(0, "Conn failed");
      return false;
    }
    M5.Display.print(".");
    delay(200);
  }

  // connected!
  M5.Display.clear();
  showLine(0, "WiFi connected");
  showLine(1, WiFi.localIP().toString());
  delay(1000);
  return true;
}

// ---- Change domain via POST /options.json ----
bool updateDomain(uint8_t newDomain) {
  HTTPClient http;
  String url = String("http://") + FWIP + "/options.json";
  http.begin(url);
  http.addHeader("Content-Type", "application/json");

  String payload = String("{\"domain\":") + newDomain +
                   ",\"wifi-on-interval\":60"
                   ",\"rcvr-uart-baud\":420000"
                   ",\"lock-on-first-connection\":true"
                   ",\"is-airport\":false"
                   ",\"customised\":true"
                   ",\"flash-discriminator\":3887363341"
                   "}";
  int code = http.POST(payload);
  http.end();

  M5.Display.clear();
  if (code == HTTP_CODE_OK || code == 204) {
    showLine(0, "Domain=" + String(newDomain));
    showLine(1, "HTTP " + String(code));
    return true;
  } else {
    showLine(0, "Upd error");
    showLine(1, "HTTP " + String(code));
    return false;
  }
}

// ---- Reboot ELRS via GET /reboot ----
void rebootELRS() {
  HTTPClient http;
  String url = String("http://") + FWIP + "/reboot";
  http.begin(url);
  int code = http.GET();
  http.end();

  M5.Display.clear();
  if (code == HTTP_CODE_OK) {
    showLine(0, "ELRS rebooted");
  } else {
    showLine(0, "Reboot err");
    showLine(1, "HTTP " + String(code));
  }
  delay(1000);
}

void setup() {
  // init M5Stick display
  auto cfg = M5.config();
  M5.begin(cfg);
  M5.Display.setRotation(1);
  M5.Display.setSwapBytes(true);
  M5.Display.setTextSize(2);
  M5.Display.setTextColor(GREEN);

  // show startup images
  for (int i = 0; i < epd_bitmap_allArray_LEN; i++) {
    M5.Display.clear();
    const uint16_t* img = (const uint16_t*)pgm_read_ptr(&epd_bitmap_allArray[i]);
    M5.Display.pushImage(0, 0, 240, 135, img);
    delay(2000);
  }
  M5.Display.clear();
}

void loop() {
  // 1) scan & connect (blocks until SSID found)
  if (!connectWiFi()) {
    delay(5000);
    return;
  }

  // 2) update domain to 5
  if (updateDomain(5)) {
    // 3) reboot ELRS
    rebootELRS();
  }

  // 4) wait before next cycle
  delay(5000);
}
