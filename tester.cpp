#include "tester.h"
#include <sys/inotify.h>

 
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
	if (nAttempts == 0) nAttempts = 1;

	m_Log << "Creating EVXA objects..." << CUtil::CLog::endl;

	// let's attempt n number of times to connect
  	while(nAttempts < 0 || nAttempts--) 
	{
		disconnect();

		// connect to test head
    		m_pTestHead = new TestheadConnection(strTesterName.c_str(), m_nHead);
    		if(m_pTestHead->getStatus() !=  EVXA::OK) { if(nSleep) sleep(nSleep); continue; } 
		else m_Log << "TestheadConnection object created..." << CUtil::CLog::endl;

		// connect to tester
    		m_pTester = new TesterConnection(strTesterName.c_str());	
    		if(m_pTester->getStatus() != EVXA::OK){ if(nSleep) sleep(nSleep); continue; }
		else m_Log << "TesterConnection object created..." << CUtil::CLog::endl;
		
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

	// get the max file descriptor number. needed for select()
	int nMaxFd = fdState;
	nMaxFd = fdEvxio > nMaxFd? fdEvxio : nMaxFd;
	nMaxFd = fdError > nMaxFd? fdError : nMaxFd;

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
	}
}

/* ------------------------------------------------------------------------------------------
unload program
nWait (seconds) sets the wait time from unloading program before bailing out.
this allows app to go ahead do other things while program is still unloading.
if nWait = 0, we wait forever until test program is unloaded or error occurs
------------------------------------------------------------------------------------------ */
bool CTester::unload(bool bWait, int nWait)
{
	// is no program loaded?
	if (!m_pProgCtrl->isProgramLoaded())
	{
		m_Log << "No program loaded. nothing to unload." << CUtil::CLog::endl;
		return true;
	}

	// get program name to unload for logging later
	std::string szLoadedProg(m_pProgCtrl->getProgramPath());

	// unload program with evxa command
	if (m_pProgCtrl->unload( bWait? EVXA::WAIT : EVXA::NO_WAIT, nWait ) != EVXA::OK)
	{
		m_Log << "Error occured unloading test program." << CUtil::CLog::endl;
		return false;
	}

	// check if program is loaded succesfully
	if (m_pProgCtrl->isProgramLoaded())
	{
		m_Log << "Error, program '" << m_pProgCtrl->getProgramPath() << "' is still loaded after an attempt to unload it." << CUtil::CLog::endl;
		return false;
	}
	else m_Log << "Program '" << szLoadedProg << "' is unloaded successfully." << CUtil::CLog::endl;

	return true;
}

/* ------------------------------------------------------------------------------------------
load program
-	we're not checking if existing program is loaded. we'll just unload without 
	question 
------------------------------------------------------------------------------------------ */
bool CTester::load(const std::string& name, bool bDisplay)
{
	// is program already loaded? unload it.
	if (!unload()) return false;

	// load program with evxa command
	if (m_pProgCtrl->load( name.c_str(), EVXA::WAIT, bDisplay? EVXA::DISPLAY : EVXA::NO_DISPLAY ) != EVXA::OK)
	{
		m_Log << "Error loading '" << name << "'." << CUtil::CLog::endl;
		return false;
	}	

	// check if program is loaded succesfully
	if (!m_pProgCtrl->isProgramLoaded())
	{
		m_Log << "Error, program '" << name << "' failed to load." << CUtil::CLog::endl;
		return false;
	}
	else m_Log << "Program '" << m_pProgCtrl->getProgramPath() << "' is successfully loaded." << CUtil::CLog::endl;

	return true;
}

