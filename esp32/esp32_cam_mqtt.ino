#include "esp_camera.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <esp_system.h>
#include "esp_heap_caps.h"
#include "time.h"        
#include <ArduinoJson.h>
#include <Base64.h> 

#define PART_BOUNDARY "123456789000000000000987654321"
#define CAMERA_MODEL_AI_THINKER
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22


// Wi-Fi credentials
const char* ssid = "ssid";
const char* password = "password";

// AWS IoT Core settings
const char* aws_endpoint = "ayiu0asc1w4i8h-ats.iot.ap-south-1.amazonaws.com";
const char* topic = "esp32-images";
const char* clientId = "esp32_cam";
const int mqttPort = 8883;

// AWS IoT X.509 certificate files as strings (replace with actual certificate data)
const char* certificate = "-----BEGIN CERTIFICATE-----\n"
"SW5jLiBMPVNlYXR0bGUgU1Q9V2FzaGluZ3RvbiBDPVVTMB4XDTI0MTAwMjEyNDMy\n"
"OVoXDTQ5MTIzMTIzNTk1OVowHjEcMBoGA1UEAwwTQVdTIElvVCBDZXJ0aWZpY2F0\n"
"hL/KrF0FNcjjcK3IykoIHjP05by+Dow9a2JrDZUGl7MNOCdJK0xA5anRWfWuHoha\n"
"M+OiSN/aV1WWdF+UStfZxt2sc2lW/ta6eFmAo1hy8FXEC2Tpp0In6HjvxcGxhd1p\n"
"UAZGjN5nbJJqutjgpwKVGhwwQ9+bNQa0S23OD8hPNWemk49QsjfljEQm3Nr6RgNr\n"
"AQH/BAQDAgeAMA0GCSqGSIb3DQEBCwUAA4IBAQBEHPq1kJjsE+6qcuBT2HVyQkrT\n"
"MIIDWTCCAkGgAwIBAgIUAH3Tw/9yK1xrI2npCGswDQYJKoZaV9+Gfi/IhvcNAQEL\n"
"xDkNgRLPMNRrkldI3Kr3o8sJ+tsvicWwnLpvqbDB3vvgcxEoIFRf6meehr9dz/b2\n"
"dkHAboPc8hyluZC9XCHJZHzB/qp5AF4+GFPPU1Cr7+NBl7zmYZzPN6zhHZxauQiw\n"
"ftpgIOuWI96vNl8blXXrhEzKfioF54YQDPLaXaDVkyc3M838YSwCbAzK45GTY5Oy\n"
"ZTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAPJN4SqRSVY9LW+wmD1A\n"
"Sh8CAwEAAaNgMF4wHwYDVR0jBBgwFoAUtCX317y6HAa+mqVTsz63McPpt64wHQYD\n"
"BQAwTTFLMEkGA1UECwxCQW1hem9uIFdlYiBTZXJ2aWNlcyBPPUFtYXpvbi5jb20g\n"
"VR0OBBYEFAOm7Hs8B5tmaKRuUOIIT4KFve28MAwGA1UdEwEB/wQCMAAwDgYDVR0P\n"
"MIIEogIBAAKCAQEA8k3hKpFJVj0tb7CYPUCEv8qsXQU1yONwrcjKSggeM/TlvL4O\n"
"Eog3dwuJ5RpDPr2/mBzvY6BFLaOM3ZbNVOKQIX1OE1NIES7VHGJY835vEt+9e16j\n"
"nqjpfp8lkccQZomnigA5iT/DtFwCN8Z3OSzxcuXEWzFbwFuDG0b+1MzEQIzjQfEk\n"
"6AXXgVj1Y2/jEtp9duq0OxUwYJRlQjQN21LUUUbewci67wAd4C1KnrRDsT9o\n"
"-----END CERTIFICATE-----";


const char* private_key = "-----BEGIN RSA PRIVATE KEY-----\n"
"jD1rYmsNlQaXsw04J0krTEDlqdFZ9a4eiFoE1SlKN84EpuSWf4CKDJhxZg/jxsLj\n"
"lpqIs07e1mjduobss8qSEikAMTblNc7OF7VQBkaM3mdskmq62OCnApUbyE81Z6aT\n"
"j1CyN+WHDBD35s1BrRLbc4OMRCbc2vpGA2vEOQ2BEs8w1GuSV0jcqvejywn62y+J\n"
"xbCcum+psMHe++BzESggVF/qZ56Gv13P9vZ+2mAg65Yj3q82XxuVdeuETMp+KgXn\n"
"hhAM8tpdoNWTJzczzfxhLAJsDMrjkZNjkBAB28lxStTe7JKHwIDAQABAoI3Khkf9\n"
"yUFDKMeM7Qhhek9UMmDxknnxcxt3Q7adMJXfd81VTA5tB7MOvUR9A1ZRZJ4ptjuj\n"
"idW8DnnGnngozYNZ7GprcQXIs6oN6ytagana/0naNRjlN8kLhAMwqiqWY5KmpQys\n"
"e7iHJPIQOHuW/8kcViAFuhlkCUvUx5djllZzLj+9W6OfnLrIQejcS3+/i1JHOhM8\n"
"A/fC0U9IhKa46EaVqHV9+IDOQZvUo4Wra6JRy+CszRHcf5On5boEpXWSK3K2Wpyl\n"
"1pfJ6mKz0NJ2rvNp3PG8Bx2KWwRYR9EVCObi7WJrJzOh0t3zRen1a4g0hMr5eKzn\n"
"qDGKVObr438U2uGvHbQ4qF4/y86vP2LdNvHviDgvZw+fKsUnvBGML0aHxtxj+8Kf\n"
"5sVv5uECgYEA/2CDPUbKvdoFOtTKD8vcHMZfZhzdWJZ8kr9HT+kG957aH2JMzMWY\n"
"BNUpSjfOBKbkln+AigyYcWYP48bC45aaiLNO3tZo3bqG7LPKkhIpADE25TXOzhe1\n"
"tRLQ6RV/4fO0hC/5Rmp/oRT75nrO0izRSJivs+N0N1Fu+F9WkYinlE8CgYEA8uUz\n"
"5ZNumgnsii/663dv7bTBQrqngS/8txNc1nH4gv6E4PbmGQCSfX9to+F5pgDJ74ne\n"
"2dfJhbTEsDDJS8w2we1e7zrhtZ8yf/0GeTIBVykLcfRAB/I/kpIEcKMAVkscd6I+\n"
"/R7Lpmi4AhziXGNzBnHcpQx+HrNwSNFMJkBt6TECgYBu+kIOHvV8C4K6Tj6BHtZI\n"
"quxKM8D8/mikXPtgfqPId//SI6W8VyINap7bjGwWB9uaNu76l1LMWQjYHq7vj+D4\n"
"6+w7D/Ti/GJ1kST4yocNP4DzL3k180I8rjCbGfy49G8+ApsRPra4CaZuQnAeZduM\n"
"8M1qHy4h3AGdbdDw7bHlgQKBgFycf7kdCaK5/fKCcM2u8IJV1Qo4WP2s6lJzk4ZA\n"
"rwoooDCGzJM7Dno4KnE39Tzxzpzew92HDGcarm4/zR8k3AqtB0/2/299Q2bARqSf\n"
"VOujw5H5SNz/0egwLX0tdHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L\n"
"ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM\n"
"FElBAoGAP/n9cla59B7yS94gdd4atA9RBZ5RW9HQYbC5hcEOp1XdSUlrM0JDIH9K\n"
"s07AhHIYPvN+xLp3xd5c5lr0Uaf4aQf3YAhg2mIEB3Qqrad180U=\n"
"-----END RSA PRIVATE KEY-----";


const char* root_ca = "-----BEGIN CERTIFICATE-----\n"
"MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF\n"
"b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj\n"
"aIxz/ikU6MJ1IPP0Ig1BvQKjHI/HwvWIobhQ8qU1uBbilsWbjFjJhKxWI7M/RoYi\n"
"A4IBAQCY8jdaQZChGsV2USggNiMOruYou6r4lK5IpDB/G/wkjUu0yKGX9rbxenDI\n"
"IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6\n"
"MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv\n"
"b24gUm9vdCBDQSAxMB4XDTE1MDUyNjAwMDAwMFoXDTM4MDExNzAwMDAwMFowOTEL\n"
"ADA5MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6\n"
"jgSubJrIqg0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMC\n"
"Vly75rZP7VPDIBexqoA9zEA7NMOccdflqvTMA+KZ92ZrKsWuV/AnJ0A0vvCF7R3w\n"
"93FcXmn/6pUCyziKrlA4b9v7LWIbxcceVOF34GfID5yHI9Y/QCB/IIDEgEw+OyQm\n"
"9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw\n"
"N+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJbYK8U90vv\n"
"AYYwHQYDVR0OBBYEFIQYzIU07LwMlJQuCFmcx7IQTgoIMA0GCSqGSIb3DQEBCwUA\n"
"U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUs\n"
"o/ufQJVtMVT8QtPHRh8jrdkPSHCa2XV4cdFyQzR1bldZwgJcJmApzyMZFo6IQ6XU\n"
"5MsI+yMRQ+hDKXJioaldXgjUkK642M4UwtBV8ob2xJNDd2ZhwLnoQdeXeGADbkpy\n"
"rqXRfboQnoZsG4q5WTP468SQvvG5\n"
"-----END CERTIFICATE-----";







// Initialize WiFi, MQTT client, and camera config
WiFiClientSecure wifiClient;
PubSubClient client(wifiClient);

void setup() {
    Serial.begin(115200);

    // Connect to WiFi
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi...");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConnected to WiFi");

    // Setup secure connection with AWS IoT Core
    wifiClient.setCACert(root_ca);
    wifiClient.setCertificate(certificate);
    wifiClient.setPrivateKey(private_key);
    client.setServer(aws_endpoint, mqttPort);

    // Initialize camera
    if (!initCamera()) {
        Serial.println("Camera initialization failed");
        return;
    }

    // Connect to AWS IoT Core
    connectToAWS();
}

bool initCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.frame_size = FRAMESIZE_VGA;
  config.pixel_format = PIXFORMAT_JPEG;
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.fb_count = 1;

  esp_err_t err = esp_camera_init(&config);
  return err == ESP_OK;
}

void connectToAWS() {
    Serial.print("Connecting to AWS IoT Core...");
    while (!client.connect("esp32_cam")) {
        Serial.print(".");
        delay(500);
    }
    Serial.println("\nConnected to AWS IoT Core");
}





void captureAndSendImage() {
    // Capture image
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println("Image capture failed");
        return;
    }

    // Log the size of the image
    Serial.print("Image size (bytes): ");
    Serial.println(fb->len);

    // Prepare to encode image data to Base64
    int encodedLength = Base64.encodedLength(fb->len); // Get the length for the encoded data
    char *encodedString = new char[encodedLength + 1]; // Allocate memory for encoded string (+1 for null terminator)

    // Encode the image data
    Base64.encode(encodedString, (char*)fb->buf, fb->len); // Use the Base64 instance to encode the image data
    Serial.print("Encoded string length: ");
    Serial.println(encodedLength);
    
    // Ensure the encoded string is null-terminated
    encodedString[encodedLength] = '\0';

    // Construct JSON payload
    StaticJsonDocument<1024 * 128> doc;
    doc["image"] = encodedString;  // Use the Base64-encoded string

    // Serialize the JSON payload
    String payload;
    if (!serializeJson(doc, payload)) {
        Serial.println("JSON serialization failed.");
        delete[] encodedString; // Free allocated memory
        esp_camera_fb_return(fb); // Release memory for captured image
        return;
    }

    // Log JSON payload details
    Serial.print("JSON payload size (bytes): ");
    Serial.println(payload.length());

    // Check memory before publish
    Serial.print("Free heap before publish: ");
    Serial.println(ESP.getFreeHeap());

    // Publish JSON message to AWS IoT Core
    if (client.publish(topic, (uint8_t*)payload.c_str(), payload.length())) {
        Serial.println("Image JSON published successfully.");
    } else {
        Serial.println("Image JSON publish failed.");
    }

    // Check memory after publish
    Serial.print("Free heap after publish: ");
    Serial.println(ESP.getFreeHeap());

    // Clean up
    delete[] encodedString; // Free allocated memory
    esp_camera_fb_return(fb); // Release memory for captured image
}










void loop() {
    if (!client.connected()) {
        connectToAWS();
    }
    client.loop();

    // Capture and send image
    captureAndSendImage();
    delay(10000); // Send image every 10 seconds
}
