#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <wiringPi.h>
#include "serial.h"

#define byte uint8_t
#define boolean char
#define word uint16_t
#define false 0
#define true 1
#define F(x) x


#define SKETCH_PATH .
#define BUILDNR                         0x07                                    // shown in version
#define REVNR                           0x33                                    // shown in version and startup string
#define MIN_RAW_PULSES                    20                                    // =8 bits. Minimal number of bits*2 that need to have been received before we spend CPU time on decoding the signal.
// Sample width / resolution in uSec for raw RF pulses :
#define RAWSIGNAL_SAMPLE_RATE              1
#define MIN_PULSE_LENGTH                  25                                    // Pulses shorter than this value in uSec. will be seen as garbage and not taken as actual pulses.
#define SIGNAL_TIMEOUT                     7                                    // Timeout, after this time in mSec. the RF signal will be considered to have stopped.
#define SIGNAL_REPEAT_TIME               500                                    // Time in mSec. in which the same RF signal should not be accepted again. Filters out retransmits.
#define BAUD                           57600                                    // Baudrate for serial communication.
#define TRANSMITTER_STABLE_DELAY         500                                    // delay to let the transmitter become stable (Note: Aurel RTX MID needs 500µS/0,5ms).
#define RAW_BUFFER_SIZE                  512                                    // Maximum number of pulses that is received in one go.
#define PLUGIN_MAX                        55                                    // Maximum number of Receive plugins
#define PLUGIN_TX_MAX                     26                                    // Maximum number of Transmit plugins
#define SCAN_HIGH_TIME                    50                                    // tijdsinterval in ms. voor achtergrondtaken snelle verwerking
#define FOCUS_TIME                        50                                    // Duration in mSec. that, after receiving serial data from USB only the serial port is checked. 
#define INPUT_COMMAND_SIZE                60                                    // Maximum number of characters that a command via serial can be.
#define PRINT_BUFFER_SIZE                 60                                    // Maximum number of characters that a command should print in one go via the print buffer.

// PIN Definition 
//#define PIN_RF_TX_DATA              0                                          // Data to the 433Mhz transmitter on this pin
extern int PIN_RF_TX_DATA;
//#define PIN_RF_RX_DATA              2                                          // On this input, the 433Mhz-RF signal is received. LOW when no signal.
extern int PIN_RF_RX_DATA;
// FIXME obsolete PINs used in some plugins
#define PIN_RF_TX_VCC			0
#define PIN_RF_RX_VCC			0

// 
#define VALUE_PAIR                      44
#define VALUE_ALLOFF                    55
#define VALUE_OFF                       74
#define VALUE_ON                        75
#define VALUE_DIM                       76
#define VALUE_BRIGHT                    77
#define VALUE_UP                        78
#define VALUE_DOWN                      79
#define VALUE_STOP                      80
#define VALUE_CONFIRM                   81
#define VALUE_LIMIT                     82
#define VALUE_ALLON                     141

// main.cpp
extern boolean RFDebug;
extern CSerial Serial;
extern byte PKSequenceNumber;                                                        // 1 byte packet counter
extern char pbuffer[PRINT_BUFFER_SIZE];                                                // Buffer for printing data
extern boolean RFUDebug;                                                         // debug RF signals with plugin 254 
extern boolean QRFDebug;                                                         // debug RF signals with plugin 254 but no multiplication
extern char InputBuffer_Serial[INPUT_COMMAND_SIZE];                                    // Buffer for Seriel data
void RFLinkHW( void );                                                          // prototype
extern unsigned long SignalCRC;                                                     // holds the bitstream value for some plugins to identify RF repeats


// Plugin.cpp
byte PluginRXCall(byte Function, char *str);
extern byte Plugin_id[PLUGIN_MAX];
extern byte PluginTX_id[PLUGIN_TX_MAX];
extern boolean (*Plugin_ptr[PLUGIN_MAX])(byte, char*);                                 // Receive plugins
extern boolean (*PluginTX_ptr[PLUGIN_TX_MAX])(byte, char*);                            // Transmit plugins
byte PluginTXCall(byte Function, char *str);

extern unsigned long SignalHash;                                                    // holds the processed plugin number
extern unsigned long SignalHashPrevious;                                            // holds the last processed plugin number

// RawSignal.cpp
extern unsigned long RepeatingTimer;
boolean ScanEvent(void);
struct RawSignalStruct                                                          // Raw signal variabelen places in a struct
  {
  int  Number;                                                                  // Number of pulses, times two as every pulse has a mark and a space.
  byte Repeats;                                                                 // Number of re-transmits on transmit actions.
  byte Delay;                                                                   // Delay in ms. after transmit of a single RF pulse packet
  byte Multiply;                                                                // Pulses[] * Multiply is the real pulse time in microseconds 
  unsigned long Time;                                                           // Timestamp indicating when the signal was received (millis())
  uint32_t Pulses[RAW_BUFFER_SIZE+2];                                               // Table with the measured pulses in microseconds divided by RawSignal.Multiply. (halves RAM usage)
                                                                                // First pulse is located in element 1. Element 0 is used for special purposes, like signalling the use of a specific plugin
};
extern struct RawSignalStruct RawSignal;
void RawSendRF(void);

//Serial.cpp
void pinMode(int pin,int mode);

// Misc.cpp
void PrintHex8(uint32_t *data, uint32_t length);                                  // prototype
void PrintHex8(uint8_t *data, uint32_t length);                                  // prototype
float ul2float(unsigned long ul);
unsigned long str2int(char *string);
int str2cmd(char *command);
void PrintHexByte(uint8_t data);
long map(long x, long in_min, long in_max, long out_min, long out_max);
