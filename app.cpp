#include <app.h>
#include <unistd.h>

/* ------------------------------------------------------------------------------------------
constructor
------------------------------------------------------------------------------------------ */
CApp::CApp(int argc, char **argv)
{
	// initialize some of the members here to be safe
	m_pMonitorFileDesc = 0;
	m_pStateNotificationFileDesc = 0;
	m_pSummaryFileDesc = 0;
	m_pSTDFFileDesc = 0;
	m_pClientFileDesc = 0;
	m_lotinfo.clear();
	m_map.clear();
	m_bDebug = false;

	// remember this app's name
	m_szName = std::string(argv[0]);
   
	// parse command line arguments
	scan(argc, argv);

	m_pStateOnInit = new CAppState(*this, "onInit", &CApp::onInitLoadState);
	m_pStateOnIdle = new CAppState(*this, "onIdle", &CApp::onIdleLoadState, &CApp::onIdleUnloadState);
	m_pStateOnEndLot = new CAppState(*this, "onEndLot", &CApp::onEndLotLoadState, &CApp::onEndLotUnloadState);
	m_pStateOnUnloadProg = new CAppState(*this, "onUnloadProg", &CApp::onUnloadProgLoadState, &CApp::onUnloadProgUnloadState);
	m_pStateOnKillTester = new CAppState(*this, "onKillTester", &CApp::onKillTesterLoadState);
	m_pStateOnLaunch = new CAppState(*this, "onLaunch", &CApp::onLaunchLoadState, &CApp::onLaunchUnloadState);
	m_pSendLotInfo = new CAppState(*this, "onSetLotInfo", &CApp::onSetLotInfoLoadState);

	// ceate our UDP client file descriptor that will talk to interface server
	m_pClientFileDesc = new CAppClientUDPFileDesc(*this, &CApp::onReceiveFromServer);

	// add set the first active state 
	m_StateMgr.set(m_pStateOnInit);

	// run the state machine
	m_StateMgr.run(); 
}

/* ------------------------------------------------------------------------------------------
destructor
------------------------------------------------------------------------------------------ */
CApp::~CApp()
{
}

/* ------------------------------------------------------------------------------------------
STATE (load): onInit
------------------------------------------------------------------------------------------ */
void CApp::onInitLoadState(CState& state)
{
	CSequence* pSeq = new CSequence("seq0", true, true );
	pSeq->queue(new CAppTask(*this, &CApp::init, "init", 0, true, false));
	pSeq->queue(new CAppTask(*this, &CApp::setLogFile, "setLogFile", 200, true, false));
	pSeq->queue(new CSwitchTask(*this, *m_pStateOnIdle, 200, true));
	state.add(pSeq);
}

/* ------------------------------------------------------------------------------------------
STATE (load): onIdle
------------------------------------------------------------------------------------------ */
void CApp::onIdleLoadState(CState& state)
{
	// create the fd objects for both inotify and state notification. we need them both for this state
	m_pMonitorFileDesc = new CMonitorFileDesc(*this, &CApp::onReceiveFile, m_CONFIG.szLotInfoFilePath);
	m_pStateNotificationFileDesc = new CAppFileDesc(*this, &CApp::onStateNotificationResponse, m_pState? m_pState->getSocketId(): -1);
	if(m_CONFIG.bSummary)m_pSummaryFileDesc = new CMonitorFileDesc(*this, &CApp::onSummaryFile, m_CONFIG.szSummaryPath, IN_CREATE | IN_MOVED_TO);
	if(m_CONFIG.bZipSTDF || m_CONFIG.bRenameSTDF)m_pSTDFFileDesc = new CMonitorFileDesc(*this, &CApp::onWatchSTDF, m_CONFIG.szSTDFPath, IN_CREATE | IN_MOVED_TO);

	// add them to FD manager so we'll use them in select() task
	m_FileDescMgr.add( *m_pMonitorFileDesc );
 	m_FileDescMgr.add( *m_pStateNotificationFileDesc );
 	if (m_pClientFileDesc) m_FileDescMgr.add( *m_pClientFileDesc );
	if (m_CONFIG.bSummary) m_FileDescMgr.add( *m_pSummaryFileDesc );
	if(m_CONFIG.bZipSTDF || m_CONFIG.bRenameSTDF)m_FileDescMgr.add( *m_pSTDFFileDesc );

	state.add(new CAppTask(*this, &CApp::connect, "connect", 1000, true, true));
	state.add(new CAppTask(*this, &CApp::select, "select", 0, true, true));
	state.add(new CAppTask(*this, &CApp::listen, "listen", 1000, true, true));

	CSequence* pSeq = new CSequence("seq0", true, true );
	state.add(pSeq);
	pSeq->queue(new CAppTask(*this, &CApp::setLogFile, "setLogFile", 30000, true, true));
	pSeq->queue(new CAppTask(*this, &CApp::updateConfig, "updateConfig", 30000, true, true));
}

/* ------------------------------------------------------------------------------------------
STATE (unload): onIdle
------------------------------------------------------------------------------------------ */
void CApp::onIdleUnloadState(CState& state)
{
	// reset launch counter
	m_nLaunchAttempt = 0;

	// remove the FD objects (pointers)
	m_FileDescMgr.clear();

	// now delete FD objects 
	if (m_pMonitorFileDesc){ delete m_pMonitorFileDesc; m_pMonitorFileDesc = 0; }
	if (m_pSummaryFileDesc){ delete m_pSummaryFileDesc; m_pSummaryFileDesc = 0; }
	if (m_pSTDFFileDesc){ delete m_pSTDFFileDesc; m_pSTDFFileDesc = 0; }
	if (m_pStateNotificationFileDesc){ delete m_pStateNotificationFileDesc; m_pStateNotificationFileDesc = 0; }
}

/* ------------------------------------------------------------------------------------------
STATE (load): onEndLot
------------------------------------------------------------------------------------------ */
void CApp::onEndLotLoadState(CState& state)
{
	// create fd object only for state notification and add to FD manager
	m_pStateNotificationFileDesc = new CAppFileDesc(*this, &CApp::onStateNotificationResponse, m_pState? m_pState->getSocketId(): -1);
 	m_FileDescMgr.add( *m_pStateNotificationFileDesc );

	state.add(new CAppTask(*this, &CApp::select, "select", 0, true, true));
	state.add(new CAppTask(*this, &CApp::timeOutEndLot, "timeOutEndLot", m_CONFIG.nEndLotTimeOutMS, true, false));
	state.add(new CAppTask(*this, &CApp::endLot, "endLot", 0, true, false));
}

/* ------------------------------------------------------------------------------------------
STATE (unload): onEndLot
------------------------------------------------------------------------------------------ */
void CApp::onEndLotUnloadState(CState& state)
{
	// clear FD manager and delete the fd object we used for this state
	m_FileDescMgr.clear();
	if (m_pStateNotificationFileDesc){ delete m_pStateNotificationFileDesc; m_pStateNotificationFileDesc = 0; }
}

/* ------------------------------------------------------------------------------------------
STATE (load): onUnloadProg
------------------------------------------------------------------------------------------ */
void CApp::onUnloadProgLoadState(CState& state)
{
	// create fd object only for state notification and add to FD manager
	m_pStateNotificationFileDesc = new CAppFileDesc(*this, &CApp::onStateNotificationResponse, m_pState? m_pState->getSocketId(): -1);
 	m_FileDescMgr.add( *m_pStateNotificationFileDesc );

	state.add(new CAppTask(*this, &CApp::select, "select", 0, true, true));
	state.add(new CAppTask(*this, &CApp::timeOutUnloadProg, "timeOutUnloadProg", m_CONFIG.nUnloadProgTimeOutMS, true, false));
	state.add(new CAppTask(*this, &CApp::unloadProg, "unloadProg", 0, true, false));
}

/* ------------------------------------------------------------------------------------------
STATE (unload): onUnloadProg
------------------------------------------------------------------------------------------ */
void CApp::onUnloadProgUnloadState(CState& state)
{
	// clear FD manager and delete the fd object we used for this state
	m_FileDescMgr.clear();
	if (m_pStateNotificationFileDesc){ delete m_pStateNotificationFileDesc; m_pStateNotificationFileDesc = 0; }
}

/* ------------------------------------------------------------------------------------------
STATE (load): onKillTester
------------------------------------------------------------------------------------------ */
void CApp::onKillTesterLoadState(CState& state)
{
	CSequence* pSeq = new CSequence("seqKillTester", true, true );
	pSeq->queue(new CAppTask(*this, &CApp::killTester, "killTester", 0, true, false));
	pSeq->queue(new CAppTask(*this, &CApp::isTesterDead, "isTesterDead", 1000, true, true));
	state.add(pSeq);

	state.add(new CAppTask(*this, &CApp::timeOutKillTester, "timeOutKillTester", m_CONFIG.nKillTesterTimeOutMS, true, false));
}

/* ------------------------------------------------------------------------------------------
STATE (load): onLaunch
------------------------------------------------------------------------------------------ */
void CApp::onLaunchLoadState(CState& state)
{
	// create fd object only for state notification and add to FD manager
	m_pStateNotificationFileDesc = new CAppFileDesc(*this, &CApp::onStateNotificationResponse, m_pState? m_pState->getSocketId(): -1);
 	m_FileDescMgr.add( *m_pStateNotificationFileDesc );

	state.add(new CAppTask(*this, &CApp::connect, "connect", 1000, true, true));
	state.add(new CAppTask(*this, &CApp::select, "select", 0, true, true));
	state.add(new CAppTask(*this, &CApp::timeOutLaunch, "timeOutLaunch", m_CONFIG.nRelaunchTimeOutMS, false, false));
	state.add(new CAppTask(*this, &CApp::launch, "launch", 0, true, false));
}

/* ------------------------------------------------------------------------------------------
STATE (unload): onLaunch
------------------------------------------------------------------------------------------ */
void CApp::onLaunchUnloadState(CState& state)
{
	// clear FD manager and delete the fd object we used for this state
	m_FileDescMgr.clear();
	if (m_pStateNotificationFileDesc){ delete m_pStateNotificationFileDesc; m_pStateNotificationFileDesc = 0; }
}


/* ------------------------------------------------------------------------------------------
STATE (load): onSetLotInfo
------------------------------------------------------------------------------------------ */
void CApp::onSetLotInfoLoadState(CState& state)
{
	CSequence* pSeq = new CSequence("seq0", true, true );
	state.add(pSeq);

	// if sending lotinfo.txt STDF values to STDF file is enabled, let's add it in task sequence. we do it first
	if (m_CONFIG.bSendInfo) pSeq->queue(new CAppTask(*this, &CApp::setSTDF, "sendSTDFfromLotInfo", 1000, true, false));

	// add task that will wait for famodule to finish processing lotinfo file 
	pSeq->queue(new CAppTask(*this, &CApp::sendLotInfoToFAModule, "sendLotInfoToFAModule", 1000, true, true));

	// add task that will time-out wait for famodule
	state.add(new CAppTask(*this, &CApp::timeOutSendLotInfoToFAModule, "timeOutSendLotInfoToFAModule", m_CONFIG.nFAModuleTimeOutMS, true, false));
}

/* ------------------------------------------------------------------------------------------
TASK (load): onSetLotInfo
------------------------------------------------------------------------------------------ */
void CApp::sendLotInfoToFAModule(CTask& task)
{
	if (!m_pProgCtrl)
	{
		m_StateMgr.set(m_pStateOnIdle);
		return;
	}
	
	// if this feature is disabled, let's just move on to next state
	if (!m_CONFIG.bFAModule)
	{
		m_StateMgr.set(m_pStateOnIdle);
		return;
	}
	
	// create string that holds full path + monitor file
	std::stringstream ssFullPathMonitorName;
	ssFullPathMonitorName << m_CONFIG.szLotInfoFilePath << "/" << m_CONFIG.szLotInfoFileName;

	// let's try to read back token from famodule to find out if they are done.
	std::string s;
	m_pProgCtrl->faprocGet("Load Lot Info File", s);

	// if it's set to "Ready" then it's good to load the recipe file
	if (s.compare("Ready") == 0)
	{
		m_Log << "Looks like FAModule is ready to load the lotinfo file...: '" << s << "'" << CUtil::CLog::endl;
		// send the lotinfo file path to famodule
		if ( m_pProgCtrl) m_pProgCtrl->faprocSendCommand("Load Lot Info File", ssFullPathMonitorName.str());		
	}
	// if it's set to "OK" or starts with "NOK", then file loading has completed
	// either the good way or the bad way. We can leave this state in both cases
	else if (s.compare("OK") == 0 || s.compare(0, 3, "NOK") == 0)
	{
		if(s.compare("OK") == 0) m_Log << "Looks like FAModule is done...: '" << s << "'" << CUtil::CLog::endl;
		else m_Log << "Looks like FAModule had an issue loading the lotinfo file...: '" << s << "'" << CUtil::CLog::endl;

		// delete the lotinfo.txt
		if (m_CONFIG.bDeleteLotInfo) unlink(ssFullPathMonitorName.str().c_str());

		// move to next state
		m_StateMgr.set(m_pStateOnIdle);
		return;
	}
	else
	{
		m_Log << "FAModule is not done with " << m_CONFIG.szLotInfoFileName << " yet. let's wait..." << CUtil::CLog::endl;
		return;
	}
}

/* ------------------------------------------------------------------------------------------
TASK: this task sends all MIR, SDR, and all other possible STDF fields to STDF file
------------------------------------------------------------------------------------------ */
void CApp::setSTDF(CTask&)
{
	m_Log << "Sending lotinfo.txt values to STDF files..." << CUtil::CLog::endl;
	if (!setLotInfo()) m_Log << "WARNING: something went wrong in setting STDF fields. Please check." << CUtil::CLog::endl;
}

/* ------------------------------------------------------------------------------------------
TASK: 	if this is executed, time-out for wait on FAModule lotinfo.txt process is done
------------------------------------------------------------------------------------------ */
void CApp::timeOutSendLotInfoToFAModule(CTask& task)
{
	m_Log << "task(" << task.getName() << "): Wait for FAModule expired. Let's delete the " << m_CONFIG.szLotInfoFileName << " and move on to idle state." << CUtil::CLog::endl;

	// delete the lotinfo.txt
	std::stringstream ssFullPathMonitorName;
	ssFullPathMonitorName << m_CONFIG.szLotInfoFilePath << "/" << m_CONFIG.szLotInfoFileName;
	if ( m_CONFIG.bDeleteLotInfo) unlink(ssFullPathMonitorName.str().c_str());

	m_StateMgr.set(m_pStateOnIdle);
}


/* ------------------------------------------------------------------------------------------
TASK: unload program
------------------------------------------------------------------------------------------ */
void CApp::unloadProg(CTask& task)
{
	// not really part of this state's flow but for safety reasons, we do sanity check
	if (!isReady())
	{
		m_Log << "Not connected to tester..." << CUtil::CLog::endl;
		m_StateMgr.set(m_pStateOnKillTester);
		return;
	}

	// if program is not loaded, switch to kill state now
	if (!m_pProgCtrl->isProgramLoaded())
	{
		m_Log << "No program loaded. nothing to unload." << CUtil::CLog::endl;
		m_StateMgr.set(m_pStateOnKillTester);
		return;
	}
	// for version 20190918, if program is loaded and the same as the jobfile for new lotinfo.txt, we don't reload anymore.
	else
	{
		if (!m_CONFIG.bForceLoad)
		{
			// check if program loaded is same as jobfile from new lotinfo.txt. if yes, let
			if (getProgramFullPath().compare( m_lotinfo.szProgramFullPathName ) == 0)
			{
				m_StateMgr.set(m_pSendLotInfo);
				return;
			}
		}
	}

	// get program name to unload for logging later
	m_Log << "Program '" << m_pProgCtrl->getProgramPath() << "' is loaded. Let's unload it." << CUtil::CLog::endl; 

	// unload program with evxa command
	if (m_pProgCtrl->unload( EVXA::NO_WAIT, 0, true ) != EVXA::OK)
	{
		m_Log << "Error occured unloading test program." << CUtil::CLog::endl;
		m_StateMgr.set(m_pStateOnKillTester);
	}
}

/* ------------------------------------------------------------------------------------------
TASK: 	if this is executed, time-out for wait on program unload expired
------------------------------------------------------------------------------------------ */
void CApp::timeOutUnloadProg(CTask& task)
{
	m_Log << "task(" << task.getName() << "): Wait for program unload expired. moving on to kill state." << CUtil::CLog::endl;
	m_StateMgr.set(m_pStateOnKillTester);
}

/* ------------------------------------------------------------------------------------------
TASK: disconnect, kill unison execs - oicu, optool, dataviewer, bintool, etc...
------------------------------------------------------------------------------------------ */
void CApp::killTester(CTask& task)
{
	// in case we're connected from previous OICu load, let's make sure we're disconnected now.
	disconnect();

	// kill the following apps that has these names
	std::string apps[] = { "optool", "utlrt", "utlmon", "utlnic", "utlio_main", "utlinv", "faproc", "dataviewer", \
				"tad", "unisonif", "breakTool", "marginTool", "packageTool", "gnuplot440", "wafermapTool", "captureTool", \
				"logTool", "memoryTool", "patternDebugTool", "waveformTool", "patternSetupTool", "wfcellTool", "expressionTool", \
				"graphicalDebugTool", "patternTool", "axisTool", "errorTool", "flowTool", "pinStateTool", "realTimeStats", "binTool", \
				"ltxplot", "plotDataTool", "specTool", "bitmapTool", "consolidator", "levelsTool", "objectManagerTool", "plotViewTool", \
				 "TestTool", "oicu", "nicmond", "nicif", "evxio", "evtc", "tad", "cgem", "restart_tester" };



	for (unsigned i = 0; i < sizeof(apps)/sizeof(*apps); i++)
	{
		while ( CUtil::getFirstPIDByName(apps[i]) != -1 )
		{
			std::stringstream ssCmd;
			ssCmd << "./" << KILLAPPCMD << " " << apps[i];
			m_Log << "KILL: " << ssCmd.str() << CUtil::CLog::endl;		
			system(ssCmd.str().c_str());	
		}
	}

	// kill tester
	if (m_CONFIG.bKillTesterOnLaunch)
	{
		std::stringstream ssCmd;
		ssCmd << "./" << KILLTESTERCMD << " " << m_szTesterName;
		m_Log << "END TESTER: " << ssCmd.str() << CUtil::CLog::endl;		
		system(ssCmd.str().c_str());
	}

	// after killing unison threads, let's enable the task in current state that time-out wait for unison execs getting killed
	if ( m_StateMgr.get() == m_pStateOnKillTester)
	{
		CTask* pTask = m_pStateOnKillTester->get("onLaunch");
		if (pTask) pTask->enable(true);
	}
}

/* ------------------------------------------------------------------------------------------
TASK: check if tester is still running or not
------------------------------------------------------------------------------------------ */
void CApp::isTesterDead(CTask& task)
{
	if ( CUtil::getFirstPIDByName("optool") != -1)
	{
		m_Log << "WARNING: it seems like optool (" << CUtil::getFirstPIDByName("optool") << ") is still running after attempt to kill it." << CUtil::CLog::endl;
		return;
	}

	if ( CUtil::getFirstPIDByName("oicu") != -1)
	{
		m_Log << "WARNING: it seems like oicu (" << CUtil::getFirstPIDByName("optool") << ") is still running after attempt to kill it." << CUtil::CLog::endl;
		return;
	}
	m_Log << "Looks like OICU and/or optool is not running anymore. let's launch!" << CUtil::CLog::endl;

	// set state to launch 
	m_StateMgr.set(m_pStateOnLaunch);
}

/* ------------------------------------------------------------------------------------------
TASK: 	if this is executed, time-out for kill-teser has expired
------------------------------------------------------------------------------------------ */
void CApp::timeOutKillTester(CTask& task)
{
	m_Log << "task(" << task.getName() << "): Wait for kill-tester expired. moving on to launch state." << CUtil::CLog::endl;

	// set state to launch 
	m_StateMgr.set(m_pStateOnLaunch);
}

/* ------------------------------------------------------------------------------------------
TASK: update config parameters by reading config file
------------------------------------------------------------------------------------------ */
void CApp::updateConfig(CTask& task)
{
	m_CONFIG.parse( m_szConfigFullPathName );

	m_Log << "--------------------------------------------------------" << CUtil::CLog::endl;
	m_Log << "Version: " << VERSION << CUtil::CLog::endl;
	m_Log << "Developer: " << DEVELOPER << CUtil::CLog::endl;
	m_Log << "Tester: " << m_szTesterName << CUtil::CLog::endl; 
	m_Log << "Config File: " << m_szConfigFullPathName << CUtil::CLog::endl;
	m_CONFIG.print();
	m_Log << "--------------------------------------------------------" << CUtil::CLog::endl;
}

/* ------------------------------------------------------------------------------------------
TASK: update logger file output
------------------------------------------------------------------------------------------ */
void CApp::setLogFile(CTask& task)
{
	// if logger to file is disabled, let's quickly reset it and bail
	if (!m_CONFIG.bLogToFile)
	{
		m_Log.file("");
		return;
	}

	// get the host name
	char szHostName[32];
	gethostname(szHostName, 32);

	// get date stamp
	struct timeval tv;
	gettimeofday(&tv, NULL);
	struct tm* tmNow = localtime(&tv.tv_sec);				

	// set log file to <path>/apl.<hostname>.<yyyymmdd>.log
	std::stringstream ssLogToFile;
	ssLogToFile << m_CONFIG.szLogPath << "/apl." << VERSION << "." << szHostName << "." << (tmNow->tm_year + 1900);
	ssLogToFile << (tmNow->tm_mon + 1 < 10? "0" : "") << (tmNow->tm_mon + 1) << (tmNow->tm_mday < 10? "0" : "") << tmNow->tm_mday << ".log";
	
	// if this new log file name the same one already set in the logger? if yes, then we don't have to do anything
	if (ssLogToFile.str().compare( m_Log.file() ) == 0) return;

	// otherwise, set this 
	m_Log.file(ssLogToFile.str());
	m_Log << "Log is saved to " << ssLogToFile.str() << CUtil::CLog::endl;
}

/* ------------------------------------------------------------------------------------------
TASK: to perform initialization
------------------------------------------------------------------------------------------ */
void CApp::init(CTask& task)
{
	// if tester name is not set through command line argument, assign default name
	if (m_szTesterName.empty())
	{
		std::stringstream ss;
		getUserName().empty()? (ss << "sim") : (ss << getUserName() << "_sim");
		m_szTesterName = ss.str();
	}

	// if config file is not set, assign default
	if (m_szConfigFullPathName.empty()){ m_szConfigFullPathName = "./config.xml"; }

	// parse config file
	m_CONFIG.parse( m_szConfigFullPathName );
	m_Log << "--------------------------------------------------------" << CUtil::CLog::endl;
	m_Log << "Version: " << VERSION << CUtil::CLog::endl;
	m_Log << "Developer: " << DEVELOPER << CUtil::CLog::endl;
	m_Log << "Tester: " << m_szTesterName << CUtil::CLog::endl; 
	m_Log << "Config File: " << m_szConfigFullPathName << CUtil::CLog::endl;
	m_CONFIG.print();
	m_Log << "--------------------------------------------------------" << CUtil::CLog::endl;

	// if we're running popup server, let's launch it
	if (m_CONFIG.bPopupServer)
	{
		// launch popup server. launching it now forces other instance of this server to be killed off
		std::stringstream ssCmd;
		ssCmd << "./" << POPUPSERVER << " " << m_CONFIG.szServerIP << " " << m_CONFIG.nServerPort << " " << m_szName << " " << (m_bDebug? "DEBUG ":" ") << "&";
		m_Log << "Launching pop-up server... " << ssCmd.str() << CUtil::CLog::endl;		
		system(ssCmd.str().c_str());		

		// check if popup server is running. launch again if not.
		while ( CUtil::getFirstPIDByName(POPUPSERVER) == -1 )
		{
			m_Log << "pop-up server is not running, attemping to launch again... " << ssCmd.str() << CUtil::CLog::endl;		
			sleep(1);
			system(ssCmd.str().c_str());		
		}

		// connect to popup server, send message to popup server to "register" to it
		m_pClientFileDesc->connect(m_CONFIG.szServerIP,  m_CONFIG.nServerPort);
		sleep(1);
		m_pClientFileDesc->send("APL_REGISTER|init");
	}
}

/* ------------------------------------------------------------------------------------------
TASK: 	attempts to connect to tester
------------------------------------------------------------------------------------------ */
void CApp::connect(CTask& task)
{
	// are we connected to tester? should we try? attempt once
	if (!isReady()) CTester::connect(m_szTesterName, 1);
}

/* ------------------------------------------------------------------------------------------
TASK: 	query select(), for input from inotify of incoming file
------------------------------------------------------------------------------------------ */
void CApp::select(CTask& task)
{
	// before calling select(), update file descriptors of evxa objects. this is to ensure the FD manager
	// don't use a bad file descriptor. when evxa objects are destroyed, their file descriptors are considered bad.
	if (m_pStateNotificationFileDesc) m_pStateNotificationFileDesc->set(m_pState? m_pState->getSocketId(): -1);

	// proccess any file descriptor notification on select every second.
	m_FileDescMgr.select(200);
}

/* ------------------------------------------------------------------------------------------
TASK: 	query listen, listen to popup server
------------------------------------------------------------------------------------------ */
void CApp::listen(CTask& task)
{
	if (m_CONFIG.bPopupServer)
	{
		// check if popup server is running. launch again if not.
		if ( CUtil::getFirstPIDByName(POPUPSERVER) == -1 )
		{
			while ( CUtil::getFirstPIDByName(POPUPSERVER) == -1 )
			{
				std::stringstream ssCmd;
				ssCmd << "./" << POPUPSERVER << " " << m_CONFIG.szServerIP << " " << m_CONFIG.nServerPort << " " << m_szName << " " << (m_bDebug? "DEBUG ":" ") << "&";
				m_Log << "pop-up server is not running, attemping to launch again... " << ssCmd.str() << CUtil::CLog::endl;		
				sleep(1);
				system(ssCmd.str().c_str());		
			}

			// connect to popup server, send message to popup server to "register" to it. 
			// we do this everytime server has to launch
			m_pClientFileDesc->connect(m_CONFIG.szServerIP,  m_CONFIG.nServerPort);
			sleep(1);
			m_pClientFileDesc->send("APL_REGISTER|idle");
		}
		//else m_pClientFileDesc->send("APL_PING");
	}
}

/* ------------------------------------------------------------------------------------------
utility: process all UPD message from APL server
------------------------------------------------------------------------------------------ */
void CApp::onReceiveFromServer(const std::string& msg)
{
	if ( msg.compare("APL_ACKNOWLEDGE") == 0){ m_Log << "Connection to APL Commander is GOOD"<< CUtil::CLog::endl; }
	else if ( msg.compare("LOADLOTINFO") == 0)
	{
		processLotInfoFile(m_CONFIG.szLotInfoFileName);
	}
	else{}
}


/* ------------------------------------------------------------------------------------------
TASK: 	end lot only if program exists
------------------------------------------------------------------------------------------ */
void CApp::endLot(CTask& task)
{
	// are we connected? if not, let's do a 1 attempt to connect, if tester exists then we 
	// should be able to connect at this time.
	if (!isReady())
	{
		m_Log << "We're not connected to tester. let's try connecting before killing it..." << CUtil::CLog::endl;
		CTester::connect(m_szTesterName, 1);
	}
	
	// if we're still not connected to tester at this point, then we can safely assume tester
	// is not running. we don't need to end lot or unload program at all
	if (!isReady() || !m_pProgCtrl)
	{
		m_Log << "We're still not connected to tester. It is now safe to assume that tester isn't running at all.Le's go kill and launch now..." << CUtil::CLog::endl;		
		m_StateMgr.set(m_pStateOnUnloadProg);
		return;
	}
	else
	{
		// is program loaded?
		if (m_pProgCtrl->isProgramLoaded())
		{
			m_Log << "Program '" << m_pProgCtrl->getProgramPath() << "' is loaded. Let's end lot first before unloading it." << CUtil::CLog::endl; 

			// pause CURI so it stops unison from testing next cycle after ending lot
			m_pProgCtrl->faprocInvoke("suspend");

			// is there a lot being tested? if yes, let's end the lot.
			if (m_pProgCtrl->setEndOfLot(EVXA::NO_WAIT, true) != EVXA::OK)
			{
				m_Log << "ERROR: something went wrong in ending lot..." << CUtil::CLog::endl;
				return;
			}
	
			m_Log << "end of lot/wafer done" << CUtil::CLog::endl;

			// is there a lot being tested? if yes, let's end the lot.
			if (m_pProgCtrl->setEndOfWafer(EVXA::NO_WAIT) != EVXA::OK)
			{
				m_Log << "ERROR: something went wrong in ending lot..." << CUtil::CLog::endl;
				return;
			}			


			m_Log << "end of lot/wafer done" << CUtil::CLog::endl;
		}
		else
		{
			m_Log << "There's no program loaded. We can safely kill tester now, before launching." << CUtil::CLog::endl;
			m_StateMgr.set(m_pStateOnUnloadProg);
			return;
		}
	}
}

/* ------------------------------------------------------------------------------------------
TASK: 	if this is executed, time-out for wait on end-lot/wafer event expired
------------------------------------------------------------------------------------------ */
void CApp::timeOutEndLot(CTask& task)
{
	m_Log << "task(" << task.getName() << "): Wait for end lot/wafer expired. moving on to program unload state." << CUtil::CLog::endl;
	m_StateMgr.set(m_pStateOnUnloadProg);
}


/* ------------------------------------------------------------------------------------------
TASK: launch OICu
------------------------------------------------------------------------------------------ */
void CApp::launch(CTask& task)
{
	// have we made more launch attempts that we're allowed?
	if (m_nLaunchAttempt >= m_CONFIG.nRelaunchAttempt)
	{
		m_Log << "WARNING: APL has already made " << (m_nLaunchAttempt) << " attempts to launch OICu and load program since receiving ";
		m_Log << m_CONFIG.szLotInfoFileName << " file. We'll stop now. Check for issue please." << CUtil::CLog::endl;
		m_StateMgr.set(m_pStateOnIdle);
		return;
	}

	// let's enable and reset the launch time-out task. it's countdown must always start on every launch attempt
	if ( m_StateMgr.get() == m_pStateOnLaunch)
	{
		CTask* pTask = m_pStateOnLaunch->get("timeOutLaunch");
		if (pTask)
		{
			pTask->enable(true);
			pTask->load(); // reset its timer
		}
	}

	// setup system command to launch OICu
	m_Log << "launching(" << (m_nLaunchAttempt) << ") OICu and loading '" << m_lotinfo.szProgramFullPathName << "'..." << CUtil::CLog::endl;

	// let's incrementer launch attempt
	m_nLaunchAttempt++;

	// >launcher -nodisplay -prod -load <program> -T <tester> -qual
	std::stringstream ssCmd;
	ssCmd.str(std::string());
	ssCmd.clear();
	
	ssCmd << "UNISON_NEWLOT_CONFIG_FILE=" << m_CONFIG.szNewLotConfigPath << "/" << m_CONFIG.szNewLotConfigFile;
	if (m_lotinfo.szCustomer.compare("QUALCOMM") == 0) ssCmd << ".xml ";
	else if (m_lotinfo.szCustomer.empty() && (m_CONFIG.szCustomer.compare("QUALCOMM") == 0) ) ssCmd << ".xml ";
	else ssCmd << "_" << m_lotinfo.mir.ProcId << ".xml ";
	ssCmd << "launcher -nodisplay " << (m_CONFIG.bProd? "-prod " : "");
	ssCmd << (m_lotinfo.szProgramFullPathName.empty()? "": "-load ") << (m_lotinfo.szProgramFullPathName.empty()? "" : m_lotinfo.szProgramFullPathName);
	ssCmd << " -qual " << " -T " << m_szTesterName;// << "&";
	system(ssCmd.str().c_str());	
	m_Log << "LAUNCH: " << ssCmd.str() << CUtil::CLog::endl;
}

/* ------------------------------------------------------------------------------------------
TASK: 	if this is executed, time-out for launch OICu and load program expired
------------------------------------------------------------------------------------------ */
void CApp::timeOutLaunch(CTask& task)
{
	// not really part of this state's flow but for safety reasons, we do sanity check
	if (!isReady())
	{
		m_Log << "Launch time-out expired and we're not even connected to tester. Let's try to launch again..." << CUtil::CLog::endl;
		m_StateMgr.set(m_pStateOnKillTester);
		return;
	}

	// if program is not loaded, switch to kill state now
	if (!m_pProgCtrl->isProgramLoaded())
	{
		m_Log << "Launch time-out expired and no program is loaded. Let's try to launch again..." << CUtil::CLog::endl;
		m_StateMgr.set(m_pStateOnKillTester);
		return;
	}

	// get program name to unload for logging later
	m_Log << "Launch time-out expired but we found that program '" << m_pProgCtrl->getProgramPath() << "' is loaded." << CUtil::CLog::endl; 
	m_StateMgr.set(m_pStateOnIdle);
}

/* ------------------------------------------------------------------------------------------
utility: parse command line arguments
------------------------------------------------------------------------------------------ */
bool CApp::scan(int argc, char **argv)
{
	// move all args into list
	std::list< std::string > args;
	for (int i = 1; i < argc; i++) args.push_back( argv[i] );

	// loop through all args
	for (std::list< std::string >::iterator it = args.begin(); it != args.end(); it++)
	{
		// look for -tester arg. if partial match is at first char, we found it
		std::string s("-tester");
		if (s.find(*it) == 0)
		{
			// expect the next arg (and it must exist) to be tester name
			it++;
			if (it == args.end())
			{
				m_Log << "ERROR: " << s << " argument found but option is missing." << CUtil::CLog::endl;
				return false;
			}
			else
			{
				m_szTesterName = *it;
				continue;
			}
		}	
		// look for -config arg. if partial match is at first char, we found it
		s = "-config";
		if (s.find(*it) == 0)
		{
			// expect the next arg (and it must exist) to be path/name of the config file
			it++;
			if (it == args.end())
			{
				m_Log << "ERROR: " << s << " argument found but option is missing." << CUtil::CLog::endl;
				return false;
			}
			else
			{
				m_szConfigFullPathName = *it;
				continue;
			}	
		}
		// look for -debug arg. if partial match is at first char, we found it
		s = "-debug";
		if (s.find(*it) == 0)
		{
			m_bDebug = true;
			continue;
		}
	}	
	return true;
}

/* ------------------------------------------------------------------------------------------
utility: get user name
------------------------------------------------------------------------------------------ */
const std::string CApp::getUserName() const
{ 
	uid_t uid = getuid();
	char buf_passw[1024];     
	struct passwd password;
	struct passwd *passwd_info;

	getpwuid_r(uid, &password, buf_passw, 1024, &passwd_info);
	return std::string(passwd_info->pw_name);
} 

/* ------------------------------------------------------------------------------------------
Utility: parse and update lotinfo.txt file
------------------------------------------------------------------------------------------ */
void CApp::processLotInfoFile(const std::string& name)
{
	// create string that holds full path + monitor file 
	std::stringstream ssFullPathMonitorName;
	ssFullPathMonitorName << m_CONFIG.szLotInfoFilePath << "/" << name;

	// if this is the file, let's parse it.
	// does it contain JobFile field? if yes, does it point to a valid unison program? 
	// if yes, let's use this file contents to load program
	// from here on, we switch to launch OICu and load program state
	if (!parse(ssFullPathMonitorName.str()))
	{
		m_Log << ssFullPathMonitorName.str() << " file received but didn't find a program to load.";
		m_Log << " can you check if " << name << " has '" << m_CONFIG.fields.getValue("JOBFILE") << "' field. and is valid?" << CUtil::CLog::endl;
		return;
	}
	else m_Log << "successfully parsed '" << ssFullPathMonitorName.str() << "'" << CUtil::CLog::endl;

	// if you reach this point, m_lotinfo object must now contain data from latest lotinfo.txt file received.
	// does lotinfo.txt file contain STEP field? if yes, we have to handle it
	if (m_CONFIG.bStep) updateLotinfoFile(ssFullPathMonitorName.str());

	// let's halt this state now to make sure no other tasks from this state gets executed anymore
	if ( m_StateMgr.get() ) m_StateMgr.get()->halt();

	// we also move on to "end lot" state and end the lot that is currently running, if any.	
	m_StateMgr.set(m_pStateOnEndLot);

	// at this point, it is now safe to append lotinfo.txt file with GDR stuff. notify object won't be triggered anymore 
	// because we already halted this state and we are also moving to next state after this.

	// Ok, need to append APL name and version to the lotinfo file so we can push it to GDR.AUTOMATION
	// Need to wait for a while to avoid writing the file while the OS may be copying it.
	// Not so elegant, but it does the job, so sleep 1 second.
	sleep(1);
	std::ofstream lotInfoFile;
	lotInfoFile.open(ssFullPathMonitorName.str().c_str(), std::ofstream::out | std::ofstream::app);
	if(lotInfoFile.good()) 
	{
		lotInfoFile << "AutomationNam " << DELIMITER << " " << "APL" << std::endl;
		lotInfoFile << "AutomationVer " << DELIMITER << " " << VERSION << std::endl;
		lotInfoFile << "LoaderApiNam "  << DELIMITER << " " << "APL" << std::endl;
		lotInfoFile << "LoaderApiRev "  << DELIMITER << " " << VERSION << std::endl;
		lotInfoFile.close();
		m_Log << "Added GDR.AUTOMATION data to " << ssFullPathMonitorName.str().c_str() << CUtil::CLog::endl;
	}
	else 
	{
		m_Log << "Could not open " << ssFullPathMonitorName.str().c_str() << " to add GDR.AUTOMATION data." << CUtil::CLog::endl;
	}

}

/* ------------------------------------------------------------------------------------------
Utility: check the file if this is lotinfo.txt; parse if yes
------------------------------------------------------------------------------------------ */
void CApp::onReceiveFile(const std::string& name)
{
	// is this the file we're waiting for? if not, bail out
	if (name.compare(m_CONFIG.szLotInfoFileName) != 0)
	{
		m_Log << "File received but is not what we're waiting for: " << name << CUtil::CLog::endl;
		return;
	}
	else m_Log << "'" << name << "' file received." << CUtil::CLog::endl;

	// if parsing lotinfo file is success, let's tell notify fd object to stop processing any incoming select() at this point
	m_pMonitorFileDesc->halt();

	// is tester even ready? if no, we're safe to process this lotinfo.txt file
	if (!isReady() || !m_pProgCtrl)
	{
		processLotInfoFile(name);
		return;
	}
	else
	{
		// is program loaded?, if no, we're safe to process this lotinfo.txt file
		if (!m_pProgCtrl->isProgramLoaded())
		{
			processLotInfoFile(name);
			return;
		}
		// if program is loaded, then we need to ask operator first if we wish to proceed
		else
		{
			// if popup server is disabled, we proceed to load lotinfo.txt
			if(m_CONFIG.bPopupServer)
			{
				std::stringstream ss;
				ss << "LOADLOTINFO|" << name << "|" << m_pProgCtrl->getProgramPath();
				m_pClientFileDesc->send( ss.str() );
				return;
			}
			else
			{
				processLotInfoFile(name);
				return;
			}
		}	
	}
	return;
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
	}  		
}

/* ------------------------------------------------------------------------------------------
parse config xml file
------------------------------------------------------------------------------------------ */
bool CApp::CONFIG::parse(const std::string& file)
{
	m_Log << "Parsing " << file << "..." << CUtil::CLog::endl;

	// open xml config file and try to extract root tag	
	XML_Node *root = new XML_Node (file.c_str());
	if (root)
	{
		// since it's confirmed we found a valid config file, let's reset values to default first
		clear();

		std::string tag = CUtil::toUpper(root->fetchTag());
		if (tag.compare("APL_CONF") != 0)
		{
			m_Log << "Error: Invalid root tag '" << root->fetchTag() << "' found. Please check your '" << file << "'." << CUtil::CLog::endl;
			return false;
		}
		
		// let's find <CurrentSiteConfiguration> on <APL_CONF>'s nodes 
		XML_Node *pCurrSiteConfig = root->fetchChild( "CurrentSiteConfiguration" );
		if ( !pCurrSiteConfig ) 
		{
			m_Log << "Error: did not find <CurrentSiteConfiguration> in '" << file << "'." << CUtil::CLog::endl;
			return false;
		}
		else m_Log << "<CurrentSiteConfiguration> : '" << pCurrSiteConfig->fetchText() << "'" << CUtil::CLog::endl;

		// let's find the <SiteConfiguration> that matches <CurrentSiteConfiguration>
		XML_Node *pConfig = 0;
		//m_Log << "<SiteConfiguration> : ";
		for( int i = 0; i < root->numChildren(); i++ )
		{
			// only <SiteConfiguration> is handled, we ignore everything else
			if (!root->fetchChild(i)) continue;
			if (root->fetchChild(i)->fetchTag().compare("SiteConfiguration") != 0) continue;

		//	m_Log << "'" << root->fetchChild(i)->fetchVal("name") <<  "', ";

			// we take only the first <CurrentSiteConfiguration> match, ignore succeeding ones
			if ( root->fetchChild(i)->fetchVal("name").compare( pCurrSiteConfig->fetchText() ) == 0 ){ if (!pConfig) pConfig = root->fetchChild(i); }
		}
//		m_Log << CUtil::CLog::endl;
	
		// bail out if we didn't find <SiteConfiguration> with <CurrentSiteConfiguration>
		if (!pConfig)
		{
			m_Log << "Error: did not find <SiteConfiguration> with '" << pCurrSiteConfig->fetchText() << "' attribute." << CUtil::CLog::endl;
			return false;
		}	

		// let's now find the <Binning> from <SiteConfiguration> 
		XML_Node* pLaunch = pConfig->fetchChild("Launch");
		if (pLaunch)
		{ 
			for (int i = 0; i < pLaunch->numChildren(); i++)
			{
				if (!pLaunch->fetchChild(i)) continue;
				if (pLaunch->fetchChild(i)->fetchTag().compare("Param") != 0) continue;

				if (pLaunch->fetchChild(i)->fetchVal("name").compare("Type") == 0){ bProd = CUtil::toUpper(pLaunch->fetchChild(i)->fetchText()).compare("PROD") == 0? true: false; }					
				if (pLaunch->fetchChild(i)->fetchVal("name").compare("Force Load") == 0){ bForceLoad = CUtil::toUpper(pLaunch->fetchChild(i)->fetchText()).compare("TRUE") == 0? true: false; }
				if (pLaunch->fetchChild(i)->fetchVal("name").compare("Kill Tester Before Launch") == 0){ bKillTesterOnLaunch = pLaunch->fetchChild(i)->fetchText().compare("true") == 0? true: false; }					
				if (pLaunch->fetchChild(i)->fetchVal("name").compare("Wait Time To Launch") == 0){ nRelaunchTimeOutMS = CUtil::toLong( pLaunch->fetchChild(i)->fetchText() ) * 1000; }					
				if (pLaunch->fetchChild(i)->fetchVal("name").compare("Max Attempt To Launch") == 0){ nRelaunchAttempt = CUtil::toLong( pLaunch->fetchChild(i)->fetchText() ); }				
				if (pLaunch->fetchChild(i)->fetchVal("name").compare("Wait Time To End Lot") == 0){ nEndLotTimeOutMS = CUtil::toLong( pLaunch->fetchChild(i)->fetchText() ) * 1000; }					
				if (pLaunch->fetchChild(i)->fetchVal("name").compare("Wait Time To Unload Program") == 0){ nUnloadProgTimeOutMS = CUtil::toLong( pLaunch->fetchChild(i)->fetchText() ) * 1000; }					
				if (pLaunch->fetchChild(i)->fetchVal("name").compare("Wait Time To FAModule") == 0){ nFAModuleTimeOutMS = CUtil::toLong( pLaunch->fetchChild(i)->fetchText() ) * 1000; }					
				if (pLaunch->fetchChild(i)->fetchVal("name").compare("Wait Time To Kill Tester") == 0){ nKillTesterTimeOutMS = CUtil::toLong( pLaunch->fetchChild(i)->fetchText() ) * 1000; }					
				if (pLaunch->fetchChild(i)->fetchVal("name").compare("NewLot Config Path") == 0){ szNewLotConfigPath = pLaunch->fetchChild(i)->fetchText(); }					
				if (pLaunch->fetchChild(i)->fetchVal("name").compare("NewLot Config File") == 0){ szNewLotConfigFile = pLaunch->fetchChild(i)->fetchText(); }					
				if (pLaunch->fetchChild(i)->fetchVal("name").compare("Customer") == 0){ szCustomer = CUtil::toUpper(pLaunch->fetchChild(i)->fetchText()); }					
				if (pLaunch->fetchChild(i)->fetchVal("name").compare("Test Site") == 0){ szTestSite = CUtil::toUpper(pLaunch->fetchChild(i)->fetchText()); }					
				if (pLaunch->fetchChild(i)->fetchVal("name").compare("Supplier") == 0){ szSupplier = CUtil::toUpper(pLaunch->fetchChild(i)->fetchText()); }					
			}
		}
		else m_Log << "Warning: Didn't find <Launch>. " << CUtil::CLog::endl;

		// let's now find the <Binning> from <SiteConfiguration> 
		XML_Node* pBinning = 0;
		if (pConfig->fetchChild("Binning")){ if ( CUtil::toUpper( pConfig->fetchChild("Binning")->fetchVal("state") ).compare("TRUE") == 0) pBinning = pConfig->fetchChild("Binning"); }
		else m_Log << "Warning: Didn't find <Binning>. " << CUtil::CLog::endl;

		// check if <logging> is set
		XML_Node* pLogging = 0;
		if (pConfig->fetchChild("Logging"))
		{ 
			if ( CUtil::toUpper( pConfig->fetchChild("Logging")->fetchVal("state") ).compare("TRUE") == 0) pLogging = pConfig->fetchChild("Logging"); 
		}
		else m_Log << "Warning: Didn't find <Logging>. " << CUtil::CLog::endl;

		// check if <LotInfo> is set
		XML_Node* pLotInfo = pConfig->fetchChild("LotInfo");
		if (pLotInfo)
		{
			if (pLotInfo->fetchChild("File")){ szLotInfoFileName = pLotInfo->fetchChild("File")->fetchText(); }
			if (pLotInfo->fetchChild("Path")){ szLotInfoFilePath = pLotInfo->fetchChild("Path")->fetchText(); }		
			if (pLotInfo->fetchChild("Delete")){ bDeleteLotInfo = CUtil::toUpper(pLotInfo->fetchChild("Delete")->fetchText()).compare("TRUE") == 0? true: false; }
			if (pLotInfo->fetchChild("STDF")){ bSendInfo = CUtil::toUpper(pLotInfo->fetchChild("STDF")->fetchText()).compare("TRUE") == 0? true: false; }

			if (pLotInfo->fetchChild("Field"))
			{
				XML_Node* pField = pLotInfo->fetchChild("Field");
				for (int i = 0; i < pField->numChildren(); i++)
				{
					if (!pField->fetchChild(i)) continue;
					if (pField->fetchChild(i)->fetchTag().compare("Param") != 0) continue;

					// try mapping lotinfo field to MIR
					if ( mir.setValue( pField->fetchChild(i)->fetchVal("name"), CUtil::toUpper( pField->fetchChild(i)->fetchText()) ) )
					{
					}
					// if fail on MIR, try on SDR
					else if ( sdr.setValue( pField->fetchChild(i)->fetchVal("name"), CUtil::toUpper( pField->fetchChild(i)->fetchText()) ) )
					{
					}
					// if fail on SDR, try generic field
					else if ( fields.setValue( pField->fetchChild(i)->fetchVal("name"), CUtil::toUpper( pField->fetchChild(i)->fetchText()) ) )
					{
					}
					// if does not exist on SDR, MIR, and existing generic field list, let's add it to generic field list			
					else 
					{
						fields.add( pField->fetchChild(i)->fetchVal("name"), CUtil::toUpper( pField->fetchChild(i)->fetchText()) );
						m_Log << "NOTICE: Didn't find field value pair for '" << pField->fetchChild(i)->fetchVal("name");
						m_Log << "'. adding to generic field list as '" << CUtil::toUpper( pField->fetchChild(i)->fetchText()) << "'" << CUtil::CLog::endl;
					}
				}
			}
			

			if (pLotInfo->fetchChild("Step"))
			{
				bStep = false; // disabled by default
				if ( CUtil::toUpper( pLotInfo->fetchChild("Step")->fetchVal("state") ).compare("TRUE") == 0) bStep = true;

				for( int i = 0; i < pLotInfo->fetchChild("Step")->numChildren(); i++ )
				{
					if (!pLotInfo->fetchChild("Step")->fetchChild(i)) continue; // skip invalid object
					if (pLotInfo->fetchChild("Step")->fetchChild(i)->fetchTag().compare("Param") != 0 ) continue; // skip tags that are not "Param"
					if (!pLotInfo->fetchChild("Step")->fetchChild(i)->fetchText().size() ) continue; // skip <Param> with empty values
				
					// if this step value already exist in the list, we ignore this. we can only have unique step values
					bool bFound = false;
					for (unsigned int j = 0; j < steps.size(); j++)
					{
						if (steps[j].szStep.compare( CUtil::toUpper(pLotInfo->fetchChild("Step")->fetchChild(i)->fetchText() )) == 0 )
						{
							bFound = true;
							break;
						}
					}
					if (!bFound)
					{
						STEP s;
						s.szStep = CUtil::toUpper(pLotInfo->fetchChild("Step")->fetchChild(i)->fetchText());
						s.szFlowId = CUtil::toUpper(pLotInfo->fetchChild("Step")->fetchChild(i)->fetchVal("flow_id"));
						s.nRtstCod = CUtil::toLong(pLotInfo->fetchChild("Step")->fetchChild(i)->fetchVal("rtst_cod"));
						s.szCmodCod = CUtil::toUpper(pLotInfo->fetchChild("Step")->fetchChild(i)->fetchVal("cmod_cod"));
						if (s.nRtstCod > 255) m_Log << "ERROR! RTST_COD value found in " << file << " is " <<  s.nRtstCod << ". acceptable values are 0-255." << CUtil::CLog::endl;
						else steps.push_back(s);
					}
				}
			}
			else m_Log << "Warning: Didn't find <Step>. " << CUtil::CLog::endl;
		}
		else m_Log << "Warning: Didn't find <LotInfo>. " << CUtil::CLog::endl;

		// check if <Summary> is set
		if (pConfig->fetchChild("GUI"))
		{
			bPopupServer = CUtil::toUpper( pConfig->fetchChild("GUI")->fetchVal("state") ).compare("TRUE") == 0? true : false;

			if (pConfig->fetchChild("GUI")->fetchChild("IP")){ szServerIP = pConfig->fetchChild("GUI")->fetchChild("IP")->fetchText(); }		
			if (pConfig->fetchChild("GUI")->fetchChild("Port")){ nServerPort = CUtil::toLong(pConfig->fetchChild("GUI")->fetchChild("Port")->fetchText()); }		
		}

		// check if <Summary> is set
		if (pConfig->fetchChild("Summary"))
		{
			bSummary = false; // disabled by default
			if ( CUtil::toUpper( pConfig->fetchChild("Summary")->fetchVal("state") ).compare("TRUE") == 0) bSummary = true;

			if (pConfig->fetchChild("Summary")->fetchChild("Path")){ szSummaryPath = pConfig->fetchChild("Summary")->fetchChild("Path")->fetchText(); }		
		}
		else m_Log << "Warning: Didn't find <Summary>. " << CUtil::CLog::endl;

		// lets extract STDF stuff
		if (pConfig->fetchChild("STDF"))
		{
			if (pConfig->fetchChild("STDF")->fetchChild("Path")){ szSTDFPath = pConfig->fetchChild("STDF")->fetchChild("Path")->fetchText(); }		
			if (pConfig->fetchChild("STDF")->fetchChild("Rename"))
			{ 
				bRenameSTDF = CUtil::toUpper(pConfig->fetchChild("STDF")->fetchChild("Rename")->fetchVal("state")).compare("TRUE") == 0? true: false;
				szRenameSTDFFormat = CUtil::toUpper(pConfig->fetchChild("STDF")->fetchChild("Rename")->fetchVal("format"));
				bTimeStampIsEndLot = CUtil::toUpper(pConfig->fetchChild("STDF")->fetchChild("Rename")->fetchVal("timestamp")).compare("END") == 0? true: false;
			}
			if (pConfig->fetchChild("STDF")->fetchChild("Compress"))
			{	
				bZipSTDF = CUtil::toUpper(pConfig->fetchChild("STDF")->fetchChild("Compress")->fetchVal("state")).compare("TRUE") == 0? true: false;
				if (pConfig->fetchChild("STDF")->fetchChild("Compress")->fetchVal("cmd").size()) szZipSTDFCmd = pConfig->fetchChild("STDF")->fetchChild("Compress")->fetchVal("cmd");
				if (pConfig->fetchChild("STDF")->fetchChild("Compress")->fetchVal("ext").size()) szZipSTDFExt = pConfig->fetchChild("STDF")->fetchChild("Compress")->fetchVal("ext");
			}		
		}
		// lets extract FAModule stuff
		if (pConfig->fetchChild("FAMODULE"))
		{
			bFAModule = CUtil::toUpper(pConfig->fetchChild("FAMODULE")->fetchVal("state")).compare("TRUE") == 0? true: false;
		}

		// if we found binning node, it means <Binning> is in config which means binning@EOT is enabled
		if (pBinning)
		{
			bSendBin = true;
			if (pBinning->fetchChild("BinType"))
			{
				if (CUtil::toUpper(pBinning->fetchChild("BinType")->fetchText()).compare("HARD") == 0) bUseHardBin = true;
				if (CUtil::toUpper(pBinning->fetchChild("BinType")->fetchText()).compare("SOFT") == 0) bUseHardBin = false;
			}
			if (pBinning->fetchChild("TestType"))
			{
				if (CUtil::toUpper(pBinning->fetchChild("TestType")->fetchText()).compare("WAFER") == 0) nTestType = CONFIG::APL_WAFER;
				if (CUtil::toUpper(pBinning->fetchChild("TestType")->fetchText()).compare("FINAL") == 0) nTestType = CONFIG::APL_FINAL;
			}
			if (pBinning->fetchChild("SocketType"))
			{
				if (CUtil::toUpper(pBinning->fetchChild("SocketType")->fetchText()).compare("UDP") == 0) nSocketType = SOCK_DGRAM;
				if (CUtil::toUpper(pBinning->fetchChild("SocketType")->fetchText()).compare("TCP") == 0) nSocketType = SOCK_STREAM;
				if (CUtil::toUpper(pBinning->fetchChild("SocketType")->fetchText()).compare("RAW") == 0) nSocketType = SOCK_RAW;
			}
			if (pBinning->fetchChild("IP")){ IP = pBinning->fetchChild("IP")->fetchText(); }	
			if (pBinning->fetchChild("Port")){ nPort = CUtil::toLong(pBinning->fetchChild("Port")->fetchText()); }	
		}

		// if <logging> was set, let's see if logging is enabled and file path is specified
		if (pLogging)
		{
			bLogToFile = true;
			if (pLogging->fetchChild("Path")){ szLogPath = pLogging->fetchChild("Path")->fetchText(); }
		}
	}	

	if (root) delete root;

	return true;
}

/* ------------------------------------------------------------------------------------------
print config parameters
------------------------------------------------------------------------------------------ */
void CApp::CONFIG::print()
{
	// print config values here
	m_Log << "Path: " << szLotInfoFilePath << CUtil::CLog::endl;
	m_Log << "File: " << szLotInfoFileName << CUtil::CLog::endl;
	m_Log << "Delete LotInfo After Parse: " << (bDeleteLotInfo? "enabled" : "disabled") << CUtil::CLog::endl;
	m_Log << "Binning: " << (bSendBin? "enabled" : "disabled") << CUtil::CLog::endl;
	m_Log << "Bin type (if binning enabled): " << (bUseHardBin? "hard" : "soft") << CUtil::CLog::endl;
	m_Log << "Test type (if binning enabled): ";
	switch (nTestType)
	{
		case CONFIG::APL_WAFER: m_Log << "wafer test" << CUtil::CLog::endl; break;
		case CONFIG::APL_FINAL: m_Log << "final test" << CUtil::CLog::endl; break;
		default: m_Log << "unknown" << CUtil::CLog::endl; break;
	};
	m_Log << "IP: " << IP << CUtil::CLog::endl;
	m_Log << "Port: " << nPort << CUtil::CLog::endl;
	m_Log << "Socket type (if binning enabled): ";
	switch (nSocketType)
	{
		case SOCK_DGRAM: m_Log << "UDP" << CUtil::CLog::endl; break;
		case SOCK_STREAM: m_Log << "TCP" << CUtil::CLog::endl; break;
		case SOCK_RAW: m_Log << "RAW" << CUtil::CLog::endl; break;
		default: m_Log << "unknown" << CUtil::CLog::endl; break;
	};
	m_Log << "Log To File: " << (bLogToFile? "enabled" : "disabled") << CUtil::CLog::endl;
	m_Log << "Log Path (if enabled): " << szLogPath << CUtil::CLog::endl;
	m_Log << "LotInfo to STDF: " << (bSendInfo? "enabled" : "disabled") << CUtil::CLog::endl;
	m_Log << "STDF Dlog Path: " << szSTDFPath << CUtil::CLog::endl;

	m_Log << "Launch Type: " << (bProd? "OICu" : "OpTool") << CUtil::CLog::endl;
	m_Log << "Kill Tester Before Launch: " << (bKillTesterOnLaunch? "true" : "false") << CUtil::CLog::endl;
	m_Log << "Launch Attempt Time-out (s): " << (nRelaunchTimeOutMS /1000) << CUtil::CLog::endl;
	m_Log << "End Lot Time-out (s): " << (nEndLotTimeOutMS /1000)<< CUtil::CLog::endl;
	m_Log << "Unload Program Time-out (s): " << (nUnloadProgTimeOutMS /1000)<< CUtil::CLog::endl;
	m_Log << "Kill Tester Time-out (s): " << (nKillTesterTimeOutMS /1000)<< CUtil::CLog::endl;
	m_Log << "Max Launch Attempts: " << (nRelaunchAttempt) << CUtil::CLog::endl;	
	m_Log << "FAmodule Time-out (s): " << (nFAModuleTimeOutMS /1000)<< CUtil::CLog::endl;
	m_Log << "Summary Appending with \"Step\" : " << (bSummary? "enabled" : "disabled") << CUtil::CLog::endl;
	m_Log << "Summary Path (if enabled): " << szSummaryPath << CUtil::CLog::endl;
	m_Log << "Force Load: " << (bForceLoad? "enabled" : "disabled") << CUtil::CLog::endl;
	m_Log << "Step: " << (bStep? "enabled" : "disabled") << CUtil::CLog::endl;
	for (unsigned int i = 0; i < steps.size(); i++)
	{
		m_Log << "STEP: " << steps[i].szStep << ", flow_id: " << steps[i].szFlowId << ", rtst_cod: " << steps[i].nRtstCod << ", cmod_cod: " << steps[i].szCmodCod<< CUtil::CLog::endl;
	}
	m_Log << "GUI: " << (bPopupServer? "enabled" : "disabled") << CUtil::CLog::endl;
	m_Log << "GUI Server IP: " << szServerIP << CUtil::CLog::endl;
	m_Log << "Port Server IP: " << nServerPort << CUtil::CLog::endl;
	m_Log << "New Lot Config Path: " << szNewLotConfigPath << CUtil::CLog::endl;
	m_Log << "New Lot Config File: " << szNewLotConfigFile << CUtil::CLog::endl;
	m_Log << "Customer: " << szCustomer << CUtil::CLog::endl;
	m_Log << "Test Site: " << szTestSite << CUtil::CLog::endl;
	m_Log << "Supplier: " << szSupplier << CUtil::CLog::endl;
	m_Log << "Rename STDF: " << (bRenameSTDF? "enabled" : "disabled") << CUtil::CLog::endl;
	m_Log << "Rename STDF Format: " << szRenameSTDFFormat << CUtil::CLog::endl;
	m_Log << "Rename TimeStamp Format: " << (bTimeStampIsEndLot? "end lot" : "start lot") << CUtil::CLog::endl;
	m_Log << "Zip STDF: " << (bZipSTDF? "enabled" : "disabled") << CUtil::CLog::endl;
	m_Log << "Zip STDF Command: " << szZipSTDFCmd << CUtil::CLog::endl;
	m_Log << "Zip STDF Extension: " << szZipSTDFExt << CUtil::CLog::endl;
	m_Log << "LotInfo2FAModule: " << (bFAModule? "enabled" : "disabled") << CUtil::CLog::endl;
}

/* ------------------------------------------------------------------------------------------
Utility: monitor STDF file with .std extension. rename them 
------------------------------------------------------------------------------------------ */
void CApp::onWatchSTDF(const std::string& name)
{
	// check if this file has .std extension 
	size_t pos = name.find_last_of('.');

	// if pos = npos, there's no '.' meaning no file extension
	if (pos == std::string::npos)
	{
		//m_Log << "onWatchSTDF(): " << name << " has no file extension." << CUtil::CLog::endl;
		return;
	}

	// if extension is not .sum, we bail
	std::string ext = name.substr(pos + 1);
	if (ext.compare("std") != 0 && ext.compare("sum") != 0)
	{
		//m_Log << "onWatchSTDF(): " << name << " has extension: " << ext << ", we're expecting .std" << CUtil::CLog::endl;
		return;
	}

	// create string that holds full path + STDF file 
	std::stringstream ssFullPathSTDF;
	ssFullPathSTDF << m_CONFIG.szSTDFPath << "/" << name;

	// are we renaming STDF file?
	if (m_CONFIG.bRenameSTDF)
	{
		// are we renaming to qualcomm's format?
		if (m_CONFIG.szRenameSTDFFormat.compare("QUALCOMM") == 0)
		{
			// extract MIR and MRR from this STDF file
			APL::CSTDF stdf;
			APL::MRR mrr; 
			APL::MIR mir;
			if (!stdf.readMRR(ssFullPathSTDF.str(), mrr)){ m_Log << "ERROR: Something went wrong extracting MRR from '" << ssFullPathSTDF.str() << "'" << CUtil::CLog::endl; return; }
			if (!stdf.readMIR(ssFullPathSTDF.str(), mir)){ m_Log << "ERROR: Something went wrong extracting MIR from '" << ssFullPathSTDF.str() << "'" << CUtil::CLog::endl; return; }

			// ensure we only process STDF that comes from this tester
			if (m_szTesterName.compare(mir.NODE_NAM) != 0) 
			{
				m_Log << "WARNING: This STDF file '" << ssFullPathSTDF.str() << "' is tested on '" << mir.NODE_NAM << "'. Our tester name is '" << m_szTesterName << "'" << CUtil::CLog::endl;
				return;
			}	

			// if we reach this point, we found a valid STDF file with good MIR and MRR records. let's ignore any incoming select() notification
			m_pSTDFFileDesc->halt();

			// create end-lot timestamp formatted for qualcomm
			time_t t = (unsigned long)(m_CONFIG.bTimeStampIsEndLot? mrr.FINISH_T : mir.START_T);
			tm* p = gmtime((const time_t*)&t); 
			std::stringstream ssEndLotTimeStamp; // YYYYMMDDhhmmss
			ssEndLotTimeStamp << (p->tm_year + 1900); // YYYY
			ssEndLotTimeStamp << (p->tm_mon + 1 < 10? "0":"") << (p->tm_mon + 1); // MM
		 	ssEndLotTimeStamp << (p->tm_mday < 10? "0":"") << (p->tm_mday); // DD
		 	ssEndLotTimeStamp << (p->tm_hour < 10? "0":"") << (p->tm_hour); // hh
		 	ssEndLotTimeStamp << (p->tm_min < 10? "0":"") << (p->tm_min); // mm
		 	ssEndLotTimeStamp << (p->tm_sec < 10? "0":"") << (p->tm_sec); // ss

			// create the filename to be replacement. somehow this does not trigger inotify MOVE_TO and CREATE so that's good
			std::stringstream ssNewFileName;
			ssNewFileName << m_CONFIG.szSTDFPath << "/";
			ssNewFileName << m_CONFIG.szSupplier << "_" << m_CONFIG.szTestSite << "_";
			ssNewFileName << mir.PART_TYP << "_" << mir.JOB_NAM << "_" << mir.LOT_ID << "_";
			ssNewFileName << ssEndLotTimeStamp.str() << "_" << mir.RTST_COD << "_" << mir.FLOW_ID << "_" << mir.NODE_NAM << "_" << m_lotinfo.szDeviceNickName <<  ".std";
   
			// rename the file
			if (!CUtil::renameFile(ssFullPathSTDF.str(), ssNewFileName.str()))
			{ 
				m_Log << "ERROR: Failed to rename '" << ssFullPathSTDF.str() << "' to '" << ssNewFileName.str() << "'" << CUtil::CLog::endl; 
			}
			else
			{ 
				m_Log << "Successfully renamed STDF file to: '" << ssNewFileName.str() << "'" << CUtil::CLog::endl; 
				ssFullPathSTDF.str(std::string());
				ssFullPathSTDF << ssNewFileName.str();
			}

			// let's do the summary file here
			/*
			CUtil::CTimer tm;
			tm.start();
			m_Log << "summary time: " << tm.stop() << " ms" << CUtil::CLog::endl;
			*/
		}
	}

	// are we compressing STDF file?
	if (m_CONFIG.bZipSTDF)
	{
		// create system command to compress STDF file
		std::stringstream ssCmd;
		ssCmd << m_CONFIG.szZipSTDFCmd << " " << ssFullPathSTDF.str();
		system(ssCmd.str().c_str());

		// now let's check if the compressed file exists
		std::stringstream ssCompressedFile;
		ssCompressedFile << ssFullPathSTDF.str() << m_CONFIG.szZipSTDFExt;
		if (!CUtil::isFileExist(ssCompressedFile.str(), 20, 1))
		{
			m_Log << "ERROR: Failed to compress '" << ssFullPathSTDF.str() << "' into '" << ssCompressedFile.str() << "'" << CUtil::CLog::endl;
		}
		else m_Log << "Successfully compressed STDF file to '" << ssCompressedFile.str() << "'" << CUtil::CLog::endl;	
	}
}

/* ------------------------------------------------------------------------------------------
Utility: if monitoring incoming sublot summary file for appending amkor's "step" values
	is enabled, it's done here.
------------------------------------------------------------------------------------------ */
void CApp::onSummaryFile(const std::string& name)
{
	m_Log << "Summary file '" << name << "' received, parsing it... " << CUtil::CLog::endl;

	// check if this file has .sum extension 
	size_t pos = name.find_last_of('.');

	// if pos = npos, there's no '.' meaning no file extension
	if (pos == std::string::npos)
	{
//		m_Log << name << " has no file extension." << CUtil::CLog::endl;
		return;
	}

	// if extension is not .sum, we bail
	std::string ext = name.substr(pos + 1);
	if (ext.compare("sum") != 0)
	{
//		m_Log << name << " has extension: " << ext << ", we're expecting .sum" << CUtil::CLog::endl;
		return;
	}

	// create string that holds full path + monitor file 
	std::stringstream ssFullPathSummaryName;
	ssFullPathSummaryName << m_CONFIG.szSummaryPath << "/" << name;

	// open the file
	sleep(1);
	std::fstream fs;
	fs.open(ssFullPathSummaryName.str().c_str(), std::ios::in);
	std::stringstream ss;
	ss << fs.rdbuf();
	std::string s = ss.str();
	fs.close();

	// return false if it finds an empty file to allow app to try again
	if (s.empty())
	{
		m_Log << "Received a file '" << name << "' while expecting a summary file but it's empty." << CUtil::CLog::endl;
		return;
	}

	// parse the file content
	bool bFileName = false, bLotId = false, bStep = false;
	while(s.size())  
	{ 
		// find next '\n' position 
		size_t pos = s.find_first_of('\n');

		// get current line
		std::string l = s.substr(0, pos);

		// remove current line from the file string		
		s = s.substr(pos + 1);

		// get the field/value pair from the line. empty value is ignored. trail/lead spaces removed
		std::string field, value;
		if (getFieldValuePair(l, DELIMITER, field, value))
		{
			m_Log << field << " : " << value << CUtil::CLog::endl;

			// check if FILE NAME contains test program full path
			if (field.compare("FILE NAME") == 0)
			{
				m_Log << field << ": " << value << " found, checking if matches loaded program..." << CUtil::CLog::endl;
				if (value.compare(m_lotinfo.szProgramFullPathName) == 0){ bFileName = true; }
				else{ m_Log << value << " does not match loaded test program '" << m_lotinfo.szProgramFullPathName << "'. this file is invalid." << CUtil::CLog::endl; }			
			}
			
			// if we receive "step" field from amkor
			if (field.compare("LOT ID") == 0)
			{
				m_Log << field << ": " << value << " found, checking if matches lot id..." << CUtil::CLog::endl;
				if (value.compare(m_lotinfo.szLotId) == 0){ bLotId = true; }
				else{ m_Log << value << " does not match lot id '" << m_lotinfo.szLotId << "'. this file is invalid." << CUtil::CLog::endl; }
			}
			if (field.compare("STEP") == 0)
			{
				m_Log << "This lot summary already contains 'step' field. APL will not append anymore." << CUtil::CLog::endl;
				bStep = true;
			}
		}
	
		// if this is the last line then bail
		if (pos == std::string::npos) break;		
	}	

	if (m_pSummaryFileDesc) m_pSummaryFileDesc->halt();

	// if this file refers to different test program and lot id and/or STEP field is already appended to it, we bail
	if ( !bFileName || !bLotId || bStep )
	{
		return;
	}

	// if we found matching job file and lot id, let's append to this file	
	sleep(1);
	std::ofstream sf;
	sf.open(ssFullPathSummaryName.str().c_str(), std::ofstream::out | std::ofstream::app);
	if(sf.good()) 
	{
		m_Log << "opened " << ssFullPathSummaryName.str().c_str() << CUtil::CLog::endl;	
		sf << "STEP:" << m_lotinfo.szStep << std::endl;
		sf.close();
		m_Log << "STEP value '" << m_lotinfo.szStep << "' appended to " << ssFullPathSummaryName.str().c_str() << CUtil::CLog::endl;
	}
	else 
	{
		m_Log << "ERROR: Could not open " << ssFullPathSummaryName.str().c_str() << CUtil::CLog::endl;
	}

}


/* ------------------------------------------------------------------------------------------
Utility: checks STEP value from lotinfo.txt and checks lotinfo.txt if ACTIVEFLOWNAME
	 and RTSTCODE matches corresponding value for STEP. if any of them does not match,
	 this function will edit lotinfo.txt file with correct ACTIVEFLOWNAME and RTSTCODE
- if lotinfo.txt does not contain STEP value, returns FALSE
- if the STEP value found in lotinfo.txt is not valid, returns FALSE
- if failed to remove lotinfo.txt (to be replace with new one), returns FALSE
- if failed to rename lotinfo.txt.temp to lotinfo.txt, returns FALSE
------------------------------------------------------------------------------------------ */
bool CApp::updateLotinfoFile(const std::string& name)
{
	// is the STEP field empty? bail out
	if (!m_lotinfo.szStep.size())
	{
		m_Log << "STEP field from " << name << " is empty. " << CUtil::CLog::endl;
		return false;
	}

	// check if the STEP field is valid
	CONFIG::STEP* pStep = 0;
	for (unsigned int i = 0; i < m_CONFIG.steps.size(); i++)
	{
		// found a match in list of valid step values
		if ( CUtil::toUpper(m_CONFIG.steps[i].szStep).compare( CUtil::toUpper(m_lotinfo.szStep) ) == 0 )
		{
			pStep = &m_CONFIG.steps[i];
			break;
		}
	}
	// if the step we got from lotinfo.txt is not valid, bail out
	if (!pStep)
	{
		m_Log << "WARNING: " << m_lotinfo.szStep << " is not a valid STEP value" << CUtil::CLog::endl;
		return false;
	}
	else m_Log << m_lotinfo.szStep << " is a valid value" << CUtil::CLog::endl;

	// did lotinfo.txt contain flow_id already? if yes, does it match the step format? if yes, let's do nothing
	bool bEditLotInfo = false;
	if (m_lotinfo.mir.FlowId.compare( pStep->szFlowId ) != 0)
	{
		m_Log << m_CONFIG.szLotInfoFileName << " contains FlowId: '" << m_lotinfo.mir.FlowId << "' but " << pStep->szFlowId << " is expected." << CUtil::CLog::endl;
		bEditLotInfo = true;
	} 
	else m_Log << m_CONFIG.szLotInfoFileName << " contains FlowId: '" << m_lotinfo.mir.FlowId << "', matching expected value: " << pStep->szFlowId << CUtil::CLog::endl;

	// did lotinfo.txt contain rtst_cod already? if yes, does it match the step format? if yes, let's do nothing
	if (CUtil::toLong(m_lotinfo.mir.RtstCod) != pStep->nRtstCod )
	{
		m_Log << m_CONFIG.szLotInfoFileName << " contains RstCod: '" << m_lotinfo.mir.RtstCod << "' but " << pStep->nRtstCod << " is expected." << CUtil::CLog::endl;
		bEditLotInfo = true;
	}
	else m_Log << m_CONFIG.szLotInfoFileName << " contains RstCod: '" << m_lotinfo.mir.RtstCod << "', matching expected value: " << pStep->nRtstCod << CUtil::CLog::endl;

	// did lotinfo.txt contain cmod_cod already? if yes, does it match the step format? if yes, let's do nothing
	if (m_lotinfo.mir.CmodCod.compare( pStep->szCmodCod ) != 0)
	{
		m_Log << m_CONFIG.szLotInfoFileName << " contains CmodCod: '" << m_lotinfo.mir.CmodCod << "' but " << pStep->szCmodCod << " is expected." << CUtil::CLog::endl;
		bEditLotInfo = true;
	} 
	else m_Log << m_CONFIG.szLotInfoFileName << " contains CmodCod: '" << m_lotinfo.mir.CmodCod << "', matching expected value: " << pStep->szCmodCod << CUtil::CLog::endl;

	if (bEditLotInfo)
	{
		// let's just quickly update our STDF variable flowid, cmodcod and rtstcod with valid STEP formats
		m_lotinfo.mir.FlowId = pStep->szFlowId;
		m_lotinfo.mir.CmodCod = pStep->szCmodCod;
		std::stringstream n; n << pStep->nRtstCod;
		m_lotinfo.mir.RtstCod = n.str();

		// open lotinfo.txt for reading and transfer content to string
		std::fstream fs;
		if (!CUtil::openFile(name, fs, std::ios::in))
		{
			m_Log << "ERROR! Failed to open " << name << " for reading." << CUtil::CLog::endl;
			return false;
		}
		std::stringstream ss;
		ss << fs.rdbuf();
		std::string s = ss.str();
		fs.close();

		// return false if it finds an empty file to allow app to try again
		if (s.empty())
		{
			m_Log << "ERROR! " << name << " is empty." << CUtil::CLog::endl;
			return false;
		}

		// open lotinfo.txt.temp file for writing
		ss.str(std::string());
		ss << name << ".tmp";
		if (!CUtil::openFile(ss.str(), fs, std::ios::out))
		{
			m_Log << "ERROR! Failed to open " << ss.str() << " for writing." << CUtil::CLog::endl;
			return false;
		}
 
		// line by line copy from .txt to .txt.temp until you find 'ACTIVEFLOWNAME' and 'RTSTCODE'
		// for this line, we insead use the valid values corresponding to STEP value
		while(s.size())  
		{ 
			// find next '\n' position 
			size_t pos = s.find_first_of('\n');

			// get current line
			std::string l = s.substr(0, pos);

			// remove current line from the file string		
			s = s.substr(pos + 1);

			// get the field/value pair from the line. empty value is ignored. trail/lead spaces removed
			std::string field, value;
			if (getFieldValuePair(l, DELIMITER, field, value))
			{
				if (field.compare( m_CONFIG.mir.getValue("FLOW_ID") ) == 0){ continue; } // don't copy existing flowid
				else if (field.compare( m_CONFIG.mir.getValue("RTST_COD") ) == 0){ continue; } // don't copy existing rtstcod
				else if (field.compare( m_CONFIG.mir.getValue("CMOD_COD") ) == 0){ continue; } // don't copy existing cmodcod
				else{ fs << l << std::endl; }
			}
	
			// if this is the last line then bail
			if (pos == std::string::npos) break;		
		}	

		// finally we insert the flowid, cmodcod and rtstcod with new flowid, cmodcod and rtstcod value
		fs << m_CONFIG.mir.getValue("FLOW_ID") << " " << DELIMITER << " " << pStep->szFlowId << std::endl;
		fs << m_CONFIG.mir.getValue("RTST_COD") << " " << DELIMITER << " " << pStep->nRtstCod << std::endl;
		fs << m_CONFIG.mir.getValue("CMOD_COD") << " " << DELIMITER << " " << pStep->szCmodCod << std::endl;

		// close file
		fs.close();

		// delete lotinfo.txt file
		if (!CUtil::removeFile(name))
		{
			m_Log << "ERROR: " << name << " still exist after an attempt to delete it." << CUtil::CLog::endl;
			return false;
		}

		// rename lotinfo.txt.temp to lotinfo.txt file		
		if (!CUtil::renameFile(ss.str(), name))
		{
			m_Log << "ERROR: Failed to rename " << ss << " to " << name << CUtil::CLog::endl;
			return false;
		}

		m_Log << "Successfully edited " << name.c_str() << " with valid STEP values." << CUtil::CLog::endl;
	}

	return true;
}

/* ------------------------------------------------------------------------------------------
parse the lotinfo.txt file
-	STDF fields and test program will only be acknowledged to use if a JOBFILE
	field exists and has a valid test program path/name value
------------------------------------------------------------------------------------------ */
bool CApp::parse(const std::string& name)
{
	// open file and transfer content to string
	std::fstream fs;
	fs.open(name.c_str(), std::ios::in);
	std::stringstream ss;
	ss << fs.rdbuf();
	std::string s = ss.str();
	fs.close();

	// return false if it finds an empty file to allow app to try again
	if (s.empty()) return false;

	// for debugging purpose, let's dump the contents of the file
	m_Log << "---------------------------------------------------------------------" << CUtil::CLog::endl;
	m_Log << "Content extracted from " << name << "." << CUtil::CLog::endl;
	m_Log << "It's file size is " << s.length() << " bytes. Contents: " << CUtil::CLog::endl;
	m_Log << s << CUtil::CLog::endl;
	m_Log << "---------------------------------------------------------------------" << CUtil::CLog::endl;
 
	LOTINFO li;
	while(s.size())  
	{ 
		// find next '\n' position 
		size_t pos = s.find_first_of('\n');

		// get current line
		std::string l = s.substr(0, pos);

		// remove current line from the file string		
		s = s.substr(pos + 1);

		// get the field/value pair from the line. empty value is ignored. trail/lead spaces removed
		std::string field, value;
		if (getFieldValuePair(l, DELIMITER, field, value))
		{  
			// extract STDF fields
			if (m_CONFIG.mir.getField(field).compare("OPER_NAM") == 0) 		{ li.mir.OperNam = value; }
			if (m_CONFIG.mir.getField(field).compare("OPER_FRQ") == 0)		{ li.mir.OperFrq = value; }
			if (m_CONFIG.mir.getField(field).compare("LOT_ID") == 0)		{ li.mir.LotId = value; }
			if (m_CONFIG.mir.getField(field).compare("SBLOT_ID") == 0)		{ li.mir.SblotId = value; }
			if (m_CONFIG.mir.getField(field).compare("PART_TYP") == 0)		{ li.mir.PartTyp = value; }
			if (m_CONFIG.mir.getField(field).compare("FAMLY_ID") == 0)		{ li.mir.FamlyId = value; }
			if (m_CONFIG.mir.getField(field).compare("PKG_TYP") == 0)		{ li.mir.PkgTyp = value; }
			if (m_CONFIG.mir.getField(field).compare("JOB_REV") == 0)	 	{ li.mir.JobRev = value; }
			if (m_CONFIG.mir.getField(field).compare("MODE_COD") == 0)		{ li.mir.ModeCod = value; }
			if (m_CONFIG.mir.getField(field).compare("SETUP_ID") == 0)		{ li.mir.SetupId = value; }
			if (m_CONFIG.mir.getField(field).compare("CMOD_COD") == 0)		{ li.mir.CmodCod = value; }
			if (m_CONFIG.mir.getField(field).compare("DATE_COD") == 0)		{ li.mir.DateCod = value; }
			if (m_CONFIG.mir.getField(field).compare("PROC_ID") == 0)		{ li.mir.ProcId = value; }
			if (m_CONFIG.mir.getField(field).compare("FLOOR_ID") == 0)		{ li.mir.FloorId = value; }
			if (m_CONFIG.mir.getField(field).compare("FACIL_ID") == 0)		{ li.mir.FacilId = value; }
			if (m_CONFIG.mir.getField(field).compare("TST_TEMP") == 0)		{ li.mir.TstTemp = value; }
			if (m_CONFIG.mir.getField(field).compare("USER_TXT") == 0)		{ li.mir.UserTxt = value; }
			if (m_CONFIG.mir.getField(field).compare("DSGN_REV") == 0)		{ li.mir.DsgnRev = value; }
			if (m_CONFIG.mir.getField(field).compare("ENG_ID") == 0)		{ li.mir.EngId = value; }
			if (m_CONFIG.mir.getField(field).compare("NODE_NAM") == 0)		{ li.mir.NodeNam = value; }
			if (m_CONFIG.mir.getField(field).compare("AUX_FILE") == 0)		{ li.mir.AuxFile = value; }
			if (m_CONFIG.mir.getField(field).compare("TEST_COD") == 0)		{ li.mir.TestCod = value; }
			if (m_CONFIG.mir.getField(field).compare("ROM_COD") == 0)		{ li.mir.RomCod = value; }
			if (m_CONFIG.mir.getField(field).compare("SERL_NUM") == 0)		{ li.mir.SerlNum = value; }
			if (m_CONFIG.mir.getField(field).compare("TSTR_TYP") == 0)		{ li.mir.TstrTyp = value; }
			if (m_CONFIG.mir.getField(field).compare("SUPR_NAM") == 0)		{ li.mir.SuprNam = value; }
			if (m_CONFIG.mir.getField(field).compare("EXEC_TYP") == 0)		{ li.mir.ExecTyp = value; }
			if (m_CONFIG.mir.getField(field).compare("EXEC_VER") == 0)		{ li.mir.ExecVer = value; }
			if (m_CONFIG.mir.getField(field).compare("SPEC_NAM") == 0)		{ li.mir.SpecNam = value; }
			if (m_CONFIG.mir.getField(field).compare("SPEC_VER") == 0)		{ li.mir.SpecVer = value; }
			if (m_CONFIG.mir.getField(field).compare("PROT_COD") == 0)		{ li.mir.ProtCod = value; }
			if (m_CONFIG.mir.getField(field).compare("BURN_TIM") == 0)		{ li.mir.BurnTim = value; }
			if (m_CONFIG.mir.getField(field).compare("FLOW_ID") == 0)		{ li.mir.FlowId = value; }
			if (m_CONFIG.mir.getField(field).compare("RTST_COD") == 0)		{ li.mir.RtstCod = value; }

			if (m_CONFIG.sdr.getField(field).compare("HAND_TYP") == 0)		{ li.sdr.HandTyp = value; }
			if (m_CONFIG.sdr.getField(field).compare("HAND_ID") == 0)		{ li.sdr.HandId = value; }
			if (m_CONFIG.sdr.getField(field).compare("CARD_ID") == 0)		{ li.sdr.CardId = value; }
			if (m_CONFIG.sdr.getField(field).compare("CARD_TYP") == 0)		{ li.sdr.CardTyp = value; }
			if (m_CONFIG.sdr.getField(field).compare("LOAD_ID") == 0)		{ li.sdr.LoadId = value; }
			if (m_CONFIG.sdr.getField(field).compare("LOAD_TYP") == 0)		{ li.sdr.LoadTyp = value; }
			if (m_CONFIG.sdr.getField(field).compare("DIB_TYP") == 0)		{ li.sdr.DibTyp = value; }
			if (m_CONFIG.sdr.getField(field).compare("DIB_ID") == 0)		{ li.sdr.DibId = value; }
			if (m_CONFIG.sdr.getField(field).compare("CABL_ID") == 0)		{ li.sdr.CableId = value; }
			if (m_CONFIG.sdr.getField(field).compare("CABL_TYP") == 0)		{ li.sdr.CableTyp = value; }
			if (m_CONFIG.sdr.getField(field).compare("CONT_TYP") == 0)		{ li.sdr.ContTyp = value; }
			if (m_CONFIG.sdr.getField(field).compare("CONT_ID") == 0)		{ li.sdr.ContId = value; }
			if (m_CONFIG.sdr.getField(field).compare("LASR_TYP") == 0)		{ li.sdr.LasrTyp = value; }
			if (m_CONFIG.sdr.getField(field).compare("LASR_ID") == 0)		{ li.sdr.LasrId = value; }
			if (m_CONFIG.sdr.getField(field).compare("EXTR_TYP") == 0)		{ li.sdr.ExtrTyp = value; }
			if (m_CONFIG.sdr.getField(field).compare("EXTR_ID") == 0)		{ li.sdr.ExtrId = value; }

			// if we didn't find anything we looked for including jobfile, let's move to next field
			if (field.compare(m_CONFIG.fields.getValue("JOBFILE")) == 0)
			{
				m_Log << field << " field found, test program to load: '" << value << "'. checking if valid..." << CUtil::CLog::endl;

				// does the program/path in value refer to exising file?
				if (!CUtil::isFileExist(value))
				{
					m_Log << "ERROR: '" << value << "' program cannot be accessed." << CUtil::CLog::endl;
					continue;
				} 
				else
				{
					m_Log << "'" << value << "' exists. let's try to load it." << CUtil::CLog::endl;
					li.szProgramFullPathName = value;			
				}
			}

			if (m_CONFIG.fields.getField(field).compare("CUSTOMER") == 0)		{ li.szCustomer = value; }
			if (m_CONFIG.fields.getField(field).compare("DEVICENICKNAME") == 0)	{ li.szDeviceNickName = value; }
			
			// if we receive "step" field from amkor
			if (m_CONFIG.fields.getField(field).compare("STEP") == 0)
			{
				m_Log << field << " field found, value: '" << value << CUtil::CLog::endl;
				li.szStep = value;
			}
			// if we receive "lotid" field from amkor
			if (m_CONFIG.mir.getField(field).compare("LOT_ID") == 0)
			{
				m_Log << field << " field found, value: '" << value << CUtil::CLog::endl;
				li.szLotId = value;
			}
			// if we receive "device" field from amkor
			if (m_CONFIG.mir.getField(field).compare("PART_TYP") == 0)
			{
				m_Log << field << " field found, value: '" << value << CUtil::CLog::endl;
				li.szDevice = value;
			}
		}
	
		// if this is the last line then bail
		if (pos == std::string::npos) break;		
	}	

	// ok did we find a valid JobFile in the lotinfo.txt?
	if (li.szProgramFullPathName.empty()) return false;
	else
	{ 
		m_lotinfo = li; 
		return true;
	}
} 


/* ------------------------------------------------------------------------------------------
utility: get field/value pair string of a line from lotinfo.txt 
	 file content loaded into string
------------------------------------------------------------------------------------------ */
bool CApp::getFieldValuePair(const std::string& line, const char delimiter, std::string& field, std::string& value)
{
	// strip the 'field' and 'value' of this line
	size_t pos = line.find_first_of(delimiter);
	field = line.substr(0, pos);

	// is there field/value pair? if not, bail
	if (pos == std::string::npos)
	{
		m_Log << "WARNING: This line: '" << line << "' doesn't contain delimiter '" << delimiter << "'..." << CUtil::CLog::endl;
		return false;	
	}

	// is 'value' empty? if yes, move to next line.
	if (pos + 1 >= line.size()) 
	{
		m_Log << "WARNING: This line: '" << line << "' has empty value " << CUtil::CLog::endl;
		return false;
	}

	value = line.substr(pos + 1);
	if (value.empty())
	{
		m_Log << "WARNING: This line: '" << line << "' has empty value " << CUtil::CLog::endl;
		return false;
	}

	// strip leading/trailing space 
	field = CUtil::removeLeadingTrailingSpace(field);
	value = CUtil::removeLeadingTrailingSpace(value);

	// convert to upper case field
	for (std::string::size_type i = 0; i < field.size(); i++){ field[i] = CUtil::toUpper(field[i]); }

	return true;
}

/* ------------------------------------------------------------------------------------------
event handler for state notification EOL
------------------------------------------------------------------------------------------ */
void CApp::onLotChange(const EVX_LOT_STATE state, const std::string& szLotId)
{
	switch (state) 
	{
		case EVX_LOT_END:
		{
			m_Log << "Lot End" << CUtil::CLog::endl;
			if (m_StateMgr.get() == m_pStateOnEndLot) m_StateMgr.set(m_pStateOnUnloadProg);
			break;
		}
		case EVX_LOT_START:
		{
			m_Log << "Lot Start" << CUtil::CLog::endl;
			if (m_StateMgr.get() == m_pStateOnEndLot) m_StateMgr.set(m_pStateOnUnloadProg);
			break;
		}
		default: break; 
	}
}

/* ------------------------------------------------------------------------------------------
event handler for state notification EOW
------------------------------------------------------------------------------------------ */
void CApp::onWaferChange(const EVX_WAFER_STATE state, const std::string& szWaferId)
{
	switch (state) 
	{
		case EVX_WAFER_END:
		{
			// every time wafer ends, let's clear map counter
			m_map.clear();

			m_Log << "Wafer End" << CUtil::CLog::endl;
			if (m_StateMgr.get() == m_pStateOnEndLot) m_StateMgr.set(m_pStateOnUnloadProg);
			break;
		}
		case EVX_WAFER_START:
		{
			// every time wafer starts, let's clear map counter
			m_map.clear();

			m_Log << "Wafer Start" << CUtil::CLog::endl;
			if (m_StateMgr.get() == m_pStateOnEndLot) m_StateMgr.set(m_pStateOnUnloadProg);
			break;
		}
		default: break; 
	}
}

/* ------------------------------------------------------------------------------------------
event handler called by state notification class when something happens in test program
------------------------------------------------------------------------------------------ */
void CApp::onProgramChange(const EVX_PROGRAM_STATE state, const std::string& msg)
{
	// make sure we have valid evxa object
	if (!m_pProgCtrl) return;

	switch(state)
	{
		case EVX_PROGRAM_LOADING:
			m_Log << "programChange[" << state << "]: EVX_PROGRAM_LOADING" << CUtil::CLog::endl;
			break;
		case EVX_PROGRAM_LOAD_FAILED:
		{
			m_StateMgr.set(m_pStateOnIdle);
			m_Log << "programChange[" << state << "]: EVX_PROGRAM_LOAD_FAILED" << CUtil::CLog::endl;
			break;
		}
		case EVX_PROGRAM_LOADED:
		{
		    	m_Log << "programChange[" << state << "]: EVX_PROGRAM_LOADED" << CUtil::CLog::endl;
			m_StateMgr.set(m_pSendLotInfo);
			break;
		}
		case EVX_PROGRAM_START: 
			m_Log << "programChange[" << state << "]: EVX_PROGRAM_START" << CUtil::CLog::endl;
			break;
		case EVX_PROGRAM_RUNNING:
			m_Log << "programChange[" << state << "]: EVX_PROGRAM_RUNNING" << CUtil::CLog::endl;
			break;
		case EVX_PROGRAM_UNLOADING:
			m_Log << "programChange[" << state << "]: EVX_PROGRAM_UNLOADING" << CUtil::CLog::endl;
			break;
		case EVX_PROGRAM_UNLOADED: 
		{
			m_Log << "programChange[" << state << "]: EVX_PROGRAM_UNLOADED" << CUtil::CLog::endl;
			if (m_StateMgr.get() == m_pStateOnUnloadProg) m_StateMgr.set(m_pStateOnKillTester);
			
		    	EVXAStatus status = m_pProgCtrl->UnblockRobot();
			if (status != EVXA::OK) m_Log << "Error PROGRAM_UNLAODED UnblockRobot(): status: " << status << " " << m_pProgCtrl->getStatusBuffer() << CUtil::CLog::endl;
			break;
		}
		case EVX_PROGRAM_ABORT_LOAD:
			m_Log << "programChange[" << state << "]: EVX_PROGRAM_ABORT_LOAD" << CUtil::CLog::endl;
			break;
		case EVX_PROGRAM_READY:
			m_Log << "programChange[" << state << "]: EVX_PROGRAM_READY" << CUtil::CLog::endl;
			break;
		default:
		{
			if(m_pProgCtrl->getStatus() !=  EVXA::OK)
			{
				m_Log << "An Error occured[" << state << "]: " << m_pProgCtrl->getStatusBuffer() << CUtil::CLog::endl;
				m_pProgCtrl->clearStatus();
			}
			break;
		}
	}
}


/* ------------------------------------------------------------------------------------------
event handler for state notification EOT
------------------------------------------------------------------------------------------ */
void CApp::onEndOfTest(const int array_size, int site[], int serial[], int sw_bin[], int hw_bin[], int pass[], EVXA_ULONG dsp_status)
{
	// if we're not sending bin, bail out
	if (!m_CONFIG.bSendBin) return;

	// if wafer test, extract x/y coords. needed as per Amkor specs
	int *sites = 0;
	int *xCoords = 0;
	int *yCoords = 0;

	bool bRetest = false;
	if (m_CONFIG.nTestType == CONFIG::APL_WAFER)
	{	
		m_Log << "wafer test" << CUtil::CLog::endl;
		// initialize arrays and set to defaults
		sites = new int[array_size];
		xCoords = new int[array_size];
		yCoords = new int[array_size]; 

		if ( m_pProgCtrl->getWaferCoords(array_size, sites, xCoords, yCoords) != EVXA::OK )
		{
			m_Log << "Error: Something went wrong int querying Unison for X/Y coords." << CUtil::CLog::endl;
		}

		// add these dies (coords) in our map to remember it. if one of the dies fail to be added, then that die
		// is being retested instead. and we assume this whole test cycle is a retest
		for (int i = 0; i < array_size; i++)
		{ 
			// if this site is not selected, we skip it
			if (site[i] == 0) continue;

			if (!m_map.add(xCoords[i], yCoords[i]))
			{
				//m_Log << "Coord: " << xCoords[i] << ", " << yCoords[i] << " already exist " << CUtil::CLog::endl;
				bRetest = true;
			}
			//else m_Log << "Coord: " << xCoords[i] << ", " << yCoords[i] << " added. " << (bRetest?"RT":"FT") << CUtil::CLog::endl;
		}		
	}

	// set host/tester name
	std::stringstream send;
	send << m_szTesterName;

	// as per Amkor specs, if wafer test, insert wafer id (lot id) here
	if (m_CONFIG.nTestType == CONFIG::APL_WAFER) send << "," << (bRetest?"RT":"FT") << "/" <<  m_pProgCtrl->getLotInformation(EVX_LotWaferID);

	// loop through all loaded sites, start at 1. for unison, site 1 = 1
	for (int i = 1; i < m_pProgCtrl->getNumberOfLoadedSites(); i++)
	{
		// if there's at least 1 site, let's print the comma before first site as separator to host name
		if (i == 1)
		{
			send << ",";
		}

		// if this site is beyond array size, or is this is not selected, we skip it
		if (i >= array_size || site[i] == 0)
		{
			send << ""; 
		}

		// otherwise, this is a selected site with valid bin. print it
		else
		{
			if (m_CONFIG.nTestType == CONFIG::APL_WAFER) 
			{
				send << xCoords[i] << "/" << yCoords[i] << "/";
			}

			send << (m_CONFIG.bUseHardBin? hw_bin[i] : sw_bin[i]);
		}	
		// if this is the last site, insert asterisk as end char for string to send. otherwise, a coma separator
		send << (i == m_pProgCtrl->getNumberOfLoadedSites() - 1? "*" : ",");
	}

	// if we used these arrays, destroy them on the way
	if (sites) delete []sites;
	if (xCoords) delete []xCoords;
	if (yCoords) delete []yCoords;

	// finally, send the string to remote host
	CClient c;
	if (!c.connect(m_CONFIG.IP, m_CONFIG.nPort, m_CONFIG.nSocketType)) 
	{
		m_Log << "ERROR: Failed to connect to server." << CUtil::CLog::endl;
		return;
	}
	c.send(send.str());
	c.disconnect();
}

/* ------------------------------------------------------------------------------------------
wraps progctrl method to set lotinfo/stdf
-	returns false if it fails to set, true otherwise
------------------------------------------------------------------------------------------ */
bool CApp::setLotInformation(const EVX_LOTINFO_TYPE type, const std::string& field, const std::string& label, bool bForce)
{
	if (!m_pProgCtrl) return false;

	// if field is empty, we won't set it, unless we're forced to, which clears the lot information instead
   	if (!field.empty() || bForce) 
	{
		// set to unison
		m_pProgCtrl->setLotInformation(type, field.c_str());

		// check if successful
		if (field.compare( m_pProgCtrl->getLotInformation(type) ) != 0)
		{
			m_Log << "ERROR: Failed to set " << label << " to '" << field << "'" << CUtil::CLog::endl;
			return false;
		}
		// otherwise it's successful
		m_Log << "Successfully set " << label << " to '" << m_pProgCtrl->getLotInformation(type) << "'" << CUtil::CLog::endl;
		return true;
   	}
	// if field is empty from XTRF, let's leave this field with whatever its current value is
	else
	{ 
		m_Log << "Field is empty. " << label << " will not be set." << CUtil::CLog::endl; 
		return true;
	}
}


/* ------------------------------------------------------------------------------------------
set lotinfo taken from lotinfo.txt
------------------------------------------------------------------------------------------ */
bool CApp::setLotInfo()
{
	if (!m_pProgCtrl) return false;
	bool bRslt = true;

	// set MIR
	if ( !setLotInformation(EVX_LotLotID, 			m_lotinfo.mir.LotId, 	"MIR.LotLotID")) bRslt = false;
	if ( !setLotInformation(EVX_LotCommandMode, 		m_lotinfo.mir.CmodCod, 	"MIR.LotCommandMode")) bRslt = false;
	if ( !setLotInformation(EVX_LotActiveFlowName, 		m_lotinfo.mir.FlowId, 	"MIR.LotActiveFlowName")) bRslt = false;
	if ( !setLotInformation(EVX_LotDesignRev, 		m_lotinfo.mir.DsgnRev, 	"MIR.LotDesignRev")) bRslt = false;
	if ( !setLotInformation(EVX_LotDateCode, 		m_lotinfo.mir.DateCod, 	"MIR.LotDateCode")) bRslt = false;
	if ( !setLotInformation(EVX_LotOperFreq, 		m_lotinfo.mir.OperFrq, 	"MIR.LotOperFreq")) bRslt = false;
	if ( !setLotInformation(EVX_LotOperator, 		m_lotinfo.mir.OperNam, 	"MIR.LotOperator")) bRslt = false;
	if ( !setLotInformation(EVX_LotTcName, 			m_lotinfo.mir.NodeNam, 	"MIR.LotTcName")) bRslt = false;
	if ( !setLotInformation(EVX_LotDevice, 			m_lotinfo.mir.PartTyp, 	"MIR.LotDevice")) bRslt = false;
	if ( !setLotInformation(EVX_LotEngrLotId, 		m_lotinfo.mir.EngId, 	"MIR.LotEngrLotId")) bRslt = false;
	if ( !setLotInformation(EVX_LotTestTemp, 		m_lotinfo.mir.TstTemp, 	"MIR.LotTestTemp")) bRslt = false;
	if ( !setLotInformation(EVX_LotTestFacility, 		m_lotinfo.mir.FacilId, 	"MIR.LotTestFacility")) bRslt = false;
	if ( !setLotInformation(EVX_LotTestFloor, 		m_lotinfo.mir.FloorId, 	"MIR.LotTestFloor")) bRslt = false;
	//if ( !setLotInformation(EVX_LotHead, 			m_lotinfo.mir.StatNum, 	"MIR.LotHead")) bRslt = false; // this is read only 
	if ( !setLotInformation(EVX_LotFabricationID, 		m_lotinfo.mir.ProcId, 	"MIR.LotFabricationID")) bRslt = false;
	if ( !setLotInformation(EVX_LotTestMode, 		m_lotinfo.mir.ModeCod, 	"MIR.LotTestMode")) bRslt = false;
	if ( !setLotInformation(EVX_LotProductID, 		m_lotinfo.mir.FamlyId,  "MIR.LotProductID")) bRslt = false;
	if ( !setLotInformation(EVX_LotPackage, 		m_lotinfo.mir.PkgTyp, 	"MIR.LotPackage")) bRslt = false;
	if ( !setLotInformation(EVX_LotSublotID, 		m_lotinfo.mir.SblotId, 	"MIR.LotSublotID")) bRslt = false;
	if ( !setLotInformation(EVX_LotTestSetup, 		m_lotinfo.mir.SetupId, 	"MIR.LotTestSetup")) bRslt = false;
	if ( !setLotInformation(EVX_LotFileNameRev, 		m_lotinfo.mir.JobRev, 	"MIR.LotFileNameRev")) bRslt = false;
	if ( !setLotInformation(EVX_LotAuxDataFile, 		m_lotinfo.mir.AuxFile, 	"MIR.LotAuxDataFile")) bRslt = false;
	if ( !setLotInformation(EVX_LotTestPhase, 		m_lotinfo.mir.TestCod, 	"MIR.LotTestPhase")) bRslt = false;
	if ( !setLotInformation(EVX_LotUserText, 		m_lotinfo.mir.UserTxt, "MIR.LotUserText")) bRslt = false;
	if ( !setLotInformation(EVX_LotRomCode, 		m_lotinfo.mir.RomCod, 	"MIR.LotRomCode")) bRslt = false;
	if ( !setLotInformation(EVX_LotTesterSerNum, 		m_lotinfo.mir.SerlNum, 	"MIR.LotTesterSerNum")) bRslt = false;
	if ( !setLotInformation(EVX_LotTesterType, 		m_lotinfo.mir.TstrTyp, 	"MIR.LotTesterType")) bRslt = false;
	if ( !setLotInformation(EVX_LotSupervisor, 		m_lotinfo.mir.SuprNam, 	"MIR.LotSupervisor")) bRslt = false;
	if ( !setLotInformation(EVX_LotSystemName, 		m_lotinfo.mir.ExecTyp, 	"MIR.LotSystemName")) bRslt = false;
	if ( !setLotInformation(EVX_LotTargetName, 		m_lotinfo.mir.ExecVer, 	"MIR.LotTargetName")) bRslt = false;
	if ( !setLotInformation(EVX_LotTestSpecName, 		m_lotinfo.mir.SpecNam, 	"MIR.LotTestSpecName")) bRslt = false;
	if ( !setLotInformation(EVX_LotTestSpecRev, 		m_lotinfo.mir.SpecVer, 	"MIR.LotTestSpecRev")) bRslt = false;
	if ( !setLotInformation(EVX_LotProtectionCode, 		m_lotinfo.mir.ProtCod, 	"MIR.LotProtectionCode")) bRslt = false;
	if ( !setLotInformation(EVX_LotLotState, 		m_lotinfo.mir.RtstCod, 	"MIR.LotLotState")) bRslt = false;

	// use this commmand because there's no matching id from evxa
#if 0
	m_pProgCtrl->setExpression("TestProgData.BurnInTime", m_lotinfo.mir.BurnTim.c_str());
	if (m_lotinfo.mir.BurnTim.compare( m_pProgCtrl->getExpression("TestProgData.BurnInTime", EVX_SHOW_VALUE) ) == 0)
		m_Log << "Successfully set " << "TestProgData.BurnInTime" << " to '" << m_pProgCtrl->getExpression("TestProgData.BurnInTime", EVX_SHOW_VALUE) << "'" << CUtil::CLog::endl;
	else m_Log << "ERROR: Failed to set " << "TestProgData.BurnInTime" << ": '" << m_pProgCtrl->getExpression("TestProgData.BurnInTime", EVX_SHOW_VALUE) << "'" << CUtil::CLog::endl;
#endif
	// set SDR
	if ( !setLotInformation(EVX_LotProberHandlerID, 	m_lotinfo.sdr.HandId, 	"SDR.LotProberHandlerID")) bRslt = false;
	if ( !setLotInformation(EVX_LotHandlerType, 		m_lotinfo.sdr.HandTyp, 	"SDR.LotHandlerType")) bRslt = false;
	if ( !setLotInformation(EVX_LotCardId, 			m_lotinfo.sdr.CardId, 	"SDR.LotCardId")) bRslt = false;
	if ( !setLotInformation(EVX_LotCardType, 		m_lotinfo.sdr.CardTyp, 	"SDR.LotCardType")) bRslt = false;
	if ( !setLotInformation(EVX_LotLoadBrdId, 		m_lotinfo.sdr.LoadId, 	"SDR.LotLoadBrdId")) bRslt = false;
	if ( !setLotInformation(EVX_LotLoadBrdType, 		m_lotinfo.sdr.LoadTyp, 	"SDR.LotLoadBrdType")) bRslt = false;
	if ( !setLotInformation(EVX_LotIfCableId, 		m_lotinfo.sdr.CableId, 	"SDR.LotIfCableId")) bRslt = false;
	if ( !setLotInformation(EVX_LotIfCableType, 		m_lotinfo.sdr.CableTyp, "SDR.LotIfCableType")) bRslt = false;
	if ( !setLotInformation(EVX_LotContactorId, 		m_lotinfo.sdr.ContId, 	"SDR.LotContactorId")) bRslt = false;
	if ( !setLotInformation(EVX_LotContactorType, 		m_lotinfo.sdr.ContTyp, 	"SDR.LotContactorType")) bRslt = false;
	if ( !setLotInformation(EVX_LotLaserId, 		m_lotinfo.sdr.LasrId, 	"SDR.LotLaserId")) bRslt = false;
	if ( !setLotInformation(EVX_LotLaserType, 		m_lotinfo.sdr.LasrTyp,  "SDR.LotLaserType")) bRslt = false;
	if ( !setLotInformation(EVX_LotExtEquipId, 		m_lotinfo.sdr.ExtrId, 	"SDR.LotExtEquipId")) bRslt = false;
	if ( !setLotInformation(EVX_LotExtEquipType, 		m_lotinfo.sdr.ExtrTyp, 	"SDR.LotExtEquipType")) bRslt = false;
	if ( !setLotInformation(EVX_LotActiveLoadBrdName, 	m_lotinfo.sdr.DibId, 	"SDR.LotActiveLoadBrdName")) bRslt = false;
	if ( !setLotInformation(EVX_LotDIBType, 		m_lotinfo.sdr.DibTyp, 	"SDR.LotDIBType")) bRslt = false;

	// special case if customer is QUALCOMM (set in lotinfo.txt, which takes precedence)
	if (m_lotinfo.szCustomer.compare("QUALCOMM") == 0)
	{
		if ( !setLotInformation(EVX_LotEngrLotId, 		m_lotinfo.mir.LotId, 	"MIR.LotEngrLotId")) bRslt = false;
		if ( !setLotInformation(EVX_LotTestFacility, 		m_CONFIG.szTestSite, 	"MIR.LotTestFacility")) bRslt = false;
	}
	// if no customer is specified in lotinfo.txt and QUALCOMM is set as customer in config.xml, let's handle it
	else if (m_lotinfo.szCustomer.empty() && (m_CONFIG.szCustomer.compare("QUALCOMM") == 0) )
	{
		if ( !setLotInformation(EVX_LotEngrLotId, 		m_lotinfo.mir.LotId, 	"MIR.LotEngrLotId")) bRslt = false;
		if ( !setLotInformation(EVX_LotTestFacility, 		m_CONFIG.szTestSite, 	"MIR.LotTestFacility")) bRslt = false;
	}

	return bRslt;
}

