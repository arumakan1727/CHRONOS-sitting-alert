#include <Arduino.h>
#include <armkn/accumulator/summing_queue.hpp>
#include <armkn/net/http_client.hpp>
#include <armkn/net/wifi.hpp>
#include <armkn/task_polling/periodic_digital_signal.hpp>
#include <armkn/task_polling/periodic_task.hpp>

#include "./env.h"

constexpr int BUZZER_PIN = 11;
constexpr int FSR_PIN = A0;

constexpr int SITTING_PRESSURE_THRESHOLD = 60;

constexpr armkn::MilliTimeUInt SITTING_CHECK_INTERVAL_MS = 2000;
constexpr armkn::MilliTimeUInt SITTING_STATE_POST_INTERVAL_MS = 5000;

// 30分間のうち20分間のデータポイントが座り状態なら「座りすぎ状態」とする
constexpr unsigned SITTING_CHECK_WINDOW_DATA_POINTS =
  30 * 60 * 1000 / SITTING_CHECK_INTERVAL_MS;
constexpr unsigned TOO_MUCH_SITTING_THRESHOLD_DATA_POINTS =
  25 * 60 * 1000 / SITTING_CHECK_INTERVAL_MS;

auto recent_sitting_counter =
  armkn::SummingQueue<unsigned>(SITTING_CHECK_WINDOW_DATA_POINTS);

auto http_request_err_buzzer = armkn::PeriodicDigitalSignal(
  "http_request_err_buzzer",
  BUZZER_PIN,
  armkn::RepeatCount::of(6),
  {
    {HIGH, 50},
    {LOW, 50},
  }
);

auto too_much_sitting_buzzer = armkn::PeriodicDigitalSignal(
  "too_much_sitting_buzzer",
  BUZZER_PIN,
  armkn::RepeatCount::INF,
  {
    {HIGH, 250},
    {LOW, 250},
  }
);

WiFiClient client;

void setup() {
  Serial.begin(9600);
  while (!Serial) {
    delay(100);
  }
  Serial.println("[INFO] setup!!!!!!!!!");

  pinMode(FSR_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  if (armkn::setup_wifi(WIFI_SSID, WIFI_PASSWORD) == armkn::Result::Ok) {
    Serial.println("[INFO] WiFi setup success");
    digitalWrite(BUZZER_PIN, HIGH);
    delay(200);
    digitalWrite(BUZZER_PIN, LOW);
    delay(200);
    digitalWrite(BUZZER_PIN, HIGH);
    delay(200);
    digitalWrite(BUZZER_PIN, LOW);
    delay(200);
  } else {
    Serial.println("[ERR] WiFi setup failed");
    digitalWrite(BUZZER_PIN, HIGH);
    delay(3000);
    digitalWrite(BUZZER_PIN, LOW);
    while (true) {  // Don't continue
      delay(1000);
    }
  }
}

auto check_sitting_pressure_task =
  armkn::PeriodicTask(SITTING_CHECK_INTERVAL_MS, []() {
    const auto sitting_pressure = analogRead(FSR_PIN);
    Serial.print("sitting_pressure: ");
    Serial.println(sitting_pressure);

    const auto is_sitting = sitting_pressure > SITTING_PRESSURE_THRESHOLD;

    recent_sitting_counter.add(is_sitting);
    const auto sitting_count = recent_sitting_counter.get_sum();
    Serial.print("sitting_count: ");
    Serial.println(sitting_count);

    if (is_sitting && sitting_count > TOO_MUCH_SITTING_THRESHOLD_DATA_POINTS) {
      too_much_sitting_buzzer.enable();
    } else {
      too_much_sitting_buzzer.disable();
    }
  });

auto post_sitting_puressure_task =
  armkn::PeriodicTask(SITTING_STATE_POST_INTERVAL_MS, []() {
    const auto sitting_pressure = analogRead(FSR_PIN);
    const auto is_sitting = sitting_pressure > SITTING_PRESSURE_THRESHOLD;

    char req_json[256];
    const auto body_size = snprintf(
      req_json,
      sizeof(req_json),
      "{\"sitting_pressure\": %d"
      ", \"is_sitting\": %s"
      ", \"sitting_check_window_data_points\": %d"
      ", \"window_sitting_count\": %d"
      "}",
      sitting_pressure,
      is_sitting ? "true" : "false",
      SITTING_CHECK_WINDOW_DATA_POINTS,
      recent_sitting_counter.get_sum()
    );

    const auto res = armkn::send_http_request_without_response(
      client,
      "POST",
      IPAddress(API_SERVER_IP),
      8000,
      "/sitting/status",
      "application/json",
      req_json,
      body_size
    );

    too_much_sitting_buzzer.tick();

    if (res != armkn::Result::Ok) {
      Serial.println("[ERR] HTTP request failed. Enabling error buzzer.");
      http_request_err_buzzer.enable();
    } else {
      http_request_err_buzzer.disable();
    }
  });

void loop() {
  check_sitting_pressure_task.tick();
  post_sitting_puressure_task.tick();
  http_request_err_buzzer.tick();
  too_much_sitting_buzzer.tick();
}
