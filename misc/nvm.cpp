	/**
 *	File:       	nvm.cpp
 *	Version:  		1.0
 *	Date:       	2017
 *	License:		GPL v3
 *	Description:	Non volatile memory objects (eeprom)
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
	
#include "../misc/nvm.h"
extern NVMConfig Running;



// void NVM::setConfigTube(uint8_t tubeT) {
// 	Running.geigerTube = tubeT;
// }

// void NVM::setConfigOSCCAL(uint8_t calibration) {
// 	Running.calibration = calibration;
// }

// void NVM::setConfigSpeaker(uint8_t speakerT) {
// 	Running.speakerState = speakerT;
// }

// void NVM::setConfigWifi(uint8_t WifiT) {
// 	Running.WifiState = WifiT;
// }

// void NVM::setConfigSSID(char *ssidT) {
// 	char *p;
// 	p=Running.SSID;
// 	strcpy(p, ssidT);
// }

// void NVM::setConfigLogInterval(uint8_t intervalT) {
// 	Running.logInterval = intervalT;
// }

// void NVM::setConfigLoggingState(uint8_t loggingT) {
// 	Running.loggingState = loggingT;
// }

// void NVM::setConfigAlarmTimeStamp(uint32_t alarmtimestampT) {
// 	Running.alarmTimeStamp = alarmtimestampT;
// }
// void NVM::setConfigMacAddress(uint32_t macaddressT) {
// 	Running.unitID = macaddressT;
// }

// uint8_t NVM::getConfigOSCCAL() {
// 	return Running.calibration;
// }

// uint8_t NVM::getConfigTube() {
// 	return Running.geigerTube;
// }
// uint8_t NVM::getConfigLogInterval() {
// 	return Running.logInterval;
// }
// uint8_t NVM::getConfigLoggingState() {
// 	return Running.loggingState;
// }
// uint8_t NVM::getConfigSpeaker() {
// 	return Running.speakerState;
// }
// uint8_t NVM::getConfigWifi() {
// 	return Running.WifiState;
// }
// char* NVM::getConfigSSID() {
// 	return &Running.SSID[0];
// }
// uint32_t NVM::getConfigAlarmTimeStamp() {
// 	return Running.alarmTimeStamp;
// }
// uint32_t NVM::getConfigMacAddress() {
// 	return Running.unitID;
//}