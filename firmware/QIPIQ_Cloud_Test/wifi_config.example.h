#pragma once
// Copy this file to wifi_config.h, fill locally, DO NOT COMMIT to git.
#define QIPIQ_WIFI_SSID "YOUR_WIFI_SSID"
#define QIPIQ_WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
#define QIPIQ_GATEWAY_URL "https://your-qipiq-server.example.com"
// Device credential, NOT the OpenAI API key.
#define QIPIQ_DEVICE_TOKEN "YOUR_LONG_DEVICE_TOKEN"
// Replace with the REAL CA certificate for the HTTPS server chain.
// MUST BE VALID PEM and verified against your deployment.
#define QIPIQ_ROOT_CA_PEM R"QIPIQCA(
-----BEGIN CERTIFICATE-----
PASTE_VALID_ROOT_CA_CERTIFICATE_HERE
-----END CERTIFICATE-----
)QIPIQCA"
