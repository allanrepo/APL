#pragma once


#include <evxa/faModule.h>
#include <sstream>

template < typename T >
std::string num2stdstring(T value) {
	return static_cast< std::ostringstream* >(&(std::ostringstream() << value))->str();
}

#define UH_NAME "Unison FAPROC User Hooks"
#define UH_VERSION_NUMBER 1.1

class CFademoHooks : public CFaModule {
private:
    std::map<std::string, evo_flow_entry_point>  m_str2flow;
public:
	CFademoHooks(
		CfaModuleInterface& cCtrl,
		std::string& errStr,
		FA_LONG& errCode,
		const std::string& name,
		const std::string& versionStr,
		FA_LONG versionNum);

	virtual ~CFademoHooks();


	bool equipmentStatusChange(const std::string &equipmentID, bool loadStatus, bool mappingEquipment, const _TIMEVAL_ &startTime, const _TIMEVAL_ &runTime);
	bool lotStart(std::string &lotID, const _TIMEVAL_ &startTime, const _TIMEVAL_ &runTime);
	bool subLotStart(std::string &subLotID, const _TIMEVAL_ &startTime, const _TIMEVAL_ &runTime);
	bool mapStart(std::string &mapID, FA_LONG &mapNumber, const _TIMEVAL_ &startTime, const _TIMEVAL_ &runTime);
	bool mapEnd(const std::string &mapID, const FA_LONG mapNumber, const _TIMEVAL_ &endTime, const _TIMEVAL_ &runTime);
	bool subLotEnd(const std::string &subLotID, const _TIMEVAL_ &endTime, const _TIMEVAL_ &runTime);
	bool lotEnd(const std::string &lotID, const _TIMEVAL_ &endTime, const _TIMEVAL_ &runTime);
	bool deviceStart(FA_LONG site, FA_LONG &deviceNumber, bool isMap, const mapInfo &currentMapInfo, bool &isRetest, const _TIMEVAL_ &startTime, const _TIMEVAL_ &runTime);
	bool deviceEnd(FA_LONG deviceNumber, binInfo &currentBinInfo, bool isMap, const mapInfo &currentMapInfo, const _TIMEVAL_ &endTime, const _TIMEVAL_ &runTime);
	bool modifySitesReady(const std::vector<FA_LONG> &siteReadyList, std::vector<bool> &testDut, std::vector<bool> &binDut);
	bool syncGroupStart(const std::vector<FA_LONG> &syncSites, const _TIMEVAL_ &markTime, const _TIMEVAL_ &runTime);
	bool syncGroupEnd(const std::vector<FA_LONG> &syncSites, const _TIMEVAL_ &markTime, const _TIMEVAL_ &runTime);
	bool onAlarm(bool system_alarm, bool critical_alarm, const std::string alarmReference, const std::string& alarmStr, FA_LONG alarmID, std::string& alarmReply, const _TIMEVAL_ &alarmTime);
	bool clearAlarm(const std::string& alarmReference);
	bool datalogUpdate(const FA_LONG site, const std::string &token, const std::string &value, bool newToken);
	bool onRobotCondition(const std::string& condition);

	bool stateChange(const std::string& stateStr, const std::string& stateInfo, bool stateChanged, FA_LONG stateMajorKey,FA_LONG stateMinorKey, const _TIMEVAL_& markTime, const _TIMEVAL_& runTime);
	bool onPoll(const std::string& pollReference, const std::string& pollStr, FA_LONG pollID, std::string& pollReply, const _TIMEVAL_& markTime, const _TIMEVAL_& runTime);
	bool onCommand(const std::string& command, const std::string& param1, FA_ULONG param2, bool param3, const _TIMEVAL_& markTime, const _TIMEVAL_& runTime);

	bool onUserError(FA_LONG errorKey, const std::string& errMsg, FA_LONG code, const std::string& codeStr, const std::vector<std::string>& options, const _TIMEVAL_& markTime, const _TIMEVAL_& runTime);
	bool onUserErrorResponse(FA_LONG errorKey, const std::vector<FA_LONG>& vVals, const std::vector<std::string>& vStrs, const _TIMEVAL_& markTime, const _TIMEVAL_& runTime);
    	bool evxioMessage(bool responseNeeded, bool responseAquired, const std::string& evxio_msg);
    	bool onLogCommunications(FA_LONG cmdType, FA_LONG errorCode, std::vector<std::string>& logData, const _TIMEVAL_& markTime, const _TIMEVAL_& runTime);

private:

};

