#include <app1.h>
#include <unistd.h>

/* ------------------------------------------------------------------------------------------
constructor
------------------------------------------------------------------------------------------ */
CApp::CApp(int argc, char **argv)
{
	// parse command line arguments
	scan(argc, argv);

	// create init state and its tasks
	m_pStateOnInit = new CState("onInit");
	CSequence* pseq = new CSequence("onInit");
	pseq->queue(new CAppTask(*this, &CApp::onInit, 0, "onInit"));
	pseq->queue(new CAppTask(*this, &CApp::onUpdateLogFile, 0, "onUpdateLogFile"));
	pseq->queue(new CAppTask(*this, &CApp::onSwitchToIdleState, 0, "onSwitchToIdleState"));
	m_pStateOnInit->add(pseq);	

	// create idle state and and its tasks
	m_pStateOnIdle = new CState("onIdle");
	pseq = new CSequence("onConnect", true);
	pseq->queue(new CAppTask(*this, &CApp::onConnect, 1000, "onConnect", true));
	m_pStateOnIdle->add(pseq); 
	m_pStateOnIdle->add(new CAppTask(*this, &CApp::onSelect, 0, "onSelect", true)); 

	// setup launch state and its asks
	m_pStateOnLaunch = new CState("onLaunch");
	m_pStateOnLaunch->add(new CAppTask(*this, &CApp::onLaunch, 0, "onLaunch", true)); 
	m_pStateOnLaunch->add(new CAppTask(*this, &CApp::onSelect, 0, "onSelect", true)); 

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
TASK: update logger file output
------------------------------------------------------------------------------------------ */
void CApp::onUpdateLogFile(CTask& task)
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
void CApp::onInit(CTask& task)
{
	m_Log << "Executing onInit() task..." << CUtil::CLog::endl;

	// if tester name is not set through command line argument, assign default name
	if (m_szTesterName.empty())
	{
		std::stringstream ss;
		getUserName().empty()? (ss << "sim") : (ss << getUserName() << "_sim");
		m_szTesterName = ss.str();
	}

	m_szProgramFullPathName = "";

	// if config file is not set, assign default
	if (m_szConfigFullPathName.empty()){ m_szConfigFullPathName = "./config.xml"; }

	// parse config file
	config( m_szConfigFullPathName );

	// create notify file descriptor and add to fd manager. this will monitor incoming lotinfo.txt file
	m_pMonitorFileDesc = new CMonitorFileDesc(*this, &CApp::onReceiveFile, m_CONFIG.szLotInfoFilePath);
	m_FileDescMgr.add( *m_pMonitorFileDesc );

	// get file descriptor of tester state notification and add to our fd manager		
	m_pStateNotificationFileDesc = new CAppFileDesc(*this, &CApp::onStateNotificationResponse, m_pState? m_pState->getSocketId(): -1);
 	m_FileDescMgr.add( *m_pStateNotificationFileDesc );
}

/* ------------------------------------------------------------------------------------------
TASK: 	attempts to connect to tester
------------------------------------------------------------------------------------------ */
void CApp::onConnect(CTask& task)
{
	// are we connected to tester? should we try? attempt once
	if (!isReady()) connect(m_szTesterName, 1);
}

/* ------------------------------------------------------------------------------------------
TASK: 	query select(), for input from inotify of incoming file
------------------------------------------------------------------------------------------ */
void CApp::onSelect(CTask& task)
{
	// if the inotify fires up with an incoming file, it will sometimes fire up twice
	// for the same file. we don't want to process same file more than once so as soon
	// as we get one, we "ignore" the others by setting this flag to true. only then when
	// this function is executed again that we enable it.	
	m_bIgnoreFile = false;

	// before calling select(), update file descriptors of evxa objects. this is to ensure the FD manager
	// don't use a bad file descriptor. when evxa objects are destroyed, their file descriptors are considered bad.
	m_pStateNotificationFileDesc->set(m_pState? m_pState->getSocketId(): -1);

	// proccess any file descriptor notification on select every second.
	m_FileDescMgr.select(200);
}

/* ------------------------------------------------------------------------------------------
TASK: 	end lof if any, unload program if any, kill tester if any,
	kill all unison threads if any, then launch OICu
------------------------------------------------------------------------------------------ */
void CApp::onLaunch(CTask& task)
{
	m_Log << "onLaunch State Task" << CUtil::CLog::endl;
	sleep(1);
}

/* ------------------------------------------------------------------------------------------
Utility: check the file if this is lotinfo.txt; parse if yes
------------------------------------------------------------------------------------------ */
void CApp::onReceiveFile(const std::string& name)
{
	// create string that holds full path + monitor file 
	std::stringstream ssFullPathMonitorName;
	ssFullPathMonitorName << m_CONFIG.szLotInfoFilePath << "/" << name;

	// is this the file we're waiting for? if not, bail out
	if (name.compare(m_CONFIG.szLotInfoFileName) != 0)
	{
		m_Log << "File received but is not what we're waiting for: " << name << CUtil::CLog::endl;
		return;
	}
	else m_Log << "'" << name << "' file received." << CUtil::CLog::endl;

	// is this that double inotify event trigger? if yes, let's ignore this.	
	if (m_bIgnoreFile)
	{ 
		m_Log << "Detected multiple events from inotify. ignoring this..." << CUtil::CLog::endl;
		return; 
	}

	// if this is the file, let's parse it.
	// does it contain JobFile field? if yes, does it point to a valid unison program? 
	// if yes, let's use this file contents to load program
	// from here on, we switch to launch OICu and load program state
	if (!parse(ssFullPathMonitorName.str()))
	{
		m_Log << ssFullPathMonitorName.str() << " file received but didn't find a program to load.";
		m_Log << " can you check if " << name << " has '" << JOBFILE << "' field. and is valid?" << CUtil::CLog::endl;
		return;
	}		

	// is there a lot being tested? if yes, let's end the lot.
	if ( m_pProgCtrl->setEndOfLot(EVXA::WAIT, true) != EVXA::OK)
	{
		m_Log << "ERROR: something went wrong in ending lot..." << CUtil::CLog::endl;
		return;
	}
	


//	else
	{
		m_Log << "successfully parsed '" << ssFullPathMonitorName.str() << "'" << CUtil::CLog::endl;
		m_bIgnoreFile = true;
		m_StateMgr.set(m_pStateOnLaunch);
		return;
	}
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
 
	MIR l_MIR;
	SDR l_SDR;
	std::string szJobFile;
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
			if (field.compare("OPERATOR") == 0){ l_MIR.OperNam = value; continue; }
			if (field.compare("LOTID") == 0){ l_MIR.LotId = value; continue; }
			if (field.compare("SUBLOTID") == 0){ l_MIR.SblotId = value; continue; }
			if (field.compare("DEVICE") == 0){ l_MIR.PartTyp = value; continue; }
			if (field.compare("PRODUCTID") == 0){ l_MIR.FamlyId = value; continue; }
			if (field.compare("PACKAGE") == 0){ l_MIR.PkgTyp = value; continue; }
			if (field.compare("FILENAMEREV") == 0){ l_MIR.JobRev = value; continue; }
			if (field.compare("TESTMODE") == 0){ l_MIR.ModeCod = value; continue; }
			if (field.compare("COMMANDMODE") == 0){ l_MIR.CmodCod = value; continue; }
			if (field.compare("ACTIVEFLOWNAME") == 0){ l_MIR.FlowId = value; continue; }
			if (field.compare("DATECODE") == 0){ l_MIR.DateCod = value; continue; }
			if (field.compare("CONTACTORTYPE") == 0){ l_SDR.ContTyp = value; continue; }
			if (field.compare("CONTACTORID") == 0){ l_SDR.ContId = value; continue; }
			if (field.compare("FABRICATIONID") == 0){ l_MIR.ProcId = value; continue; }
			if (field.compare("TESTFLOOR") == 0){ l_MIR.FloorId = value; continue; }
			if (field.compare("TESTFACILITY") == 0){ l_MIR.FacilId = value; continue; }
			if (field.compare("PROBERHANDLERID") == 0){ l_SDR.HandId = value; continue; }
			if (field.compare("TEMPERATURE") == 0){ l_MIR.TestTmp = value; continue; }
			if (field.compare("BOARDID") == 0){ l_SDR.LoadId = value; continue; }

			// if we didn't find anything we looked for including jobfile, let's move to next field
			if (field.compare(JOBFILE) == 0)
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
					szJobFile = value;			
				}
			}
		}
	
		// if this is the last line then bail
		if (pos == std::string::npos) break;		
	}	

	// ok did we find a valid JobFile in the lotinfo.txt?
	if (szJobFile.empty()) return false;
	else
	{
		m_szProgramFullPathName = szJobFile;
		m_MIR = l_MIR;
		m_SDR = l_SDR;
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
	value = line.substr(pos + 1);
	if (value.empty())
	{
		m_Log << "WARNING: This line: '" << line << "' has empty value '" << CUtil::CLog::endl;
		return false;
	}

	// strip leading/trailing space 
	field = CUtil::removeLeadingTrailingSpace(field);
	value = CUtil::removeLeadingTrailingSpace(value);

	// captilaze field
	for (std::string::size_type i = 0; i < field.size(); i++){ field[i] = CUtil::toUpper(field[i]); }

	return true;
}

/* ------------------------------------------------------------------------------------------
event handler for state notification EOT
------------------------------------------------------------------------------------------ */
void CApp::onEndOfTest(const int array_size, int site[], int serial[], int sw_bin[], int hw_bin[], int pass[], EVXA_ULONG dsp_status)
{
	m_Log << "EOT" << CUtil::CLog::endl;
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
			break;
		}
		case EVX_LOT_START:
		{
			m_Log << "Lot Start" << CUtil::CLog::endl;
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
			m_Log << "Wafer End" << CUtil::CLog::endl;
			break;
		}
		case EVX_WAFER_START:
		{
			m_Log << "Wafer Start" << CUtil::CLog::endl;
			break;
		}
		default: break; 
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
	}  		
}

