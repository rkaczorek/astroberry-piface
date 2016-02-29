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

#ifndef PIFACECAD_H
#define PIFACECAD_H

#include <defaultdevice.h>
#include <piface_relay.h>

class IndiPiFaceCAD : public INDI::DefaultDevice
{
protected:
private:
	int counter;
	IText PortT[2];
	ITextVectorProperty PortTP;
	IText DispLineT[1];
	ITextVectorProperty DispLineTP;
	ISwitch DispLightS[3];
	ISwitchVectorProperty DispLightSP;
	ILight ButtonL[5];
	ILightVectorProperty ButtonLP;
	ILight NavL[3];
	ILightVectorProperty NavLP;

	IText SysInfoT[5];
	ITextVectorProperty SysInfoTP;
	IText NetInfoT[3];
	ITextVectorProperty NetInfoTP;
	INumber FocusAbsPosN[1];
	INumberVectorProperty FocusAbsPosNP;
	IText TimeT[2];
	ITextVectorProperty TimeTP;
	INumber LocationN[2];
	INumberVectorProperty LocationNP;
	INumber EqN[2];
	INumberVectorProperty EqNP;
public:
    IndiPiFaceCAD();
	virtual ~IndiPiFaceCAD();

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
	virtual int UpdateInfo(int id);
	virtual int LCDPrint(char* text);
	virtual int btnAction(int index);
	int mcp23s17_fd;
	int timer_counter = 0;
	time_t last_action_time;
	int vdispid;
	char dispinfo[32];
};

#endif
