/**
 *	File:       	detectors.h
 *	Version:  		1.0
 *	Date:       	2017
 *	License:		GPL v3
 *	Description:	Define Geiger tubes and conversion factors for CPM to equivalent dose
 *	Project:		Geiger.Counter.1, Logging, USB Interface and low power field operations.
 *  
 *	Copyright 2013 by Radu Motisan, radu.motisan@gmail.com
 *	Copyright 2016 by Magnasci SRL, www.magnasci.com
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

#pragma once

#include <stdint.h>

// define Radiation DETECTORS
#define GEIGER_TUBE_SBM20 0x1
#define GEIGER_TUBE_SI29BG 0x2
#define GEIGER_TUBE_SBM19 0x3
#define GEIGER_TUBE_LND712 0x4
#define GEIGER_TUBE_SBM20M 0x5
#define GEIGER_TUBE_SI22G 0x6
#define GEIGER_TUBE_STS5 0x7
#define GEIGER_TUBE_SI3BG 0x8
#define GEIGER_TUBE_SBM21 0x9
#define GEIGER_TUBE_SBT9 0xA
#define GEIGER_TUBE_SI1G 0xB


const char *aux_detectorName(uint8_t param);
float aux_detectorFactor(uint8_t param);
// dose equivalent linear approximative conversion from CPM
float aux_CPM2uSVh(uint8_t tube, uint32_t cpm);
uint16_t aux_detectorVoltage(uint8_t param);

