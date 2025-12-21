# Connections to the instrument cluster
##Connector on the PCB
If you are using my PCB design, you can use a 'Vector standard' DSUB9 connector to provide the CAN and the supply voltage.
Other side of the DSUB9 goes directly to the PCB (the board goes between the two rows of terminals)
Pinout on the DSUB9 is:
- Pin#2 -> CAN Low
- Pin#3 -> GND
- Pin#7 -> CAN High
- Pin#9 -> +12V (battery)
Reference: https://www.csselectronics.com/cdn/shop/files/DB9-Generic-Adapter-Cable-Pinout.svg
## Connector on the cluster
**- Pin #1 -> Battery +12V**
**- Pin #2 -> Battery +12V**
**- Pin #3 -> GND**
**- Pin #4 -> GND**
**- Pin #5 -> Ignition (active high, 12V)**
**- Pin #18 -> Battery +12V**
**- Pin #19 -> GND**
**- Pin #29 -> CAN High**
**- Pin #30 -> CAN Low**
- Pin #7 -> Break pad wear sensor (switch to GND)
- Pin #8 -> Engine oil sensor
- Pin #14 -> Brake fluid level (switch to pin#28)
- Pin #15 -> Engine oil pressure (switch to GND)
- Pin #17 -> AC/Heater control
- Pin #20 -> Low washer fluid level (switch to pin#28)
- Pin #21 -> Engine coolant level (switch to pin#28)
- Pin #24 -> Ambient temp (outside) sensor (other side is pin#28)
- Pin #28 -> Sensor supply voltage
- Pin #9 -> DCF77 clock signal receiver
- Pin #10 -> DCF77 clock signal receiver
- Pin #6 -> Quattro fuel gauge (other side is pin#28)
- Pin #26 -> Quattro fuel gauge (other side is pin#28)
- Pin #31 -> Immo reader (RFID) coil
- Pin #32 -> Immo reader (RFID) coil

##ESP32 Development board version
- If you are using an ESP32 development board instead of my PCB design, it is enough to tap the CAN wires:
-> Tap Pin#29 of cluster and connect to CAN H on your board (or external transceiver)
-> Tap Pin#30 of cluster and connect to CAN L on your board (or external transceiver)
5V power supply can be safely provided from a random USB phone charger

**Be careful** the CAN transceivers are usually 5V devices, you need a level shifter before the ESP32 pins (they are expecting 3v3)!
Or you can use one with a VDDIO (pin#5) input (for example: SIT1051AT/3 or TJA1044**GT**) directly.

###Connections between the two
- Tap Pin#29 of cluster and connect to DSUB pin#7
- Tap Pin#30 of cluster and connect to DSUB pin#2
- Tap Pin#3 of cluster and connect to DSUB pin#3
- Tap Pin#18 of cluster and connect to DSUB pin#9 (esp32 will go to sleep when ignition is off, quiescent current should be low...)
**OR**
- Tap Pin#5 of cluster and connect to DSUB pin#9 (not tested, sleep current is zero in this case)

Ugly help: <img src="https://github.com/tothpetiszilard/a3_8p_redFIS/blob/main/media/connector.jpg" alt="wires">
