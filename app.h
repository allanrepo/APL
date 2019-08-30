#ifndef __APP__
#define __APP__

#include <utility.h>
#include <state.h>
#include <fd.h>
#include <notify.h>
#include <pwd.h>
#include <sys/time.h>
#include <tester.h>
#include <xml.h>
#include <stdf.h>
#include <socket.h>

/*
B.2.0.2019xxxx
-	added function in file descriptor class that allows you to process only the first trigger and ignore the rest.
	this happens in notify class. 
-	added config flag to tell if appending summary with amkor's "step" field is enable or not
-	added summary path in config

*/


/* ------------------------------------------------------------------------------------------
constants
------------------------------------------------------------------------------------------ */
#define DELIMITER ':'
#define JOBFILE "JOBFILE"
#define VERSION "beta.2.0.20190726"
#define DEVELOPER "allan asis / allan.asis@gmail.com"
#define MAXCONNECT 20
#define KILLAPPCMD "kill.app.sh"
#define KILLTESTERCMD "kill.tester.sh"

/* ------------------------------------------------------------------------------------------
class declarations
------------------------------------------------------------------------------------------ */
class CApp;
class CAppTask;
class CAppState;
class CMonitorFileDesc;
class CAppFileDesc;

class CApp: public CTester
{
protected:
	struct CONFIG
	{
		enum APL_TEST_TYPE
		{
			APL_WAFER,
			APL_FINAL,
			APL_MANUAL
		};

		// launch parameters
		bool 		bProd;
		bool		bKillTesterOnLaunch;
		int		nRelaunchTimeOutMS;
		int		nRelaunchAttempt;
		int		nEndLotTimeOutMS;
		int		nUnloadProgTimeOutMS;
		int		nKillTesterTimeOutMS;
		int		nFAModuleTimeOutMS;


		// binning parameters
		bool 		bSendBin;
		bool 		bUseHardBin;
		APL_TEST_TYPE 	nTestType;
		std::string 	IP;	
		int 		nPort;
		int 		nSocketType;

		// logging parameters
		std::string 	szLogPath;
		bool 		bLogToFile;

		// STDF parameters
		bool 		bSendInfo;

		// lotinfo file parameters
		std::string 	szLotInfoFileName;
		std::string 	szLotInfoFilePath;

		// summary appending feature
		bool 		bSummary;
		std::string	szSummaryPath;

		CONFIG()
		{
			bProd = true;
			bKillTesterOnLaunch = false;
			nRelaunchTimeOutMS = 120000;
			nRelaunchAttempt = 3;
			nEndLotTimeOutMS = 30000;
			nUnloadProgTimeOutMS = 30000;
			nKillTesterTimeOutMS = 30000;
			nFAModuleTimeOutMS = 30000;
			bSendInfo = false;
			bSendBin = false;
			bUseHardBin = false;
			nTestType = APL_FINAL;		
			nPort = 54000;
			IP = "127.0.0.1";
			szLogPath = "/tmp";	
			bLogToFile = false;
			nSocketType = SOCK_STREAM;
			szLotInfoFileName = "lotinfo.txt";
			szLotInfoFilePath = "/tmp";
			bSummary = false;
			szSummaryPath = "/tmp";
		}
	};

protected:
	// resources
	CUtil::CLog m_Log;

	// flag to ensure we process only 1 trigger of inotify FD
	bool m_bIgnoreFile;

	// count how many launch attempts we made
	long m_nLaunchAttempt;

	// parameters
	CONFIG m_CONFIG;
	std::string m_szConfigFullPathName;
	std::string m_szTesterName;
	std::string m_szProgramFullPathName;

	// stdf stuff data holders and functions
	MIR m_MIR;
	SDR m_SDR;	

	// file descriptor engine
	CFileDescriptorManager m_FileDescMgr;
	CFileDescriptorManager::CFileDescriptor* m_pMonitorFileDesc; 
	CFileDescriptorManager::CFileDescriptor* m_pStateNotificationFileDesc;
	CFileDescriptorManager::CFileDescriptor* m_pSummaryFileDesc; 

	// state machine
	CStateManager m_StateMgr;

	// state objects
	CState* m_pStateOnInit;
	CState* m_pStateOnIdle;
	CState* m_pStateOnEndLot;
	CState* m_pStateOnUnloadProg;
	CState* m_pStateOnKillTester;
	CState* m_pStateOnLaunch;
	CState* m_pSendLotInfo;

	// state functions (load)
	void onInitLoadState(CState&);
	void onIdleLoadState(CState&);
	void onEndLotLoadState(CState&);
	void onKillTesterLoadState(CState&);
	void onUnloadProgLoadState(CState&);
	void onLaunchLoadState(CState&);
	void onSetLotInfoLoadState(CState&);

	// state functions (unload)
	void onIdleUnloadState(CState&);
	void onEndLotUnloadState(CState&);
	void onUnloadProgUnloadState(CState&);
	void onLaunchUnloadState(CState&);

	// tasks/events
	void init(CTask&);
	void setLogFile(CTask&);
	void connect(CTask&);
	void select(CTask&);
	void endLot(CTask&);
	void timeOutEndLot(CTask&);
	void unloadProg(CTask&);
	void timeOutUnloadProg(CTask&);
	void killTester(CTask&);
	void isTesterDead(CTask&);
	void launch(CTask&);
	void timeOutLaunch(CTask&);
	void timeOutKillTester(CTask&);
	void sendLotInfoToFAModule(CTask&);
	void timeOutSendLotInfoToFAModule(CTask&);

	// functions executed by file descriptor handlers
	void onReceiveFile(const std::string& name);	
	void onStateNotificationResponse(int fd);
	void onSummaryFile(const std::string& name);	
	
	// parse XML config file
	bool config(const std::string& config);
	
	// utility functions. purpose are obvious in their function name and arguments
	bool scan(int argc, char **argv);
	const std::string getUserName() const;
	bool setLotInformation(const EVX_LOTINFO_TYPE type, const std::string& field, const std::string& label, bool bForce = true);
	bool setLotInfo();

	// functions to parse lotinfo.txt file 
	bool parse(const std::string& name);
	bool getFieldValuePair(const std::string& line, const char delimiter, std::string& field, std::string& value);

public:
	CApp(int argc, char **argv);
	virtual ~CApp();

	void setState(CState& state){ m_StateMgr.set(&state); }

	// event handler for state notification
	virtual void onLotChange(const EVX_LOT_STATE state, const std::string& szLotId);
	virtual void onWaferChange(const EVX_WAFER_STATE state, const std::string& szWaferId);
	virtual void onProgramChange(const EVX_PROGRAM_STATE state, const std::string& msg);
	virtual void onEndOfTest(const int array_size, int site[], int serial[], int sw_bin[], int hw_bin[], int pass[], EVXA_ULONG dsp_status = 0);
};

/* ------------------------------------------------------------------------------------------

------------------------------------------------------------------------------------------ */
class CAppState: public CState
{
protected:
	CApp& m_App;
	void (CApp::* m_pLoad)(CState&);
	void (CApp::* m_pUnload)(CState&);
	
public:
	CAppState(CApp& app, const std::string& name = "", void (CApp::* load)(CState&) = 0, void (CApp::* unload)(CState&) = 0)
	:CState(name), m_App(app)
	{
		m_pLoad = load;
		m_pUnload = unload;
	}
	virtual ~CAppState(){}

	virtual void load()
	{ 
		m_Log << "loading state: " << getName() << CUtil::CLog::endl;
		if (m_pLoad) (m_App.*m_pLoad)(*this); 
	}
	virtual void unload()
	{
		m_Log << "unloading state: " << getName() << CUtil::CLog::endl; 
		if (m_pUnload) (m_App.*m_pUnload)(*this); 
		CState::unload(); 
	}
};

/* ------------------------------------------------------------------------------------------

------------------------------------------------------------------------------------------ */
class CAppTask: public CTask
{
protected:
	CApp& m_App;
	void (CApp::* m_pRun)(CTask&);

public:
	CAppTask(CApp& app, void (CApp::* p)(CTask&) = 0, const std::string& name = "", long nDelayMS = 0, bool bEnable = true, bool bLoop = true)
	:CTask(name, nDelayMS, bEnable, bLoop), m_App(app)
	{
		m_pRun = p;		
	}
	
	virtual void run(){ if (m_pRun) (m_App.*m_pRun)(*this); }
};

/* ------------------------------------------------------------------------------------------

------------------------------------------------------------------------------------------ */
class CSwitchTask: public CTask
{
protected:
	CApp& m_App;
	CState& m_state;
public:
	CSwitchTask(CApp& app, CState& state, long nDelayMS = 0, bool bEnable = true)
	:CTask(state.getName(), nDelayMS, bEnable, false), m_App(app), m_state(state){}

	virtual void run(){ m_App.setState(m_state); }
	
};


/* ------------------------------------------------------------------------------------------
inherit notify file descriptor class and customize event handlers
------------------------------------------------------------------------------------------ */
class CMonitorFileDesc: public CNotifyFileDescriptor
{
protected:
	CApp& m_App;
	void (CApp::* m_pOnReceiveFile)(const std::string& name);

public:
	CMonitorFileDesc(CApp& app, void (CApp::* p)(const std::string&), const std::string& path, unsigned short mask = IN_MODIFY | IN_CREATE | IN_DELETE | IN_MOVED_TO | IN_MOVED_FROM)
	: CNotifyFileDescriptor(path, mask), m_App(app){ m_pOnReceiveFile = p; }

	virtual	void onFileCreate( const std::string& name ){ m_Log << "onFileCreate" << CUtil::CLog::endl; if (m_pOnReceiveFile) (m_App.*m_pOnReceiveFile)(name); }	
	virtual	void onFileMoveTo( const std::string& name ){ m_Log << "onFileMoveTo" << CUtil::CLog::endl; if (m_pOnReceiveFile) (m_App.*m_pOnReceiveFile)(name); }
	virtual	void onFileModify( const std::string& name ){ m_Log << "onFileModify" << CUtil::CLog::endl; if (m_pOnReceiveFile) (m_App.*m_pOnReceiveFile)(name); }
};

/* ------------------------------------------------------------------------------------------
file desc class specifically for CApp to handle state notification and evxio
------------------------------------------------------------------------------------------ */
class CAppFileDesc: public CFileDescriptorManager::CFileDescriptor
{
protected:
	CApp& m_App;
	void (CApp::* m_onSelectPtr)(int);

public:
	CAppFileDesc(CApp& app, void (CApp::* p)(int) = 0, int fd = -1): m_App(app)
	{
		m_onSelectPtr = p;
		set(fd);
	}	
	virtual void onSelect()
	{ 
		if (m_onSelectPtr) (m_App.*m_onSelectPtr)(m_fd);	
	}
};


#endif
