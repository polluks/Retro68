/*
    Copyright 2019 Wolfgang Thaller.

    This file is part of Retro68.

    Retro68 is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Retro68 is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Retro68.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "MacSerialStream.h"

#include <Serial.h>
#include <Devices.h>

#include <string.h>

MacSerialStream::MacSerialStream(int port, int baud)
    : port(port), curBaud(baud)
{
    open();
}

void MacSerialStream::close()
{
    if(inRefNum == 0)
        return;
    SerSetBuf(inRefNum, NULL, 0);
    
    CloseDriver(inRefNum);
    CloseDriver(outRefNum);
    inRefNum = outRefNum = 0;
}

void MacSerialStream::open()
{
    OpenDriver(port ? "\p.BOut" : "\p.AOut", &outRefNum);
    OpenDriver(port ? "\p.BIn" : "\p.AIn", &inRefNum);
    SerSetBuf(inRefNum, inputBuffer, kInputBufferSize);

    SerShk shk;
    memset(&shk, 0, sizeof(shk));
    shk.fCTS = true;
    Control(outRefNum, kSERDHandshake, &shk);
    
    setBaud(curBaud);
}

MacSerialStream::~MacSerialStream()
{
    close();
}

void MacSerialStream::write(const void* p, size_t n)
{
    ParamBlockRec pb;
    memset(&pb, 0, sizeof(pb));
    pb.ioParam.ioRefNum = outRefNum;
        pb.ioParam.ioBuffer = (Ptr)p;
        pb.ioParam.ioReqCount = n;
	OSErr err = PBWriteSync(&pb);
}

void MacSerialStream::idle()
{
    long count = 0;
    SerGetBuf(inRefNum, &count);
    while(count > 0)
    {
        long count1 = count > kReadBufferSize ? kReadBufferSize : count;
        ParamBlockRec pb;
        memset(&pb, 0, sizeof(pb));
        pb.ioParam.ioRefNum = inRefNum;
        pb.ioParam.ioBuffer = (Ptr)&readBuffer;
        pb.ioParam.ioReqCount = count1;
        OSErr err = PBReadSync(&pb);
        if(err)
            return;
        count -= count1;

        notifyReceive((uint8_t*)readBuffer, count1);
    }
}

void MacSerialStream::setBaud(int baud)
{
    curBaud = baud;
    short baudval = 0;
    switch(baud)
    {
        case 9600:      baudval = baud9600; break;
        case 14400:     baudval = baud14400; break;
        case 19200:     baudval = baud19200; break;
        case 28800:     baudval = baud28800; break;
        case 38400:     baudval = baud38400; break;
        case 57600:     baudval = baud57600; break;
        case 115200:    baudval = 0; break;
        case 230400:    baudval = 0; break;
    }
    SerReset(outRefNum, baudval | data8 | noParity | stop10);

    if(baud == 115200)
        Control(outRefNum, kSERD115KBaud, nullptr);
    else if(baud == 230400)
        Control(outRefNum, kSERD230KBaud, nullptr);
}
