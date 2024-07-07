#include <Arduino.h>
#include <armkn/task_polling/http_client.hpp>
#include <armkn/task_polling/periodic_digital_signal.hpp>
#include <armkn/task_polling/periodic_task.hpp>
#include <armkn/utils/wifi.hpp>

#include "./env.h"

auto blink_led_13_task = armkn::PeriodicDigitalSignal(
  13,
  armkn::RepeatCount::of(10),
  {
    {HIGH, 250},
    {LOW, 250},
  }
);

auto http_client_task = armkn::SimpleHttpClient();

auto ping_task = armkn::PeriodicTask(1000, []() {
  http_client_task.send_http_request(
    "GET",
    IPAddress(API_SERVER_IP),
    8000,
    "/health",
    [](const auto &resp) {
      Serial.println("Response:");
      Serial.println(resp.status_code);
      Serial.println(resp.body);
    }
  );
});

void setup() {
  armkn::setup_wifi(
    WIFI_SSID,
    WIFI_PASSWORD,
    []() {
      Serial.println("[OK] WiFi setup success");
      armkn::print_wifi_status();
    },
    []() { Serial.println("[ERR] WiFi setup failed"); }
  );

  Serial.println("[INFO] setup!!!!!!!!!");
}

void loop() {
  ping_task.tick();
  blink_led_13_task.tick();
  http_client_task.tick();
}
