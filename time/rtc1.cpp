/**
 *	File:       	rtc.cpp
 *	Version:  		1.0
 *	Date:       	2017
 *	License:		GPL v3
 *	Description:	Real Time Clock Low power time counter
 *	Project:		Geiger.Counter.1, Logging, USB Interface and low power field operations.
 *
 *  Copyright 2017 by Gelidus Research Inc, mike.laspina@gelidus.ca
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



//use the real-time counter 32768 oscillator
#define USE_INTERNAL_RTC
#include "rtc.h"
#include <stdio.h>
#include <avr/interrupt.h>
#include <avr/common.h>


//global variables
RTC *lib;
volatile uint8_t starter = 0;
volatile uint8_t counterT = 0;
volatile uint8_t hours = 0;
volatile uint8_t minutes = 0;
volatile uint8_t seconds = 0;
volatile uint8_t day = 0;
volatile uint8_t month = 0;
volatile uint16_t year = 0;
volatile uint32_t runticks = 0;
volatile uint32_t ticks = 0;
volatile uint8_t daysPerMonth[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
volatile uint8_t editMode = 0;

volatile bool onSecInterval = false;
volatile bool onMinInterval = false;
volatile bool onHrInterval = false;
volatile bool onDayInterval = false;
volatile bool onAlarmInterval = false;

// ****** MAIN FUNCTIONS *******
//instructions to execute when a new instance of the lib is created

void RTC::setTimer() {
	
	//Reset the async timer
	GTCCR |= (1<<PSRASY);
	
	
	//during setup, disable all the interrupts based on timer
	TIMSK2 &= ~(1<<TOIE2);
	TIMSK2 &= ~((1<<OCIE2A) | (1<<OCIE2B));

	/* Reset timer */
	TCNT2 = 0;
	
	//set the timer for asynchronous
	ASSR |= (1<<AS2);	
	
	/* Prescale the timer to be clock source */
	/* TC0 overflow precisely once every 1/128 second. */
	TCCR2B |= (1 << CS20);
	TCCR2B &= ~((1 << CS22) | (1 << CS21));
	

	//CTC (counter) mode: the timer increments the counter until overflow
	TCCR2A &= ~((1<<WGM21) | (1<<WGM20));
	TCCR2B &= ~(1<<WGM22);



	/* Wait until TC0 is updated */
	while (ASSR & ((1 << TCN2UB) | (1 << OCR2BUB) | (1 << TCR2BUB))) {
	}

	/* Enable Timer/Counter2 Overflow Interrupts */
	TIMSK2 |= (1 << TOIE2);
	
	// set up timer0 
	TCCR0B |= (1 << CS02 | 1<< CS00); //Prescale of 1024
		
	// initialize timer0 counter
	TCNT0 = 0;
	
	/* Set the Global Interrupt Enable Bit */
	sei();

}


void RTC::TimeEvent()  {
	TCNT2 = starter;
	counterT++;	
	ticks++;						//ticks since the last CPU reset
	runticks++;						//elapsed tick count 
	onSecInterval = false;
	onMinInterval = false;
	onHrInterval = false;
	onDayInterval = false;
	onAlarmInterval = false;
	
	if (counterT > 125) { // equal to 1 s ( 127 ticks per second. 32768KHz clock ~ 4 ms per tick 0.0039/s )
		counterT = 0;
		onSecInterval = true;
		if (!editMode) {
			if (++seconds == 60) {
				seconds = 0;
				onMinInterval = true;
				if (++minutes == 60) {
					minutes = 0;
					onHrInterval = true;
					if (++hours == 24) {
						hours = 0;
						if (++day == 32) {
							month++;
							day = 1;
							} else if (day == 31) {
							if ((month == 4) || (month == 6) || (month == 9) || (month == 11)) {
								month++;
								day = 1;
							}
							} else if (day == 30) {
							if (month == 2) {
								month++;
								day = 1;
							}
							} else if (day == 29) {
							if ((month == 2) && !(isLeapYear())) {
								month++;
								day = 1;
							}
						}

						if (month == 13) {
							month = 1;
							year++;
						}
					}
				}
			}
		}
		
	
	

		


		
		
// 		if (onSecInterval == true) {
// 			if (ptrFuncSec != 0) ptrFuncSec();
// 		}	
// 		if (onMinInterval == true) {
// 			if (ptrFuncMin != 0) ptrFuncMin();
// 		}
// 		if (onSecInterval == true) {
// 			if (ptrFuncAlarm != 0) ptrFuncAlarm();
// 		}
	}
}

//sets the time
bool RTC::setTime(uint8_t hourT, uint8_t minuteT, uint8_t secondT) {

	//check if the params are correct
	if (hourT > 23) { return false; }
	if (minuteT > 59) { return false; }
	if (secondT > 59) { return false; }

	hours = hourT;
	minutes = minuteT;
	seconds = secondT;
	return true;
}


//sets the date
bool RTC::setDate(uint8_t dayT, uint8_t monthT, uint16_t yearT) {

	//check if the params are correct
	if ((dayT < 1) || (dayT > 31)) { return false; }
	if ((monthT < 1) || (monthT > 12)) { return false; }
	if (yearT < 0)  { return false; }

	day = dayT;
	month = monthT;
	year = yearT;
	return true;
}



//start the software RTC
void RTC::startRTC(TimeCallback callSec, TimeCallback callMin, TimeCallback callAlarm) {
	ptrFuncSec = callSec;
	ptrFuncMin = callMin;
	ptrFuncMin = callAlarm;

	setTimer();
	counterT = 0;
	ticks = 0;
	TCNT2 = starter;
	TIMSK2 |= (1<<TOIE2);
	SREG |= (1<<SREG_I);
	isRunning=true;
}


void RTC::stopRTC() {
	ptrFuncSec = 0;
	ptrFuncMin = 0;
	TIMSK2 &= ~(1<<TOIE2);
	SREG &= ~(1<<SREG_I);
	isRunning = false;
}


//returns the tick count
uint32_t RTC::getTicks() {
	return ticks;
}

uint8_t RTC::getCount() {
	return counterT;
}

void RTC::setElaspsedTicks(uint32_t setTicks) {
	runticks = setTicks;
}

//returns the elapsed ticks since the last call for elapsed tick
uint32_t RTC::getElaspsedTicks() {
	uint32_t elapsed = runticks;
	return elapsed;
}

//returns the seconds
uint8_t RTC::getSeconds() {
	return seconds;
}


//return the minutes
uint8_t RTC::getMinutes() {
	return minutes;
}


//return the hours
uint8_t RTC::getHours() {
	return hours;
}


//return the day
uint8_t RTC::getDay() {
	return day;
}


//return the month
uint8_t RTC::getMonth() {
	return month;
}


//return the year
uint16_t RTC::getYear() {
	return year;
}

void RTC::setSeconds(uint8_t secondsT) {
	if (secondsT < 60) seconds = secondsT;
}
	
void RTC::setMinutes(uint8_t minutesT) {
	if (minutesT < 60) minutes = minutesT;
}

void RTC::setHours(uint8_t hoursT) {
	if (hoursT < 24) hours = hoursT;
}

void RTC::setDay(uint8_t dayT) {
	if (dayT < 32) day = dayT;
}

void RTC::setMonth(uint8_t monthT) {
	if (monthT < 13) month = monthT;
}

void RTC::setYear(uint16_t yearT) {
	if (yearT < 9999) year = yearT;
}

void RTC::setEditMode(uint8_t editT) {
	editMode = editT;
}


	

void setDay(uint8_t day);
void setMonth(uint8_t month);
void setYear(uint16_t year);

//check if the current year is a leap year
bool RTC::isLeapYear(uint16_t yearT) {
	if (yearT == 0) { yearT = year; }
	if ((!(yearT % 4) && (yearT % 100)) || !(yearT % 400)) {
		return true;
		} else {
		return false;
	}
}


//returns a timestamp giving the number of seconds since a part year (default=1970)
uint32_t RTC::getTimestamp(uint16_t yearT){
	
	uint32_t time=0;

	//check the epoch
	if (yearT == 0) {
		yearT = 1970;
		} else if (yearT < 1900) {
		yearT = 1900;
		} else if (yearT > 1970) {
		yearT = 1970;
		} else if ((yearT != 1900) && (yearT != 1970)) {
		yearT = 1970;
	}

	//One revolution of the Earth is not 365 days but accurately 365.2422 days.
	//It is leap year that adjusts this decimal fraction. But...
	time += (getYear() - yearT) * 365.2422;
	for (uint8_t i = 0; i < getMonth() - 1; i++){
		time += daysPerMonth[i]; //find day from month
	}
	time = (time + getDay()) * 24UL; //find hours from day
	time = (time + getHours()) * 60UL; //find minutes from hours
	time = (time + getMinutes()) * 60UL; //find seconds from minute
	time += getSeconds(); // add seconds
	if (time > 951847199UL) { time += 86400UL; } //year 2000 is a special leap year, so +1 day if date is greater than 29/02/2000
	//Check if the date is before or past the 29th of February:
	//if no, the leap day hasn't been yet reached so we have to subtract a day
	if (isLeapYear(getYear())) {
		if (getMonth() <= 2 ) {
			time -= 86400UL;
		}
	}
	return (time - 86400UL); //year starts at day 0 not day 1.
}


// set the internal clock using a timestamp using the epoch passed as argument
uint8_t RTC::setClockWithTimestamp(uint32_t timeT, uint16_t yearRef) {
	unsigned long dayT;

	dayT = timeT / (60UL * 60UL * 24UL);
	float remaining = float(timeT - dayT * (60UL * 60UL * 24UL));
	uint16_t yearT = (dayT / 365.2422);
	float dayRemaining = dayT - yearT * 365.2422;

	if (yearRef == 0) {
		yearRef = 1970;
		} else if (yearRef < 1900) {
		yearRef = 1900;
		} else if (yearRef > 1970) {
		yearRef = 1970;
		} else if ((yearRef != 1900) && (yearRef != 1970)) {
		yearRef = 1970;
	}

	yearT += yearRef;
	if (dayRemaining >= 365.2422) {
		return 1;//my math is wrong!
	}
	if (yearT < yearRef) {
		return 2;//year not supported!
	}
	uint8_t monthT = 0;
	while (dayRemaining > daysPerMonth[monthT]){
		dayRemaining -= daysPerMonth[monthT];
		if (monthT == 1 && isLeapYear(yearT)) {
			dayRemaining--;
		}
		monthT++;
	}

	monthT++;//because month 0 doesn't exist
	if (monthT > 12) {
		monthT = 1;
		yearT++;
		//return 3;//my math is wrong!
	}
	if (dayRemaining >= (60UL*60UL*24UL)) {
		return 4;//my math is wrong!
	}
	dayT = dayRemaining;
	if (dayRemaining - dayT > 0){ //add partial day!
		dayT++;
	}
	uint8_t hoursT = remaining / (60UL * 60UL);
	remaining = remaining - hoursT * (60UL * 60UL);
	if (remaining >= (60UL * 60UL)) {
		return 5;//my math is wrong!
	}
	uint8_t minutesT = remaining / 60UL;
	remaining = remaining - minutesT * 60UL;
	if (remaining >= 60) {
		return 6;//my math is wrong!
	}
	year = yearT;
	month = monthT;
	day = dayT;
	//with leap years we must consider that there's a day more in the calendar
	//and at the jan., 1st the count rolls back to dec, 31th. so we must check
	//the next year
	if ((day == 31) && (month == 12)) {
		if (isLeapYear(year + 1)) {
			yearT++;
		}
	};
	//If we are on Feb., 28th
	//go to Feb., 29th on leap year
	if (isLeapYear(yearT)) {
		day++;
		month == 2 ? dayT = daysPerMonth[month - 1] + 1 : dayT = daysPerMonth[month - 1];
		if (day > dayT) {
			day = 1;
			month++;
			if (month > 12) {
				month = 1;
				year++;
			}
		}
	}
	hours = hoursT;
	minutes = minutesT;
	seconds = remaining;
	return 0;
}


//return the day of the week giving as a timestamp
uint8_t RTC::weekDay(unsigned long timestamp) {
	return ((timestamp / 86400UL) + 4 ) % 7;
}


//return the current week day
uint8_t RTC::getWeekDay() {
	return (RTC::weekDay(RTC::getTimestamp()));
}






