//#######################################################################################################
//#################################### Plugin-251 4-voudige PulseCounter ################################
//#######################################################################################################

/*********************************************************************************************\
 * Dit device zorgt voor ondersteuning van een 4-voudige pulsteller met 1 enkele Nodo Mega of Small.
 * Is vooral bedoeld als "Meterkast Slave" unit voor het meten van zowel Gas, Elektra en Water met 1 Nodo.
 *
 * Auteur             : esp8266nu
 * Support            : www.esp8266.nu
 * Datum              : 1 Okt 2016
 * Versie             : 1.0
 * Nodo productnummer : 
 * Compatibiliteit    : 
 * Syntax             : 
 ***********************************************************************************************
 * Technische beschrijving:
 * We gebruiken de Nodo WiredIn aansluitingen als digitale pulstellers.
 * Het is n.l. mogelijk om de diverse Atmel pinnen voor alternatieve functies te gebruiken.
 * De WiredIn pinnen zijn standaard analoog, maar ook digitaal te gebruiken en daar maken we gebruik van.
 * Met dit device is het mogelijk om ook zeer korte pulsen te tellen, hetgeen met de normale Nodo WiredIn functie niet lukt.
 * Dit device gebruikt twee variabelen voor elke pulsteller, 1 voor de pulsecount en 1 voor de pulsetime
 * Deze variabelen worden alleen gebruikt na het geven van het commando pulse x,y
 \*********************************************************************************************/

#include "Arduino.h"
#include <avr/interrupt.h>

#ifdef PLUGIN_TX_251

void SetPinIRQ(byte _receivePin);
inline void pulse_handle_interrupt();
boolean plugin_251_init=false;

volatile uint8_t *_receivePortRegister;
volatile unsigned long PulseCounter[4]={0,0,0,0};
volatile unsigned long PulseTimer[4]={0,0,0,0};
volatile unsigned long PulseTimerPrevious[4]={0,0,0,0};
volatile byte PulseState[4]={0,0,0,0};

boolean Plugin_251(byte function, char *string)
{
  return false;
}

boolean PluginTX_251(byte function, char *string)
{
  boolean success=false;
  char TmpStr1[80];
  TmpStr1[0] = 0;
  long Par1 = 0;
  long Par2 = 0;
  if (Plugin_250_GetArgv(string, TmpStr1, 3)) Par1 = atol(TmpStr1);
  if (Plugin_250_GetArgv(string, TmpStr1, 4)) Par2 = atol(TmpStr1);

  if (strncasecmp_P(InputBuffer_Serial+3,PSTR("COUNT"),5) == 0)
  {
    if (!plugin_251_init)
    {
      SetPinIRQ(A0);
      SetPinIRQ(A1);
      SetPinIRQ(A2);
      SetPinIRQ(A3);
    }

    // Store countervalue into uservar and reset internal counter
    unsigned long counter = PulseCounter[Par1-1];
    PulseCounter[Par1-1]=0;

    // If actual time difference > last known pulstime, update pulstimer now.
    if ((millis() - PulseTimerPrevious[Par1-1]) > PulseTimer[Par1-1])
      PulseTimer[Par1-1] = millis() - PulseTimerPrevious[Par1-1];

    if (PulseTimer[Par1-1] > 999999) PulseTimer[Par1-1] = 999999; // WebApp cannot handle larger values! (bug or by design?)

    sprintf(pbuffer,"20;%02X;ESPEASY;PulseCount%u", PKSequenceNumber++, Par1);
    Serial.print(pbuffer); 
    sprintf(pbuffer,"=%u", counter);
    Serial.println(pbuffer);

    delay(200);
    unsigned long time = PulseTimer[Par1-1];
    sprintf(pbuffer,"20;%02X;ESPEASY;PulseTime%u", PKSequenceNumber++, Par1);
    Serial.print(pbuffer); 
    sprintf(pbuffer,"=%u", time);
    Serial.println(pbuffer);

    success=true;
  }

  return success;

}


void SetPinIRQ(byte _receivePin)
{
  uint8_t _receiveBitMask;
  pinMode(_receivePin, INPUT);
  digitalWrite(_receivePin, HIGH);
  _receiveBitMask = digitalPinToBitMask(_receivePin);
  uint8_t port = digitalPinToPort(_receivePin);
  _receivePortRegister = portInputRegister(port);
  if (digitalPinToPCICR(_receivePin))
    {
      *digitalPinToPCICR(_receivePin) |= _BV(digitalPinToPCICRbit(_receivePin));
      *digitalPinToPCMSK(_receivePin) |= _BV(digitalPinToPCMSKbit(_receivePin));
    }
}

/*********************************************************************/
inline void pulse_handle_interrupt()
/*********************************************************************/
{
  byte portstate= *_receivePortRegister & 0xf;
  for (byte x=0; x<4; x++)
    {
      if ((portstate & (1 << x)) == 0)
        {
          if (PulseState[x] == 1)
            {
              PulseCounter[x]++;
              PulseTimer[x]=millis()-PulseTimerPrevious[x];
              PulseTimerPrevious[x]=millis();
            }
          PulseState[x]=0;
        }
      else
        PulseState[x]=1;
    }
}

#if defined(PCINT1_vect)
ISR(PCINT1_vect) { pulse_handle_interrupt(); }
#endif
#if defined(PCINT2_vect)
ISR(PCINT2_vect) { pulse_handle_interrupt(); }
#endif

#endif // Plugin_TX_251