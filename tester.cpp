#include "tester.h"
#include <sys/inotify.h>

#define EVENT_SIZE  ( sizeof (struct inotify_event) )
#define BUF_LEN     ( 1024 * ( EVENT_SIZE + 255 ) )

 
/* ------------------------------------------------------------------------------------------
constructor
------------------------------------------------------------------------------------------ */
CTester::CTester()
{ 
	// play safe. initialize pointers to null
	m_pTester = 0;
	m_pTestHead = 0;
	m_pProgCtrl = 0;
	m_pState = 0;
	m_pEvxio = 0;

	// configure loggers
	m_Log.immediate = true;
	m_Log.enable = true; 
	m_Log.silent = false;

	// default
	m_nHead = 1;
}


/* ------------------------------------------------------------------------------------------
destructor
------------------------------------------------------------------------------------------ */
CTester::~CTester()
{
	disconnect();
}

/* ------------------------------------------------------------------------------------------
connects to tester by first creating EVXA tester objects and then hooking up to tester's IO
------------------------------------------------------------------------------------------ */
bool CTester::connect(const std::string& strTesterName, int nSleep, int nAttempts)
{
	// always make sure we make at least 1 attempt
	if (!nAttempts) nAttempts = 1;

	m_Log << "Creating EVXA objects..." << CUtil::CLog::endl;

	// let's attempt n number of times to connect
  	while(nAttempts < 0 || nAttempts--) 
	{
		disconnect();

		// connect to tester
    		m_pTester = new TesterConnection(strTesterName.c_str());	
    		if(m_pTester->getStatus() != EVXA::OK){ if(nSleep) sleep(nSleep); continue; }
		else m_Log << "TesterConnection object created..." << CUtil::CLog::endl;
		
		// connect to test head
    		m_pTestHead = new TestheadConnection(strTesterName.c_str(), m_nHead);
    		if(m_pTestHead->getStatus() !=  EVXA::OK) { if(nSleep) sleep(nSleep); continue; } 
		else m_Log << "TestheadConnection object created..." << CUtil::CLog::endl;

		// create program control object, does not check if program is loaded
    		m_pProgCtrl = new ProgramControl(*m_pTestHead);
    		if(m_pProgCtrl->getStatus() !=  EVXA::OK) { if(nSleep) sleep(nSleep); continue; }
		else m_Log << "ProgramControl object created..." << CUtil::CLog::endl;

		// create notification object
    		m_pState = new CStateNotification(*m_pTestHead);
    		if(m_pState->getStatus() !=  EVXA::OK) { if(nSleep) sleep(nSleep); continue; } 
		m_Log << "CStateNotification object created..." << CUtil::CLog::endl;

		// lets convert our tester name from std::string to crappy old C style string because the stupid software team 
		// who designed EVXA libraries sucks so bad and too lazy to set string arguments as constants...
		char szTesterName[1024] = "";
		sprintf(szTesterName, "%s", strTesterName.c_str());

		// create stream client
    		m_pEvxio = new CEvxioStreamClient(szTesterName, m_nHead);
    		if(m_pEvxio->getStatus() != EVXA::OK) { if(nSleep) sleep(nSleep); continue; } 
		else m_Log << "CEvxioStreamClient object created..." << CUtil::CLog::endl;

		// if we reached this point, we are able connect to tester. let's connect to evx stream now...
		// same issue here... i could have used stringstream but forced to use C style string because the damn EVXA class
		// wants a C style string argument that is not a constant!!!
		char szPid[1024] = "";
		sprintf(szPid, "client_%d", getpid());

		m_Log << "Connecting to " << strTesterName << "..." << CUtil::CLog::endl;
    		if(m_pEvxio->ConnectEvxioStreams(m_pTestHead, szPid) != EVXA::OK) { if(nSleep) sleep(nSleep); continue; } 
    		else
		{
			// once the tester objects are created, let's wait until tester is ready
		  	while(!m_pTester->isTesterReady(m_nHead))
			{
				m_Log << "Tester NOT yet ready..." << CUtil::CLog::endl; 
				if (nSleep) sleep(nSleep);
			}
			m_Log << "Tester ready for use." << CUtil::CLog::endl;
		  	return true; 	 
		}
  	}

	// if we reach this point, we failed to connect to tester after n number of attempts...
	m_Log << "CEX Error: Can't connect to tester: Tester '" << strTesterName << "' does not exist." << CUtil::CLog::endl;//[print]
  
	return false; 	 
}

/* ------------------------------------------------------------------------------------------
destroys the EVXA tester objects
------------------------------------------------------------------------------------------ */
void CTester::disconnect()
{
	if (m_pTester) 		delete(m_pTester); 	m_pTester = 0;
	if (m_pTestHead) 	delete(m_pTestHead); 	m_pTestHead = 0;
	if (m_pProgCtrl) 	delete(m_pProgCtrl); 	m_pProgCtrl = 0;
	if (m_pState) 		delete(m_pState); 	m_pState = 0;
	if (m_pEvxio) 		delete(m_pEvxio); 	m_pEvxio = 0;
} 

/* ------------------------------------------------------------------------------------------
application's loop
------------------------------------------------------------------------------------------ */
void CTester::loop()
{

	char buffer[BUF_LEN];
	int length, i = 0; 
	int fdNotify; 
	int wd;

	// create file descriptor for inotify
	fdNotify = inotify_init();
	if ( fdNotify < 0 ) perror( "inotify_init" );

	// let's watch the specified path
	wd = inotify_add_watch( fdNotify, "/tmp", IN_MODIFY | IN_CREATE | IN_DELETE | IN_MOVED_TO | IN_MOVED_FROM);
	if (wd == -1)
	{
		m_Log << "ERROR: Something went wrong in trying to watch " << "/tmp" << ". Check the path again if it's valid. " << CUtil::CLog::endl;
		return ;
	}



	// let's get the file descriptor of state notification, evxio messages, end evxio error stream. we're listening to these
	int fdState = m_pState->getSocketId();
	int fdEvxio = m_pEvxio->getEvxioSocketId();
	int fdError = m_pEvxio->getErrorSocketId();		

	// create our fd_set and add our file descriptors to it
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(fdState, &fds);
	FD_SET(fdEvxio, &fds);
	FD_SET(fdError, &fds);
	FD_SET(fdNotify, &fds);

	// get the max file descriptor number. needed for select()
	int nMaxFd = fdState;
	nMaxFd = fdEvxio > nMaxFd? fdEvxio : nMaxFd;
	nMaxFd = fdError > nMaxFd? fdError : nMaxFd;
	nMaxFd = fdNotify > nMaxFd? fdNotify : nMaxFd;

	while(true)
	{
		// wait here now for io operations from any of the fd's
		fd_set rdy = fds;
		if( select(nMaxFd + 1, &rdy, NULL, NULL, NULL) < 0 ) perror("select() failed ");

		// handle io operations from state notifications
		if((fdState > 0) && (FD_ISSET(fdState, &rdy))) 
		{
    			if (m_pState->respond(fdState) != EVXA::OK) 
			{
		      		const char *errbuf = m_pState->getStatusBuffer();
				m_Log << "Error occured on state notification IO operation: " << errbuf << CUtil::CLog::endl;
			      	break;
    			}  
  		}

		if((fdNotify > 0) && (FD_ISSET(fdNotify, &rdy)))
		{
			i = 0;
			memset(buffer, 0, sizeof(buffer)); 
			length = read( fdNotify, buffer, BUF_LEN );  
			if ( length < 0 ) perror( "read" );
 
			while ( i < length )   
			{
				struct inotify_event *event = ( struct inotify_event * ) &buffer[ i ];

				if (!event->len) continue;

				//std::cout << "mask --- " << std::hex << event->mask << ", " << event->len <<  std::endl;


				if ( event->mask & IN_CREATE ) 
				{
					if ( event->mask & IN_ISDIR ) printf( "The directory %s was created.\n", event->name );       
					else printf( "The file %s was created.\n", event->name );
				}
				else if ( event->mask & IN_DELETE ) 
				{
					if ( event->mask & IN_ISDIR ) printf( "The directory %s was deleted.\n", event->name );       
					else printf( "The file %s was deleted.\n", event->name );
				}
				else if ( event->mask & IN_MODIFY ) 
				{
					if ( event->mask & IN_ISDIR ) printf( "The directory %s was modified.\n", event->name );
					else printf( "The file %s was modified.\n", event->name );
				}
				else if ( event->mask & IN_ACCESS ) 
				{
					if ( event->mask & IN_ISDIR ) printf( "The directory %s was accessed.\n", event->name );
					else printf( "The file %s was accessed.\n", event->name ); 
				} 
				else if ( event->mask & IN_ATTRIB ) 
				{
					if ( event->mask & IN_ISDIR ) printf( "The directory %s attribute changed.\n", event->name );
					else printf( "The file %s attribute changed.\n", event->name ); 
				}
				else if ( event->mask & IN_CLOSE_WRITE ) 
				{
					if ( event->mask & IN_ISDIR ) printf( "The directory %s opened for writing was closed.\n", event->name );
					else printf( "The file %s opened for writing was closed.\n", event->name ); 
				}
				else if ( event->mask & IN_CLOSE_NOWRITE ) 
				{
					if ( event->mask & IN_ISDIR ) printf( "The directory %s not opened for writing was closed.\n", event->name );
					else printf( "The file %s not opened for writing was closed.\n", event->name ); 
				} 
				else if ( event->mask & IN_DELETE_SELF ) 
				{
					if ( event->mask & IN_ISDIR ) printf( "The directory %s, a watched directory is itself deleted.\n", event->name );
					else printf( "The file %s, a watched directory is itself deleted.\n", event->name ); 
				}
				else if ( event->mask & IN_MOVED_FROM ) 
				{
					if ( event->mask & IN_ISDIR ) printf( "The directory %s is moved from.\n", event->name );       
					else printf( "The file %s is moved from.\n", event->name );
				}
				else if ( event->mask & IN_MOVED_TO ) 
				{
					if ( event->mask & IN_ISDIR ) printf( "The directory %s is moved to.\n", event->name );       
					else printf( "The file %s is moved to.\n", event->name );
				}
				else if ( event->mask & IN_OPEN ) 
				{
					if ( event->mask & IN_ISDIR ) printf( "The directory %s is opened.\n", event->name );       
					else printf( "The file %s is opened.\n", event->name );
				}
				i += EVENT_SIZE + event->len;
			}

		}
			
#if 0
		// handle requests for evxio notifications
		if((fdEvxio > 0) && (FD_ISSET(fdEvxio, &rdy))) 
		{
			if(m_pEvxio->streamsRespond() != EVXA::OK) 
			{
				const char *errbuf = m_pEvxio->getStatusBuffer();
				m_Log << "Error occured on evxio notification IO operation: " << errbuf << CUtil::CLog::endl;
			      	break;
			}
		}

		// handle requests for error notifications
		if((fdError > 0) && (FD_ISSET(fdError, &rdy))) 
		{
			if(m_pEvxio->ErrorRespond() != EVXA::OK)
		 	{
				const char *errbuf = m_pEvxio->getStatusBuffer();
				m_Log << "Error occured on evxio error IO operation: " << errbuf << CUtil::CLog::endl;
			      	break;
			}
		}
#endif
	}

	inotify_rm_watch( fdNotify, wd );
	close( fdNotify );
}



