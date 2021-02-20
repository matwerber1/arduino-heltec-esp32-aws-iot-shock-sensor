#include <pgmspace.h>

#define SECRET
#define THINGNAME "YOUR_THING_NAME"

const char WIFI_SSID[] = "YOUR_WIFI_SSID";
const char WIFI_PASSWORD[] = "YOUR_WIFI_PASSWORD";
const char AWS_IOT_ENDPOINT[] = "YOUR_IOT_ENDPOINT.iot.REGION.amazonaws.com";

// Amazon Root CA 1
// You can copy-paste this from this URL: 
// https://www.amazontrust.com/repository/AmazonRootCA1.pem
static const char AWS_CERT_CA[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
-----END CERTIFICATE-----
)EOF";

// Device Certificate
// This is provided to you when you create your AWS IoT "Thing":
static const char AWS_CERT_CRT[] PROGMEM = R"KEY(
-----BEGIN CERTIFICATE-----
-----END CERTIFICATE-----
)KEY";

// Device Private Key
// This is provided to you when you create your AWS IoT "Thing":
static const char AWS_CERT_PRIVATE[] PROGMEM = R"KEY(
-----BEGIN RSA PRIVATE KEY-----
-----END RSA PRIVATE KEY-----
)KEY";