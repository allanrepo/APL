#include <app1.h>
#include <unistd.h>

/* ------------------------------------------------------------------------------------------
overriden state::run() function
------------------------------------------------------------------------------------------ */
void CAppState::run()
{
	// snapshot time now
	struct timeval now;
	gettimeofday(&now, NULL);

	for (std::list< CAppTask* >::iterator it = m_Tasks.begin(); it != m_Tasks.end(); it++)
	{			
		// is this a valid event?
		CAppTask* pTask = *it;
		if ( !pTask ) continue;

		// is this state enabled?
		if (!pTask->m_bEnabled) continue;
	
		// first time?
		if (pTask->m_bFirst)
		{
			pTask->m_bFirst = false;
			pTask->m_prev.tv_sec = now.tv_sec;
			pTask->m_prev.tv_usec = now.tv_usec;
		} 

		// calculate actual delta time (in milliseconds) for this event
		long nTimeMS = (((long)now.tv_sec - (long)pTask->m_prev.tv_sec ) * 1000);
		nTimeMS += (((long)now.tv_usec - (long)pTask->m_prev.tv_usec) / 1000);		

		// if elapsed time is already greater than this task's time-out, let's execute this event
		if ( nTimeMS >= pTask->m_nDelayMS )
		{
			// calculate number of times this interval happened within this elapsed time
			long nStep = pTask->m_nDelayMS > 0? nTimeMS / pTask->m_nDelayMS : 1;

			// execute event
			pTask->run();

			// update this event's timer
			nTimeMS = pTask->m_nDelayMS * nStep;
			pTask->m_prev.tv_sec += (nTimeMS / 1000);
			pTask->m_prev.tv_usec += (nTimeMS % 1000) * 1000;
		}
	}
}

/* ------------------------------------------------------------------------------------------
when state object is loaded
------------------------------------------------------------------------------------------ */
void CAppState::load()
{
	// enable all tasks
	for (std::list< CAppTask* >::iterator it = m_Tasks.begin(); it != m_Tasks.end(); it++)
	{
		// is this a valid event?
		CAppTask* pTask = *it;
		if ( !pTask ) continue;

		// enable and reset its timer
		pTask->enable();
		pTask->m_bFirst = true;				
	}
}

/* ------------------------------------------------------------------------------------------
constructor
------------------------------------------------------------------------------------------ */
CApp::CApp(int argc, char **argv)
{
	// parse command line arguments
	scan(argc, argv);

	// create init state and its tasks
	m_pStateOnInit = new CAppState("onInit");
	m_pStateOnInit->add(*this, &CApp::onInit, 0, "onInit");	
	m_pStateOnInit->add(*this, &CApp::onUpdateLogFile, 0, "onUpdateLogFile");	

	// create idle state and and its tasks
	m_pStateOnIdle = new CAppState("onIdle");
	m_pStateOnIdle->add(*this, &CApp::onConnect, 1000, "onConnect"); 
	m_pStateOnIdle->add(*this, &CApp::onSelect, 0, "onSelect"); 

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
utility function: parse command line arguments
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
	}	
	return true;
}

/* ------------------------------------------------------------------------------------------
utility function: get user name
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
TASK: update logger file output
------------------------------------------------------------------------------------------ */
void CApp::onUpdateLogFile(CState& state, CAppTask& task)
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
	ssLogToFile << m_CONFIG.szLogPath << "/apl." << szHostName << "." << (tmNow->tm_year + 1900);
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
void CApp::onInit(CState& state, CAppTask& task)
{
	m_Log << "Executing onInit() task..." << CUtil::CLog::endl;

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
	config( m_szConfigFullPathName );

	// create notify file descriptor and add to fd manager. this will monitor incoming lotinfo.txt file
	m_pMonitorFileDesc = new CMonitorFileDesc(*this, &CApp::onReceiveFile, m_CONFIG.szLotInfoFilePath);
	m_FileDescMgr.add( *m_pMonitorFileDesc );

	// we do this only once	so we disable after
	task.disable();

	// we also switch to next state: on idle
	m_StateMgr.set(m_pStateOnIdle);
}

void CApp::onConnect(CState& state, CAppTask& task)
{
	// are we connected to tester? should we try? attempt once
	if (!isReady()) connect(m_szTesterName, 1);
}

void CApp::onSelect(CState& state, CAppTask& task)
{
	// proccess any file descriptor notification on select every second.
	m_FileDescMgr.select(20);
}

void CApp::onReceiveFile(const std::string& name)
{
	m_Log << "file: " << name << CUtil::CLog::endl;
}

/* ------------------------------------------------------------------------------------------
parse config xml file
------------------------------------------------------------------------------------------ */
bool CApp::config(const std::string& file)
{
	m_Log << "Parsing " << file << "..." << CUtil::CLog::endl;

	// open xml config file and try to extract root tag	
	XML_Node *root = new XML_Node (file.c_str());
	if (root)
	{
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
		m_Log << "<SiteConfiguration> : ";
		for( int i = 0; i < root->numChildren(); i++ )
		{
			// only <SiteConfiguration> is handled, we ignore everything else
			if (!root->fetchChild(i)) continue;
			if (root->fetchChild(i)->fetchTag().compare("SiteConfiguration") != 0) continue;

			m_Log << "'" << root->fetchChild(i)->fetchVal("name") <<  "', ";

			// we take only the first <CurrentSiteConfiguration> match, ignore succeeding ones
			if ( root->fetchChild(i)->fetchVal("name").compare( pCurrSiteConfig->fetchText() ) == 0 ){ if (!pConfig) pConfig = root->fetchChild(i); }
		}
		m_Log << CUtil::CLog::endl;
	
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

				if (pLaunch->fetchChild(i)->fetchVal("name").compare("Type") == 0){ m_CONFIG.bProd = pLaunch->fetchChild(i)->fetchText().compare("prod") == 0? true: false; }					
				if (pLaunch->fetchChild(i)->fetchVal("name").compare("Wait Time To Relaunch") == 0){ m_CONFIG.nRelaunchTimeOutMS = CUtil::toLong( pLaunch->fetchChild(i)->fetchText() ) * 1000; }					
				if (pLaunch->fetchChild(i)->fetchVal("name").compare("Max Attempt to Relaunch") == 0){ m_CONFIG.nRelaunchAttempt = CUtil::toLong( pLaunch->fetchChild(i)->fetchText() ); }				
			}
		}
		else m_Log << "Warning: Didn't find <Launch>. " << CUtil::CLog::endl;


		// let's now find the <Binning> from <SiteConfiguration> 
		XML_Node* pBinning = 0;
		if (pConfig->fetchChild("Binning")){ if ( CUtil::toUpper( pConfig->fetchChild("Binning")->fetchVal("state") ).compare("TRUE") == 0) pBinning = pConfig->fetchChild("Binning"); }
		else m_Log << "Warning: Didn't find <Binning>. " << CUtil::CLog::endl;

		// check if <logging> is set
		XML_Node* pLogging = 0;
		if (pConfig->fetchChild("Logging")){ if ( CUtil::toUpper( pConfig->fetchChild("Logging")->fetchVal("state") ).compare("TRUE") == 0) pLogging = pConfig->fetchChild("Logging"); }
		else m_Log << "Warning: Didn't find <Logging>. " << CUtil::CLog::endl;

		// check if <LotInfo> is set
		if (pConfig->fetchChild("LotInfo"))
		{
			if (pConfig->fetchChild("LotInfo")->fetchChild("File")){ m_CONFIG.szLotInfoFileName = pConfig->fetchChild("LotInfo")->fetchChild("File")->fetchText(); }
			if (pConfig->fetchChild("LotInfo")->fetchChild("Path")){ m_CONFIG.szLotInfoFilePath = pConfig->fetchChild("LotInfo")->fetchChild("Path")->fetchText(); }		
		}
		else m_Log << "Warning: Didn't find <LotInfo>. " << CUtil::CLog::endl;


		// for debug purposes, print out all the parameters of <Binning>
		if (pBinning)
		{
			for (int i = 0; i < pBinning->numChildren(); i++)
			{
				if ( pBinning->fetchChild(i) ) 
				{ 
					m_Log << "<" << pBinning->fetchChild(i)->fetchTag() << ">: '" << pBinning->fetchChild(i)->fetchText() << "'" << CUtil::CLog::endl; 
				}
			}
		}

		// lets extract STDF stuff
		for (int i = 0; i < pConfig->numChildren(); i++)
		{
			if (!pConfig->fetchChild(i)) continue;
			if (pConfig->fetchChild(i)->fetchTag().compare("STDF") != 0) continue;
			
			XML_Node* pStdf = pConfig->fetchChild(i);
			
			// if an <STDF> tag is found with attribute state = true, enable STDF feature	
			if (CUtil::toUpper( pStdf->fetchVal("state") ).compare("TRUE") == 0){ m_CONFIG.bSendInfo = true; }

			m_Log << "<STDF>: '" << pStdf->fetchVal("Param") << "'" << CUtil::CLog::endl;
			for (int j = 0; j < pStdf->numChildren(); j++)
			{
				if (!pStdf->fetchChild(j)) continue;
				if (pStdf->fetchChild(j)->fetchTag().compare("Param") != 0) continue;					
				
				m_Log << "	" << pStdf->fetchChild(j)->fetchVal("name") << ": " << pStdf->fetchChild(j)->fetchText() << CUtil::CLog::endl;
			}			
		}

		// if we found binning node, it means <Binning> is in config which means binning@EOT is enabled
		if (pBinning)
		{
			m_CONFIG.bSendBin = true;
			if (pBinning->fetchChild("BinType"))
			{
				if (CUtil::toUpper(pBinning->fetchChild("BinType")->fetchText()).compare("HARD") == 0) m_CONFIG.bUseHardBin = true;
				if (CUtil::toUpper(pBinning->fetchChild("BinType")->fetchText()).compare("SOFT") == 0) m_CONFIG.bUseHardBin = false;
			}
			if (pBinning->fetchChild("TestType"))
			{
				if (CUtil::toUpper(pBinning->fetchChild("TestType")->fetchText()).compare("WAFER") == 0) m_CONFIG.nTestType = CONFIG::APL_WAFER;
				if (CUtil::toUpper(pBinning->fetchChild("TestType")->fetchText()).compare("FINAL") == 0) m_CONFIG.nTestType = CONFIG::APL_FINAL;
			}
			if (pBinning->fetchChild("SocketType"))
			{
				if (CUtil::toUpper(pBinning->fetchChild("SocketType")->fetchText()).compare("UDP") == 0) m_CONFIG.nSocketType = SOCK_DGRAM;
				if (CUtil::toUpper(pBinning->fetchChild("SocketType")->fetchText()).compare("TCP") == 0) m_CONFIG.nSocketType = SOCK_STREAM;
				if (CUtil::toUpper(pBinning->fetchChild("SocketType")->fetchText()).compare("RAW") == 0) m_CONFIG.nSocketType = SOCK_RAW;
			}
			if (pBinning->fetchChild("IP")){ m_CONFIG.IP = pBinning->fetchChild("IP")->fetchText(); }	
			if (pBinning->fetchChild("Port")){ m_CONFIG.nPort = CUtil::toLong(pBinning->fetchChild("Port")->fetchText()); }	
		}

		// if <logging> was set, let's see if logging is enabled and file path is specified
		if (pLogging)
		{
			m_CONFIG.bLogToFile = true;
			if (pLogging->fetchChild("Path")){ m_CONFIG.szLogPath = pLogging->fetchChild("Path")->fetchText(); }
		}

	}	

	if (root) delete root;
	return true;
}


