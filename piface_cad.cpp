/*******************************************************************************
  Copyright(c) 2016 Radek Kaczorek  <rkaczorek AT gmail DOT com>

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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string>
#include <iostream>
#include <memory>
#include <mcp23s17.h>
#include <pifacecad.h>

#include "piface_cad.h"

#define AUTOLIGHTOFF 5

// virtual displays - switch 5 to scroll
// 0 - time and date
// 1 - eqmod time
// 2 - eqmod location
// 3 - eqmod  RA/DEC
// 4 - piface focuser position
// 5 - piface relay memory and load
// 6 - piface relay cpu temperature
// 7 - piface relay network
#define DISPLAYS 7

// We declare a pointer to IndiPiFaceCAD
// std::auto_ptr<IndiPiFaceCAD> indiPiFaceCAD(0);
std::unique_ptr<IndiPiFaceCAD> indiPiFaceCAD(new IndiPiFaceCAD);

void ISPoll(void *p);
void ISInit()
{
   static int isInit =0;

   if (isInit == 1)
       return;

    isInit = 1;
    if(indiPiFaceCAD.get() == 0) indiPiFaceCAD.reset(new IndiPiFaceCAD());

}
void ISGetProperties(const char *dev)
{
        ISInit();
        indiPiFaceCAD->ISGetProperties(dev);
}
void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int num)
{
        ISInit();
        indiPiFaceCAD->ISNewSwitch(dev, name, states, names, num);
}
void ISNewText(	const char *dev, const char *name, char *texts[], char *names[], int num)
{
        ISInit();
        indiPiFaceCAD->ISNewText(dev, name, texts, names, num);
}
void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int num)
{
        ISInit();
        indiPiFaceCAD->ISNewNumber(dev, name, values, names, num);
}
void ISNewBLOB (const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[], char *formats[], char *names[], int num)
{
  INDI_UNUSED(dev);
  INDI_UNUSED(name);
  INDI_UNUSED(sizes);
  INDI_UNUSED(blobsizes);
  INDI_UNUSED(blobs);
  INDI_UNUSED(formats);
  INDI_UNUSED(names);
  INDI_UNUSED(num);
}
void ISSnoopDevice (XMLEle *root)
{
    ISInit();
    indiPiFaceCAD->ISSnoopDevice(root);
}
IndiPiFaceCAD::IndiPiFaceCAD()
{
	setVersion(1,0);
}
IndiPiFaceCAD::~IndiPiFaceCAD()
{
}
bool IndiPiFaceCAD::Connect()
{
	// open device
    mcp23s17_fd = pifacecad_open();

    if(mcp23s17_fd == -1)
	{
		IDMessage(getDeviceName(), "PiFace CAD device is not available.");
		return false;
	}
	
    pifacecad_lcd_clear();
    pifacecad_lcd_blink_off();
	pifacecad_lcd_cursor_off();
    pifacecad_lcd_backlight_on();

	// init auto backlight off
	time(&last_action_time);

	SetTimer(200);

    IDMessage(getDeviceName(), "PiFace CAD connected successfully.");
    return true;
}
bool IndiPiFaceCAD::Disconnect()
{
	// close device
    pifacecad_lcd_clear();
    pifacecad_lcd_backlight_off();
    pifacecad_close();

	// close mcp
	close(mcp23s17_fd);

    IDMessage(getDeviceName(), "PiFace CAD disconnected successfully.");
    return true;
}
void IndiPiFaceCAD::TimerHit()
{
	if(isConnected())
	{
		// uint8_t switch_bits = pifacecad_read_switches();
		uint8_t switch1_state = pifacecad_read_switch(0);
		uint8_t switch2_state = pifacecad_read_switch(1);
		uint8_t switch3_state = pifacecad_read_switch(2);
		uint8_t switch4_state = pifacecad_read_switch(3);
		uint8_t switch5_state = pifacecad_read_switch(4);
		uint8_t switch6_state = pifacecad_read_switch(5);
		uint8_t switch7_state = pifacecad_read_switch(6);
		uint8_t switch8_state = pifacecad_read_switch(7);

		if (DispLightS[2].s == ISS_ON & (switch1_state==0 || switch2_state==0 || switch3_state==0 || switch4_state==0 || switch5_state==0 || switch6_state==0 || switch7_state==0 || switch8_state==0))
		{
			if(DispLightS[1].s != ISS_ON)
				pifacecad_lcd_backlight_on();
			time(&last_action_time);
			DispLightSP.s = IPS_OK;
			IDSetSwitch(&DispLightSP, NULL);
		}

		// auto backlight off
		time_t now;
		if( DispLightS[2].s == ISS_ON && difftime(time(&now), last_action_time) > AUTOLIGHTOFF )
		{
			pifacecad_lcd_backlight_off();
			DispLightSP.s = IPS_IDLE;
			IDSetSwitch(&DispLightSP, NULL);
		}

		// handle switches
		if (switch1_state==0)
		{
			//button 1
			ButtonL[0].s = IPS_BUSY;
			IDSetLight(&ButtonLP, NULL);
			btnAction(1);
			ButtonL[0].s = IPS_IDLE;
			IDSetLight(&ButtonLP, NULL);
		}
		if (switch2_state==0)
		{
			//button 2
			ButtonL[1].s = IPS_BUSY;
			IDSetLight(&ButtonLP, NULL);
			btnAction(2);
			ButtonL[1].s = IPS_IDLE;
			IDSetLight(&ButtonLP, NULL);
		}
		if (switch3_state==0)
		{
			//button 3
			ButtonL[2].s = IPS_BUSY;
			IDSetLight(&ButtonLP, NULL);
			btnAction(3);
			ButtonL[2].s = IPS_IDLE;
			IDSetLight(&ButtonLP, NULL);
		}
		if (switch4_state==0)
		{
			//button 4
			ButtonL[3].s = IPS_BUSY;
			IDSetLight(&ButtonLP, NULL);
			btnAction(4);
			ButtonL[3].s = IPS_IDLE;
			IDSetLight(&ButtonLP, NULL);
		}
		if (switch5_state==0)
		{
			//button 5
			ButtonL[4].s = IPS_BUSY;
			IDSetLight(&ButtonLP, NULL);
			btnAction(5);
			ButtonL[4].s = IPS_IDLE;
			IDSetLight(&ButtonLP, NULL);
		}
		if (switch6_state==0)
		{
			//nav select
			NavL[1].s = IPS_BUSY;
			IDSetLight(&NavLP, NULL);
			btnAction(6);
			NavL[1].s = IPS_IDLE;
			IDSetLight(&NavLP, NULL);
		}
		if (switch7_state==0)
		{
			//nav 1
			NavL[0].s = IPS_BUSY;
			IDSetLight(&NavLP, NULL);
			btnAction(7);
			NavL[0].s = IPS_IDLE;
			IDSetLight(&NavLP, NULL);
		}
		if (switch8_state==0)
		{
			//nav 2
			NavL[2].s = IPS_BUSY;
			IDSetLight(&NavLP, NULL);
			btnAction(8);
			NavL[2].s = IPS_IDLE;
			IDSetLight(&NavLP, NULL);
		}

		// add to counter and reset every 60s
		if(timer_counter < 300)
		{
			timer_counter++;
		} else {
			timer_counter=0;
		}

		// update every 1s
		if(timer_counter % 5 == 0)
		{
			// print to LCD
			UpdateInfo(vdispid);

			// print to LCD
			LCDPrint((char*)dispinfo);
		}

		// update every 3s
		if(timer_counter % 15 == 0)
		{
		}

		// update every 10s
		if(timer_counter % 50 == 0)
		{
		}

		// update every 30s
		if(timer_counter % 150 == 0)
		{
		}

		// update every 60s
		if(timer_counter % 300 == 0)
		{
		}

		SetTimer(200);
		return;
    }
}
const char * IndiPiFaceCAD::getDefaultName()
{
        return (char *)"PiFace CAD";
}
bool IndiPiFaceCAD::initProperties()
{
    // We init parent properties first
    INDI::DefaultDevice::initProperties();

	// main
    IUFillText(&DispLineT[0],"LINE","Text",NULL);
    IUFillTextVector(&DispLineTP,DispLineT,1,getDeviceName(),"LCD_DISPLAY","LCD Display",MAIN_CONTROL_TAB,IP_RO,60,IPS_IDLE);

	IUFillSwitch(&DispLightS[0],"LIGHT_ON","On",ISS_OFF);
	IUFillSwitch(&DispLightS[1],"LIGHT_OFF","Off",ISS_OFF);
	IUFillSwitch(&DispLightS[2],"LIGHT_AUTO","Auto",ISS_ON);
	IUFillSwitchVector(&DispLightSP,DispLightS,3,getDeviceName(),"LCD_LIGHT","LCD Light",MAIN_CONTROL_TAB,IP_RW,ISR_1OFMANY,60,IPS_OK);

	// monitor
	IUFillLight(&ButtonL[0], "BTN1", "Button 1", IPS_IDLE);
    IUFillLight(&ButtonL[1], "BTN2", "Button 2", IPS_IDLE);
    IUFillLight(&ButtonL[2], "BTN3", "Button 3", IPS_IDLE);
    IUFillLight(&ButtonL[3], "BTN4", "Button 4", IPS_IDLE);
    IUFillLight(&ButtonL[4], "BTN5", "Button 5", IPS_IDLE);
    IUFillLightVector(&ButtonLP, ButtonL, 5, getDeviceName(), "PIFACE_BUTTONS", "Buttons", "Monitor", IPS_IDLE);

    IUFillLight(&NavL[0], "NAV1", "Nav 1", IPS_IDLE);
    IUFillLight(&NavL[1], "NAV_OK", "Nav OK", IPS_IDLE);
    IUFillLight(&NavL[2], "NAV3", "Nav 2", IPS_IDLE);
    IUFillLightVector(&NavLP, NavL, 3, getDeviceName(), "PIFACE_NAV", "Nav Button", "Monitor", IPS_IDLE);

	// options
    IUFillText(&PortT[0], "PORT", "Port","PiFace Control and Display");
    IUFillTextVector(&PortTP,PortT,1,getDeviceName(),"DEVICE_PORT","Ports",OPTIONS_TAB,IP_RO,0,IPS_OK);

	// snooping
    IUFillText(&SysInfoT[0],"HARDWARE","Hardware",NULL);
    IUFillText(&SysInfoT[1],"UPTIME","Uptime (hh:mm)",NULL);
    IUFillText(&SysInfoT[2],"LOAD","Load (1/5/15 min.)",NULL);
    IUFillText(&SysInfoT[3],"MEM","Free Memory",NULL);
    IUFillText(&SysInfoT[4],"TEMP","System Temperature",NULL);
    IUFillTextVector(&SysInfoTP,SysInfoT,5,"PiFace Relay","SYSTEM_INFO","System Info","System Info",IP_RO,60,IPS_IDLE);

    IUFillText(&NetInfoT[0],"HOSTNAME","Hostname",NULL);
    IUFillText(&NetInfoT[1],"LOCAL_IP","Local IP",NULL);
    IUFillText(&NetInfoT[2],"PUBLIC_IP","Public IP",NULL);
    IUFillTextVector(&NetInfoTP,NetInfoT,3,"PiFace Relay","NETWORK_INFO","Network Info","System Info",IP_RO,60,IPS_IDLE);

    IUFillNumber(&FocusAbsPosN[0],"FOCUS_ABSOLUTE_POSITION","Steps","%0.0f",0,10000,100,0);
    IUFillNumberVector(&FocusAbsPosNP,FocusAbsPosN,1,"PiFace Focuser","ABS_FOCUS_POSITION","Absolute",MAIN_CONTROL_TAB,IP_RW,0,IPS_OK);

	IUFillText(&TimeT[0],"UTC","UTC Time",NULL);
	IUFillText(&TimeT[1],"OFFSET","UTC Offset",NULL);
	IUFillTextVector(&TimeTP,TimeT,2,"EQMod Mount","TIME_UTC","UTC",SITE_TAB,IP_RW,60,IPS_IDLE);

    IUFillNumber(&LocationN[0],"LAT","Lat (dd:mm:ss)","%010.6m",-90,90,0,0.0);
    IUFillNumber(&LocationN[1],"LONG","Lon (dd:mm:ss)","%010.6m",-180,180,0,0.0 );
    IUFillNumberVector(&LocationNP,LocationN,2,"EQMod Mount","GEOGRAPHIC_COORD","Scope Location",SITE_TAB,IP_RW,60,IPS_OK);

	IUFillNumber(&EqN[0],"RA","RA (hh:mm:ss)","%010.6m",0,24,0,0);
    IUFillNumber(&EqN[1],"DEC","DEC (dd:mm:ss)","%010.6m",-90,90,0,0);
    IUFillNumberVector(&EqNP,EqN,2,"EQMod Mount","EQUATORIAL_EOD_COORD","Eq. Coordinates",MAIN_CONTROL_TAB,IP_RO,60,IPS_IDLE);

	defineText(&PortTP);

	// default virtual display
	vdispid = 0;

	// welcome screen
	LCDPrint((char*)"Astroberry\nPiFace v1.0");

    return true;
}
bool IndiPiFaceCAD::updateProperties()
{
    // Call parent update properties first
    INDI::DefaultDevice::updateProperties();

    if (isConnected())
    {
		defineText(&DispLineTP);
		defineSwitch(&DispLightSP);
		defineLight(&ButtonLP);
		defineLight(&NavLP);
		IDSnoopDevice("PiFace Relay", "SYSTEM_INFO");
		IDSnoopDevice("PiFace Relay", "NETWORK_INFO");
		IDSnoopDevice("PiFace Focuser", "ABS_FOCUS_POSITION");
		IDSnoopDevice("EQMod Mount", "TIME_UTC");
		IDSnoopDevice("EQMod Mount", "GEOGRAPHIC_COORD");
		IDSnoopDevice("EQMod Mount", "EQUATORIAL_EOD_COORD");
    }
    else
    {
		// We're disconnected
		deleteProperty(DispLineTP.name);
		deleteProperty(DispLightSP.name);
		deleteProperty(ButtonLP.name);
		deleteProperty(NavLP.name);
    }
    return true;
}
void IndiPiFaceCAD::ISGetProperties(const char *dev)
{
    INDI::DefaultDevice::ISGetProperties(dev);

    /* Add debug controls so we may debug driver if necessary */
    //addDebugControl();
}
bool IndiPiFaceCAD::ISNewNumber (const char *dev, const char *name, double values[], char *names[], int n)
{
	return INDI::DefaultDevice::ISNewNumber(dev,name,values,names,n);
}
bool IndiPiFaceCAD::ISNewSwitch (const char *dev, const char *name, ISState *states, char *names[], int n)
{
	// first we check if it's for our device
    if (!strcmp(dev, getDeviceName()))
    {
        // handle lcd light
        if(!strcmp(name, DispLightSP.name))
        {
			IUUpdateSwitch(&DispLightSP, states, names, n);

			// lcd light on 
            if ( DispLightS[0].s == ISS_ON )
            {
				pifacecad_lcd_backlight_on();
				DispLightSP.s = IPS_OK;
			}
			// lcd light off 
            if ( DispLightS[1].s == ISS_ON )
            {
				pifacecad_lcd_backlight_off();
				DispLightSP.s = IPS_IDLE;
			}
			IDSetSwitch(&DispLightSP, NULL);
			time(&last_action_time);
			return true;
		}
	}
	return INDI::DefaultDevice::ISNewSwitch (dev, name, states, names, n);
}
bool IndiPiFaceCAD::ISNewText (const char *dev, const char *name, char *texts[], char *names[], int n)
{
	return INDI::DefaultDevice::ISNewText (dev, name, texts, names, n);
}
bool IndiPiFaceCAD::ISNewBLOB (const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[], char *formats[], char *names[], int n)
{
	return INDI::DefaultDevice::ISNewBLOB (dev, name, sizes, blobsizes, blobs, formats, names, n);
}
bool IndiPiFaceCAD::ISSnoopDevice(XMLEle *root)
{

	if (IUSnoopText(root, &SysInfoTP) == 0)
	{
		return true;
	}

	if (IUSnoopText(root, &NetInfoTP) == 0)
	{
		return true;
	}

	if (IUSnoopNumber(root, &FocusAbsPosNP) == 0)
	{
		return true;
	}

	if (IUSnoopText(root, &TimeTP) == 0)
	{
		return true;
	}

	if (IUSnoopNumber(root, &EqNP) == 0)
	{
		return true;
	}

	if (IUSnoopNumber(root, &LocationNP) == 0)
	{
		return true;
	}

    return INDI::DefaultDevice::ISSnoopDevice(root);
}
bool IndiPiFaceCAD::saveConfigItems(FILE *fp)
{
	IUSaveConfigSwitch(fp, &DispLightSP);
    return true;
}
int IndiPiFaceCAD::btnAction(int index)
{
	switch (index)
	{
	case 1:
		// button 1
		usleep(100000);
		break;
	case 2:
		// button 2
		usleep(100000);
		break;
	case 3:
		// button 3
		usleep(100000);
		break;
	case 4:
		// button 4
		usleep(100000);
		break;
	case 5:
		// button 5
		// scroll virtual displays
		if(vdispid < DISPLAYS)
		{
			vdispid++;
		} else {
			vdispid = 0;
		}
		break;
	case 6:
		// select
		usleep(100000);
		break;
	case 7:
		// nav 1
		FocusAbsPosN[0].value = FocusAbsPosN[0].value + 100;
		TimeTP.s=IPS_OK;
		IDSetNumber(&FocusAbsPosNP, NULL);
		usleep(100000);
		break;
	case 8:
		// nav 2
		FocusAbsPosN[0].value = FocusAbsPosN[0].value - 100;
		TimeTP.s=IPS_OK;
		IDSetNumber(&FocusAbsPosNP, NULL);
		usleep(100000);
		break;
	}

	return 0;
}
int IndiPiFaceCAD::LCDPrint(char* text)
{
	DispLineTP.s = IPS_BUSY;
	IDSetText(&DispLineTP, NULL);
	//pifacecad_lcd_clear();
	pifacecad_lcd_set_cursor(0,0);

    // write line
    pifacecad_lcd_write(text);

	DispLineT[0].text = text;
	DispLineTP.s = IPS_OK;
	IDSetText(&DispLineTP, NULL);

	return 0;
}
int IndiPiFaceCAD::UpdateInfo(int id)
{
	FILE* pipe;

	// display text
	switch(id)
	{
	case 1:
		if(TimeT[0].text != NULL)
		{
			std::string s = TimeT[0].text;
			int pos = s.find('T');
			std::string dateinfo = s.substr(0, pos);
			std::string timeinfo = s.substr(pos+1, s.size() - pos);
			strcpy (dispinfo, "Date: ");
			strcat (dispinfo, dateinfo.c_str());
			strcat (dispinfo, "\nUTC: ");
			strcat (dispinfo, timeinfo.c_str());
		}
		else
		{
			strcpy (dispinfo, "EQMod Mount\nNo time info");
		}
		break;
	case 2:
		if(LocationN[0].value && LocationN[1].value)
		{
			char latitude[32];
			char longitude[32];
			int latdeg = (int) LocationN[0].value;
			int londeg = (int) LocationN[1].value;
			int latmin = (int) ((LocationN[0].value - latdeg) * 60);
			int lonmin = (int) ((LocationN[1].value - londeg) * 60);
			double latsec = ((LocationN[0].value - latdeg - latmin/60) * 3600);
			double lonsec = ((LocationN[1].value - londeg - lonmin/60) * 3600);
			snprintf(latitude, 32, "%02d:%02d:%0.1f", latdeg, latmin, latsec);
			snprintf(longitude, 32,"%02d:%02d:%0.1f", londeg, lonmin, lonsec);
			// 52.166107 - 52° 9' 57.9852"
			// 21.075572 - 21° 4' 32.0592"
			//snprintf(latitude, 32, "%0.6f", LocationN[0].value);
			//snprintf(longitude, 32, "%0.6f", LocationN[1].value);
			strcpy (dispinfo, "Lat : ");
			strcat (dispinfo, latitude);
			strcat (dispinfo, "\nLong: ");
			strcat (dispinfo, longitude);
		}
		else
		{
			strcpy (dispinfo, "EQMod Mount\nNo location info");
		}
		break;
	case 3:
		if(EqN[0].value && EqN[1].value)
		{
			char ra[32];
			char dec[32];
			snprintf(ra, 32, "%0.6f", EqN[0].value);
			snprintf(dec, 32, "%0.6f", EqN[1].value);
			strcpy (dispinfo, "RA : ");
			strcat (dispinfo, ra);
			strcat (dispinfo, "\nDEC: ");
			strcat (dispinfo, dec);
		}
		else
		{
			strcpy (dispinfo, "EQMod Mount\nNo coords info");
		}
		break;
	case 4:
		if(FocusAbsPosN[0].value)
		{
			char pos[32];
			snprintf(pos, 32, "%0.0f", FocusAbsPosN[0].value);
			strcpy (dispinfo, "PiFace Focuser");
			strcat (dispinfo, "\nPosition: ");
			strcat (dispinfo, pos);
		}
		else
		{
			strcpy (dispinfo, "PiFace Focuser\nNo position info");
		}
		break;
	case 5:
		if(SysInfoT[2].text && SysInfoT[4].text)
		{
			strcpy (dispinfo, "Uptime: ");
			strcat (dispinfo, SysInfoT[1].text);
			strcat (dispinfo, "\nCPU Temp: ");
			strcat (dispinfo, SysInfoT[4].text);
		}
		else
		{
			strcpy (dispinfo, "PiFace Relay\nNo uptime info");
		}
		break;
	case 6:
		if(SysInfoT[3].text && SysInfoT[2].text)
		{
			strcpy (dispinfo, "Free: ");
			strcat (dispinfo, SysInfoT[3].text);
			strcat (dispinfo, "\n");
			strcat (dispinfo, SysInfoT[2].text);
		}
		else
		{
			strcpy (dispinfo, "PiFace Relay\nNo system info");
		}
		break;
	case 7:
	if(NetInfoT[0].text && NetInfoT[1].text)
		{
			strcpy (dispinfo, NetInfoT[0].text);
			strcat (dispinfo, "\n");
			strcat (dispinfo, NetInfoT[1].text);
		}
		else
		{
			strcpy (dispinfo, "PiFace Relay\nNo network info");
		}
		break;
	default:
		// diplay date & time
		struct tm *local_timeinfo;
		time_t rawtime;
		time(&rawtime);
		local_timeinfo = localtime (&rawtime);
		strftime(dispinfo, 32, "Date: %Y-%m-%d\nTime: %H:%M:%S", local_timeinfo);
		//snprintf(zome, sizeof(tinfo), "Zone: %0.0f", (local_timeinfo->tm_gmtoff/3600.0));
	}
	return 0;
}
