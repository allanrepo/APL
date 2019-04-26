#include <app.h>

CHandleNotify::CHandleNotify(CApp& app, const std::string& path, unsigned short mask): CNotifyFileDescriptor(path, mask), m_App(app)
{

}

void CHandleNotify::onFileMoveTo( const std::string& name )
{
	m_App.onNewLotFileInfo();
}

void CHandleNotify::onFileCreate( const std::string& name )
{
	m_App.onNewLotFileInfo();
}


CApp::CApp()
{
	// create notify fd
	CHandleNotify NotifyFD(*this, "/home/localuser/Desktop/APL" );

	// add notify fd to our fd manager
	m_FileDescMgr.add( NotifyFD );

	// run the application loop
	while(1) 
	{
		// connect to tester. set it such that it will keep trying to connect forever
		m_Tester.connect("localuser_sim", 1, -1);

		// get file descriptor of tester state notification and add to our fd manager
		CStateNotificationFileDesc StateFileDesc(m_Tester);
		m_FileDescMgr.add( StateFileDesc );
		
		
		while(1) 
		{
			m_FileDescMgr.select();
			if(!m_Tester.isReady()) break;
		}
	}
}

CApp::~CApp()
{
	
}

void CApp::onNewLotFileInfo()
{	
	m_Log << "on new lot info" << CUtil::CLog::endl;
}
