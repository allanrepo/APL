#include <app.h>

/* ------------------------------------------------------------------------------------------
constructor
------------------------------------------------------------------------------------------ */
CApp::CApp(int argc, char **argv)
{
	// initialize variables
	init();

	// scan command line args
	scan(argc, argv);

	// if tester name is not set, assign default name
	if (m_szTesterName.empty())
	{
		std::stringstream ss;
		getUserName().empty()? (ss << "sim") : (ss << getUserName() << "_sim");
		m_szTesterName = ss.str();
	}

	// if file name to monitor is not set, assign default
	if (m_CONFIG.szLotInfoFileName.empty()){ m_CONFIG.szLotInfoFileName = "lotinfo.txt"; }

	// if path to monitor is not set, assign default
	if (m_CONFIG.szLotInfoFilePath.empty()){ m_CONFIG.szLotInfoFilePath = "/tmp"; }

	// if config file is not set, assign default
	if (m_szConfigFullPathName.empty()){ m_szConfigFullPathName = "./config.xml"; }

	// force logger to log everything from here onwards
	m_Log.silent = false;

	// parse xml config file. default is ./config.xml
	config( m_szConfigFullPathName );

	// setup logger file output if this feature is enabled
	initLogger(m_CONFIG.bLogToFile);

	// print out settings 
	m_Log << "Version: " << VERSION << CUtil::CLog::endl;
	m_Log << "Developer: " << DEVELOPER << CUtil::CLog::endl;
	m_Log << "Tester: " << m_szTesterName << CUtil::CLog::endl; 
	m_Log << "Path: " << m_CONFIG.szLotInfoFilePath << CUtil::CLog::endl;
	m_Log << "File: " << m_CONFIG.szLotInfoFileName << CUtil::CLog::endl;
	m_Log << "Config File: " << m_szConfigFullPathName << CUtil::CLog::endl;
	m_Log << "Binning: " << (m_CONFIG.bSendBin? "enabled" : "disabled") << CUtil::CLog::endl;
	m_Log << "Bin type (if binning enabled): " << (m_CONFIG.bUseHardBin? "hard" : "soft") << CUtil::CLog::endl;
	m_Log << "Test type (if binning enabled): ";
	switch (m_CONFIG.nTestType)
	{
		case CONFIG::APL_WAFER: m_Log << "wafer test" << CUtil::CLog::endl; break;
		case CONFIG::APL_FINAL: m_Log << "final test" << CUtil::CLog::endl; break;
		default: m_Log << "unknown" << CUtil::CLog::endl; break;
	};
	m_Log << "IP: " << m_CONFIG.IP << CUtil::CLog::endl;
	m_Log << "Port: " << m_CONFIG.nPort << CUtil::CLog::endl;
	m_Log << "Socket type (if binning enabled): ";
	switch (m_CONFIG.nSocketType)
	{
		case SOCK_DGRAM: m_Log << "UDP" << CUtil::CLog::endl; break;
		case SOCK_STREAM: m_Log << "TCP" << CUtil::CLog::endl; break;
		case SOCK_RAW: m_Log << "RAW" << CUtil::CLog::endl; break;
		default: m_Log << "unknown" << CUtil::CLog::endl; break;
	};
	m_Log << "Log To File: " << (m_CONFIG.bLogToFile? "enabled" : "disabled") << CUtil::CLog::endl;
	m_Log << "Log Path (if enabled): " << m_CONFIG.szLogPath << CUtil::CLog::endl;
	m_Log << "LotInfo to STDF: " << (m_CONFIG.bSendInfo? "enabled" : "disabled") << CUtil::CLog::endl;
	

	// create notify file descriptor and add to fd manager. this will monitor incoming lotinfo.txt file
	m_pMonitorFileDesc = new CMonitorFileDesc(*this, m_CONFIG.szLotInfoFilePath);
	m_FileDescMgr.add( *m_pMonitorFileDesc );

	// get file descriptor of tester state notification and add to our fd manager		
	CAppFileDesc StateNotificationFileDesc(*this, &CApp::onStateNotificationResponse, m_pState? m_pState->getSocketId(): -1);
 	m_FileDescMgr.add( StateNotificationFileDesc );

	// get file descriptor of tester evxio stream and add to our fd manager		
	CAppFileDesc StateEvxioStreamFileDesc(*this, &CApp::onEvxioResponse, m_pEvxio? m_pEvxio->getEvxioSocketId(): -1);
	m_FileDescMgr.add( StateEvxioStreamFileDesc );

	// get file descriptor of tester evxio error and add to our fd manager		
	CAppFileDesc StateEvxioErrorFileDesc(*this, &CApp::onErrorResponse, m_pEvxio? m_pEvxio->getErrorSocketId(): -1);
	m_FileDescMgr.add( StateEvxioErrorFileDesc );

	// run the application loop
	while(1) 
	{
		// are we connected to tester? should we try? attempt once
		//if(!isReady())
		if(m_bReconnect)
		{
			if (connect(m_szTesterName, 1))	m_bReconnect = false;
		}

		// before calling select(), update file descriptors of evxa objects. this is to ensure the FD manager
		// don't use a bad file descriptor. when evxa objects are destroyed, their file descriptors are considered bad.
		StateNotificationFileDesc.set(m_pState? m_pState->getSocketId(): -1);
		StateEvxioStreamFileDesc.set(m_pEvxio? m_pEvxio->getEvxioSocketId(): -1);
		StateEvxioErrorFileDesc.set(m_pEvxio? m_pEvxio->getErrorSocketId(): -1);

		// proccess any file descriptor notification on select every second.
		m_FileDescMgr.select(1000);

		if (m_bSTDF)
		{
			if (!m_pProgCtrl) continue;
			if (m_pProgCtrl->isProgramLoaded())
			{
				m_Log << "Program is loaded, setting lot information..." <<CUtil::CLog::endl;
				setSTDF();
				m_bSTDF = false;
			}
		}

		// file where logs are to be saved (if enabled) doesn't automatically change its name to updated timestamp so this is the only
		// place in the app where we can do that. so if we reach this point in the app loop, let's take the opportunity to update
		// log file's name
		initLogger(m_CONFIG.bLogToFile);
	}
}

/* ------------------------------------------------------------------------------------------
destructor
------------------------------------------------------------------------------------------ */
CApp::~CApp()
{
	
}

/* ------------------------------------------------------------------------------------------
initialize variables, reset params
------------------------------------------------------------------------------------------ */
void CApp::init()
{
	// config.xml by default is found within APL's base folder (here executable is)
	m_szConfigFullPathName = "./config.xml";

	// these variables will hold tester stuff. clear by default
	m_szProgramFullPathName = "";
	m_szTesterName = "";
	
	// used for telling this app that it's ready to set lotinfo params
	m_bSTDF = false;

	// flag used to request for tester reconnect, true by default so it tries to connect on launch
	m_bReconnect = true;

	// set lotinfo file parameters to default
	m_CONFIG.szLotInfoFileName = "lotinfo.txt";
	m_CONFIG.szLotInfoFilePath = "/tmp";

	// binning @EOT is disabled by default
	m_CONFIG.bSendBin = false;
	m_CONFIG.bUseHardBin = false;
	m_CONFIG.nTestType = CONFIG::APL_FINAL;
	m_CONFIG.IP = "127.0.0.1";
	m_CONFIG.nPort = 54000;	
	m_CONFIG.nSocketType = SOCK_STREAM;

	// logging to file is disabled by default
	m_CONFIG.bLogToFile = false;
	m_CONFIG.szLogPath = "/tmp";

	// sending data from lotinfo to unison's lotinformation/stdf is disabled by default
	m_CONFIG.bSendInfo = false;
}
 
/* ------------------------------------------------------------------------------------------
initialize logger file 
------------------------------------------------------------------------------------------ */
void CApp::initLogger( bool bEnable )
{
	// if logger to file is disabled, let's quickly reset it and bail
	if (!bEnable)
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
	ssLogToFile << m_CONFIG.szLogPath << "/apl." << szHostName << "." << (tmNow->tm_year + 1900) << (tmNow->tm_mon + 1) << tmNow->tm_mday << ".log";
	
	// if this new log file name the same one already set in the logger? if yes, then we don't have to do anything
	if (ssLogToFile.str().compare( m_Log.file() ) == 0) return;

	// otherwise, set this 
	m_Log.file(ssLogToFile.str());
	m_Log << "Log is saved to " << ssLogToFile.str() << CUtil::CLog::endl;
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

			m_Log << "<STDF>: '" << pStdf->fetchVal("field") << "'" << CUtil::CLog::endl;
			for (int j = 0; j < pStdf->numChildren(); j++)
			{
				if (!pStdf->fetchChild(j)) continue;
				if (pStdf->fetchChild(j)->fetchTag().compare("Field") != 0) continue;					
				
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
parse command line arguments
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
get user name
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
process the incoming file from the monitored path
------------------------------------------------------------------------------------------ */
void CApp::onReceiveFile(const std::string& name)
{
	// create string that holds full path + monitor file 
	std::stringstream ssFullPathMonitorName;
	ssFullPathMonitorName << m_CONFIG.szLotInfoFilePath << "/" << name;

	// is this file lotinfo.txt?   
	if (name.compare(m_CONFIG.szLotInfoFileName) != 0)
	{
		m_Log << "File received but is not what we're waiting for: " << name << CUtil::CLog::endl;
		return;
	}
	else m_Log << name << " file received." << CUtil::CLog::endl;

	// parse lotinfo.txt file 
	m_szProgramFullPathName.clear();
	parse(ssFullPathMonitorName.str());

	// for debug purposes, lets print out STDF stuff here
	printSTDF();

	// do we have the 'PROGRAM' field and its value from lotinfo.txt?
	if (m_szProgramFullPathName.empty()) 
	{
		m_Log << ssFullPathMonitorName.str() << " file received but didn't find a program to load.";
		m_Log << " can you check if " << name << " has '" << JOBFILE << "' field." << CUtil::CLog::endl;
	}
	// look's like we're good to launch OICu and load program. let's do it
	else
	{
		// are we connected?
		if (isReady())
		{
			m_Log << "We're connected and will try to disconnect. " << CUtil::CLog::endl;

			// any program loaded? unload it
			if (m_pProgCtrl->isProgramLoaded()) 
			{
				m_Log << "But first, we will unload current program loaded... " << CUtil::CLog::endl;
				unload(true, 10);
			}
		}

		// in case we're connected from previous OICu load, let's make sure we're disconnected now.
		disconnect();

		// let's kill any OICu running right now
		std::stringstream ssCmd;
		ssCmd << "./" << KILLAPPCMD << " " << OICU;
		m_Log << "KILL: " << ssCmd.str() << CUtil::CLog::endl;		
		system(ssCmd.str().c_str());

		// let's kill any OpTool running right now
		ssCmd.str(std::string());
		ssCmd.clear();
		ssCmd << "./" << KILLAPPCMD << " " << OPTOOL;
		m_Log << "KILL: " << ssCmd.str() << CUtil::CLog::endl;		
		system(ssCmd.str().c_str());

		// let's kill any dataviewer running right now
		ssCmd.str(std::string());
		ssCmd.clear();
		ssCmd << "./" << KILLAPPCMD << " " << DATAVIEWER;
		m_Log << "KILL: " << ssCmd.str() << CUtil::CLog::endl;		
		system(ssCmd.str().c_str());

		// let's kill any bintool running right now
		ssCmd.str(std::string());
		ssCmd.clear();
		ssCmd << "./" << KILLAPPCMD << " " << "binTool";
		m_Log << "KILL: " << ssCmd.str() << CUtil::CLog::endl;		
		system(ssCmd.str().c_str());

		// let's kill any testtool running right now
		ssCmd.str(std::string());
		ssCmd.clear();
		ssCmd << "./" << KILLAPPCMD << " " << "testTool";
		m_Log << "KILL: " << ssCmd.str() << CUtil::CLog::endl;		
		system(ssCmd.str().c_str());

		ssCmd.str(std::string());
		ssCmd.clear();
		ssCmd << "./" << KILLAPPCMD << " " << "flowTool";
		m_Log << "KILL: " << ssCmd.str() << CUtil::CLog::endl;		
		system(ssCmd.str().c_str());

		ssCmd.str(std::string());
		ssCmd.clear();
		ssCmd << "./" << KILLAPPCMD << " " << "errorTool";
		m_Log << "KILL: " << ssCmd.str() << CUtil::CLog::endl;		
		system(ssCmd.str().c_str());

		// kill tester
		ssCmd.str(std::string());
		ssCmd.clear();
		ssCmd << "./" << KILLTESTERCMD << " " << m_szTesterName;
		m_Log << "END TESTER: " << ssCmd.str() << CUtil::CLog::endl;		
		system(ssCmd.str().c_str());

		// launch OICu and load program
		launch(m_szTesterName, m_szProgramFullPathName, true);

		// let app know we are setting STDF fields. do it only if this feature is enabled
		m_bSTDF = true & m_CONFIG.bSendInfo;
	}
 
	// delete the lotinfo.txt
	unlink(ssFullPathMonitorName.str().c_str());
}

/* ------------------------------------------------------------------------------------------
launches unison (OICu or OpTool)
- 	after sending launch command, we're also setting APL to try reconnecting to tester
------------------------------------------------------------------------------------------ */
bool CApp::launch(const std::string& tester, const std::string& program, bool bProd)
{
	m_Log << "launching OICu and loading '" << program << "'..." << CUtil::CLog::endl;

	// setup system command to launch OICu
	// >launcher -nodisplay -prod -load <program> -T <tester> -qual
	std::stringstream ssCmd;
	ssCmd << "launcher -nodisplay " << (bProd? "-prod " : "") << (program.empty()? "": "-load ") << (program.empty()? "" : program) << " -qual " << " -T " << m_szTesterName ;
	m_Log << "LAUNCH: " << ssCmd.str() << CUtil::CLog::endl;
	system(ssCmd.str().c_str());	

	// let's try to connect to tester in our main loop
	m_bReconnect = true;

	return true;
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
 
/* ------------------------------------------------------------------------------------------

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

	// before parsing, let's reset STDF field variables
	m_MIR.clear();
	m_SDR.clear();

	// for debugging purpose, let's dump the contents of the file
	m_Log << "---------------------------------------------------------------------" << CUtil::CLog::endl;
	m_Log << "Content extracted from " << name << "." << CUtil::CLog::endl;
	m_Log << "It's file size is " << s.length() << " bytes. Contents: " << CUtil::CLog::endl;
	m_Log << s << CUtil::CLog::endl;
	m_Log << "---------------------------------------------------------------------" << CUtil::CLog::endl;
 
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
			if (field.compare("OPERATOR") == 0){ m_MIR.OperNam = value; continue; }
			if (field.compare("LOTID") == 0){ m_MIR.LotId = value; continue; }
			if (field.compare("SUBLOTID") == 0){ m_MIR.SblotId = value; continue; }
			if (field.compare("DEVICE") == 0){ m_MIR.PartTyp = value; continue; }
			if (field.compare("PRODUCTID") == 0){ m_MIR.FamlyId = value; continue; }
			if (field.compare("PACKAGE") == 0){ m_MIR.PkgTyp = value; continue; }
			if (field.compare("FILENAMEREV") == 0){ m_MIR.JobRev = value; continue; }
			if (field.compare("TESTMODE") == 0){ m_MIR.ModeCod = value; continue; }
			if (field.compare("COMMANDMODE") == 0){ m_MIR.CmodCod = value; continue; }
			if (field.compare("ACTIVEFLOWNAME") == 0){ m_MIR.FlowId = value; continue; }
			if (field.compare("DATECODE") == 0){ m_MIR.DateCod = value; continue; }
			if (field.compare("CONTACTORTYPE") == 0){ m_SDR.ContTyp = value; continue; }
			if (field.compare("CONTACTORID") == 0){ m_SDR.ContId = value; continue; }
			if (field.compare("FABRICATIONID") == 0){ m_MIR.ProcId = value; continue; }
			if (field.compare("TESTFLOOR") == 0){ m_MIR.FloorId = value; continue; }
			if (field.compare("TESTFACILITY") == 0){ m_MIR.FacilId = value; continue; }
			if (field.compare("PROBERHANDLERID") == 0){ m_SDR.HandId = value; continue; }
			if (field.compare("TEMPERATURE") == 0){ m_MIR.TestTmp = value; continue; }
			if (field.compare("BOARDID") == 0){ m_SDR.LoadId = value; continue; }

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
					m_szProgramFullPathName = value;			
				}
			}
		}
	
		// if this is the last line then bail
		if (pos == std::string::npos) break;		
	}	

	return true;
} 

/* ------------------------------------------------------------------------------------------
get field/value pair string of a line from lotinfo.txt file content loaded into string
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

------------------------------------------------------------------------------------------ */
void CApp::printSTDF()
{
	m_Log << "OPERATOR: " << m_MIR.OperNam << CUtil::CLog::endl;
	m_Log << "DEVICE: " << m_MIR.PartTyp << CUtil::CLog::endl;
	m_Log << "LOTID: " << m_MIR.LotId << CUtil::CLog::endl;
	m_Log << "PRODUCTID: " << m_MIR.FamlyId << CUtil::CLog::endl;
	m_Log << "TEMP: " << m_MIR.TestTmp << CUtil::CLog::endl;
	m_Log << "PROBERHANDLERID: " << m_SDR.HandId << CUtil::CLog::endl;
	m_Log << "BOARDID: " << m_SDR.LoadId << CUtil::CLog::endl;
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
			m_Log << "ERROR: Failed to set Lot Information to '" << field << "'" << CUtil::CLog::endl;
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
		return false;
	}
}

/* ------------------------------------------------------------------------------------------

------------------------------------------------------------------------------------------ */
void CApp::setSTDF()
{
	// make sure we have valid evxa object
	if (!m_pProgCtrl) return;

	// set MIR
	setLotInformation(EVX_LotLotID, 		m_MIR.LotId, 	"MIR.LotLotID");
	setLotInformation(EVX_LotCommandMode, 		m_MIR.CmodCod, 	"MIR.LotCommandMode");
	setLotInformation(EVX_LotActiveFlowName, 	m_MIR.FlowId, 	"MIR.LotActiveFlowName");
	setLotInformation(EVX_LotDesignRev, 		m_MIR.DsgnRev, 	"MIR.LotDesignRev");
	setLotInformation(EVX_LotDateCode, 		m_MIR.DateCod, 	"MIR.LotDateCode");
	setLotInformation(EVX_LotOperFreq, 		m_MIR.OperFrq, 	"MIR.LotOperFreq");
	setLotInformation(EVX_LotOperator, 		m_MIR.OperNam, 	"MIR.LotOperator");
	setLotInformation(EVX_LotTcName, 		m_MIR.NodeNam, 	"MIR.LotTcName");
	setLotInformation(EVX_LotDevice, 		m_MIR.PartTyp, 	"MIR.LotDevice");
	setLotInformation(EVX_LotEngrLotId, 		m_MIR.EngId, 	"MIR.LotEngrLotId");
	setLotInformation(EVX_LotTestTemp, 		m_MIR.TestTmp, 	"MIR.LotTestTemp");
	setLotInformation(EVX_LotTestFacility, 		m_MIR.FacilId, 	"MIR.LotTestFacility");
	setLotInformation(EVX_LotTestFloor, 		m_MIR.FloorId, 	"MIR.LotTestFloor");
	setLotInformation(EVX_LotHead, 			m_MIR.StatNum, 	"MIR.LotHead");
	setLotInformation(EVX_LotFabricationID, 	m_MIR.ProcId, 	"MIR.LotFabricationID");
	setLotInformation(EVX_LotTestMode, 		m_MIR.ModeCod, 	"MIR.LotTestMode");
	setLotInformation(EVX_LotProductID, 		m_MIR.FamlyId,  "MIR.LotProductID");
	setLotInformation(EVX_LotPackage, 		m_MIR.PkgTyp, 	"MIR.LotPackage");
	setLotInformation(EVX_LotSublotID, 		m_MIR.SblotId, 	"MIR.LotSublotID");
	setLotInformation(EVX_LotTestSetup, 		m_MIR.SetupId, 	"MIR.LotTestSetup");
	setLotInformation(EVX_LotFileNameRev, 		m_MIR.JobRev, 	"MIR.LotFileNameRev");
	setLotInformation(EVX_LotAuxDataFile, 		m_MIR.AuxFile, 	"MIR.LotAuxDataFile");
	setLotInformation(EVX_LotTestPhase, 		m_MIR.TestCod, 	"MIR.LotTestPhase");
	setLotInformation(EVX_LotUserText, 		m_MIR.UserText, "MIR.LotUserText");
	setLotInformation(EVX_LotRomCode, 		m_MIR.RomCod, 	"MIR.LotRomCode");
	setLotInformation(EVX_LotTesterSerNum, 		m_MIR.SerlNum, 	"MIR.LotTesterSerNum");
	setLotInformation(EVX_LotTesterType, 		m_MIR.TstrTyp, 	"MIR.LotTesterType");
	setLotInformation(EVX_LotSupervisor, 		m_MIR.SuprNam, 	"MIR.LotSupervisor");
	setLotInformation(EVX_LotSystemName, 		m_MIR.ExecTyp, 	"MIR.LotSystemName");
	setLotInformation(EVX_LotTargetName, 		m_MIR.ExecVer, 	"MIR.LotTargetName");
	setLotInformation(EVX_LotTestSpecName, 		m_MIR.SpecNam, 	"MIR.LotTestSpecName");
	setLotInformation(EVX_LotTestSpecRev, 		m_MIR.SpecVer, 	"MIR.LotTestSpecRev");
	setLotInformation(EVX_LotProtectionCode, 	m_MIR.ProtCod, 	"MIR.LotProtectionCode");

	// set SDR
	setLotInformation(EVX_LotHandlerType, 		m_SDR.HandTyp, 	"SDR.LotHandlerType");
	setLotInformation(EVX_LotCardId, 		m_SDR.CardId, 	"SDR.LotCardId");
	setLotInformation(EVX_LotLoadBrdId, 		m_SDR.LoadId, 	"SDR.LotLoadBrdId");
	setLotInformation(EVX_LotProberHandlerID, 	m_SDR.HandId, 	"SDR.LotProberHandlerID");
	setLotInformation(EVX_LotDIBType, 		m_SDR.DibTyp, 	"SDR.LotDIBType");
	setLotInformation(EVX_LotIfCableId, 		m_SDR.CableId, 	"SDR.LotIfCableId");
	setLotInformation(EVX_LotContactorType, 	m_SDR.ContTyp, 	"SDR.LotContactorType");
	setLotInformation(EVX_LotLoadBrdType, 		m_SDR.LoadTyp, 	"SDR.LotLoadBrdType");
	setLotInformation(EVX_LotContactorId, 		m_SDR.ContId, 	"SDR.LotContactorId");
	setLotInformation(EVX_LotLaserType, 		m_SDR.LaserTyp, "SDR.LotLaserType");
	setLotInformation(EVX_LotLaserId, 		m_SDR.LaserId, 	"SDR.LotLaserId");
	setLotInformation(EVX_LotExtEquipType, 		m_SDR.ExtrTyp, 	"SDR.LotExtEquipType");
	setLotInformation(EVX_LotExtEquipId, 		m_SDR.ExtrId, 	"SDR.LotExtEquipId");
	setLotInformation(EVX_LotActiveLoadBrdName, 	m_SDR.DibId, 	"SDR.LotActiveLoadBrdName");
	setLotInformation(EVX_LotCardType, 		m_SDR.CardTyp, 	"SDR.LotCardType");
	setLotInformation(EVX_LotIfCableType, 		m_SDR.CableTyp, "SDR.LotIfCableType");
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
			m_Log << "programChange[" << state << "]: EVX_PROGRAM_LOAD_FAILED" << CUtil::CLog::endl;
			break;
		case EVX_PROGRAM_LOADED:
		    	m_Log << "programChange[" << state << "]: EVX_PROGRAM_LOADED" << CUtil::CLog::endl;
			break;
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
			if(m_pProgCtrl->getStatus() !=  EVXA::OK) m_Log << "An Error occured[" << state << "]: " << m_pProgCtrl->getStatusBuffer() << CUtil::CLog::endl;
		break;
	}
}

/* ------------------------------------------------------------------------------------------
event handler for state notification EOT
------------------------------------------------------------------------------------------ */
void CApp::onEndOfTest(const int array_size, int site[], int serial[], int sw_bin[], int hw_bin[], int pass[], EVXA_ULONG dsp_status)
{
	// if we're not sending bin, bail out
	if (!m_CONFIG.bSendBin) return;

	// set host/tester name
	std::stringstream send;
	send << m_szTesterName;

	// as per Amkor specs, if wafer test, insert wafer id (lot id) here
	if (m_CONFIG.nTestType == CONFIG::APL_WAFER) send << "," <<  m_pProgCtrl->getLotInformation(EVX_LotLotID);

	// if wafer test, extract x/y coords. needed as per Amkor specs
	int *sites = 0;
	int *xCoords = 0;
	int *yCoords = 0;

	if (m_CONFIG.nTestType == CONFIG::APL_WAFER)
	{	
		m_Log << "wafer test" << CUtil::CLog::endl;
		// initialize arrays and set to defaults
		sites = new int[array_size];
		xCoords = new int[array_size];
		yCoords = new int[array_size];

		if ( m_pProgCtrl->getWaferCoords(array_size, sites, xCoords, yCoords) != EVXA::OK )
		{
			m_Log << "Error: Something went wrong int qerying Unison for X/Y coords." << CUtil::CLog::endl;
		}
	}

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
			if (m_CONFIG.nTestType == CONFIG::APL_WAFER) send << xCoords[i] << "/" << yCoords[i] << "/";
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
	if (!c.connect(m_CONFIG.IP, m_CONFIG.nPort)) 
	{
		m_Log << "ERROR: Failed to connect to server." << CUtil::CLog::endl;
		return;
	}
	c.send(send.str());
	c.disconnect();
}
