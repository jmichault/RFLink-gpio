#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>


#include "config.h"


int sockfd, connfd, len; 
struct sockaddr_in servaddr, cli; 
#define PORT 1969

fd_set active_fd_set, read_fd_set;

int CSerial::available()
{
//FIXME
return 0;
};

#define MAXMSG  512

int
read_from_client (int filedes)
{
  char buffer[MAXMSG];
  bzero(buffer,sizeof(buffer));
  int nbytes;

  nbytes = read (filedes, buffer, MAXMSG);
  if (nbytes < 0)
    {
      /* Read error. */
      perror ("read");
      //exit (EXIT_FAILURE);
    }
  else if (nbytes == 0)
    /* End-of-file. */
    return -1;
  else
    {
      /* Data read. */
      fprintf (stderr, "Server: got message: `%s'\n", buffer);
      byte ValidCommand=0;
      int SerialInByteCounter=0;
            if (strlen(buffer) > 7)         // need to see minimal 8 characters on the serial port
 	    {                                // need to see minimal 8 characters on the serial port
               // 10;....;..;ON; 
               if (strncmp (buffer,"10;",3) == 0) {                 // Command from Master to RFLink
                  // -------------------------------------------------------
                  // Handle Device Management Commands
                  // -------------------------------------------------------
                  if (strncasecmp(buffer+3,"PING;",5) == 0) {
                     sprintf(buffer,"20;%02X;PONG;\n",PKSequenceNumber++);
                     write(filedes,buffer,strlen(buffer));
                     //Serial.println(buffer); 
                  } else
                  if (strcasecmp(buffer+3,"REBOOT;")==0) {
                     strcpy(buffer,"reboot");
                     //FIXME Reboot();
                  } else
                  if (strncasecmp(buffer+3,"RFDEBUG=O",9) == 0) {
                     if (buffer[12] == 'N' || buffer[12] == 'n' ) {
                        RFDebug=true;                                           // full debug on
                        RFUDebug=false;                                         // undecoded debug off 
                        QRFDebug=false;                                        // undecoded debug off
                        sprintf(buffer,"20;%02X;RFDEBUG=ON;",PKSequenceNumber++);
                     } else {
                        RFDebug=false;                                          // full debug off
                        sprintf(buffer,"20;%02X;RFDEBUG=OFF;",PKSequenceNumber++);
                     }
                     Serial.println(buffer); 
                  } else                 
                  if (strncasecmp(buffer+3,"RFUDEBUG=O",10) == 0) {
                     if (buffer[13] == 'N' || buffer[13] == 'n') {
                        RFUDebug=true;                                          // undecoded debug on 
                        QRFDebug=false;                                        // undecoded debug off
                        RFDebug=false;                                          // full debug off
                        sprintf(buffer,"20;%02X;RFUDEBUG=ON;",PKSequenceNumber++);
                     } else {
                        RFUDebug=false;                                         // undecoded debug off
                        sprintf(buffer,"20;%02X;RFUDEBUG=OFF;",PKSequenceNumber++);
                     }
                     Serial.println(buffer); 
                  } else                 
                  if (strncasecmp(buffer+3,"QRFDEBUG=O",10) == 0) {
                     if (buffer[13] == 'N' || buffer[13] == 'n') {
                        QRFDebug=true;                                         // undecoded debug on 
                        RFUDebug=false;                                         // undecoded debug off 
                        RFDebug=false;                                          // full debug off
                        sprintf(buffer,"20;%02X;QRFDEBUG=ON;",PKSequenceNumber++);
                     } else {
                        QRFDebug=false;                                        // undecoded debug off
                        sprintf(buffer,"20;%02X;QRFDEBUG=OFF;",PKSequenceNumber++);
                     }
                     Serial.println(buffer); 
                  } else                 
                  if (strncasecmp(buffer+3,"VERSION",7) == 0) {
                      sprintf(buffer,"20;%02X;VER=1.1;REV=%02x;BUILD=%02x;\n",PKSequenceNumber++,REVNR, BUILDNR);
                     write(filedes,buffer,strlen(buffer));
                      //Serial.println(buffer); 
                  } else {
                     // -------------------------------------------------------
                     // Handle Generic Commands / Translate protocol data into Nodo text commands 
                     // -------------------------------------------------------
                     // check plugins
                     if (buffer[SerialInByteCounter-1]==';') buffer[SerialInByteCounter-1]=0;  // remove last ";" char
strcpy(InputBuffer_Serial,buffer);
                     if(PluginTXCall(0, buffer)) {
                        ValidCommand=1;
                     } else {
                        // Answer that an invalid command was received?
                        ValidCommand=2;
                     }
                  }
               }
            } 
      return 0;
    }
}

void *begin(void *)
{
// socket create and verification 
 size_t size;
    sockfd = socket(AF_INET, SOCK_STREAM, 0); 
    if (sockfd == -1) { 
        printf("socket creation failed...\n"); 
        exit(0); 
    } 
    else
        printf("Socket successfully created..\n"); 
        /* enable SO_REUSEADDR */
        int reusaddr = 1;
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reusaddr, sizeof(int)) < 0) {
            perror ("setsockopt(SO_REUSEADDR) failed");
            exit(0); 
        }

    bzero(&servaddr, sizeof(servaddr)); 
  
    // assign IP, PORT 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    servaddr.sin_port = htons(PORT); 
  
    // Binding newly created socket to given IP and verification 
    if ((bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr))) != 0) { 
        printf("socket bind failed...\n"); 
        exit(0); 
    } 
    else
        printf("Socket successfully binded..\n"); 
  
    // Now server is ready to listen and verification 
    if ((listen(sockfd, 5)) != 0) { 
        printf("Listen failed...\n"); 
        exit(0); 
    } 
    else
        printf("Server listening..\n"); 
/* Initialize the set of active sockets. */
  FD_ZERO (&active_fd_set);
  FD_SET (sockfd, &active_fd_set);
  while (1)
    {
      /* Block until input arrives on one or more active sockets. */
      read_fd_set = active_fd_set;
      if (select (FD_SETSIZE, &read_fd_set, NULL, NULL, NULL) < 0)
        {
          perror ("select");
          exit (EXIT_FAILURE);
        }

      /* Service all the sockets with input pending. */
      for ( int i = 0; i < FD_SETSIZE; ++i)
        if (FD_ISSET (i, &read_fd_set))
          {
            if (i == sockfd)
              {
                /* Connection request on original socket. */
                int newsock;
                size = sizeof (servaddr);
                newsock = accept (sockfd,
                              (struct sockaddr *) &servaddr,
                              &size);
                if (newsock < 0)
                  {
                    perror ("accept");
                    exit (EXIT_FAILURE);
                  }
                fprintf (stderr,
                         "Server: connect from host %s, port %hd.\n",
                         inet_ntoa (servaddr.sin_addr),
                         ntohs (servaddr.sin_port));
                FD_SET (newsock, &active_fd_set);
  sprintf(InputBuffer_Serial,"20;00;Nodo RadioFrequencyLink - RFLink Gateway V1.1 - R%02x\n;",REVNR);
  write(newsock,InputBuffer_Serial,strlen(InputBuffer_Serial)); 
              }
            else
              {
                /* Data arriving on an already-connected socket. */
                if (read_from_client (i) < 0)
                  {
                    close (i);
                    FD_CLR (i, &active_fd_set);
                  }
              }
          }
    }

};

int CSerial::print(long x)
{
static char buffer[256];
printf("print long %ld\n",x);
sprintf(buffer,"%ld",x);
print(buffer);
};

int CSerial::print(const char* str)
{
  for (int i = 0; i < FD_SETSIZE; ++i)
    if (FD_ISSET (i, &read_fd_set))
    {
      if (i != sockfd)
      {
	::write(i ,str,strlen(str));
      }
    }
};

int CSerial::print(const char* str,int mode)
{
//FIXME
printf("FIXME print(char*,int) %s;%d",str,mode);
};

int CSerial::println(const char* str)
{
static char buffer[256];
  for (int i = 0; i < FD_SETSIZE; ++i)
    if (FD_ISSET (i, &read_fd_set))
    {
      if (i != sockfd)
      {
        int len=strlen(str);
	memcpy(buffer,str,len);
	buffer[len]='\n';
	buffer[len+1]=0;
	::write(i ,buffer,len+1);
      }
    }

};

int CSerial::println(const uint32_t& val,int mode)
{
  if(mode==HEX)
  {
   char buf[10];
   sprintf(buf,"%lx",val);
   println(buf);
  }
  else if(mode==BIN)
  {
   char buf[33];
   int pos=0;
   int i;
   for(i=32;i>0; i--)
     if (val &(1<<i)) break;
   for ( ;i>0; i--)
     buf[pos++]= (val& (1 << i))? '1':'0';
   buf[pos]=0;
   println(buf);
  }
  else
    //FIXME
    printf("FIXME println(uint32,int) %ld;%d",val,mode);
};

int CSerial::read()
{
//FIXME
printf("FIXME read\n");
};

size_t CSerial::write(const int chr)
{
  for (int i = 0; i < FD_SETSIZE; ++i)
    if (FD_ISSET (i, &read_fd_set))
    {
      if (i != sockfd)
      {
	::write(i ,&chr,1);
      }
    }
};


void pinMode(int pin,int mode)
{
//FIXME
printf("FIXME pinMode %d;%d\n",pin,mode);
}
