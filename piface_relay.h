/*******************************************************************************
  Copyright(c) 2014 Radek Kaczorek  <rkaczorek AT gmail DOT com>

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Library General Public
 License version 2 as published by the Free Software Foundation.
 .
 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Library General Public License for more details.
 .
 You should have received a copy of the GNU Library General Public License
 along with this library; see the file COPYING.LIB.  If not, write to
 the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 Boston, MA 02110-1301, USA.
*******************************************************************************/

#ifndef PIFACERELAY_H
#define PIFACERELAY_H

#include <string>
#include <iostream>
#include <stdio.h>

#include <defaultdevice.h>

class IndiPiFaceRelay : public INDI::DefaultDevice
{
protected:
private:
	int counter;
	IText PortT[2];
	ITextVectorProperty PortTP;
	IText SysTimeT[2];
	ITextVectorProperty SysTimeTP;
	IText SysInfoT[5];
	ITextVectorProperty SysInfoTP;
	IText NetInfoT[3];
	ITextVectorProperty NetInfoTP;
	ISwitch SwitchS[4];
	ISwitchVectorProperty SwitchSP;
	ISwitch Relay1S[1];
	ISwitchVectorProperty Relay1SP;
	ISwitch Relay2S[1];
	ISwitchVectorProperty Relay2SP;
	ISwitch Relay3S[1];
	ISwitchVectorProperty Relay3SP;
	ISwitch Relay4S[1];
	ISwitchVectorProperty Relay4SP;
public:
    IndiPiFaceRelay();
	virtual ~IndiPiFaceRelay();

	virtual const char *getDefaultName();

	virtual void TimerHit();
	virtual bool Connect();
	virtual bool Disconnect();
	virtual bool initProperties();
	virtual bool updateProperties();
	virtual void ISGetProperties(const char *dev);
	virtual bool ISNewNumber (const char *dev, const char *name, double values[], char *names[], int n);
	virtual bool ISNewSwitch (const char *dev, const char *name, ISState *states, char *names[], int n);
	virtual bool ISNewText (const char *dev, const char *name, char *texts[], char *names[], int n);
	virtual bool ISNewBLOB (const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[], char *formats[], char *names[], int n);
	virtual bool ISSnoopDevice(XMLEle *root);
	virtual bool saveConfigItems(FILE *fp);
	virtual int Relays(int index);
	virtual ISState RelayState(int index);
	virtual void LoadStates();
	int mcp23s17_fd;
};

#endif
