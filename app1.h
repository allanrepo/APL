#ifndef __APP1__
#define __APP1__

#include <utility.h>
#include <state.h>
#include <fd.h>
#include <notify.h>
#include <pwd.h>
#include <sys/time.h>

class CApp;
class CTask;
class CAppState;
class CMonitorFileDesc;


class CApp
{
protected:
	// resources
	CUtil::CLog m_Log;

	// parameters
	std::string m_szConfigFullPathName;
	std::string m_szTesterName;

	// file descriptor engine
	CFileDescriptorManager m_FileDescMgr;
	CMonitorFileDesc* m_pMonitorFileDesc; 

	// state machine
	CStateManager m_StateMgr;

	// states
	CAppState* m_pStateOnIdle;

	// tasks/events
	void onConnect();
	void onSelect();

	// process the incoming file from the monitored path
	void onReceiveFile(const std::string& name);	
	
	bool scan(int argc, char **argv);

	// utility function that acquire linux login username
	const std::string getUserName() const;

public:
	CApp(int argc, char **argv);
	virtual ~CApp();
};

/* ------------------------------------------------------------------------------------------

------------------------------------------------------------------------------------------ */
class CTask
{
protected:
	CApp& m_App;
	std::string m_szName;
	void (CApp::* m_pRun)();
	CUtil::CLog m_Log;

	struct timeval m_prev;
	bool m_bFirst;
	long m_nDelayMS;
	bool m_bEnabled;
	
public:
	CTask(CApp& app, void (CApp::* p)() = 0, long nDelayMS = 0, const std::string& name = ""):
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
		if (m_pRun) (m_App.*m_pRun)();
	}

	friend class CAppState;
};

class CAppState: public CStateManager::CState
{
protected:
	std::list<CTask*> m_Tasks;
	CUtil::CLog m_Log;

public:
	CAppState(const std::string& name = ""):CState(name){}

	virtual ~CAppState(){}

	// overwritten to allow it to execute a method from CApp class
	virtual void run();

	void add(CApp& app, void (CApp::* p)(), long nDelayMS = 0, const std::string& name = "")
	{
		CTask* pTask = new CTask(app, p, nDelayMS, name);
		if (pTask) m_Tasks.push_back(pTask);
	};

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

	virtual	void onFileCreate( const std::string& name ){ if (m_pOnReceiveFile) (m_App.*m_pOnReceiveFile)(name); }	
	virtual	void onFileMoveTo( const std::string& name ){ if (m_pOnReceiveFile) (m_App.*m_pOnReceiveFile)(name); }
	virtual	void onFileModify( const std::string& name ){ if (m_pOnReceiveFile) (m_App.*m_pOnReceiveFile)(name); }
};

#endif
