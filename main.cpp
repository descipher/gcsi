/**
 *	File:       	main.cpp
 *	Version:  	1.0
 *	Date:       	2018
 *	License:	GPL v3
 *	Description:	Geiger.Counter.1 hardware main code
 *	Project:	Geiger.Counter.1, WiFi, Logging, USB Interface and low power field operations.
 *
 *	Copyright 2013 by Radu Motisan, radu.motisan@gmail.com
 *	Copyright 2016 by Magnasci SRL, www.magnasci.com
 *  	Copyright 2017 by Gelidus Research Inc, mike.laspina@gelidus.ca
 *
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 3 of the License, or
 * 	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 * 	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * 	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <avr/sfr_defs.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/wdt.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "config.h"
#include "jsmn/jsoncmd.h"
#include "lcd/5110.h"
#include "gpio/pins.h"
#include "time/rtc.h"
#include "jsmn/jsmn.h"
#include "geiger/detectors.h"
#include "misc/utils.h"
#include "misc/nvm.h"
#include "user/ui.h"
#include "logger/logger.h"
#include "uart/uart.h"
#include "wstring/wstring.h"

bool onConfigChange = false;
bool calibrated = false;
bool collect = false;
bool collect_active = false;
bool timeSetOK = false;
bool dateSetOK = false;
bool ssidSetOK = false;
bool configSent = false;
bool initOk = false;
uint8_t cmdq_timeout = 0;

uint8_t collect_sec = 0;

uint8_t tickDiff = 0;
uint32_t tickSnap = 0;
uint8_t calibration = 120;		//counter ref val for internal rc calibration	

volatile time rtc;

/************************************************************************************************************************************************/
/* Global Objects                                                       																		*/
/************************************************************************************************************************************************/
char	buffer[85] = { 0 };			// general purpose buffer min 85 for LCD data
pin	speaker(&PORTC, PC5),			// used to create digital pulse on radiation event
	backlight(&PORTD, PD4),			// used to toggle LCD light on and off
	button1(&PORTC, PC3, pin::INPUT), // user button1, configured as input pin
	button2(&PORTD, PD3, pin::INPUT), // user button2, configured as input pin
	ch_pd(&PORTC, PC4),				// ESP8266 WIFI module
	lcdReset(&PORTB, PB0), 			// lcd RST pin
	lcdCE(&PORTD, PD7),				// lcd CE pin
	lcdDC(&PORTD, PD6), 			// lcd DC pin
	lcdDATA(&PORTD, PD5), 			// lcd DATA pin
	lcdCLK(&PORTC, PC1);			// lcd CLK pin

LCD_5110		lcd(&lcdReset, &lcdCE, &lcdDC, &lcdDATA, &lcdCLK, &backlight);	// handle the LCD ops for drawing content on screen

DATA			Data;
UI			ui(&lcd, &ch_pd, &speaker, &button1, &button2, &Data, buffer, sizeof(buffer));// lcd, ch_pd, speaker, button and data, form the GUI for user interaction

uint32_t		deviceNumber = 0;				// If null, Dynamic ID  is enabled . Add a non null value to set number manually
volatile uint32_t 	geigerPulses = 0;				// geiger: total number of pulses detected
bool			cmdRefresh = 0;					// if true will refresh display

uint16_t inv_duty = INVERTER_DUTY_MIN;	//Basic HV inverter duty control

NVMConfig EEMEM Startup;		//Non volatile config in EEROM
NVMConfig Running;			//Non volatile config running copy 
extern uart Serial;
uint8_t CPS[60] = {0};			// Count per second array used to find accurate average CPM 
uint16_t geigerCPM = 0;	
uint8_t geigerCPS = 0;
char tdStr[12] = {0};
uint8_t ip[4] = {0};
uint8_t cmdq = 0;
bool WifiActive = false;
char jbuff[160] = {0};
char wbuff[64] = {0};
bool uartActive = 0;	
jsmn_parser parser;
jsmntok_t token[6]; // We expect no more than 6 tokens



void * operator new[] (size_t size)
{
	return malloc(size);
}
void operator delete[] (void * ptr)
{
	free(ptr);
}


static uint8_t jsoneq(char *json, jsmntok_t *tok, const char *s) {
	if (tok->type == JSMN_STRING && (int) strlen(s) == tok->end - tok->start &&
	strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
		return 0;
	}
	return 1;
}

static uint8_t read_json() {
	
	int i;
	int r;
	int j;
	char * sp = nullptr;
	
	jsmn_init(&parser);
	
	r = jsmn_parse(&parser, jbuff, strlen(jbuff), token, sizeof(token)/sizeof(token[0]));
	if (r < 0) {
		//Serial.printf("Err %d\n",r);
		//Serial.printf("JSON: %s\n",jbuff);
		return 1;
	}
	
	memset( wbuff,0,sizeof(wbuff));

	// Loop over all keys of the root object 
	for (i = 1; i < r; i++) {
		
		if (jsoneq(jbuff, &token[i], "time") == 0) {
			cli();
			strncpy(wbuff,&jbuff[token[i+1].start],(token[i+1].end) - (token[i+1].start));
			memcpy (tdStr, wbuff, 2);
			tdStr[2] = 0;
			rtc.hour = atoi(tdStr);
			memcpy(tdStr, wbuff+3,2);
			tdStr[2] = 0;
			rtc.minute = atoi(tdStr);
			memcpy(tdStr, wbuff+5,2);
			tdStr[2] = 0;
			rtc.second = atoi(tdStr);
			sei();
			cmdq = 0;
			timeSetOK = true;			
			i++;
			Serial.printf(jcmd(ACK_TIME));

		}
		
		if (jsoneq(jbuff, &token[i], "date") == 0) {
			cli();
		    strncpy(wbuff,&jbuff[token[i+1].start],(token[i+1].end) - (token[i+1].start));	
			memcpy(tdStr, wbuff+8, 2);
			tdStr[2] = 0;
			rtc.day = atoi(tdStr);
			memcpy(tdStr, wbuff+5, 2);
			tdStr[2] = 0;
			rtc.month = atoi(tdStr);
			memcpy(tdStr,wbuff,4);
			tdStr[4] = 0;
			rtc.year = atoi(tdStr);
			sei();	
			dateSetOK = true;
		    i++;
			Serial.printf(jcmd(ACK_DATE));
			cmdq = 0;
		}
		
		if (jsoneq(jbuff, &token[i], "ip") == 0) {
			
			memset( wbuff,0,sizeof(wbuff));
		    strncpy(wbuff,&jbuff[token[i+1].start],(token[i+1].end) - (token[i+1].start));

			sp = strtok (wbuff,".");
			ip[0] = atoi(sp);
			j=1;
			while (sp != NULL)
			{
			  sp = strtok (NULL,".");
			  ip[j] = atoi(sp);
			  j++;
			}
		    i++;
			Serial.printf(jcmd(ACK_IP));
			cmdq = 0;		
		}
		
		if (jsoneq(jbuff, &token[i], "ssid") == 0) {

			strncpy(Running.SSID,&jbuff[token[i+1].start],(token[i+1].end) - (token[i+1].start));
			onConfigChange = true;
			ssidSetOK = true;
			i++;
			Serial.printf(jcmd(ACK_SSID));
			cmdq = 0;
		}
		
		if (jsoneq(jbuff, &token[i], "config") == 0) {

			Serial.printf("{\"mac\":%lu,\"tube\":%d,\"ssid\":\"%s\",\"sound\":%d}\n", Running.unitID,Running.geigerTube,Running.SSID,Running.speakerState);
			i++;
			Serial.printf(jcmd(ACK_CONFIG));
			cmdq = 0;

		}
	}		
	//zero the buffer
	memset( jbuff,0,sizeof(jbuff));

	return 0;
}


//interrupt-driven routine to update the software clock
ISR(TIMER2_OVF_vect) { 
	timeEvent();
}
	
// callback function called from the RTC object when a full minute has elapsed
void callback_timeMinute() {
}

// called on elapsed second: keep code here light!
void callback_timeSecond() {
	uint8_t sec;
	// read sensors, listen to button presses and refresh screen	
	sec = rtc.second;
	CPS[sec] = (uint8_t) geigerPulses; 
	geigerCPS = (uint8_t) geigerPulses;
	for (uint8_t i = 0; i < 60; i++) {
		geigerCPM += CPS[i];
	}
	Data.State.geigerCPM = geigerCPM;
	Data.State.geigerDose = aux_CPM2uSVh(GEIGER_TUBE, geigerCPM);
	geigerCPM = 0;
	geigerPulses = 0;
	Data.State.Beep = false;	
	
	// alarm condition
	if (Data.State.Alarm) {
		if (Running.speakerState) speaker.toggle();
		backlight.toggle();
	}
	cmdRefresh = true;
}

void callback_timeAlarm() {
	
}

// watchdog overflow interrupt, set to 1sec
//ISR (WDT_vect) { wd.timerEvent(); }
	
// int0 interrupt handler
// we have a top limit of 2^32-1 pulses. We don't go over it.
ISR (INT0_vect) {
	geigerPulses++; // count this pulse
	Data.State.Beep = true;
}

void saveConfig() {
	eeprom_update_block (( uint8_t *) &Running, EEPROM_ADDR_BEGIN, EEPROM_ADDR_END);
}

void loadConfig() {
	eeprom_read_block(( uint8_t *) &Running, EEPROM_ADDR_BEGIN, EEPROM_ADDR_END);
}

// RC Calibration routine  returns the number of RTC loops
uint8_t TickCalc() {
		uint32_t i;
		i = rtc.ticks;
		_delay_ms(1000);								//Run builtin delay code
		i = rtc.ticks - i;
		return i;							//Should be 127 ticks on the RTC time loop
}

void inv_setDuty(uint16_t d) {
	inv_duty = d;
	OCR1A = (uint16_t)( (float)ICR1 * (float) d  / 1000.0);
}

// CREATE Timer T1 PWM to drive inverter for regulated Geiger tube voltage
void inv_initPWM() { 
	TCCR1A = 0;     // disable all PWM on Timer1 whilst we set it up
	TCCR1B = 0;
	DDRB |= _BV(PB1); // Set PB1 as output (PB1 is OC1A)
	ICR1 = F_CPU / INVERTER_FREQUENCY; // set the frequency FCPU/(ICR1 * PRESCALLING) Hz . Prescalling set to 1X
	inv_setDuty(INVERTER_DUTY_MIN);
	TCCR1B = (1<<WGM13) | (1<<WGM12) | (1 << CS10); //Fast PWM mode via ICR1 (CS10 = 1 = no prescaling)
	TCCR1A |= (1 << COM1A1) |(1<< WGM11); // set none-inverting mode and start PWM
}

void inv_disable() {
	TCCR1A = 0;     // disable all PWM on Timer1
	DDRB |= ~_BV(PB1); // Set PB1 to open
}


void inv_enable() {
	DDRB |= _BV(PB1); // Set PB1 as output (PB1 is OC1A)
	TCCR1A |= (1 << COM1A1) |(1<< WGM11); // set none-inverting mode and start PWM*/
	}

// check tube voltage and adjust duty cycle to match tube given threshold level 
bool inv_adjustDutyCycle(uint16_t measuredVoltage, uint16_t targetVoltage) {
	if ( (measuredVoltage >= targetVoltage + INVERTER_TOLERANCE) && (inv_duty > INVERTER_DUTY_MIN)) {
	inv_setDuty(inv_duty - 1); // we need to decrease duty cycle to decrease voltage
	}
	if ( (measuredVoltage <= targetVoltage - INVERTER_TOLERANCE) && (inv_duty < INVERTER_DUTY_MAX)) {
	inv_setDuty(inv_duty + 1); // we need to increase duty cycle to increase voltage
	}
	if (inv_duty == INVERTER_DUTY_MAX) return false; // error
	else return true;
	}

/************************************************************************************************************************************************/
/* Main entry point                                                    																			*/
/************************************************************************************************************************************************/
 int main(void) {
	 
	stopRTC();
	
	rtc.hour = 12;
	rtc.minute = 0;
	rtc.second = 0;
	rtc.day = 1;
	rtc.month = 4;
	rtc.year = 2018;
	
	startRTC();
	
	// INT0  to count pulses from Geiger Counter, connected on PIN PD2
	EICRA |= _BV(ISC00) | _BV(ISC01);// Configure INT0 to trigger on RISING EDGE - Instead of MCUCR, you need to use EICRA. mega8:MCUCR , mega328p: EICR
	EIMSK |= _BV(INT0); // Configure INT0 to fire interrupts - Instead of GICR, you need to use EIMSK mega8:GICR , mega328p: EIMSK
	
	// Timer T1 PWM to drive inverter for regulated Geiger tube voltage
	inv_initPWM();
	
	//Init timer0 for millis() function	
	millis_init(); 
	
	// lcd init 
	lcd.init();
	lcd.setBacklight(true);	
	lcd.clear(); 
	//lcd.printPictureOnLCD(introScreen);
	
	loadConfig();
	if (Running.unitID == 0xFFFFFFFF ) { //Not initialized, load defaults
		Running.speakerState = 0;
		Running.loggingState = 0;
		Running.alarmState = 0;
		Running.logInterval = 0;
		Running.geigerTube = 1;
		Running.calibration = 126;	//Default to center of calibration registers value range 
		Running.WifiState = 1;
		Running.WifiMode = 2;					//default to AP mode			
		strcpy(Running.SSID,"GC.1\0");			//default AP SSID is GC.1
		for (uint8_t i=0;i<16;i++) Running.Password[i] = 0;  
		Running.alarmTimeStamp = 0;
		Running.unitID= 0x2055AA55;
		saveConfig();
	}
#ifndef CLOCK_12MHZ
	OSCCAL = Running.calibration;		//load last calculated calibration value
	lcd.goto_xy(0,2);
	lcd.send(buffer,15,PSTR("Calibrating RC"));

	while (!calibrated) {

		rtc.runticks=0;	
		_delay_ms(1000);
	
		if (rtc.runticks != 0x7F) {							//Dynamically adjust OSCCAL accordingly to be an 8.0 Mhz RC
			(rtc.runticks > 0x7F) ? OSCCAL++: OSCCAL--;		//We do this by comparing the RTC ticks e.g. 128 = 1Hz
		}
		else
		{
			if (Running.calibration != OSCCAL) {
			Running.calibration = OSCCAL;
			onConfigChange = true;
			}
		calibrated = true;
		}
		lcd.goto_xy(5,3);
		lcd.send(buffer,4,PSTR("%3u"),OSCCAL);
	}	

#endif	

	if (Running.WifiState == 1) {
		ch_pd = true;
	}
	else
	{
		ch_pd = false;
	}
	lcd.clear();
	Serial.setTimeout(800);	//Maximum wait time for serial r/w is 800ms
	Serial.begin(38400,SERIAL_8N1); //Note @ 8Mhz 38400 is 0.2% error rate, 115200 is 8% it needs to be accurate, check the baud tables before changing this
	//espCom.InterfaceInit(&Serial);

	// 8.Main program loop
	while (1) {
		
	ui.loop(&cmdRefresh);

	if (onConfigChange) {
		
		if (eeprom_is_ready()) {
			cli();
			saveConfig();
			sei();
			lcd.goto_xy(0,5);
			lcd.send(PSTR("Saved"));
			
		}
		onConfigChange = false;
	}
	
	//Calls that happen every second after RTC 32768Khz Overflow IRQ is complete	
	if (rtc.onSecInterval) {
		callback_timeSecond();
		if ((Running.loggingState == false)&&(cmdq == 0)) {
			Serial.printf("{\"cpm\":%lu,\"cps\":%u,\"voltage\":%u,\"dose\":%f}",Data.State.geigerCPM, geigerCPS, Data.State.inverterVoltage, Data.State.geigerDose);
			Serial.println();
		}

		if (cmdq == 1) cmdq_timeout++;
		if (cmdq_timeout > 3) {
			cmdq_timeout = 0;
			cmdq = 0;
		}
		rtc.onSecInterval = false;
		if (collect_active == true) collect_sec++;
		
		if (Running.loggingState == true) {
			switch (Running.logInterval) {
				case 0:
					if (rtc.second == 0) {
						
						//if (sd_card == 1) write_log();
						collect = false;

					}
					collect_active = true;
					break;
				case 1:
				if ((rtc.minute  % 5) == 0) collect_active = true;
				case 2:
				if ((rtc.minute % 15) == 0) collect_active = true;
				case 3:
				if ((rtc.minute % 30) == 0) collect_active = true;
				case 4:
				if ((rtc.hour % 1) == 0 && rtc.minute == 0) collect_active = true;
				case 5:
				if ((rtc.hour % 2) == 0 && rtc.minute == 0) collect_active = true;
				case 6:
				if ((rtc.hour % 4) == 0 && rtc.minute == 0) collect_active = true;
				case 7:
				if ((rtc.hour % 8) == 0 && rtc.minute == 0) collect_active = true;
				case 8:
				if (((rtc.hour - 12) == 0 || (rtc.hour == 0)) && rtc.minute == 0) collect_active = true;
				case 9:
				if (rtc.hour == 0 && rtc.minute == 0) collect_active = true;
				default: collect = false;
			}
		}
	}


	if (collect == false && collect_active == true) {

		if (collect_sec > 60) {
			collect_active = false;
			collect_sec=0;
			//result = write_log();
			
		} else {
			collect = true;
		}
		
	}
	
	if (Data.State.Sleep == true && collect_active == false) {
		inv_disable();
		lcd.setBacklight(false);
		WifiActive = false;
		lcd.sleep();
		set_sleep_mode(SLEEP_MODE_PWR_SAVE);
		sleep_enable();
		sleep_mode();
	} else 	{
		inv_enable();
		lcd.wake();
		sleep_disable();
	
		if (Running.WifiState == 1 && !WifiActive) {
			lcd.clear();
			lcd.goto_xy(0,2);
			lcd.send(buffer,15,PSTR("Starting WiFi"));
            Running.WifiMode = 1;
		}		
	if (Running.WifiState == 1)  {
			
		WifiActive = true;
	
		if (dateSetOK == 0) {
			if (cmdq == 0) {
				Serial.print(jcmd(GET_DATE));
				cmdq = 1;
			}
		}
				
		if (timeSetOK == 0) {
			
			if (cmdq == 0) {
				Serial.print(jcmd(GET_TIME));
				cmdq = 1;
			}
		}
			
		if (ssidSetOK == 0) {
							
			if (cmdq == 0) {
				Serial.print(jcmd(GET_SSID));
				cmdq = 1;
			}
		}
		
		if (cmdq == 0) {
			if (ip[0] == 0) {
				Serial.print(jcmd(GET_IP));
				cmdq = 1;
			}
		}
		
		if (configSent == 0) {
			if (cmdq == 0) {
				//Serial.printf("{\"ssid\":\"%s\"}\n",Running.SSID);
				//Serial.printf("{\"sound\":%d}\n",Running.speakerState);
				Serial.printf("{\"mac\":%lu,\"tube\":%d,\"ssid\":\"%s\",\"sound\":%d}\n", Running.unitID,Running.geigerTube,Running.SSID,Running.speakerState);
				configSent = true;
			}
		}
		
		} else {
			WifiActive = false;
		}
		
		if (Serial.available()) {
			Serial.readBytesUntil(10,jbuff,sizeof(jbuff));
			read_json();
		}
    }
  }
}  
