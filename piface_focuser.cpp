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
#include <memory>
#include <mcp23s17.h>

#include "piface_focuser.h"

#define MAJOR_VERSION 2
#define MINOR_VERSION 0

#define FOCUSNAMEF1 "PiFace Focuser 1"
#define FOCUSNAMEF2 "PiFace Focuser 2"

#define CHECK_BIT(var,pos) (((var)>>(pos)) & 1)
#define MAX_STEPS 20000

// We declare a pointer to indiPiFaceFocuser.
std::unique_ptr<IndiPiFaceFocuser1> indiPiFaceFocuser1(new IndiPiFaceFocuser1);
std::unique_ptr<IndiPiFaceFocuser2> indiPiFaceFocuser2(new IndiPiFaceFocuser2);

void ISPoll(void *p);
void ISGetProperties(const char *dev)
{
        indiPiFaceFocuser1->ISGetProperties(dev);
        indiPiFaceFocuser2->ISGetProperties(dev);
}

void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int num)
{
		if (!strcmp(dev, indiPiFaceFocuser1->getDeviceName()))
			indiPiFaceFocuser1->ISNewSwitch(dev, name, states, names, num);
		else if (!strcmp(dev, indiPiFaceFocuser2->getDeviceName()))
			indiPiFaceFocuser2->ISNewSwitch(dev, name, states, names, num);
}

void ISNewText(	const char *dev, const char *name, char *texts[], char *names[], int num)
{
		if (!strcmp(dev, indiPiFaceFocuser1->getDeviceName()))
			indiPiFaceFocuser1->ISNewText(dev, name, texts, names, num);
		else if (!strcmp(dev, indiPiFaceFocuser2->getDeviceName()))
			indiPiFaceFocuser2->ISNewText(dev, name, texts, names, num);
}

void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int num)
{
		if (!strcmp(dev, indiPiFaceFocuser1->getDeviceName()))
			indiPiFaceFocuser1->ISNewNumber(dev, name, values, names, num);
		else if (!strcmp(dev, indiPiFaceFocuser2->getDeviceName()))
			indiPiFaceFocuser2->ISNewNumber(dev, name, values, names, num);
}

void ISNewBLOB (const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[], char *formats[], char *names[], int n)
{
  INDI_UNUSED(dev);
  INDI_UNUSED(name);
  INDI_UNUSED(sizes);
  INDI_UNUSED(blobsizes);
  INDI_UNUSED(blobs);
  INDI_UNUSED(formats);
  INDI_UNUSED(names);
  INDI_UNUSED(n);
}

void ISSnoopDevice (XMLEle *root)
{
	indiPiFaceFocuser1->ISSnoopDevice(root);
	indiPiFaceFocuser2->ISSnoopDevice(root);
}

/************************************************************************************
*
*               First Focuser (IndiPiFaceFocuser1)
*
*************************************************************************************/

IndiPiFaceFocuser1::IndiPiFaceFocuser1()
{
	setVersion(MAJOR_VERSION,MINOR_VERSION);
}

IndiPiFaceFocuser1::~IndiPiFaceFocuser1()
{

}

const char * IndiPiFaceFocuser1::getDefaultName()
{
	return FOCUSNAMEF1;
}

bool IndiPiFaceFocuser1::Connect()
{
	// open device
	mcp23s17_fd = mcp23s17_open(0,0);

	if(mcp23s17_fd == -1)
	{
		IDMessage(getDeviceName(), "PiFace Focuser 1 device is not available.");
		return false;
	}

	// config register
	const uint8_t ioconfig = BANK_OFF | \
                             INT_MIRROR_OFF | \
                             SEQOP_OFF | \
                             DISSLW_OFF | \
                             HAEN_ON | \
                             ODR_OFF | \
                             INTPOL_LOW;
	mcp23s17_write_reg(ioconfig, IOCON, 0, mcp23s17_fd);

	// I/O direction
	mcp23s17_write_reg(0x00, IODIRB, 0, mcp23s17_fd);

	// pull ups
	mcp23s17_write_reg(0x00, GPPUB, 0, mcp23s17_fd);

	IDMessage(getDeviceName(), "PiFace Focuser 1 connected successfully.");
	return true;
}

bool IndiPiFaceFocuser1::Disconnect()
{
	// park focuser
	if ( FocusParkingS[0].s == ISS_ON )
	{
		IDMessage(getDeviceName(), "PiFace Focuser 1 is parking...");
		MoveAbsFocuser(FocusAbsPosN[0].min);
	}

	// close device
	close(mcp23s17_fd);

	IDMessage(getDeviceName(), "PiFace Focuser 1 disconnected successfully.");
	return true;
}

bool IndiPiFaceFocuser1::initProperties()
{
    INDI::Focuser::initProperties();

	// options tab
	IUFillText(&PortT[0], "PORT", "Port","PiFace Relay+");
	IUFillTextVector(&PortTP,PortT,1,getDeviceName(),"DEVICE_PORT","Ports",OPTIONS_TAB,IP_RO,0,IPS_OK);

	IUFillNumber(&MotorDelayN[0],"MOTOR_DELAY","milliseconds","%0.0f",1,100,1,2);
	IUFillNumberVector(&MotorDelayNP,MotorDelayN,1,getDeviceName(),"MOTOR_CONFIG","Step Delay",OPTIONS_TAB,IP_RW,0,IPS_OK);

	IUFillSwitch(&MotorDirS[0],"FORWARD","Normal",ISS_ON);
	IUFillSwitch(&MotorDirS[1],"REVERSE","Reverse",ISS_OFF);
	IUFillSwitchVector(&MotorDirSP,MotorDirS,2,getDeviceName(),"MOTOR_DIR","Motor Dir",OPTIONS_TAB,IP_RW,ISR_1OFMANY,60,IPS_OK);

	IUFillNumber(&FocusBacklashN[0], "FOCUS_BACKLASH_VALUE", "Steps", "%0.0f", 0, 100, 1, 0);
	IUFillNumberVector(&FocusBacklashNP, FocusBacklashN, 1, getDeviceName(), "FOCUS_BACKLASH", "Backlash", OPTIONS_TAB, IP_RW, 0, IPS_IDLE);

	IUFillSwitch(&FocusParkingS[0],"FOCUS_PARKON","Enable",ISS_ON);
	IUFillSwitch(&FocusParkingS[1],"FOCUS_PARKOFF","Disable",ISS_OFF);
	IUFillSwitchVector(&FocusParkingSP,FocusParkingS,2,getDeviceName(),"FOCUS_PARK","Parking",OPTIONS_TAB,IP_RW,ISR_1OFMANY,60,IPS_OK);

	IUFillSwitch(&FocusResetS[0],"FOCUS_RESET","Reset",ISS_OFF);
	IUFillSwitchVector(&FocusResetSP,FocusResetS,1,getDeviceName(),"FOCUS_RESET","Position Reset",OPTIONS_TAB,IP_RW,ISR_1OFMANY,60,IPS_OK);

	// main tab
	IUFillSwitch(&FocusMotionS[0],"FOCUS_INWARD","Focus In",ISS_OFF);
	IUFillSwitch(&FocusMotionS[1],"FOCUS_OUTWARD","Focus Out",ISS_ON);
	IUFillSwitchVector(&FocusMotionSP,FocusMotionS,2,getDeviceName(),"FOCUS_MOTION","Direction",MAIN_CONTROL_TAB,IP_RW,ISR_ATMOST1,60,IPS_OK);

	IUFillNumber(&FocusRelPosN[0],"FOCUS_RELATIVE_POSITION","Steps","%0.0f",0,(int)MAX_STEPS/10,(int)MAX_STEPS/100,(int)MAX_STEPS/100);
	IUFillNumberVector(&FocusRelPosNP,FocusRelPosN,1,getDeviceName(),"REL_FOCUS_POSITION","Relative",MAIN_CONTROL_TAB,IP_RW,60,IPS_OK);

	IUFillNumber(&FocusAbsPosN[0],"FOCUS_ABSOLUTE_POSITION","Steps","%0.0f",0,MAX_STEPS,(int)MAX_STEPS/100,0);
	IUFillNumberVector(&FocusAbsPosNP,FocusAbsPosN,1,getDeviceName(),"ABS_FOCUS_POSITION","Absolute",MAIN_CONTROL_TAB,IP_RW,0,IPS_OK);

	IUFillNumber(&PresetN[0], "Preset 1", "", "%0.0f", 0, MAX_STEPS, (int)(MAX_STEPS/100), 0);
	IUFillNumber(&PresetN[1], "Preset 2", "", "%0.0f", 0, MAX_STEPS, (int)(MAX_STEPS/100), 0);
	IUFillNumber(&PresetN[2], "Preset 3", "", "%0.0f", 0, MAX_STEPS, (int)(MAX_STEPS/100), 0);
	IUFillNumberVector(&PresetNP, PresetN, 3, getDeviceName(), "Presets", "Presets", "Presets", IP_RW, 0, IPS_IDLE);

	IUFillSwitch(&PresetGotoS[0], "Preset 1", "Preset 1", ISS_OFF);
	IUFillSwitch(&PresetGotoS[1], "Preset 2", "Preset 2", ISS_OFF);
	IUFillSwitch(&PresetGotoS[2], "Preset 3", "Preset 3", ISS_OFF);
	IUFillSwitchVector(&PresetGotoSP, PresetGotoS, 3, getDeviceName(), "Presets Goto", "Goto", MAIN_CONTROL_TAB,IP_RW,ISR_1OFMANY,60,IPS_OK);

	// set capabilities
	SetFocuserCapability(FOCUSER_CAN_ABS_MOVE | FOCUSER_CAN_REL_MOVE);

	// set default values
	dir = FOCUS_OUTWARD;
	step_index = 0;

	// load options
	defineText(&PortTP);
	defineSwitch(&MotorDirSP);
	defineNumber(&MotorDelayNP);
//	defineNumber(&FocusBacklashNP);

    return true;
}

void IndiPiFaceFocuser1::ISGetProperties (const char *dev)
{
	if(dev && strcmp(dev,getDeviceName()))
		return;

	INDI::Focuser::ISGetProperties(dev);

    // addDebugControl();
    return;
}

bool IndiPiFaceFocuser1::updateProperties()
{

    INDI::Focuser::updateProperties();

    if (isConnected())
    {
		defineNumber(&FocusAbsPosNP);
		defineNumber(&FocusRelPosNP);
		defineSwitch(&FocusMotionSP);
		defineSwitch(&FocusParkingSP);
		defineSwitch(&FocusResetSP);
    }
    else
    {
		deleteProperty(FocusAbsPosNP.name);
		deleteProperty(FocusRelPosNP.name);
		deleteProperty(FocusMotionSP.name);
		deleteProperty(FocusParkingSP.name);
		deleteProperty(FocusResetSP.name);
    }

    return true;
}

bool IndiPiFaceFocuser1::ISNewNumber (const char *dev, const char *name, double values[], char *names[], int n)
{
	// first we check if it's for our device
	if(strcmp(dev,getDeviceName())==0)
	{

        // handle focus absolute position
        if (!strcmp(name, FocusAbsPosNP.name))
        {
			int newPos = (int) values[0];
            if ( MoveAbsFocuser(newPos) == IPS_OK )
            {
               IUUpdateNumber(&FocusAbsPosNP,values,names,n);
               FocusAbsPosNP.s=IPS_OK;
               IDSetNumber(&FocusAbsPosNP, NULL);
            }
            return true;
        }

        // handle focus relative position
        if (!strcmp(name, FocusRelPosNP.name))
        {
			IUUpdateNumber(&FocusRelPosNP,values,names,n);
			FocusRelPosNP.s=IPS_OK;
			IDSetNumber(&FocusRelPosNP, NULL);

			//FOCUS_INWARD
            if ( FocusMotionS[0].s == ISS_ON )
				MoveRelFocuser(FOCUS_INWARD, FocusRelPosN[0].value);

			//FOCUS_OUTWARD
            if ( FocusMotionS[1].s == ISS_ON )
				MoveRelFocuser(FOCUS_OUTWARD, FocusRelPosN[0].value);

			return true;
        }

        // handle step delay
        if (!strcmp(name, MotorDelayNP.name))
        {
            IUUpdateNumber(&MotorDelayNP,values,names,n);
            MotorDelayNP.s=IPS_OK;
            IDSetNumber(&MotorDelayNP, "PiFace Focuser 1 step delay set to %d milliseconds", (int) MotorDelayN[0].value);
            return true;
        }

        // handle focus backlash
        if (!strcmp(name, FocusBacklashNP.name))
        {
            IUUpdateNumber(&FocusBacklashNP,values,names,n);
            FocusBacklashNP.s=IPS_OK;
            IDSetNumber(&FocusBacklashNP, "PiFace Focuser 1 backlash set to %d steps", (int) FocusBacklashN[0].value);
            return true;
        }

    }
    return INDI::Focuser::ISNewNumber(dev,name,values,names,n);
}

bool IndiPiFaceFocuser1::ISNewSwitch (const char *dev, const char *name, ISState *states, char *names[], int n)
{
	// first we check if it's for our device
    if (!strcmp(dev, getDeviceName()))
    {
        // handle focus presets
        if (!strcmp(name, PresetGotoSP.name))
        {
            IUUpdateSwitch(&PresetGotoSP, states, names, n);

			//Preset 1
            if ( PresetGotoS[0].s == ISS_ON )
				MoveAbsFocuser(PresetN[0].value);

			//Preset 2
            if ( PresetGotoS[1].s == ISS_ON )
				MoveAbsFocuser(PresetN[1].value);

			//Preset 2
            if ( PresetGotoS[2].s == ISS_ON )
				MoveAbsFocuser(PresetN[2].value);

			PresetGotoS[0].s = ISS_OFF;
			PresetGotoS[1].s = ISS_OFF;
			PresetGotoS[2].s = ISS_OFF;
			PresetGotoSP.s = IPS_OK;
            IDSetSwitch(&PresetGotoSP, NULL);
            return true;
        }

        // handle focus reset
        if(!strcmp(name, FocusResetSP.name))
        {
			IUUpdateSwitch(&FocusResetSP, states, names, n);

            if ( FocusResetS[0].s == ISS_ON && FocusAbsPosN[0].value == FocusAbsPosN[0].min  )
            {
				FocusAbsPosN[0].value = (int)MAX_STEPS/100;
				IDSetNumber(&FocusAbsPosNP, NULL);
				MoveAbsFocuser(0);
			}
            FocusResetS[0].s = ISS_OFF;
            IDSetSwitch(&FocusResetSP, NULL);
			return true;
		}

        // handle parking mode
        if(!strcmp(name, FocusParkingSP.name))
        {
			IUUpdateSwitch(&FocusParkingSP, states, names, n);
			FocusParkingSP.s = IPS_BUSY;
			IDSetSwitch(&FocusParkingSP, NULL);

			FocusParkingSP.s = IPS_OK;
			IDSetSwitch(&FocusParkingSP, NULL);
			return true;
		}

        // handle motor direction
        if(!strcmp(name, MotorDirSP.name))
        {
			IUUpdateSwitch(&MotorDirSP, states, names, n);
			MotorDirSP.s = IPS_BUSY;
			IDSetSwitch(&MotorDirSP, NULL);

			MotorDirSP.s = IPS_OK;
			IDSetSwitch(&MotorDirSP, NULL);
			return true;
		}

        // handle focus abort - TODO
/*
        if (!strcmp(name, AbortSP.name))
        {
            IUUpdateSwitch(&AbortSP, states, names, n);
            if ( AbortFocuser() )
            {
				//FocusAbsPosNP.s = IPS_IDLE;
				//IDSetNumber(&FocusAbsPosNP, NULL);
				AbortS[0].s = ISS_OFF;
				AbortSP.s = IPS_OK;
			}
			else
			{
				AbortSP.s = IPS_ALERT;
			}

			IDSetSwitch(&AbortSP, NULL);
            return true;
        }
*/
    }
    return INDI::Focuser::ISNewSwitch(dev,name,states,names,n);
}
bool IndiPiFaceFocuser1::saveConfigItems(FILE *fp)
{
	IUSaveConfigNumber(fp, &PresetNP);
	IUSaveConfigNumber(fp, &MotorDelayNP);
	IUSaveConfigNumber(fp, &FocusBacklashNP);
	IUSaveConfigSwitch(fp, &FocusParkingSP);
	IUSaveConfigSwitch(fp, &MotorDirSP);

	if ( FocusParkingS[0].s == ISS_ON )
		IUSaveConfigNumber(fp, &FocusAbsPosNP);

	return true;
}

IPState IndiPiFaceFocuser1::MoveFocuser(FocusDirection direction, int speed, int duration)
{
	int ticks = (int) ( duration / MotorDelayN[0].value );
	return 	MoveRelFocuser(direction, ticks);
}


IPState IndiPiFaceFocuser1::MoveRelFocuser(FocusDirection direction, int ticks)
{
	int targetTicks = FocusAbsPosN[0].value + (ticks * (direction == FOCUS_INWARD ? -1 : 1));
	return MoveAbsFocuser(targetTicks);
}

IPState IndiPiFaceFocuser1::MoveAbsFocuser(int targetTicks)
{
    if (targetTicks < FocusAbsPosN[0].min || targetTicks > FocusAbsPosN[0].max)
    {
        IDMessage(getDeviceName(), "Requested position is out of range.");
        return IPS_ALERT;
    }

    if (targetTicks == FocusAbsPosN[0].value)
    {
        // IDMessage(getDeviceName(), "PiFace Focuser 1 already in the requested position.");
        return IPS_OK;
    }

	// set focuser busy
	FocusAbsPosNP.s = IPS_BUSY;
	IDSetNumber(&FocusAbsPosNP, NULL);

	// check last motion direction for backlash triggering
	FocusDirection lastdir = dir;

    // set direction
    if (targetTicks > FocusAbsPosN[0].value)
    {
		dir = FOCUS_OUTWARD;
		IDMessage(getDeviceName() , "PiFace Focuser 1 is moving outward by %d", abs(targetTicks - FocusAbsPosN[0].value));
    }
    else
    {
		dir = FOCUS_INWARD;
		IDMessage(getDeviceName() , "PiFace Focuser 1 is moving inward by %d", abs(targetTicks - FocusAbsPosN[0].value));
    }

	// if direction changed do backlash adjustment - TO DO
	if ( lastdir != dir && FocusAbsPosN[0].value != 0 && FocusBacklashN[0].value != 0 )
	{
		IDMessage(getDeviceName() , "PiFace Focuser 1 backlash compensation by %0.0f steps...", FocusBacklashN[0].value);
		StepperMotor(FocusBacklashN[0].value, dir);
	}

	// process targetTicks
	int ticks = abs(targetTicks - FocusAbsPosN[0].value);

	// GO
	StepperMotor(ticks, dir);

	// update abspos value and status
	IDSetNumber(&FocusAbsPosNP, "PiFace Focuser 1 moved to position %0.0f", FocusAbsPosN[0].value );
	FocusAbsPosNP.s = IPS_OK;
	IDSetNumber(&FocusAbsPosNP, NULL);

    return IPS_OK;
}

int IndiPiFaceFocuser1::StepperMotor(int steps, FocusDirection direction)
{
    int step_states[8];
    int value;
    int payload = 0x00;

	if(direction == FOCUS_OUTWARD)
	{
		if ( MotorDirS[1].s == ISS_ON )
		{
			//clockwise out
			int step_states[8] = {0xa, 0x2, 0x6, 0x4, 0x5, 0x1, 0x9, 0x8};
		}
		else
		{
			//clockwise in
			int step_states[8] = {0x8, 0x9, 0x1, 0x5, 0x4, 0x6, 0x2, 0xa};
		}
	}
	else
	{
		if ( MotorDirS[1].s == ISS_ON )
		{
			//clockwise in
			int step_states[8] = {0x8, 0x9, 0x1, 0x5, 0x4, 0x6, 0x2, 0xa};
		}
		else
		{
			//clockwise out
			int step_states[8] = {0xa, 0x2, 0x6, 0x4, 0x5, 0x1, 0x9, 0x8};
		}
	}

    for (int i = 0; i < steps; i++)
    {
	// update position for a client

        if ( dir == FOCUS_INWARD )
        	FocusAbsPosN[0].value -= 1;
        if ( dir == FOCUS_OUTWARD )
        	FocusAbsPosN[0].value += 1;
        IDSetNumber(&FocusAbsPosNP, NULL);

		if(step_index == (sizeof(step_states)/sizeof(int)))
			step_index = 0;
		value = step_states[step_index];
		step_index++;

		// GPIOB lower nibble, polarity reversed
		payload = payload & 0xf0;
		payload = payload | ((value & 0xf) ^ 0xf);

		// make step
		mcp23s17_write_reg(payload, GPIOB, 0, mcp23s17_fd);
		usleep(MotorDelayN[0].value * 1000);
		}

		// Coast motors
		mcp23s17_write_reg(0x00, GPIOB, 0, mcp23s17_fd);

		return 0;
}
bool IndiPiFaceFocuser1::AbortFocuser()
{
	IDMessage(getDeviceName() , "PiFace Focuser 1 aborted");

	// Brake
	mcp23s17_write_reg(0xff, GPIOB, 0, mcp23s17_fd);

	// Cost
	mcp23s17_write_reg(0x00, GPIOB, 0, mcp23s17_fd);

	return true;
}

/************************************************************************************
*
*               Seconf Focuser (IndiPiFaceFocuser2)
*
*************************************************************************************/

IndiPiFaceFocuser2::IndiPiFaceFocuser2()
{
	setVersion(1,0);
}

IndiPiFaceFocuser2::~IndiPiFaceFocuser2()
{

}

const char * IndiPiFaceFocuser2::getDefaultName()
{
	return FOCUSNAMEF2;
}

bool IndiPiFaceFocuser2::Connect()
{
	// open device
	mcp23s17_fd = mcp23s17_open(0,0);

	if(mcp23s17_fd == -1)
	{
		IDMessage(getDeviceName(), "PiFace Focuser 2 device is not available.");
		return false;
	}

	// config register
	const uint8_t ioconfig = BANK_OFF | \
                             INT_MIRROR_OFF | \
                             SEQOP_OFF | \
                             DISSLW_OFF | \
                             HAEN_ON | \
                             ODR_OFF | \
                             INTPOL_LOW;
	mcp23s17_write_reg(ioconfig, IOCON, 0, mcp23s17_fd);

	// I/O direction
	mcp23s17_write_reg(0x00, IODIRA, 0, mcp23s17_fd);

	// pull ups
	mcp23s17_write_reg(0x00, GPPUA, 0, mcp23s17_fd);

	IDMessage(getDeviceName(), "PiFace Focuser 2 connected successfully.");
	return true;
}

bool IndiPiFaceFocuser2::Disconnect()
{
	// park focuser
	if ( FocusParkingS[0].s == ISS_ON )
	{
		IDMessage(getDeviceName(), "PiFace Focuser 2 is parking...");
		MoveAbsFocuser(FocusAbsPosN[0].min);
	}

	// close device
	close(mcp23s17_fd);

	IDMessage(getDeviceName(), "PiFace Focuser 2 disconnected successfully.");
	return true;
}

bool IndiPiFaceFocuser2::initProperties()
{
    INDI::Focuser::initProperties();

	// options tab
	IUFillText(&PortT[0], "PORT", "Port","PiFace Relay+");
	IUFillTextVector(&PortTP,PortT,1,getDeviceName(),"DEVICE_PORT","Ports",OPTIONS_TAB,IP_RO,0,IPS_OK);

	IUFillNumber(&MotorDelayN[0],"MOTOR_DELAY","milliseconds","%0.0f",1,100,1,2);
	IUFillNumberVector(&MotorDelayNP,MotorDelayN,1,getDeviceName(),"MOTOR_CONFIG","Step Delay",OPTIONS_TAB,IP_RW,0,IPS_OK);

	IUFillSwitch(&MotorDirS[0],"FORWARD","Normal",ISS_ON);
	IUFillSwitch(&MotorDirS[1],"REVERSE","Reverse",ISS_OFF);
	IUFillSwitchVector(&MotorDirSP,MotorDirS,2,getDeviceName(),"MOTOR_DIR","Motor Dir",OPTIONS_TAB,IP_RW,ISR_1OFMANY,60,IPS_OK);

	IUFillNumber(&FocusBacklashN[0], "FOCUS_BACKLASH_VALUE", "Steps", "%0.0f", 0, 100, 1, 0);
	IUFillNumberVector(&FocusBacklashNP, FocusBacklashN, 1, getDeviceName(), "FOCUS_BACKLASH", "Backlash", OPTIONS_TAB, IP_RW, 0, IPS_IDLE);

	IUFillSwitch(&FocusParkingS[0],"FOCUS_PARKON","Enable",ISS_ON);
	IUFillSwitch(&FocusParkingS[1],"FOCUS_PARKOFF","Disable",ISS_OFF);
	IUFillSwitchVector(&FocusParkingSP,FocusParkingS,2,getDeviceName(),"FOCUS_PARK","Parking",OPTIONS_TAB,IP_RW,ISR_1OFMANY,60,IPS_OK);

	IUFillSwitch(&FocusResetS[0],"FOCUS_RESET","Reset",ISS_OFF);
	IUFillSwitchVector(&FocusResetSP,FocusResetS,1,getDeviceName(),"FOCUS_RESET","Position Reset",OPTIONS_TAB,IP_RW,ISR_1OFMANY,60,IPS_OK);

	// main tab
	IUFillSwitch(&FocusMotionS[0],"FOCUS_INWARD","Focus In",ISS_OFF);
	IUFillSwitch(&FocusMotionS[1],"FOCUS_OUTWARD","Focus Out",ISS_ON);
	IUFillSwitchVector(&FocusMotionSP,FocusMotionS,2,getDeviceName(),"FOCUS_MOTION","Direction",MAIN_CONTROL_TAB,IP_RW,ISR_ATMOST1,60,IPS_OK);

	IUFillNumber(&FocusRelPosN[0],"FOCUS_RELATIVE_POSITION","Steps","%0.0f",0,(int)MAX_STEPS/10,(int)MAX_STEPS/100,(int)MAX_STEPS/100);
	IUFillNumberVector(&FocusRelPosNP,FocusRelPosN,1,getDeviceName(),"REL_FOCUS_POSITION","Relative",MAIN_CONTROL_TAB,IP_RW,60,IPS_OK);

	IUFillNumber(&FocusAbsPosN[0],"FOCUS_ABSOLUTE_POSITION","Steps","%0.0f",0,MAX_STEPS,(int)MAX_STEPS/100,0);
	IUFillNumberVector(&FocusAbsPosNP,FocusAbsPosN,1,getDeviceName(),"ABS_FOCUS_POSITION","Absolute",MAIN_CONTROL_TAB,IP_RW,0,IPS_OK);

	IUFillNumber(&PresetN[0], "Preset 1", "", "%0.0f", 0, MAX_STEPS, (int)(MAX_STEPS/100), 0);
	IUFillNumber(&PresetN[1], "Preset 2", "", "%0.0f", 0, MAX_STEPS, (int)(MAX_STEPS/100), 0);
	IUFillNumber(&PresetN[2], "Preset 3", "", "%0.0f", 0, MAX_STEPS, (int)(MAX_STEPS/100), 0);
	IUFillNumberVector(&PresetNP, PresetN, 3, getDeviceName(), "Presets", "Presets", "Presets", IP_RW, 0, IPS_IDLE);

	IUFillSwitch(&PresetGotoS[0], "Preset 1", "Preset 1", ISS_OFF);
	IUFillSwitch(&PresetGotoS[1], "Preset 2", "Preset 2", ISS_OFF);
	IUFillSwitch(&PresetGotoS[2], "Preset 3", "Preset 3", ISS_OFF);
	IUFillSwitchVector(&PresetGotoSP, PresetGotoS, 3, getDeviceName(), "Presets Goto", "Goto", MAIN_CONTROL_TAB,IP_RW,ISR_1OFMANY,60,IPS_OK);

	// set capabilities
	SetFocuserCapability(FOCUSER_CAN_ABS_MOVE | FOCUSER_CAN_REL_MOVE);

	// set default values
	dir = FOCUS_OUTWARD;
	step_index = 0;

	// load options
	defineText(&PortTP);
	defineSwitch(&MotorDirSP);
	defineNumber(&MotorDelayNP);
//	defineNumber(&FocusBacklashNP);

    return true;
}

void IndiPiFaceFocuser2::ISGetProperties (const char *dev)
{
	if(dev && strcmp(dev,getDeviceName()))
		return;

    INDI::Focuser::ISGetProperties(dev);

    // addDebugControl();
    return;
}

bool IndiPiFaceFocuser2::updateProperties()
{

    INDI::Focuser::updateProperties();

    if (isConnected())
    {
		defineNumber(&FocusAbsPosNP);
		defineNumber(&FocusRelPosNP);
		defineSwitch(&FocusMotionSP);
		defineSwitch(&FocusParkingSP);
		defineSwitch(&FocusResetSP);
    }
    else
    {
		deleteProperty(FocusAbsPosNP.name);
		deleteProperty(FocusRelPosNP.name);
		deleteProperty(FocusMotionSP.name);
		deleteProperty(FocusParkingSP.name);
		deleteProperty(FocusResetSP.name);
    }

    return true;
}

bool IndiPiFaceFocuser2::ISNewNumber (const char *dev, const char *name, double values[], char *names[], int n)
{
	// first we check if it's for our device
	if(strcmp(dev,getDeviceName())==0)
	{
        // handle focus absolute position
        if (!strcmp(name, FocusAbsPosNP.name))
        {
			int newPos = (int) values[0];
            if ( MoveAbsFocuser(newPos) == IPS_OK )
            {
               IUUpdateNumber(&FocusAbsPosNP,values,names,n);
               FocusAbsPosNP.s=IPS_OK;
               IDSetNumber(&FocusAbsPosNP, NULL);
            }
            return true;
        }

        // handle focus relative position
        if (!strcmp(name, FocusRelPosNP.name))
        {
			IUUpdateNumber(&FocusRelPosNP,values,names,n);
			FocusRelPosNP.s=IPS_OK;
			IDSetNumber(&FocusRelPosNP, NULL);

			//FOCUS_INWARD
            if ( FocusMotionS[0].s == ISS_ON )
				MoveRelFocuser(FOCUS_INWARD, FocusRelPosN[0].value);

			//FOCUS_OUTWARD
            if ( FocusMotionS[1].s == ISS_ON )
				MoveRelFocuser(FOCUS_OUTWARD, FocusRelPosN[0].value);

			return true;
        }

        // handle step delay
        if (!strcmp(name, MotorDelayNP.name))
        {
            IUUpdateNumber(&MotorDelayNP,values,names,n);
            MotorDelayNP.s=IPS_OK;
            IDSetNumber(&MotorDelayNP, "PiFace Focuser 2 step delay set to %d milliseconds", (int) MotorDelayN[0].value);
            return true;
        }

        // handle focus backlash
        if (!strcmp(name, FocusBacklashNP.name))
        {
            IUUpdateNumber(&FocusBacklashNP,values,names,n);
            FocusBacklashNP.s=IPS_OK;
            IDSetNumber(&FocusBacklashNP, "PiFace Focuser 2 backlash set to %d steps", (int) FocusBacklashN[0].value);
            return true;
        }

    }
    return INDI::Focuser::ISNewNumber(dev,name,values,names,n);
}

bool IndiPiFaceFocuser2::ISNewSwitch (const char *dev, const char *name, ISState *states, char *names[], int n)
{
	// first we check if it's for our device
    if (!strcmp(dev, getDeviceName()))
    {
        // handle focus presets
        if (!strcmp(name, PresetGotoSP.name))
        {
            IUUpdateSwitch(&PresetGotoSP, states, names, n);

			//Preset 1
            if ( PresetGotoS[0].s == ISS_ON )
				MoveAbsFocuser(PresetN[0].value);

			//Preset 2
            if ( PresetGotoS[1].s == ISS_ON )
				MoveAbsFocuser(PresetN[1].value);

			//Preset 2
            if ( PresetGotoS[2].s == ISS_ON )
				MoveAbsFocuser(PresetN[2].value);

			PresetGotoS[0].s = ISS_OFF;
			PresetGotoS[1].s = ISS_OFF;
			PresetGotoS[2].s = ISS_OFF;
			PresetGotoSP.s = IPS_OK;
            IDSetSwitch(&PresetGotoSP, NULL);
            return true;
        }

        // handle focus reset
        if(!strcmp(name, FocusResetSP.name))
        {
			IUUpdateSwitch(&FocusResetSP, states, names, n);

            if ( FocusResetS[0].s == ISS_ON && FocusAbsPosN[0].value == FocusAbsPosN[0].min  )
            {
				FocusAbsPosN[0].value = (int)MAX_STEPS/100;
				IDSetNumber(&FocusAbsPosNP, NULL);
				MoveAbsFocuser(0);
			}
            FocusResetS[0].s = ISS_OFF;
            IDSetSwitch(&FocusResetSP, NULL);
			return true;
		}

        // handle parking mode
        if(!strcmp(name, FocusParkingSP.name))
        {
			IUUpdateSwitch(&FocusParkingSP, states, names, n);
			FocusParkingSP.s = IPS_BUSY;
			IDSetSwitch(&FocusParkingSP, NULL);

			FocusParkingSP.s = IPS_OK;
			IDSetSwitch(&FocusParkingSP, NULL);
			return true;
		}

        // handle motor direction
        if(!strcmp(name, MotorDirSP.name))
        {
			IUUpdateSwitch(&MotorDirSP, states, names, n);
			MotorDirSP.s = IPS_BUSY;
			IDSetSwitch(&MotorDirSP, NULL);

			MotorDirSP.s = IPS_OK;
			IDSetSwitch(&MotorDirSP, NULL);
			return true;
		}

        // handle focus abort - TODO
/*
        if (!strcmp(name, AbortSP.name))
        {
            IUUpdateSwitch(&AbortSP, states, names, n);
            if ( AbortFocuser() )
            {
				//FocusAbsPosNP.s = IPS_IDLE;
				//IDSetNumber(&FocusAbsPosNP, NULL);
				AbortS[0].s = ISS_OFF;
				AbortSP.s = IPS_OK;
			}
			else
			{
				AbortSP.s = IPS_ALERT;
			}

			IDSetSwitch(&AbortSP, NULL);
            return true;
        }
*/
    }
    return INDI::Focuser::ISNewSwitch(dev,name,states,names,n);
}
bool IndiPiFaceFocuser2::saveConfigItems(FILE *fp)
{
	IUSaveConfigNumber(fp, &PresetNP);
	IUSaveConfigNumber(fp, &MotorDelayNP);
	IUSaveConfigNumber(fp, &FocusBacklashNP);
	IUSaveConfigSwitch(fp, &FocusParkingSP);
	IUSaveConfigSwitch(fp, &MotorDirSP);

	if ( FocusParkingS[0].s == ISS_ON )
		IUSaveConfigNumber(fp, &FocusAbsPosNP);

	return true;
}

IPState IndiPiFaceFocuser2::MoveFocuser(FocusDirection direction, int speed, int duration)
{
	int ticks = (int) ( duration / MotorDelayN[0].value );
	return 	MoveRelFocuser(direction, ticks);
}


IPState IndiPiFaceFocuser2::MoveRelFocuser(FocusDirection direction, int ticks)
{
	int targetTicks = FocusAbsPosN[0].value + (ticks * (direction == FOCUS_INWARD ? -1 : 1));
	return MoveAbsFocuser(targetTicks);
}

IPState IndiPiFaceFocuser2::MoveAbsFocuser(int targetTicks)
{
    if (targetTicks < FocusAbsPosN[0].min || targetTicks > FocusAbsPosN[0].max)
    {
        IDMessage(getDeviceName(), "Requested position is out of range.");
        return IPS_ALERT;
    }

    if (targetTicks == FocusAbsPosN[0].value)
    {
        // IDMessage(getDeviceName(), "PiFace Focuser 2 already in the requested position.");
        return IPS_OK;
    }

	// set focuser busy
	FocusAbsPosNP.s = IPS_BUSY;
	IDSetNumber(&FocusAbsPosNP, NULL);

	// check last motion direction for backlash triggering
	FocusDirection lastdir = dir;

    // set direction
    if (targetTicks > FocusAbsPosN[0].value)
    {
		dir = FOCUS_OUTWARD;
		IDMessage(getDeviceName() , "PiFace Focuser 2 is moving outward by %d", abs(targetTicks - FocusAbsPosN[0].value));
    }
    else
    {
		dir = FOCUS_INWARD;
		IDMessage(getDeviceName() , "PiFace Focuser 2 is moving inward by %d", abs(targetTicks - FocusAbsPosN[0].value));
    }

	// if direction changed do backlash adjustment - TO DO
	if ( lastdir != dir && FocusAbsPosN[0].value != 0 && FocusBacklashN[0].value != 0 )
	{
		IDMessage(getDeviceName() , "PiFace Focuser 2 backlash compensation by %0.0f steps...", FocusBacklashN[0].value);
		StepperMotor(FocusBacklashN[0].value, dir);
	}

	// process targetTicks
	int ticks = abs(targetTicks - FocusAbsPosN[0].value);

	// GO
	StepperMotor(ticks, dir);

	// update abspos value and status
	IDSetNumber(&FocusAbsPosNP, "PiFace Focuser 2 moved to position %0.0f", FocusAbsPosN[0].value );
	FocusAbsPosNP.s = IPS_OK;
	IDSetNumber(&FocusAbsPosNP, NULL);

    return IPS_OK;
}

int IndiPiFaceFocuser2::StepperMotor(int steps, FocusDirection direction)
{
    int step_states[8];
    int value;
    int payload = 0x00;

	if(direction == FOCUS_OUTWARD)
	{
		if ( MotorDirS[1].s == ISS_ON )
		{
			//clockwise out
			int step_states[8] = {0xa, 0x2, 0x6, 0x4, 0x5, 0x1, 0x9, 0x8};
		}
		else
		{
			//clockwise in
			int step_states[8] = {0x8, 0x9, 0x1, 0x5, 0x4, 0x6, 0x2, 0xa};
		}
	}
	else
	{
		if ( MotorDirS[1].s == ISS_ON )
		{
			//clockwise in
			int step_states[8] = {0x8, 0x9, 0x1, 0x5, 0x4, 0x6, 0x2, 0xa};
		}
		else
		{
			//clockwise out
			int step_states[8] = {0xa, 0x2, 0x6, 0x4, 0x5, 0x1, 0x9, 0x8};
		}
	}

    for (int i = 0; i < steps; i++)
    {
	// update position for a client

        if ( dir == FOCUS_INWARD )
        	FocusAbsPosN[0].value -= 1;
        if ( dir == FOCUS_OUTWARD )
        	FocusAbsPosN[0].value += 1;
        IDSetNumber(&FocusAbsPosNP, NULL);

		if(step_index == (sizeof(step_states)/sizeof(int)))
			step_index = 0;
		value = step_states[step_index];
		step_index++;

		// GPIOB upper nibble, polarity reversed
		payload = payload & 0x0f;
		payload = payload | ((value & 0xf) << 4);

		// make step
		mcp23s17_write_reg(payload, GPIOA, 0, mcp23s17_fd);
		usleep(MotorDelayN[0].value * 1000);
		}

		// Coast motor
		mcp23s17_write_reg(0x00, GPIOA, 0, mcp23s17_fd);

		return 0;
}
bool IndiPiFaceFocuser2::AbortFocuser()
{
	IDMessage(getDeviceName() , "PiFace Focuser 2 aborted");

	// Brake
	mcp23s17_write_reg(0xff, GPIOA, 0, mcp23s17_fd);

	// Cost
	mcp23s17_write_reg(0x00, GPIOA, 0, mcp23s17_fd);

	return true;
}
