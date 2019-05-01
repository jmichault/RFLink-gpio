#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>


#include "config.h"

#define BUFFER_SIZE 256

int Verb=0;

int PIN_RF_TX_DATA=0;
int PIN_RF_RX_DATA=2;

static unsigned long timingsTemp[BUFFER_SIZE+1];
static unsigned long duration = 0;
static unsigned long oldDuration = 0;
static unsigned long lastTime = 0;
static unsigned long Sync = 0;
static unsigned int ringIndex = 0;
static unsigned int syncCount = 0;
static unsigned int ATraiter = 0;
static int affiche=0;

struct timespec clockNext;

void  delayMicroseconds (unsigned int microSecs)
{
  clock_gettime(CLOCK_MONOTONIC, &clockNext);
  clockNext.tv_nsec += (microSecs-10) * (1000-10);
  if(clockNext.tv_nsec  >= 1000000000)
  {
    clockNext.tv_nsec -= 1000000000;
    clockNext.tv_sec++;
  }
  clock_nanosleep(CLOCK_MONOTONIC,TIMER_ABSTIME,&clockNext,NULL);

}

static int cmpUlong(const void * p1, const void *p2)
{
    return ( *(unsigned long * )p1 > *(unsigned long *)p2 ? 1:0 );
}

void handler()
{
// int valeur=digitalRead(PIN_RF_RX_DATA);
 long time = micros();
  oldDuration=duration;
  duration = time - lastTime;
  lastTime = time;
//printf(" d : %ld\n",duration);
  if(!syncCount && (duration >2500) && (duration< 100000) )
  { // signal long : début de séquence
    timingsTemp[syncCount++]=oldDuration;
    timingsTemp[syncCount++]=duration;
    if(duration<10000) Sync=duration; else Sync=17000;
  }
  else if(duration<100)
  { // anomalie : on repart à zéro
     // fprintf(stderr,"signal trop court.\n");
    syncCount=0;
  }
  else if( duration>(Sync*8)/10  && syncCount)
  {// fin de séquence
    timingsTemp[syncCount++]=duration;
    if( ATraiter)       // données précédentes pas encore traitées
    {
      syncCount=0;
//      fprintf(stderr,"trame avant fin de traitement précédent.\n");
      return;
    }
    if(syncCount<8)     // pas assez de données
    {
//      fprintf(stderr,"trame trop courte, lgr=%d Sync=%ld.\n",syncCount,Sync);
      syncCount=0;
      timingsTemp[syncCount++]=oldDuration;
      timingsTemp[syncCount++]=duration;
    if(duration<10000) Sync=duration; else Sync=17000;
//      struct timespec tp;
//      clock_gettime(CLOCK_REALTIME_COARSE,&tp);
//      printf("\n au temps %ld.%02ld \n",tp.tv_sec,tp.tv_nsec/10000000);
      return;
    }
    RawSignal.Number=syncCount-2;
    memmove(RawSignal.Pulses,timingsTemp+1,syncCount*sizeof(long));
    ATraiter=syncCount;
//    struct timespec tp;
//    clock_gettime(CLOCK_REALTIME_COARSE,&tp);
//    printf("\n fin au temps %ld.%02ld \n",tp.tv_sec,tp.tv_nsec/10000000);
    syncCount=0;
    timingsTemp[syncCount++]=oldDuration;
    timingsTemp[syncCount++]=duration;
  }
  else if( syncCount>=BUFFER_SIZE )
  {// pas possible à traiter
//      fprintf(stderr,"trame trop longue.\n");
    syncCount=0;
  }
  else if(syncCount)
    timingsTemp[syncCount++]=duration;
}

extern char *optarg;
extern int optind, opterr, optopt;



CSerial Serial;

// to include Config_xx.c files:
#define stringify(x) #x
#define CONFIGFILE2(a, b) stringify(a/Config/b)
#define CONFIGFILE(a, b) CONFIGFILE2(a, b)
#define CONFIG_FILE Config_01.c
#include CONFIGFILE(SKETCH_PATH,CONFIG_FILE)


//****************************************************************************************************************************************
byte dummy=1;                                                                   // get the linker going. Bug in Arduino. (old versions?)

void(*Reboot)(void)=0;                                                          // reset function on adres 0.
byte PKSequenceNumber=0;                                                        // 1 byte packet counter
boolean RFDebug=false;                                                          // debug RF signals with plugin 001 
boolean RFUDebug=false;                                                         // debug RF signals with plugin 254 
boolean QRFDebug=false;                                                         // debug RF signals with plugin 254 but no multiplication

uint8_t RFbit;                                                           // for processing RF signals.

char pbuffer[PRINT_BUFFER_SIZE];                                                // Buffer for printing data
char InputBuffer_Serial[INPUT_COMMAND_SIZE];                                    // Buffer for Seriel data

// Van alle devices die worden mee gecompileerd, worden in een tabel de adressen opgeslagen zodat hier naar toe gesprongen kan worden
void PluginInit(void);
void PluginTXInit(void);
byte Plugin_id[PLUGIN_MAX];
byte PluginTX_id[PLUGIN_TX_MAX];

void PrintHexByte(uint8_t data);                                                // prototype
byte reverseBits(byte data);                                                    // prototype
void RFLinkHW( void )
{
//FIXME
}

// ===============================================================================
unsigned long RepeatingTimer=0L;
unsigned long SignalCRC=0L;                                                     // holds the bitstream value for some plugins to identify RF repeats
unsigned long SignalHash=0L;                                                    // holds the processed plugin number
unsigned long SignalHashPrevious=0L;                                            // holds the last processed plugin number

void setup() {
 //start TCP server in a new thread :
 pthread_t server_thread;
  if(pthread_create(&server_thread,NULL,begin, NULL) < 0)
        {
            perror("could not create thread");
            exit(1);
        }

  pinMode(PIN_RF_RX_DATA, INPUT);                                               // Initialise in/output ports
  pinMode(PIN_RF_TX_DATA, OUTPUT);                                              // Initialise in/output ports
  //FIXME digitalWrite(PIN_RF_RX_DATA,INPUT_PULLUP);                                    // pull-up resister on (to prevent garbage)
  
  //FIXME RFbit=digitalPinToBitMask(PIN_RF_RX_DATA);
  Serial.print("20;00;Nodo RadioFrequencyLink - RFLink Gateway V1.1 - ");
  sprintf(InputBuffer_Serial,"R%02x;",REVNR);
  Serial.println(InputBuffer_Serial); 

  PKSequenceNumber++;
  PluginInit();
  PluginTXInit();
}

void loop() {
  byte SerialInByte=0;                                                          // incoming character value
  int SerialInByteCounter=0;                                                    // number of bytes counter 

  byte ValidCommand=0;
  unsigned long FocusTimer=0L;                                                  // Timer to keep focus on the task during communication
  InputBuffer_Serial[0]=0;                                                      // erase serial buffer string 

  while(true) {
    ScanEvent();                                                                // Scan for RF events
    // SERIAL: *************** kijk of er data klaar staat op de seriele poort **********************
    if(Serial.available()) {
      FocusTimer=millis()+FOCUS_TIME;

      while(FocusTimer>millis()) {                                              // standby 
        if(Serial.available()) {
          SerialInByte=Serial.read();                
          
          if(isprint(SerialInByte))
            if(SerialInByteCounter<(INPUT_COMMAND_SIZE-1))
              InputBuffer_Serial[SerialInByteCounter++]=SerialInByte;
              
          if(SerialInByte=='\n') {                                              // new line character
            InputBuffer_Serial[SerialInByteCounter]=0;                          // serieel data is complete
            //Serial.print("20;incoming;"); 
            //Serial.println(InputBuffer_Serial); 
            if (strlen(InputBuffer_Serial) > 7){} // if > 7
            if (ValidCommand != 0) {
               if (ValidCommand==1) {
                  sprintf(InputBuffer_Serial,"20;%02X;OK;",PKSequenceNumber++);
                  Serial.println( InputBuffer_Serial ); 
               } else {
                  sprintf(InputBuffer_Serial, "20;%02X;CMD UNKNOWN;", PKSequenceNumber++); // Node and packet number 
                  Serial.println( InputBuffer_Serial );
               }   
            }
            SerialInByteCounter=0;  
            InputBuffer_Serial[0]=0;                                            // serial data has been processed. 
            ValidCommand=0;
            FocusTimer=millis()+FOCUS_TIME;                                             
          }// if(SerialInByte
       }// if(Serial.available())
    }// while 
   }// if(Serial.available())
  }// while 
} // void
/*********************************************************************************************/
int main(int argc, char *argv[])
{
  int opt;
  while( (opt=getopt(argc,argv,"vi:o:")) != -1)
  {
    switch(opt)
    {
     case 'i' : // input pin
      PIN_RF_RX_DATA=atoi(optarg);
      break;
     case 'o' : // output pin
      PIN_RF_TX_DATA=atoi(optarg);
      break;
     case 'v' :
      Verb++;
      break;
    }
  }

  if(wiringPiSetup() == -1)
  {
    printf("no wiring pi detected\n");
    return 0;
  }

  setup();
  lastTime = micros();
  wiringPiISR(PIN_RF_RX_DATA,INT_EDGE_BOTH,&handler);
  while(true){
    if(ATraiter)
    {
      //printf("données reçues.\n");
      PluginRXCall(0,0);
      ATraiter=0;
    }
    else
      delay(20);
  }

//loop();
}
