# RFLink-gpio
Experimental port of RFLink arduino sources to raspberry.

Unfortunately, since RFLink sources are now closed, the plugins are very old releases.

Allow to emulate a RFLink with RF modules directly connected to GPIO ports.


Usage :

1°) connect RF modules

Connect sender data on pin 17 (gpio 0, bcm 17)
Connect receiver data on pin 13 (gpio 2, bcm 27)
connect ground of both modules to some ground pins on GPIO.
connect Vcc of both modules to some Vcc pins on GPIO.

2°) run RFLink-gpio

3°) in domoticz :

add hardware , type «RFLink Gateway with LAN interface»
        adress : 127.0.0.1
        Port : 1969
