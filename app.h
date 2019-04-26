#ifndef __APP__
#define __APP__
#include <fd.h>
#include <tester.h>
#include <notify.h>

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

public:
	CApp();
	virtual ~CApp();
 
	// even handles for evxa response
	void onStateNotificationResponse(int fd);
	void onEvxioResponse(int fd);
	void onErrorResponse(int fd);

	// process the incoming file from the monitored path
	void onReceiveFile(const std::string& name);

	void onNewLotFileInfo();
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
	CApp& m_App;
	CUtil::CLog m_Log;
public:
	CHandleNotify(CApp& app, const std::string& path, unsigned short mask = IN_MODIFY | IN_CREATE | IN_DELETE | IN_MOVED_TO | IN_MOVED_FROM)
	: CNotifyFileDescriptor(path, mask), m_App(app)
	{
		//::m_Log.enable = false;
	}

	virtual	void onFileCreate( const std::string& name ){ m_App.onReceiveFile(name); }	
	virtual	void onFileMoveTo( const std::string& name ){m_App.onReceiveFile(name); }
};


#endif
