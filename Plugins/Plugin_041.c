//#######################################################################################################
//##                    This Plugin is only for use with the RFLink software package                   ##
//##                                          Plugin-43 LaCrosse                                       ##
//#######################################################################################################
/*********************************************************************************************\
 * This plugin takes care of decoding LaCrosse  weatherstation outdoor sensors
 * It also works for all non LaCrosse sensors that follow this protocol.
 * WS7000-15: Anemometer, WS7000-16: Rain precipitation, WS2500-19: Brightness Luxmeter, WS7000-20: Thermo/Humidity/Barometer
 *
 * Author             : StuntTeam
 * Support            : http://sourceforge.net/projects/rflink/
 * License            : This code is free for use in any open source project when this header is included.
 *                      Usage of any parts of this code in a commercial application is prohibited!
 *********************************************************************************************
 * Changelog: v1.0 initial release
 *********************************************************************************************
 * Meteo Sensor: (162 pulses)
 * Each frame is 80 bits long. It is composed of: 
 * 10 bits of 0 (preamble) 
 * 14 blocks of four bits separated by a 1 bit to be checked and skipped 
 *
 * 0100 0111 1000 0101 0010 0100 0000 0011 0001 0001 1000 1101 1110 1011 
 * aaaa bbbb cccc cccc cccc dddd dddd dddd eeee eeee eeee ffff gggg hhhh
 *
 * a = sensor type (04=Meteo sensor)
 * b = sensor address
 * c = temperature BCD, reversed
 * d = humidity BCD, reversed 
 * e = air pressure, BCD reversed + 200 offset in hPa
 * f = unknown?
 * g = xor value
 * h = checksum value
 *
 * Sample:
 * 20;07;DEBUG;Pulses=162;Pulses(uSec)=825,275,750,275,750,300,750,300,750,300,750,275,750,275,750,300,750,300,750,300,250,800,725,300,750,300,250,800,725,300,225,800,225,800,250,800,250,800,725,300,250,800,725,300,750,300,725,300,250,800,250,800,225,800,750,300,250,800,725,300,250,800,725,300,250,800,725,300,725,300,250,800,725,300,725,300,250,800,725,300,250,800,725,300,725,300,725,300,725,300,250,800,225,800,225,800,725,300,725,300,225,800,225,800,725,300,725,300,725,300,250,800,250,800,725,300,725,300,725,300,250,800,725,300,725,300,725,300,225,800,225,800,225,800,725,300,225,800,225,800,250,800,725,300,225,800,225,800,225,800,250,800,250,800,225,800,725,300,225,800,225,600;
 * 1010101010101010101001101001100101010110011010100101011001100110011010011010011001101010100101011010010110101001011010100110101001010110010101100101010101011001 00                                                                
   1111111111 0 1101 0 0001 0 1110 0 0101 0 1011 0 1101 0 1111 0 0011 0 0111 0 0111 0 1110 0 0100 0 1000 0 0010 0
 * 0000000000 1 0010 1 1110 1 0001 1 1010 1 0100 1 0010 1 0000 1 1100 1 1000 1 1000 1 0001 1 1011 1 0111 1 1101   0
 *              0010   1110   0001   1010   0100   0010   0000   1100   1000   1000   0001   1011   0111   1101 
 *              0100   0111   1000   0101   0010   0100   0000   0011   0001   0001   1000   1101   1110   1011
 * 4 7 852 403 118 D E B
 *    25.8 30.4 811+200
 * --------------------------------------------------------------------------------------------
 * Rain Packet: (92 pulses)
 * Each frame is 46 bits long. It is composed of: 
 * 10 bits of 0 (preamble) 
 * 7 blocks of four bits separated by a 1 bit to be checked and skipped 
 *
 * The 1st bit of each word is LSB, so we have to reverse the 4 bits of each word. 
 *  Example 
 * 0000000000 0010 1111 1011 0010 1011 1111 1101   
 *            aaaa bbbb ccc1 ccc2 ccc3 dddd eeee 
 *            2    F    B    2    B    F    D   
 *
 * a = sensor type (2=Rain meter)
 * b = sensor address 
 * c = rain data (LSB thus the right order is c3 c2 c1)
 * d = Check Xor : (2 ^ F ^ B ^ 2 ^ B ^ F) = 0
 * e = Check Sum : (const5 + 2 + F + B + 2 + B + F) and F = D   
 * --------------------------------------------------------------------------------------------
 * Wind packet: (122 pulses)
 * Each frame is composed of: 
 * 10bits of 0 (preamble) 
 * 10 blocks of four bits separated by a bit 1 to be checked and skipped 
 *
 * The 1st bit of each word is LSB, so we have to reverse the 4 bits of each word. 
 *  Example 
 * 0000000000 0011 0111 0101 0000 0001 0101 0100 0111 0110 1011  
 *            aaaa bbbb cccc cccc cccc dddd dddd ddee ffff gggg
 *            3    7    5    0    1   5   4   1 3   6   B  
 *
 * a = sensor type (2=Rain meter)
 * b = sensor address 
 * c = speed
 * d = direction
 * f = Check Xor 
 * g = Check Sum 
 * --------------------------------------------------------------------------------------------
 * UV packet (132 pulses)
 \*********************************************************************************************/
#define LACROSSE41_PULSECOUNT1 92       // Rain sensor
#define LACROSSE41_PULSECOUNT2 162      // Meteo sensor
#define LACROSSE41_PULSECOUNT3 122      // Wind sensor
#define LACROSSE41_PULSECOUNT4 132      // Brightness sensor

#define LACROSSE41_PULSEMID  500/RAWSIGNAL_SAMPLE_RATE

#ifdef PLUGIN_041
boolean Plugin_041(byte function, char *string) {
      boolean success=false;
      if ( (RawSignal.Number != LACROSSE41_PULSECOUNT1) && (RawSignal.Number != LACROSSE41_PULSECOUNT2) && 
      (RawSignal.Number != LACROSSE41_PULSECOUNT3) && (RawSignal.Number != LACROSSE41_PULSECOUNT4) ) return false; 
      
      unsigned long bitstream1=0L;                  // holds first 16 bits 
      unsigned long bitstream2=0L;                  // holds last 28 bits

      int sensor_data=0;

      byte checksum=0;

      byte bitcounter=0;                            // counts number of received bits (converted from pulses)
      byte bytecounter=0;                           // used for counting the number of received bytes
      byte data[18];
      //==================================================================================
      // Check preamble
      for(int x=1;x<20;x+=2) {
         if ((RawSignal.Pulses[x] < LACROSSE41_PULSEMID) || (RawSignal.Pulses[x+1] > LACROSSE41_PULSEMID) ) {
            return false;                           // bad preamble bit detected, abort
         }
      }         

      if ((RawSignal.Pulses[21] > LACROSSE41_PULSEMID) || (RawSignal.Pulses[22] < LACROSSE41_PULSEMID) ) { 
         return false;                              // There should be a 1 bit after the preamble
      }
      // get bits/nibbles
      for(int x=23;x<RawSignal.Number-2;x+=2) {
         if (RawSignal.Pulses[x] < LACROSSE41_PULSEMID) {  // 1 bit
            data[bytecounter] = ((data[bytecounter] >> 1) | 0x08);   // 1 bit, store in reversed bit order
         } else {
            data[bytecounter] = ((data[bytecounter] >> 1)&0x07);     // 0 bit, store in reversed bit order
         }
         bitcounter++;
         if (bitcounter == 4) { 
            x=x+2;
            if (x > RawSignal.Number-2) break;      // dont check the last marker
            if ((RawSignal.Pulses[x] > LACROSSE41_PULSEMID) || (RawSignal.Pulses[x+1] < LACROSSE41_PULSEMID) ) { 
                return false;                       // There should be a 1 bit after each nibble
            }
            bitcounter=0;
            bytecounter++;
            if (bytecounter > 17) return false;     // received too many nibbles/bytes, abort
         }
      }
      //==================================================================================
      // all bytes received, make sure checksum is okay
      //==================================================================================
      // check xor value
      checksum=0;
      for (byte i=0;i<bytecounter;i++){ 
          checksum=checksum^data[i];
      }
      if (checksum !=0) return false;               // all (excluding last) nibbles xored must result in 0
      // check checksum value
      checksum=5;
      for (byte i=0;i<bytecounter;i++){ 
          checksum=checksum + data[i];
      }
      checksum=checksum & 0x0f;
      if (checksum != (data[bytecounter])) return false;  // all (excluding last) nibble added must result in last nibble value
      //==================================================================================
      // Prevent repeating signals from showing up, skips every second packet!
      //==================================================================================
      unsigned long tempval=(data[4])>>1;
      tempval=((tempval)<<16)+((data[3])<<8)+data[2];
      if( (SignalHash!=SignalHashPrevious) || (RepeatingTimer<millis()) || (SignalCRC != tempval)  ){ 
         // not seen this RF packet recently
         SignalCRC=tempval;
      } else {
         return true;         // already seen the RF packet recently, but still want the humidity
      }  
      //==================================================================================
      // now process the various sensor types      
      //==================================================================================
      // Output
      // ----------------------------------
      if (data[0] == 0x04) {                        // Meteo sensor
         sprintf(pbuffer, "20;%02X;", PKSequenceNumber++); // Node and packet number 
         Serial.print( pbuffer );
         Serial.print(F("LaCrosseV3;ID="));           // Label
         PrintHex8( data,2);

         sensor_data = data[4]*100;
         sensor_data = sensor_data + data[3]*10;
         sensor_data = sensor_data + data[2];
         sprintf(pbuffer, ";TEMP=%04x;", sensor_data);
         Serial.print( pbuffer );

         sensor_data = data[7]*10;
         sensor_data = sensor_data + data[6];
         sprintf(pbuffer, "HUM=%04d;", sensor_data);     
         Serial.print( pbuffer );

         sensor_data = data[10]*100;
         sensor_data = sensor_data + data[9]*10;
         sensor_data = sensor_data + data[8];
         sensor_data = sensor_data + 200;
         sprintf(pbuffer, "BARO=%04x;", sensor_data);     
         Serial.println( pbuffer );
         
         RawSignal.Repeats=false; 
         RawSignal.Number=0;
         success=true;
      } else
      if (data[0] == 0x02) {                        // Rain sensor
         sprintf(pbuffer, "20;%02X;", PKSequenceNumber++); // Node and packet number 
         Serial.print( pbuffer );
         Serial.print(F("LaCrosseV3;ID="));         // Label
         PrintHex8( data,2);

         sensor_data = data[4]*100;
         sensor_data = sensor_data + data[3]*10;
         sensor_data = sensor_data + data[2];
         sprintf(pbuffer, ";RAIN=%04x;", sensor_data);     
         Serial.println( pbuffer );
         
         RawSignal.Repeats=false; 
         RawSignal.Number=0;
         success=true;
      } else
      if (data[0] == 0x03) {                        // wind sensor
         sprintf(pbuffer, "20;%02X;", PKSequenceNumber++); // Node and packet number 
         Serial.print( pbuffer );
         Serial.print(F("LaCrosseV3;ID="));         // Label
         PrintHex8( data,2);

         sensor_data = data[4]*100;
         sensor_data = sensor_data + data[3]*10;
         sensor_data = sensor_data + data[2];
         sprintf(pbuffer, ";WINSP=%04x;", sensor_data);     
         Serial.print( pbuffer );

         sensor_data = ((data[7])>>2)*100;
         sensor_data = sensor_data + data[6]*10;
         sensor_data = sensor_data + data[5];
         sensor_data = sensor_data / 22.5;
         sprintf(pbuffer, "WINDIR=%04x;", sensor_data);     
         Serial.println( pbuffer );
         
         RawSignal.Repeats=false; 
         RawSignal.Number=0;
         success=true;
      } else
      if (data[0] == 0x05) {                        // UV sensor
         sprintf(pbuffer, "20;%02X;", PKSequenceNumber++); // Node and packet number 
         Serial.print( pbuffer );
         Serial.print(F("LaCrosseV3;ID="));         // Label
         PrintHex8( data,2);

         sensor_data = data[4]*100;
         sensor_data = sensor_data + data[3]*10;
         sensor_data = sensor_data + data[2];
         sprintf(pbuffer, ";UV=%04x;", sensor_data);     
         Serial.println( pbuffer );

         RawSignal.Repeats=false; 
         RawSignal.Number=0;
         success=true;
      }
      //==================================================================================
      return success;
}
#endif // PLUGIN_041
