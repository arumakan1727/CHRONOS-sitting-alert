#include <Arduino.h>
#include <WiFiS3.h>

#include <armkn/types.hpp>

namespace armkn {

class SimpleHttpClient {
 private:
  enum class State {
    IDLE,
    SENT_REQUEST,
    READING_RESPONSE_HEADER,
    READDING_RESPONSE_BODY,
  };

  struct HttpResponse {
    int status_code;
    size_t content_length;
    String body;
  };

  using ResponseHandler = void (*)(const HttpResponse &);

  WiFiClient client;
  State state = State::IDLE;
  HttpResponse response;
  ResponseHandler handler;

 public:
  SimpleHttpClient() = default;

  void tick() {
    switch (state) {
      case State::IDLE:
        break;
      case State::SENT_REQUEST:
        read_http_response_header_first_line(client);
        break;
      case State::READING_RESPONSE_HEADER:
        read_http_response_header_after_first_line(client);
        break;
      case State::READDING_RESPONSE_BODY:
        read_http_response_body(client);
        break;
    }
  }

  void send_http_request(
    const char *method,
    const IPAddress &host,
    uint16_t port,
    const char *path,
    ResponseHandler handler
  ) {
    if (client.connect(host, port)) {
      Serial.println("connected to server");
      this->handler = handler;

      send_common_http_header(method, host, port, path);
      client.println();

      Serial.println("HTTP request sent");
      state = State::SENT_REQUEST;
    }
  }

  void send_http_request(
    const char *method,
    const IPAddress &host,
    uint16_t port,
    const char *path,
    const char *content_type,
    const char *body,
    size_t body_length,
    ResponseHandler handler
  ) {
    if (client.connect(host, port)) {
      Serial.println("connected to server");
      this->handler = handler;

      send_common_http_header(method, host, port, path);

      client.print("Content-Length: ");
      client.println(body_length);

      client.print("Content-Type: ");
      client.println(content_type);

      client.println();

      client.println(body);
      client.println();

      Serial.println("HTTP request sent");
      state = State::SENT_REQUEST;
    }
  }

 private:
  void send_common_http_header(
    const char *method,
    const IPAddress &host,
    uint16_t port,
    const char *path
  ) {
    Serial.println("Sending HTTP header:");
    Serial.println(method);
    Serial.println(host);
    Serial.println(port);
    Serial.println(path);
    Serial.println("---------------------");

    client.print(method);
    client.print(" ");
    client.print(path);
    client.println(" HTTP/1.1");

    client.print("Host: ");
    client.println(host);

    client.println("Connection: close");
  }

  void read_http_response_header_first_line(WiFiClient &client) {
    if (!client.available())
      return;

    const String status_line = client.readStringUntil('\r');
    client.read();  // Skip the '\n' after status line

    // Parse status code
    const auto first_space = status_line.indexOf(' ');
    const auto second_space = status_line.indexOf(' ', first_space + 1);
    if (first_space != -1 && second_space != -1) {
      response.status_code =
        status_line.substring(first_space + 1, second_space).toInt();
    }
    state = State::READING_RESPONSE_HEADER;
  }

  void read_http_response_header_after_first_line(WiFiClient &client) {
    if (!client.available())
      return;

    // Read headers
    const String line = client.readStringUntil('\r');
    client.readStringUntil('\n');  // Skip the newline

    if (line.isEmpty()) {
      state = State::READDING_RESPONSE_BODY;
      return;
    }

    const auto colon_index = line.indexOf(':');
    if (colon_index != -1) {
      const String key = line.substring(0, colon_index);

      // Check for Content-Length header
      if (key.equalsIgnoreCase("Content-Length")) {
        String value = line.substring(colon_index + 1);
        value.trim();
        response.content_length = value.toInt();
      }
    }
  }

  void read_http_response_body(WiFiClient &client) {
    if (!client.available())
      return;
    response.body = client.readString();
    state = State::IDLE;
    handler(response);
  }
};

}  // namespace armkn
