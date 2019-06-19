#ifndef __APP1__
#define __APP1__

#include <utility.h>
#include <state.h>
#include <fd.h>
#include <notify.h>
#include <pwd.h>
#include <sys/time.h>
#include <tester.h>
#include <xml.h>

class CApp;
class CAppTask;
class CAppState;
class CMonitorFileDesc;


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
		int		nRelaunchTimeOutMS;
		int		nRelaunchAttempt;

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

		CONFIG()
		{
			bProd = true;
			nRelaunchTimeOutMS = 120000;
			nRelaunchAttempt = 3;
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
		}
	};

protected:
	// resources
	CUtil::CLog m_Log;

	// parameters
	CONFIG m_CONFIG;
	std::string m_szConfigFullPathName;
	std::string m_szTesterName;

	// file descriptor engine
	CFileDescriptorManager m_FileDescMgr;
	CMonitorFileDesc* m_pMonitorFileDesc; 

	// state machine
	CStateManager m_StateMgr;

	// states
	CState* m_pStateOnInit;
	CState* m_pStateOnIdle;

	// tasks/events
	void onConnect(CTask&);
	void onSelect(CTask&);
	void onInit(CTask&);
	void onUpdateLogFile(CTask&);
	void onSwitchToIdleState(CTask&){ if (m_pStateOnIdle) m_StateMgr.set(m_pStateOnIdle); }

	// process the incoming file from the monitored path
	void onReceiveFile(const std::string& name);	

	// parse XML config file
	bool config(const std::string& config);
	
	// utility functions. purpose are obvious in their function name and arguments
	bool scan(int argc, char **argv);
	const std::string getUserName() const;
	void setupLogFile();

public:
	CApp(int argc, char **argv);
	virtual ~CApp();
};

/* ------------------------------------------------------------------------------------------

------------------------------------------------------------------------------------------ */
class CAppTask: public CTask
{
protected:
	CApp& m_App;
	void (CApp::* m_pRun)(CTask&);

public:
	CAppTask(CApp& app, void (CApp::* p)(CTask&) = 0, long nDelayMS = 0, const std::string& name = "", bool bLoop = false):
	CTask(name, nDelayMS, bLoop), m_App(app)
	{
		m_pRun = p;		
	}
	
	virtual void run()
	{
		if (m_pRun) (m_App.*m_pRun)(*this);
	}

/*
protected:
	CApp& m_App;
	std::string m_szName;
	void (CApp::* m_pRun)(CState&, CAppTask&);
	CUtil::CLog m_Log;

	struct timeval m_prev;
	bool m_bFirst;
	long m_nDelayMS;
	bool m_bEnabled;

	CState* m_pState;
	
public:
	CAppTask(CApp& app, void (CApp::* p)(CState&, CAppTask&) = 0, long nDelayMS = 0, const std::string& name = ""):
	m_App(app)
	{
		m_nDelayMS = nDelayMS;
		m_pRun = p;
		m_szName = name;
		m_bEnabled = true;
		m_bFirst = true;
	}
	
	virtual void run()
	{
		if (m_pRun) (m_App.*m_pRun)(*m_pState, *this);
	}

	void enable(){ m_bEnabled = true; }
	void disable(){ m_bEnabled = false; }

	friend class CAppState;
*/
};
/*
class CAppState: public CState
{
protected:
	std::list<CAppTask*> m_Tasks;
	CUtil::CLog m_Log;

public:
	CAppState(const std::string& name = ""):CState(name)
	{
	}

	virtual ~CAppState(){}

	virtual void run();
	virtual void load();

	void add(CApp& app, void (CApp::* p)(CState&, CAppTask&), long nDelayMS = 0, const std::string& name = "")
	{
		CAppTask* pTask = new CAppTask(app, p, nDelayMS, name);
		if (pTask)
		{
			m_Tasks.push_back(pTask);
			pTask->m_pState = this;
		}
	};

};
*/

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

	virtual	void onFileCreate( const std::string& name ){ if (m_pOnReceiveFile) (m_App.*m_pOnReceiveFile)(name); }	
	virtual	void onFileMoveTo( const std::string& name ){ if (m_pOnReceiveFile) (m_App.*m_pOnReceiveFile)(name); }
	virtual	void onFileModify( const std::string& name ){ if (m_pOnReceiveFile) (m_App.*m_pOnReceiveFile)(name); }
};

#endif
