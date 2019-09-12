#include "fademo_hooks.h"
#include <sstream>
#include <fstream>
#include <iostream>
#include <string>
//#include <iss/cmd_tokens.h>
#include <pstream/pstream.h>
#include <math.h>

#define __DEBUG__ true 
#define supwd typ-dx

/*
Equipment Sets Wafer Info	FALSE	Prober sets Wafer Descriptor information, Unison only.
Equipment Sets Center Die	FALSE	Prober sets Wafer Descriptor information, unison only.
Equipment Sets Reference Die	FALSE	Prober sets Wafer Descriptor information, Unison only.
Equipment Sets First Die	FALSE	Prober sets Wafer Descriptor information, Unison only. 
*/
CFademoHooks::CFademoHooks(CfaModuleInterface& cCtrl, std::string& errStr, FA_LONG& errCode, const std::string& name,const std::string& versionStr,
				FA_LONG versionNum) : CFaModule(cCtrl, errStr, errCode, name, versionStr, versionNum)
{

        m_str2flow["EVOF_ON_START"] = EVOF_ON_START;
        m_str2flow["EVOF_ON_RESTART"] = EVOF_ON_RESTART;
        m_str2flow["EVOF_ON_LOAD"] = EVOF_ON_LOAD;
        m_str2flow["EVOF_ON_UNLOAD"] = EVOF_ON_UNLOAD;
        m_str2flow["EVOF_ON_RESET"] = EVOF_ON_RESET;
        m_str2flow["EVOF_ON_RUNTIME_ERROR"] = EVOF_ON_RUNTIME_ERROR;
        m_str2flow["EVOF_ON_HALT"] = EVOF_ON_HALT;
        m_str2flow["EVOF_ON_FAULT"] = EVOF_ON_FAULT;
        m_str2flow["EVOF_ON_POWERDOWN"] = EVOF_ON_POWERDOWN;
        m_str2flow["EVOF_ON_BEGIN_LOT"] = EVOF_ON_BEGIN_LOT;
        m_str2flow["EVOF_ON_END_LOT"] = EVOF_ON_END_LOT;
        m_str2flow["EVOF_ON_DEBUG"] = EVOF_ON_DEBUG;
        m_str2flow["EVOF_ON_GPIB_SRQ"] = EVOF_ON_GPIB_SRQ;
        m_str2flow["EVOF_ON_INIT_FLOW"] = EVOF_ON_INIT_FLOW;
        m_str2flow["EVOF_ON_AFTER_BIN"] = EVOF_ON_AFTER_BIN;
        m_str2flow["EVOF_START_OF_WAFER"] = EVOF_START_OF_WAFER;
        m_str2flow["EVOF_END_OF_WAFER"] = EVOF_END_OF_WAFER;
        m_str2flow["EVOF_BIN_OVERFLOW"] = EVOF_BIN_OVERFLOW ;
        m_str2flow["EVOF_SUSPEND"] = EVOF_SUSPEND;
        m_str2flow["EVOF_RESUME"] = EVOF_RESUME;
        m_str2flow["EVOF_USR_CAL"] = EVOF_USR_CAL;
        m_str2flow["EVOF_USR_DEF_0"] = EVOF_USR_DEF_0;
        m_str2flow["EVOF_USR_DEF_1"] = EVOF_USR_DEF_1;
        m_str2flow["EVOF_USR_DEF_2"] = EVOF_USR_DEF_2;
        m_str2flow["EVOF_USR_DEF_3"] = EVOF_USR_DEF_3;
        m_str2flow["EVOF_USR_DEF_4"] = EVOF_USR_DEF_4;
        m_str2flow["EVOF_USR_DEF_5"] = EVOF_USR_DEF_5;
        m_str2flow["EVOF_USR_DEF_6"] = EVOF_USR_DEF_6;
        m_str2flow["EVOF_USR_DEF_7"] = EVOF_USR_DEF_7;
        m_str2flow["EVOF_USR_DEF_8"] = EVOF_USR_DEF_8;
        m_str2flow["EVOF_USR_DEF_9"] = EVOF_USR_DEF_9;

        cCtrl.invoke("enable event",  "group|state change|poll|on_command|user_errors|validate equipment name|exit events|alarms|log communications");

	ctrl().programCtrl().writeln(STDOUT, "<UserHooks> hook: CFademoHooks() - %s\n", "CURI UserHook Version for APL Debug");
}

CFademoHooks::~CFademoHooks()
{
}

bool CFademoHooks::equipmentStatusChange(const std::string &equipmentID, bool loadStatus, bool mappingEquipment, const _TIMEVAL_ &startTime, const _TIMEVAL_ &runTime)
{
	ctrl().debugStream("<UserHooks>: equipmentStatusChange", true);
	ctrl().programCtrl().writeln(STDOUT, "<UserHooks> hook: equipmentStatusChange() in %s mode\n", __DEBUG__? "Debug": "Release" );
	
	return true;
}

/*----------------------------------------------------------------------------------------------------------------------------------
handle lot start event in FAmodule
----------------------------------------------------------------------------------------------------------------------------------*/
bool CFademoHooks::lotStart(std::string &lotID, const _TIMEVAL_ &startTime, const _TIMEVAL_ &runTime)
{
	ctrl().programCtrl().writeln(STDOUT, "<UserHooks> hook: lotStart() - %s\n", lotID.c_str());
	return true;
}

/*----------------------------------------------------------------------------------------------------------------------------------
handle sublot start event in FAmodule (bugged, it never gets fired up)
----------------------------------------------------------------------------------------------------------------------------------*/
bool CFademoHooks::subLotStart(std::string &subLotID, const _TIMEVAL_ &startTime, const _TIMEVAL_ &runTime)
{
	ctrl().programCtrl().writeln(STDOUT, "<UserHooks> hook: subLotStart()\n");
	return true;
}

/*----------------------------------------------------------------------------------------------------------------------------------
"ur8D4F", "ur2A9B8"
7.110384 x urFF938110
6.119778 y urFFA29E9E
----------------------------------------------------------------------------------------------------------------------------------*/
float StringHexToFloat(const std::string& hex)
{
	std::string fstr;
	long f = 0;
	
	// check if string contains "ur". remove it.
	std::size_t ur = hex.find("ur");
	if (ur != std::string::npos) fstr = hex.substr(ur + 2);
	else fstr = hex;

	

	// loop through each char in string starting at the end (LSB)
	unsigned int exp = 0;
	for (std::string::reverse_iterator it = fstr.rbegin(); it != fstr.rend(); ++it, exp+=4)
	{
		// calculate exponent value
		float factor = pow(2.0, exp);
		
		// get corresponding value for the current hex
		switch (*it)
		{
			case '0': f += (long)( 0.0 * factor ); break;
			case '1': f += (long)( 1.0 * factor ); break;
			case '2': f += (long)( 2.0 * factor ); break;
			case '3': f += (long)( 3.0 * factor ); break;
			case '4': f += (long)( 4.0 * factor ); break;
			case '5': f += (long)( 5.0 * factor ); break;
			case '6': f += (long)( 6.0 * factor ); break;
			case '7': f += (long)( 7.0 * factor ); break;
			case '8': f += (long)( 8.0 * factor ); break;
			case '9': f += (long)( 9.0 * factor ); break;
			case 'A': f += (long)( 10.0 * factor ); break;
			case 'B': f += (long)( 11.0 * factor ); break;
			case 'C': f += (long)( 12.0 * factor ); break;
			case 'D': f += (long)( 13.0 * factor ); break;
			case 'E': f += (long)( 14.0 * factor ); break;
			case 'F': f += (long)( 15.0 * factor ); break;
		}					
	}

	// do we need 2's complement?
	if (f > 0x7fffffff)
	{
		f = 0xffffffff - f + 1;
		f *= (-1.0f);
	}
	float ff = (float)f;

	return ff;
}

/*----------------------------------------------------------------------------------------------------------------------------------
handle wafer start event in FAmodule
----------------------------------------------------------------------------------------------------------------------------------*/
bool CFademoHooks::mapStart(std::string &mapID, FA_LONG &mapNumber, const _TIMEVAL_ &startTime, const _TIMEVAL_ &runTime)
{
	ctrl().programCtrl().writeln(STDOUT, "<UserHooks> hook: mapStart()\n");

	mapID = ctrl().get("Current Equipment: WAFERID_FROM_SPEC");

	return true;
}


bool CFademoHooks::mapEnd(const std::string &mapID, const FA_LONG mapNumber, const _TIMEVAL_ &endTime, const _TIMEVAL_ &runTime)
{
	ctrl().programCtrl().writeln(STDOUT, "<UserHooks> hook: mapEnd()\n");
	return true;
}

bool CFademoHooks::subLotEnd(const std::string &subLotID, const _TIMEVAL_ &endTime, const _TIMEVAL_ &runTime)
{
	ctrl().programCtrl().writeln(STDOUT, "<UserHooks> hook: subLotEnd()\n");
	return true;
}

bool CFademoHooks::lotEnd(const std::string &lotID, const _TIMEVAL_ &endTime, const _TIMEVAL_ &runTime)
{
	ctrl().programCtrl().writeln(STDOUT, "fademo hook: lotEnd()\n");
	return true;
}

bool CFademoHooks::deviceStart(FA_LONG site, FA_LONG &deviceNumber, bool isMap, const mapInfo &currentMapInfo, bool &isRetest, const _TIMEVAL_ &startTime, const _TIMEVAL_ &runTime)
{
	//ctrl().debugStream("UserHooks: deviceStart", true);
	return true;
}

bool CFademoHooks::deviceEnd(FA_LONG deviceNumber, binInfo &currentBinInfo, bool isMap, const mapInfo &currentMapInfo, const _TIMEVAL_ &endTime, const _TIMEVAL_ &runTime)
{	
	//ctrl().programCtrl().writeln(STDOUT, "CFademoHooks::deviceEnd() SiteNum: %d\n", currentBinInfo.siteNumber );
	return true;
}


bool CFademoHooks::modifySitesReady(const std::vector<FA_LONG> &siteReadyList, std::vector<bool> &testDut, std::vector<bool> &binDut)
{
	//ctrl().debugStream("UserHooks: modifySitesReady", true);
	return true;
}

bool CFademoHooks::syncGroupStart(const std::vector<FA_LONG> &syncSites, const _TIMEVAL_ &markTime, const _TIMEVAL_ &runTime)
{
	//ctrl().debugStream("UserHooks: syncGroupStart", true);
	return true;
}

bool CFademoHooks::syncGroupEnd(const std::vector<FA_LONG> &syncSites, const _TIMEVAL_ &markTime, const _TIMEVAL_ &runTime)
{
	//ctrl().debugStream("UserHooks: syncGroupEnd", true);
	return true;
}

bool CFademoHooks::onAlarm(bool system_alarm, bool critical_alarm, const std::string alarmReference, const std::string& alarmStr, FA_LONG alarmID, std::string& alarmReply, const _TIMEVAL_ &alarmTime)
{
	//ctrl().debugStream("UserHooks: onAlarm", true);
	return true;
}

bool CFademoHooks::clearAlarm(const std::string& alarmReference)
{
	//ctrl().debugStream("UserHooks: clearAlarm", true);
	return true;
}

bool CFademoHooks::datalogUpdate(const FA_LONG site, const std::string &token, const std::string &value, bool newToken)
{
	//ctrl().debugStream("UserHooks: datalogUpdate", true);
	return true;
}

bool CFademoHooks::onRobotCondition(const std::string& condition)
{
	//ctrl().debugStream("UserHooks: onRobotCondition", true);
	return true;
}

bool CFademoHooks::stateChange(const std::string& stateStr, const std::string& stateInfo, bool stateChanged, FA_LONG stateMajorKey,FA_LONG stateMinorKey, const _TIMEVAL_& markTime, const _TIMEVAL_& runTime)
{
	ctrl().debugStream("--------------<<UserHooks>>: stateChange ALLAN", true);
	return true;
}

bool CFademoHooks::onPoll(const std::string& pollReference, const std::string& pollStr, FA_LONG pollID, std::string& pollReply, const _TIMEVAL_& markTime, const _TIMEVAL_& runTime)
{
	//ctrl().debugStream("UserHooks: onPoll", true);
	return true;
}

bool CFademoHooks::onCommand(const std::string& command, const std::string& param1, FA_ULONG param2, bool param3, const _TIMEVAL_& markTime, const _TIMEVAL_& runTime)
{
	std::stringstream ss;
	ss << "UserHook::onCommand >> " << command << ", Param: " << param1;

	ctrl().debugStream(ss.str(), true);
	
//	ctrl().debugStream("UserHooks: onCommand", true);
	return true;
}


bool CFademoHooks::onUserError(FA_LONG errorKey, const std::string& errMsg, FA_LONG code, const std::string& codeStr, const std::vector<std::string>& options, const _TIMEVAL_& markTime, const _TIMEVAL_& runTime)
{
	ctrl().debugStream("UserHooks: onUserError ") << errMsg << std::endl;
	for (size_t idx = 0; idx < options.size(); idx++) {
	    ctrl().debugStream("                ") << idx << ". " <<options[idx] << std::endl;
	}
	return true;
}

bool CFademoHooks::onUserErrorResponse(FA_LONG errorKey, const std::vector<FA_LONG>& vVals, const std::vector<std::string>& vStrs, const _TIMEVAL_& markTime, const _TIMEVAL_& runTime)
{
	ctrl().debugStream("UserHooks: onUserErrorResponse ") << vVals[0] << std::endl;
	return true;
}


bool CFademoHooks::evxioMessage(bool responseNeeded, bool responseAquired, const std::string& evxio_msg)
{
	ctrl().debugStream("UserHooks: evxioMessage", true);
	return true;
}

bool CFademoHooks::onLogCommunications(FA_LONG cmdType, FA_LONG errorCode, std::vector<std::string>& logData, const _TIMEVAL_& markTime, const _TIMEVAL_& runTime)
{
        
	ctrl().debugStream("UserHooks: onLogCommunications", true);
	return true;
}

