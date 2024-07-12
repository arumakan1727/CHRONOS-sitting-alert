#include <Arduino.h>
#include <WiFiS3.h>

#include <armkn/types.hpp>

namespace armkn {

struct HttpResponse {
  int status_code;
  String body;
};

namespace impl {

void send_common_http_header(
  WiFiClient &client,
  const char *method,
  const IPAddress &host,
  uint16_t port,
  const char *path
);

int read_http_response_header_first_line(
  WiFiClient &client,
  unsigned long timeout_ms,
  unsigned long request_sent_at
);
void read_http_response_header_after_first_line(WiFiClient &client);
String read_http_response_body(WiFiClient &client);

}  // namespace impl

Result send_http_request_without_response(
  WiFiClient &client,
  const char *method,
  const IPAddress &host,
  uint16_t port,
  const char *path
) {
  if (client.connect(host, port)) {
    Serial.println("connected to server");

    impl::send_common_http_header(client, method, host, port, path);
    client.println();

    Serial.println("HTTP request sent");
    return Result::Ok;
  }
  return Result::Err;
}

Result send_http_request_without_response(
  WiFiClient &client,
  const char *method,
  const IPAddress &host,
  uint16_t port,
  const char *path,
  const char *content_type,
  const char *body,
  size_t body_length
) {
  if (client.connect(host, port)) {
    Serial.println("connected to server");

    impl::send_common_http_header(client, method, host, port, path);

    client.print("Content-Length: ");
    client.println(body_length);

    client.print("Content-Type: ");
    client.println(content_type);

    client.println();

    client.println(body);
    client.println();

    Serial.println("HTTP request sent");

    return Result::Ok;
  }
  return Result::Err;
}

HttpResponse send_http_request_waiting_response(
  WiFiClient &client,
  const char *method,
  const IPAddress &host,
  uint16_t port,
  const char *path,
  unsigned long timeout_ms
) {
  const auto res =
    send_http_request_without_response(client, method, host, port, path);
  if (res != Result::Ok) {
    return {-1, ""};
  }

  const auto request_sent_at = millis();

  const auto status = impl::read_http_response_header_first_line(
    client,
    timeout_ms,
    request_sent_at
  );
  impl::read_http_response_header_after_first_line(client);
  const auto body = impl::read_http_response_body(client);
  return {status, body};
}

HttpResponse send_http_request_waiting_response(
  WiFiClient &client,
  const char *method,
  const IPAddress &host,
  uint16_t port,
  const char *path,
  const char *content_type,
  const char *body,
  size_t body_length,
  unsigned long timeout_ms
) {
  const auto res = send_http_request_without_response(
    client,
    method,
    host,
    port,
    path,
    content_type,
    body,
    body_length
  );
  if (res != Result::Ok) {
    return {-1, ""};
  }

  const auto request_sent_at = millis();

  const auto status = impl::read_http_response_header_first_line(
    client,
    timeout_ms,
    request_sent_at
  );
  impl::read_http_response_header_after_first_line(client);
  const auto resp_body = impl::read_http_response_body(client);

  return {status, resp_body};
}

namespace impl {

void send_common_http_header(
  WiFiClient &client,
  const char *method,
  const IPAddress &host,
  uint16_t port,
  const char *path
) {
  client.print(method);
  client.print(" ");
  client.print(path);
  client.println(" HTTP/1.1");

  client.print("Host: ");
  client.println(host);

  client.println("Connection: close");
}

int read_http_response_header_first_line(
  WiFiClient &client,
  unsigned long timeout_ms,
  unsigned long request_sent_at
) {
  while (!client.available()) {
    if (millis() - request_sent_at > timeout_ms) {
      Serial.println("[ERR] HTTP response timeout while reading the first line."
      );
      return -1;
    }
    delay(50);
  }

  const String status_line = client.readStringUntil('\r');
  client.read();  // Skip the '\n' after status line

  // Parse status code
  const auto first_space = status_line.indexOf(' ');
  const auto second_space = status_line.indexOf(' ', first_space + 1);
  if (first_space != -1 && second_space != -1) {
    return status_line.substring(first_space + 1, second_space).toInt();
  }
  return -1;
}

void read_http_response_header_after_first_line(WiFiClient &client) {
  while (client.available()) {
    // Read headers
    const String line = client.readStringUntil('\r');
    client.readStringUntil('\n');  // Skip the newline

    if (line.isEmpty()) {
      return;
    }
  }
}

String read_http_response_body(WiFiClient &client) {
  if (client.available()) {
    return client.readString();
  }
  return "";
}

}  // namespace impl

}  // namespace armkn
