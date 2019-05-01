
CPPFLAGS=-g

RFLink-gpio : main.o Plugin.o RawSignal.o serial.o Misc.o
	gcc -g -o $@ main.o Plugin.o RawSignal.o serial.o Misc.o -lwiringPi -lpthread
