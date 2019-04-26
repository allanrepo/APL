#ifndef __APP__
#define __APP__
#include <fd.h>
#include <tester.h>
#include <notify.h>

/* ------------------------------------------------------------------------------------------
declarations
------------------------------------------------------------------------------------------ */
class CHandleNotify;
class CApp;

class CStateNotificationFileDesc: public CFileDescriptorManager::CFileDescriptor
{
protected:
	CTester& m_Tester;
	CUtil::CLog m_Log;

public:
	CStateNotificationFileDesc( CTester& Tester ): m_Tester(Tester)
	{
		m_fd = m_Tester.getStateFileDesc();
	}
	virtual ~CStateNotificationFileDesc(){}
	virtual void onSelect()
	{
		m_Tester.onStateNotification(m_fd);		
	}
};

/* ------------------------------------------------------------------------------------------
inherit notify file descriptor class and customize event handlers
------------------------------------------------------------------------------------------ */
class CHandleNotify: public CNotifyFileDescriptor
{
	CApp& m_App;
public:
	CHandleNotify(CApp& app, const std::string& path, unsigned short mask = IN_MODIFY | IN_CREATE | IN_DELETE | IN_MOVED_TO | IN_MOVED_FROM);

	virtual	void onFileCreate( const std::string& name );	
	virtual	void onFileMoveTo( const std::string& name );
};

/* ------------------------------------------------------------------------------------------
app class. this is where stuff happens
------------------------------------------------------------------------------------------ */
class CApp
{
protected:
	CUtil::CLog m_Log;
	CFileDescriptorManager m_FileDescMgr;
	CTester m_Tester;

public:
	CApp();
	virtual ~CApp();
	
	void onNewLotFileInfo();
};

#endif
