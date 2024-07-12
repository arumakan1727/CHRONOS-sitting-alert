#include <Arduino.h>
#include <armkn/accumulator/summing_queue.hpp>
#include <armkn/net/http_client.hpp>
#include <armkn/net/wifi.hpp>
#include <armkn/task_polling/periodic_digital_signal.hpp>
#include <armkn/task_polling/periodic_task.hpp>

#include "./env.h"

constexpr int BUZZER_PIN = 11;
constexpr int FSR_PIN = A0;

constexpr int SITTING_PRESSURE_THRESHOLD = 400;

constexpr armkn::MilliTimeUInt FSR_CHECK_INTERVAL_MS = 2000;
constexpr unsigned DATA_POINTS_FSR_CHECK_WINDOW_SIZE =
  45 * 60 * 1000 / FSR_CHECK_INTERVAL_MS;
constexpr unsigned DATA_POINTS_TOO_MUCH_SITTING_THRESHOLD =
  30 * 60 * 1000 / FSR_CHECK_INTERVAL_MS;
// constexpr unsigned DATA_POINTS_FSR_CHECK_WINDOW_SIZE = 1;
// constexpr unsigned DATA_POINTS_TOO_MUCH_SITTING_THRESHOLD = 1;

constexpr armkn::MilliTimeUInt POST_SITTING_PRESSURE_INTERVAL_MS = 5000;

auto recent_sitting_counter =
  armkn::SummingQueue<unsigned>(DATA_POINTS_FSR_CHECK_WINDOW_SIZE);

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
  armkn::PeriodicTask(FSR_CHECK_INTERVAL_MS, []() {
    const auto sitting_pressure = analogRead(FSR_PIN);
    Serial.print("sitting_pressure: ");
    Serial.println(sitting_pressure);

    recent_sitting_counter.add(sitting_pressure >= SITTING_PRESSURE_THRESHOLD);
    const auto sitting_count = recent_sitting_counter.get_sum();
    Serial.print("sitting_count: ");
    Serial.println(sitting_count);

    if (sitting_count >= DATA_POINTS_TOO_MUCH_SITTING_THRESHOLD) {
      too_much_sitting_buzzer.enable();
    } else {
      too_much_sitting_buzzer.disable();
    }
  });

auto post_sitting_puressure_task =
  armkn::PeriodicTask(POST_SITTING_PRESSURE_INTERVAL_MS, []() {
    const auto sitting_pressure = analogRead(FSR_PIN);

    char req_json[64];
    const auto body_size = snprintf(
      req_json,
      sizeof(req_json),
      "{\"sitting_pressure\": %d}",
      sitting_pressure
    );

    too_much_sitting_buzzer.tick();

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
