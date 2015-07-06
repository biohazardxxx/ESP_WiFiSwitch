EESchema Schematic File Version 2
LIBS:power
LIBS:device
LIBS:transistors
LIBS:conn
LIBS:linear
LIBS:regul
LIBS:74xx
LIBS:cmos4000
LIBS:adc-dac
LIBS:memory
LIBS:xilinx
LIBS:special
LIBS:microcontrollers
LIBS:dsp
LIBS:microchip
LIBS:analog_switches
LIBS:motorola
LIBS:texas
LIBS:intel
LIBS:audio
LIBS:interface
LIBS:digital-audio
LIBS:philips
LIBS:display
LIBS:cypress
LIBS:siliconi
LIBS:opto
LIBS:atmel
LIBS:contrib
LIBS:valves
LIBS:WifiSwitch
LIBS:WifiSwitch-cache
EELAYER 25 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 1
Title ""
Date ""
Rev ""
Comp ""
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L ESP8266_ESP3 U1
U 1 1 553681EC
P 4300 3700
F 0 "U1" H 4550 3350 60  0000 C CNN
F 1 "ESP8266_ESP3" H 4300 4200 60  0000 C CNN
F 2 "WifiSwitch:ESP8266-ESP-03" H 4300 3700 60  0001 C CNN
F 3 "" H 4300 3700 60  0000 C CNN
	1    4300 3700
	1    0    0    -1  
$EndComp
$Comp
L MOC3031S U2
U 1 1 5537C87E
P 5500 2700
F 0 "U2" H 5500 2500 50  0000 L CNN
F 1 "MOC3031S" H 5500 2900 50  0000 L CNN
F 2 "WifiSwitch:DIP-6_SMD" H 5300 2500 50  0001 L CIN
F 3 "" H 5465 2700 50  0000 L CNN
	1    5500 2700
	1    0    0    -1  
$EndComp
$Comp
L AC_to_DC_convertor C1
U 1 1 5537CAC0
P 6950 4200
F 0 "C1" H 7050 4100 60  0000 C CNN
F 1 "AC_to_DC_convertor" H 6950 4450 60  0000 C CNN
F 2 "WifiSwitch:AC_DC_Conv" H 7050 4200 60  0001 C CNN
F 3 "" H 7050 4200 60  0000 C CNN
	1    6950 4200
	1    0    0    -1  
$EndComp
$Comp
L R R2
U 1 1 5537CF64
P 5500 3650
F 0 "R2" V 5580 3650 50  0000 C CNN
F 1 "10K" V 5500 3650 50  0000 C CNN
F 2 "Resistors_SMD:R_1206_HandSoldering" V 5430 3650 30  0001 C CNN
F 3 "" H 5500 3650 30  0000 C CNN
	1    5500 3650
	1    0    0    -1  
$EndComp
$Comp
L R R3
U 1 1 5537CF81
P 6100 3300
F 0 "R3" V 6180 3300 50  0000 C CNN
F 1 "330R" V 6100 3300 50  0000 C CNN
F 2 "Resistors_SMD:R_1206_HandSoldering" V 6030 3300 30  0001 C CNN
F 3 "" H 6100 3300 30  0000 C CNN
	1    6100 3300
	1    0    0    -1  
$EndComp
$Comp
L CONN_01X05 P1
U 1 1 5537CFE5
P 4300 4750
F 0 "P1" H 4300 5050 50  0000 C CNN
F 1 "CONN_01X05" V 4400 4750 50  0000 C CNN
F 2 "Pin_Headers:Pin_Header_Angled_1x05" H 4300 4750 60  0001 C CNN
F 3 "" H 4300 4750 60  0000 C CNN
	1    4300 4750
	0    1    1    0   
$EndComp
$Comp
L CONN_01X03 P2
U 1 1 5537D049
P 7750 2650
F 0 "P2" H 7750 2850 50  0000 C CNN
F 1 "CONN_01X03" V 7850 2650 50  0000 C CNN
F 2 "WifiSwitch:WirePad_3x-5mmDrill" H 7750 2650 60  0001 C CNN
F 3 "" H 7750 2650 60  0000 C CNN
	1    7750 2650
	1    0    0    -1  
$EndComp
$Comp
L TRIAC U3
U 1 1 5537D078
P 6900 2750
F 0 "U3" H 6650 3100 50  0000 C CNN
F 1 "TRIAC" H 6600 2500 50  0000 C CNN
F 2 "Transistors_TO-220:TO-220_Neutral123_Horizontal_LargePads" H 6900 2750 60  0001 C CNN
F 3 "" H 6900 2750 60  0000 C CNN
	1    6900 2750
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR01
U 1 1 5537D847
P 2900 5400
F 0 "#PWR01" H 2900 5150 50  0001 C CNN
F 1 "GND" H 2900 5250 50  0000 C CNN
F 2 "" H 2900 5400 60  0000 C CNN
F 3 "" H 2900 5400 60  0000 C CNN
	1    2900 5400
	1    0    0    -1  
$EndComp
Wire Wire Line
	2900 5400 2900 5250
Wire Wire Line
	5200 2800 5200 5250
Connection ~ 5200 5250
Connection ~ 5200 3950
Wire Wire Line
	3750 3750 3500 3750
Wire Wire Line
	3500 3750 3500 5250
Connection ~ 3500 5250
Wire Wire Line
	4950 3450 4800 3450
Wire Wire Line
	6900 2350 6900 1900
Wire Wire Line
	6250 1900 8050 1900
Wire Wire Line
	6900 3000 6900 3600
Wire Wire Line
	6100 3600 7350 3600
Wire Wire Line
	8050 1900 8050 4200
Wire Wire Line
	6100 3450 6100 3600
Connection ~ 6900 3600
Wire Wire Line
	6100 3150 6100 2800
Wire Wire Line
	6100 2800 5800 2800
Wire Wire Line
	6400 2950 6100 2950
Connection ~ 6100 2950
Wire Wire Line
	5800 2600 6250 2600
Wire Wire Line
	6250 2600 6250 1900
Connection ~ 6900 1900
Wire Wire Line
	6300 4200 6050 4200
Wire Wire Line
	3600 4100 6300 4100
Wire Wire Line
	3750 3350 3600 3350
$Comp
L R R1
U 1 1 55380183
P 4550 2600
F 0 "R1" V 4630 2600 50  0000 C CNN
F 1 "220R" V 4550 2600 50  0000 C CNN
F 2 "Resistors_SMD:R_1206_HandSoldering" V 4480 2600 30  0001 C CNN
F 3 "" H 4550 2600 30  0000 C CNN
	1    4550 2600
	0    1    1    0   
$EndComp
Wire Wire Line
	5200 2600 4700 2600
Wire Wire Line
	4400 2600 3500 2600
Wire Wire Line
	3500 2600 3500 3650
Wire Wire Line
	3500 3650 3750 3650
Wire Wire Line
	3700 3850 3750 3850
Wire Wire Line
	3700 3050 3700 3850
Wire Wire Line
	3700 3050 5500 3050
Wire Wire Line
	3600 3350 3600 4550
Wire Wire Line
	4500 4550 4500 4400
Wire Wire Line
	4500 4400 4700 4400
Wire Wire Line
	4700 4400 4700 5250
Connection ~ 4700 5250
Wire Wire Line
	3600 4550 4100 4550
Connection ~ 3600 4100
Wire Wire Line
	3750 3950 3700 3950
Wire Wire Line
	3700 3950 3700 4450
Wire Wire Line
	4800 3950 5200 3950
Wire Wire Line
	5500 3050 5500 3500
Wire Wire Line
	4950 3450 4950 3050
Connection ~ 4950 3050
Wire Wire Line
	5500 3800 5500 4100
Connection ~ 5500 4100
Wire Wire Line
	4800 3750 4900 3750
Wire Wire Line
	4300 4200 4300 4550
Wire Wire Line
	4800 3650 5000 3650
Wire Wire Line
	5000 3650 5000 4300
Wire Wire Line
	3700 4450 4200 4450
Wire Wire Line
	4200 4450 4200 4550
Wire Wire Line
	5000 4300 4400 4300
Wire Wire Line
	4400 4300 4400 4550
Wire Wire Line
	4300 4200 4900 4200
Wire Wire Line
	4900 4200 4900 3750
Wire Wire Line
	5200 5250 2900 5250
$Comp
L GND #PWR02
U 1 1 55382AE7
P 6050 5400
F 0 "#PWR02" H 6050 5150 50  0001 C CNN
F 1 "GND" H 6050 5250 50  0000 C CNN
F 2 "" H 6050 5400 60  0000 C CNN
F 3 "" H 6050 5400 60  0000 C CNN
	1    6050 5400
	1    0    0    -1  
$EndComp
Wire Wire Line
	6050 4200 6050 5400
Wire Wire Line
	8050 4200 7550 4200
Wire Wire Line
	7550 2750 7550 4100
Wire Wire Line
	7550 2550 7350 2550
Wire Wire Line
	7350 2550 7350 3600
Wire Wire Line
	7550 2650 7450 2650
Wire Wire Line
	7450 2650 7450 1900
Connection ~ 7450 1900
$Comp
L +3V3 #PWR03
U 1 1 5538D308
P 6100 4000
F 0 "#PWR03" H 6100 3850 50  0001 C CNN
F 1 "+3V3" H 6100 4140 50  0000 C CNN
F 2 "" H 6100 4000 60  0000 C CNN
F 3 "" H 6100 4000 60  0000 C CNN
	1    6100 4000
	1    0    0    -1  
$EndComp
Wire Wire Line
	6100 4000 6100 4100
Connection ~ 6100 4100
$EndSCHEMATC