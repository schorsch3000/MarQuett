
// local configuration -- copy this to local_config.h and change to your needs.

const char* ssid = "";                                                      // Name of the SSID to connect to
const char* password = "";                                                  // Password used to connect to your wifi

const char* mqtt_server = "";                                               // Hostname or IP of mqtt-Server
const char* mqtt_username = "";                                             // Username used to connect to MQTT-Server, leave empty if unused
const char* mqtt_password = "";                                             // Password used to connect to MQTT-Server, leave empty if unused

// set to false if you do not want to publish anything:
const bool do_publishes = true;

// you may change the default topic root "ledMatrix" to something different here
//#define TOPICROOT "ledMatrix"

// you may set LOG_TELNET here to 1 if you want logging via telnet (Port 23):
//#define LOG_TELNET  1

const int LEDMATRIX_SEGMENTS = 4;                                           // Number of 8x8 Segments
const uint8_t LEDMATRIX_CS_PIN = D4;                                        // CableSelect pin

#define MAX_TEXT_LENGTH 3000                      // Maximal text length
                                                  // too large might use up too much RAM and cause strange errors
#define NUM_CHANNELS 10                           // Number of text channels
uint16_t scrollDelay = 25;                        // Initial scroll delay
uint16_t cycleDelay = 5000;                       // Initial cycle delay (if delay==0, i.e. no scroll)
const char* initialText = "no such text";         // Initial Text shown before the first MQTT-Message is received
                                                  // Don't leave empty -- to show no text on startup set to " "
                                                  // Use only 7-Bit ASCII characters!
#define MAX_TEXTCYCLE 32                          // Maximum number of texts in one cycle
#define START_CYCLES  3                           // Number of cycles for the start message
