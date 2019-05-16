#ifndef __APP__
#define __APP__
#include <fd.h>
#include <tester.h>
#include <notify.h>
#include <pwd.h>
#include <evxa/EVXA.hxx>

/* ------------------------------------------------------------------------------------------
constants
------------------------------------------------------------------------------------------ */
#define DELIMITER ':'
#define JOBFILE "JOBFILE"
#define VERSION "alpha.1.0.20180516"
#define DEVELOPER "allan asis / allan.asis@gmail.com"
#define MAXCONNECT 20

/* ------------------------------------------------------------------------------------------
declarations
------------------------------------------------------------------------------------------ */
class CApp;
class CAppFileDesc;
class CMonitorFileDesc;

/* ------------------------------------------------------------------------------------------
app class. this is where stuff happens. 
inherited CTester class so as to make it a lot easier to access EVXA objects
------------------------------------------------------------------------------------------ */
class CApp: public CTester
{
protected:
	CFileDescriptorManager m_FileDescMgr;
	bool m_bReconnect;
	bool m_bRestartTester;

	// parameters
	std::string m_szProgramFullPathName;
	std::string m_szMonitorPath;
	std::string m_szTesterName;
	std::string m_szMonitorFileName;

	// file descriptor for inotify to monitor path where lotinfo.txt will be sent
	CMonitorFileDesc* m_pMonitorFileDesc;

	// initialize variabls, reset stuff
	void init();

	// utility function that acquire linux login username
	const std::string getUserName() const;

public:
	CApp(int argc, char **argv);
	virtual ~CApp();

	bool scan(int argc, char **argv);
 
	// even handlers for evxa response
	void onStateNotificationResponse(int fd);
	void onEvxioResponse(int fd);
	void onErrorResponse(int fd);

	// process the incoming file from the monitored path
	void onReceiveFile(const std::string& name);
	
	// parse the incoming file from the monitored path
	bool parse(const std::string& name);
	bool getFieldValuePair(const std::string& line, const char delimiter, std::string& field, std::string& value);

	bool launch(const std::string& tester, const std::string& program, bool bProd);
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



/* ------------------------------------------------------------------------------------------
inherit notify file descriptor class and customize event handlers
------------------------------------------------------------------------------------------ */
class CMonitorFileDesc: public CNotifyFileDescriptor
{
protected:
	CApp& m_App;
public:
	CMonitorFileDesc(CApp& app, const std::string& path, unsigned short mask = IN_MODIFY | IN_CREATE | IN_DELETE | IN_MOVED_TO | IN_MOVED_FROM)
	: CNotifyFileDescriptor(path, mask), m_App(app){}

	virtual	void onFileCreate( const std::string& name ){ m_App.onReceiveFile(name); }	
	virtual	void onFileMoveTo( const std::string& name ){ m_App.onReceiveFile(name); }
	virtual	void onFileModify( const std::string& name ){ m_App.onReceiveFile(name); }
};


#endif
