#include "tester.h"
 
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
bool CTester::connect(const std::string& strTesterName, int nAttempts)
{
	// always make sure we make at least 1 attempt
	if (!nAttempts) nAttempts = 1;

	// let's attempt n number of times to connect
  	while(nAttempts--) 
	{
		disconnect();

		m_Log << "Creating EVXA objects..." << CUtil::CLog::endl;

		// connect to tester
    		m_pTester = new TesterConnection(strTesterName.c_str());	
    		if(m_pTester->getStatus() != EVXA::OK) continue;
		else m_Log << "TesterConnection object created..." << CUtil::CLog::endl;
		
		// connect to test head
    		m_pTestHead = new TestheadConnection(strTesterName.c_str(), m_nHead);
    		if(m_pTestHead->getStatus() !=  EVXA::OK) continue; 
		else m_Log << "TestheadConnection object created..." << CUtil::CLog::endl;

		// create program control object, does not check if program is loaded
    		m_pProgCtrl = new ProgramControl(*m_pTestHead);
    		if(m_pProgCtrl->getStatus() !=  EVXA::OK) continue;
		else m_Log << "ProgramControl object created..." << CUtil::CLog::endl;

		// create notification object
    		m_pState = new CStateNotification(*m_pTestHead);
    		if(m_pState->getStatus() !=  EVXA::OK) continue; 
		m_Log << "CStateNotification object created..." << CUtil::CLog::endl;

		// lets convert our tester name from std::string to crappy old C style string because the stupid software team 
		// who designed EVXA libraries sucks so bad and too lazy to set string arguments as constants...
		char szTesterName[1024] = "";
		sprintf(szTesterName, "%s", strTesterName.c_str());

		// create stream client
    		m_pEvxio = new CEvxioStreamClient(szTesterName, m_nHead);
    		if(m_pEvxio->getStatus() != EVXA::OK) continue; 
		else m_Log << "CEvxioStreamClient object created..." << CUtil::CLog::endl;

		// if we reached this point, we are able connect to tester. let's connect to evx stream now...
		// same issue here... i could have used stringstream but forced to use C style string because the damn EVXA class
		// wants a C style string argument that is not a constant!!!
		char szPid[1024] = "";
		sprintf(szPid, "client_%d", getpid());

		m_Log << "Connecting to " << strTesterName << "..." << CUtil::CLog::endl;
    		if(m_pEvxio->ConnectEvxioStreams(m_pTestHead, szPid) != EVXA::OK) continue;
    		else
		{
			// once the tester objects are created, let's wait until tester is ready
		  	while(!m_pTester->isTesterReady(m_nHead))
			{
				m_Log << "Tester NOT yet ready..." << CUtil::CLog::endl; 
				sleep(1);
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
	delete(m_pTester); m_pTester = 0;
	delete(m_pTestHead); m_pTestHead = 0;
	delete(m_pProgCtrl); m_pProgCtrl = 0;
	delete(m_pState); m_pState = 0;
	delete(m_pEvxio); m_pEvxio = 0;
} 

/* ------------------------------------------------------------------------------------------
application's loop
------------------------------------------------------------------------------------------ */
void CTester::loop()
{
	while(true)
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

		// wait here now for io operations from any of the fd's
		fd_set rdy = fds;
		if( select(nMaxFd, &rdy, NULL, NULL, NULL) < 0 ) perror("select() failed ");

		// handle io operations from state notifications.
		if((fdState > 0) && (FD_ISSET(fdState, &rdy))) 
		{
    			if (m_pState->respond(fdState) != EVXA::OK) 
			{
		      		const char *errbuf = m_pState->getStatusBuffer();
				m_Log << "Error occured on state notification IO operation: " << errbuf << CUtil::CLog::endl;
			      	return;
    			}  
  		}			
	}
/*
	while(true)
	{
		fd_set read_fd;
		int stateSockId = m_pState->getSocketId();
		int evxioSockId = m_pEvxio->getEvxioSocketId();
		int errorSockId = m_pEvxio->getErrorSocketId();

		FD_ZERO(&read_fd);
		FD_SET(fileno(stdin), &read_fd); //add input to select
		FD_SET(stateSockId, &read_fd); //add state notifications to select
		FD_SET(evxioSockId, &read_fd); // add evxio notifications to select
		FD_SET(errorSockId, &read_fd); // add error notifications to select

		int num_fds = ((stateSockId > evxioSockId) ? stateSockId : evxioSockId);
		num_fds = ((num_fds > errorSockId) ? num_fds + 1 : errorSockId +1);
			
		
 
		int num_ready; 
		if((num_ready = select(num_fds, &read_fd, NULL, NULL, NULL)) < 0) 
		{
			perror("main_serverLoop select failed ");
		} 

		// handle requests from stdin
		if(FD_ISSET(fileno(stdin), &read_fd)) 
		{
			char buf[1024] = "";
			read(fileno(stdin), buf, 1024);
			fprintf(stdout, "%s\n", buf);
		}

		//handle requests for state notifications.
		if((stateSockId > 0) && (FD_ISSET(stateSockId, &read_fd))) 
		{
			if(m_pState->respond(stateSockId) != EVXA::OK) 
			{
				const char *errbuf = m_pState->getStatusBuffer();
				fprintf(stdout, "ERROR state respond: %s\n", errbuf);
				return ;
			}  
		}

		//handle requests for evxio notifications.
		if((evxioSockId > 0) && (FD_ISSET(evxioSockId, &read_fd))) 
		{
			if(m_pEvxio->streamsRespond() != EVXA::OK) 
			{
		//    const char *errbuf = TesterObjects.evxioPtr->getStatusBuffer();
		//    fprintf(stdout, "ERROR stream respond: %s\n", errbuf);
		//     testerReconnect = 1;
				return ;
			}
		}
		if((errorSockId > 0) && (FD_ISSET(errorSockId, &read_fd))) 
		{//handle requests for evxio notifications.
			if(m_pEvxio->ErrorRespond() != EVXA::OK) 
			{
		//     const char *errbuf = TesterObjects.evxioPtr->getStatusBuffer();
		//     fprintf(stdout, "ERROR error respond: %s\n", errbuf);
		//     testerReconnect = 1;
			return ;
			}
		}
	}
*/
}



