/*
* OneWireThermometer. Base class for Maxim One-Wire Thermometers. 
* Uses the OneWireCRC library.
*
* Copyright (C) <2010> Petras Saduikis <petras@petras.co.uk>
*
* This file is part of OneWireThermometer.
*
* OneWireThermometer is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
* 
* OneWireThermometer is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with OneWireThermometer.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef SNATCH59_ONEWIRETHERMOMETER_H
#define SNATCH59_ONEWIRETHERMOMETER_H

#include <mbed.h>
#include "OneWireCRC.h"
#include "OneWireDefs.h"

typedef unsigned char BYTE;    // something a byte wide
typedef unsigned char DEVADDRESS[8];   // stores the address of one device

class OneWireThermometer
{
public:
    OneWireThermometer(PinName pin, int device_id);
    
    bool initialize();
    float readTemperature(int device);
    virtual void setResolution(eResolution resln) = 0; 
    int getDeviceCount();

protected:
    OneWireCRC oneWire;
    const int deviceId;
    
    eResolution resolution; 
    BYTE address[8];
    DEVADDRESS devices[10];
    int deviceCount;
   
    void resetAndAddress();
    bool readAndValidateData(BYTE* data);
    virtual float calculateTemperature(BYTE* data) = 0;    // device specific
};

#endif