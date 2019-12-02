/**
 *	File:       	rtc.h
 *	Version:  		1.0
 *	Date:       	2017
 *	License:		GPL v3
 *	Description:	Real-Time Clock for atmega328 
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
#ifndef RTC_H
#define RTC_H

//library version
#define RTC_VERSION 100
#include <avr/sfr_defs.h>

typedef void (*TimeCallback)();


class RTC {
	
	private:
		void setTimer();
		bool isRunning;
		int yearSize(uint16_t yearT);
		TimeCallback ptrFuncSec, ptrFuncMin, ptrFuncAlarm;	// a full second and minute or alarm elapsed action

	public:
	
		//public methods
		RTC(){
			ptrFuncSec = 0; 
			ptrFuncMin = 0;
			ptrFuncAlarm = 0;
		}
		
		void startRTC(TimeCallback callSec, TimeCallback callMin, TimeCallback callAlarm);
		void TimeEvent();		
		
		bool setTime(uint8_t hourT, uint8_t minuteT, uint8_t secondT);
		bool setDate(uint8_t dayT, uint8_t monthT, uint16_t yearT);
		void stopRTC();
		bool isLeapYear(uint16_t yearT = 0);
		uint32_t getTimestamp(uint16_t yearT = 0);
        bool setDeltaT(int deltaT = 0);
        int getDeltaT();
		uint8_t getSeconds();
		uint8_t getMinutes();
		uint8_t getHours();
		uint8_t getDay();
		uint8_t getMonth();
		uint16_t getYear();
		void setSeconds(uint8_t secondsT);
		void setMinutes(uint8_t minutesT);
		void setHours(uint8_t hoursT);
		void setDay(uint8_t dayT);
		void setMonth(uint8_t monthT);
		void setYear(uint16_t yearT);
		void setEditMode(uint8_t editT);
		uint32_t getElaspsedTicks();
		void setElaspsedTicks(uint32_t setTicks);
		uint32_t getTicks();												//Number of irq loop counts 
		uint8_t getCount(); 

		uint8_t setClockWithTimestamp(uint32_t timeT, uint16_t yearT = 0);
		uint8_t weekDay(uint32_t timestamp);
		uint8_t getWeekDay();


};

#endif


