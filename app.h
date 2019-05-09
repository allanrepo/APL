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
#define VERSION "alpha.1.0.20180509.1"
#define DEVELOPER "allan asis / allan.asis@gmail.com"

/* ------------------------------------------------------------------------------------------
declarations
------------------------------------------------------------------------------------------ */
class CApp;
class CAppFileDesc;
class CHandleNotify;

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
	CAppFileDesc(void (CApp::* p)(int), CApp& app, int fd): m_App(app)
	{
		m_fd = fd;
		m_onSelectPtr = p;
	}	
	virtual void onSelect(){ (m_App.*m_onSelectPtr)(m_fd);	}
};



/* ------------------------------------------------------------------------------------------
inherit notify file descriptor class and customize event handlers
------------------------------------------------------------------------------------------ */
class CHandleNotify: public CNotifyFileDescriptor
{
protected:
	CApp& m_App;
public:
	CHandleNotify(CApp& app, const std::string& path, unsigned short mask = IN_MODIFY | IN_CREATE | IN_DELETE | IN_MOVED_TO | IN_MOVED_FROM)
	: CNotifyFileDescriptor(path, mask), m_App(app)
	{
		//m_Log.silent = true;
	}

	virtual	void onFileCreate( const std::string& name ){ m_App.onReceiveFile(name); }	
	virtual	void onFileMoveTo( const std::string& name ){ m_App.onReceiveFile(name); }
};


#endif
