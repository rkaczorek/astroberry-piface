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

#include "rpi_gps.h"

// We declare an auto pointer to IndiRpigps
std::unique_ptr<IndiRpigps> indiRpigps(new IndiRpigps);

void ISInit()
{
   static int isInit =0;

   if (isInit == 1)
       return;

    isInit = 1;
    if(indiRpigps.get() == 0) indiRpigps.reset(new IndiRpigps());

}

void ISGetProperties(const char *dev)
{
	ISInit();
	indiRpigps->ISGetProperties(dev);
}

void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int num)
{
	ISInit();
	indiRpigps->ISNewSwitch(dev, name, states, names, num);
}

void ISNewText(	const char *dev, const char *name, char *texts[], char *names[], int num)
{
	ISInit();
	indiRpigps->ISNewText(dev, name, texts, names, num);
}

void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int num)
{
	ISInit();
	indiRpigps->ISNewNumber(dev, name, values, names, num);
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
	INDI_UNUSED(root);
}

IndiRpigps::IndiRpigps()
{
	setVersion(2,2);
}

IndiRpigps::~IndiRpigps()
{
}

bool IndiRpigps::Connect()
{
	// init GPS
	if (gpsHandle == NULL)
		gpsHandle = new gpsmm("localhost", DEFAULT_GPSD_PORT);

	if (gpsHandle->stream(WATCH_ENABLE|WATCH_JSON|WATCH_PPS) == NULL) {
		IDMessage(getDeviceName(), "No GPSD running.");
		return false;
	}

    IDMessage(getDeviceName(), "GPS connected successfully.");
    return true;
}

bool IndiRpigps::Disconnect()
{
	// disconnect GPS
	delete gpsHandle;
    gpsHandle = NULL;

    IDMessage(getDeviceName(), "GPS disconnected successfully.");
    return true;
}

const char * IndiRpigps::getDefaultName()
{
	return (char *)"Astroberry GPS";
}

bool IndiRpigps::initProperties()
{
    // We init parent properties first
    INDI::GPS::initProperties();

    IUFillText(&GPSstatusT[0],"GPS_FIX","Fix Mode",NULL);
    IUFillTextVector(&GPSstatusTP,GPSstatusT,1,getDeviceName(),"GPS_STATUS","GPS Status",MAIN_CONTROL_TAB,IP_RO,60,IPS_IDLE);

    IUFillNumber(&PolarisN[0],"HA","Polaris Hour Angle","%010.6m",0,24,0,0.0);
    IUFillNumberVector(&PolarisNP,PolarisN,1,getDeviceName(),"POLARIS","Polaris",MAIN_CONTROL_TAB,IP_RO,60,IPS_IDLE);

    return true;
}

bool IndiRpigps::updateProperties()
{
    // Call parent update properties first
    INDI::GPS::updateProperties();

    if (isConnected())
    {
		deleteProperty(RefreshSP.name);
        defineText(&GPSstatusTP);
        defineNumber(&PolarisNP);
        defineSwitch(&RefreshSP);
    }
    else
    {
		// We're disconnected
        deleteProperty(GPSstatusTP.name);
        deleteProperty(PolarisNP.name);
    }
    return true;
}

IPState IndiRpigps::updateGPS()
{
	TimeTP.s = IPS_BUSY;
	IDSetText(&TimeTP, NULL);
	LocationNP.s = IPS_BUSY;
	IDSetNumber(&LocationNP, NULL);

	GPSstatusTP.s = IPS_BUSY;
	IDSetText(&GPSstatusTP, NULL);
	PolarisNP.s = IPS_BUSY;
	IDSetNumber(&PolarisNP, NULL);
	RefreshSP.s = IPS_BUSY;
	IDSetSwitch(&RefreshSP, NULL);

	struct gps_data_t* gpsData;

	// wait for data
	if (!gpsHandle->waiting(100000))
	{
		IDMessage(getDeviceName(), "Waiting for gps data...");
		return IPS_BUSY;
	}
	
	// check if we have gps data
	if ((gpsData = gpsHandle->read()) == NULL)
	{
		IDMessage(getDeviceName(), "GPSD read error.");
		GPSstatusTP.s = IPS_ALERT;
		IDSetText(&GPSstatusTP, NULL);
		return IPS_ALERT;
	}
	
	// detect gps fix
	if (GPSstatusT[0].text == "NO FIX" && gpsData->fix.mode > 2)
		IDMessage(getDeviceName(), "GPS fix obtained.");

	// update gps fix status
	if ( gpsData->fix.mode >= 3 )
	{
		GPSstatusT[0].text = (char*) "3D FIX";
		GPSstatusTP.s = IPS_OK;
		IDSetText(&GPSstatusTP, NULL);
	}
	else if ( gpsData->fix.mode == 2 )
	{
		GPSstatusT[0].text = (char*) "2D FIX";
		GPSstatusTP.s = IPS_BUSY;
		IDSetText(&GPSstatusTP, NULL);
		return IPS_BUSY;
	}
	else
	{
		GPSstatusT[0].text = (char*) "NO FIX";
		GPSstatusTP.s = IPS_BUSY;
		IDSetText(&GPSstatusTP, NULL);
		return IPS_BUSY;
	}

	// update gps time
	struct tm *utc_timeinfo, *local_timeinfo;
	static char ts[32];
	time_t rawtime;

	// get utc_time from gps
	//rawtime = gpsData->fix.time;
	time(&rawtime);
	utc_timeinfo = gmtime (&rawtime);
	strftime(ts, 20, "%Y-%m-%dT%H:%M:%S", utc_timeinfo);
	IUSaveText(&TimeT[0], ts);
	TimeTP.s = IPS_OK;

	// get utc offset
	local_timeinfo = localtime (&rawtime);
	snprintf(ts, sizeof(ts), "%4.2f", (local_timeinfo->tm_gmtoff/3600.0));
	IUSaveText(&TimeT[1], ts);

	// update gps location
	LocationN[0].value = gpsData->fix.latitude;
	LocationN[1].value = gpsData->fix.longitude;
	LocationN[2].value = gpsData->fix.altitude;
	LocationNP.s = IPS_OK;
	
	// calculate Polaris HA
	double jd, lst, polarislsrt;

	// polaris location - RA 02h 31m 47s DEC 89° 15' 50'' (J2000)
	jd = ln_get_julian_from_timet(&rawtime);
	lst=ln_get_apparent_sidereal_time(jd);

	// Local Hour Angle = Local Sidereal Time - Polaris Right Ascension
	polarislsrt = lst - 2.529722222 + (gpsData->fix.longitude / 15.0);
	PolarisN[0].value = polarislsrt;
	
	GPSstatusTP.s = IPS_OK;
	IDSetText(&GPSstatusTP, NULL);
	PolarisNP.s = IPS_OK;
	IDSetNumber(&PolarisNP, NULL);
	RefreshSP.s = IPS_OK;
	IDSetSwitch(&RefreshSP, NULL);

    return IPS_OK;
}
