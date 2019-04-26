#include <app.h>

//void CHandleNotify::onFileCreate( const std::string& name ){ m_App.onReceiveFile(name); }	
//void CHandleNotify::onFileMoveTo( const std::string& name ){ }//m_App.onReceiveFile(name); }

/* ------------------------------------------------------------------------------------------
constructor
------------------------------------------------------------------------------------------ */
CApp::CApp()
{
	// flag used to request for tester reconnect, false by default
	m_bReconnect = false;

	// create notify file descriptor and add to fd manager. this will monitor incoming lotinfo.txt file
	CHandleNotify NotifyFD(*this, "/home/localuser/Desktop/APL" );
	m_FileDescMgr.add( NotifyFD );

	// run the application loop
	while(1) 
	{
		// connect to tester. set it such that it will keep trying to connect forever
		connect("localuser_sim", 1, -1);

		// get file descriptor of tester state notification and add to our fd manager		
		CAppFileDesc StateNotifyFileDesc(&CApp::onStateNotificationResponse, *this, m_pState? m_pState->getSocketId(): -1);
		m_FileDescMgr.add( StateNotifyFileDesc );
		
#if 0 // i didn't wanna add these 2, not necessary i think...
		// get file descriptor of tester evxio stream and add to our fd manager		
		CAppFileDesc StateEvxioStreamFileDesc(&CApp::onEvxioResponse, *this, m_pEvxio? m_pEvxio->getEvxioSocketId(): -1);
		m_FileDescMgr.add( StateEvxioStreamFileDesc );

		// get file descriptor of tester evxio error and add to our fd manager		
		CAppFileDesc StateEvxioErrorFileDesc(&CApp::onErrorResponse, *this, m_pEvxio? m_pEvxio->getErrorSocketId(): -1);
		m_FileDescMgr.add( StateEvxioErrorFileDesc );
#endif
		// this is the select() loop. we'll stay here and process events happening in any of the fds in our list.
		// we'll break off once there's a tester reconnect request
		while(1) 
		{
			m_FileDescMgr.select();
			if(m_bReconnect) 
			{
				m_Log << "tester GOT disconnected..." << CUtil::CLog::endl;
				m_bReconnect = false;
				break;
			}
		}
	}
}

CApp::~CApp()
{
	
}

/* ------------------------------------------------------------------------------------------
// process the incoming file from the monitored path
------------------------------------------------------------------------------------------ */
void CApp::onReceiveFile(const std::string& name)
{
	// is this file lotinfo.txt?
	if (name.compare("lotinfo.txt") != 0)
	{
		m_Log << "File received but is not what we're waiting for: " << name << CUtil::CLog::endl;
		return;
	}
}

/* ------------------------------------------------------------------------------------------
handle event for state notification. if error occurs, request tester reconnect
------------------------------------------------------------------------------------------ */
void CApp::onStateNotificationResponse(int fd)
{		
	if (m_pState->respond(fd) != EVXA::OK) 
	{
      		const char *errbuf = m_pState->getStatusBuffer();
		m_Log << "State Notification Response Not OK: " << errbuf << CUtil::CLog::endl;
		m_bReconnect = true;
	}  		
}

/* ------------------------------------------------------------------------------------------
handle event for evxio stream. if error occurs, request tester reconnect
------------------------------------------------------------------------------------------ */
void CApp::onEvxioResponse(int fd)
{		
	if (m_pEvxio->streamsRespond() != EVXA::OK) 
	{
      		const char *errbuf = m_pState->getStatusBuffer();
		m_Log << "EVXIO Stream Response Not OK: " << errbuf << CUtil::CLog::endl;
		m_bReconnect = true;
	}  		
}

/* ------------------------------------------------------------------------------------------
handle event for evxio errors. if error occurs, request tester reconnect
------------------------------------------------------------------------------------------ */
void CApp::onErrorResponse(int fd)
{		
	if (m_pEvxio->ErrorRespond() != EVXA::OK) 
	{
      		const char *errbuf = m_pState->getStatusBuffer();
		m_Log << "EVXIO Error Response Not OK: " << errbuf << CUtil::CLog::endl;
		m_bReconnect = true;
	}  		
}

