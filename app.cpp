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

//	EVXA::endTester(m_szTesterName.c_str(), "-v");
	if (m_bRestartTester) EVXA::restartTester(m_szTesterName.c_str(), "-v");

	// if file name to monitor is not set, assign default
	if (m_szMonitorFileName.empty()){ m_szMonitorFileName = "lotinfo.txt"; }

	// if path to monitor is not set, assign default
	if (m_szMonitorPath.empty()){ m_szMonitorPath = "/tmp"; }

	// print out app info before proceeding. force this to be log
	m_Log.silent = false;
	m_Log << "Version: " << VERSION << CUtil::CLog::endl;
	m_Log << "Developer: " << DEVELOPER << CUtil::CLog::endl;
	m_Log << "Tester: " << m_szTesterName << CUtil::CLog::endl;
	m_Log << "Path: " << m_szMonitorPath << CUtil::CLog::endl;
	m_Log << "File: " << m_szMonitorFileName << CUtil::CLog::endl;

	// create notify file descriptor and add to fd manager. this will monitor incoming lotinfo.txt file
	m_pMonitorFileDesc = new CMonitorFileDesc(*this, m_szMonitorPath);
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
				setSTDF();
				m_bSTDF = false;
			}
		}
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
	m_szProgramFullPathName = "";
	m_szMonitorPath = "";
	m_szTesterName = "";
	m_szMonitorFileName = "";
	m_bRestartTester = false;
	
	m_bSTDF = false;

	// flag used to request for tester reconnect, true by default so it tries to connect on launch
	m_bReconnect = true;
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
		// look for -path arg. if partial match is at first char, we found it
		s = "-path";
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
				m_szMonitorPath = *it;
				continue;
			}	
		}	
		// look for -monitor arg. if partial match is at first char, we found it
		s = "-monitor";
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
				m_szMonitorFileName = *it;
				continue;
			}	
		}	
		// this option will let the app restart tester on launch so there's something to connect to
		s = "-restart_tester";
		if (s.find(*it) == 0)
		{
			m_bRestartTester = true;
			continue;
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
	ssFullPathMonitorName << m_szMonitorPath << "/" << name;

	// is this file lotinfo.txt?   
	if (name.compare(m_szMonitorFileName) != 0)
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
	else
	{
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

		// kill tester
		ssCmd.str(std::string());
		ssCmd.clear();
		ssCmd << "./" << KILLTESTERCMD << " " << m_szTesterName;
		m_Log << "END TESTER: " << ssCmd.str() << CUtil::CLog::endl;		
		system(ssCmd.str().c_str());

		// launch OICu and load program
		launch(m_szTesterName, m_szProgramFullPathName, true);

		// let app know we are setting STDF fields
		m_bSTDF = true;
	}
 
	// delete the lotinfo.txt
	unlink(ssFullPathMonitorName.str().c_str());
}

/* ------------------------------------------------------------------------------------------
launches unison (OICu or OpTool)
------------------------------------------------------------------------------------------ */
bool CApp::launch(const std::string& tester, const std::string& program, bool bProd)
{
	m_Log << "launching OICu and loading '" << program << "'..." << CUtil::CLog::endl;

	// kill the tester first. we don't want another tester object running while we launch our own
	// ** 	FIX THIS: problem is the app doesn't wait until end tester is done. we we are sending launch command
	//	in the middle of tester killing. we'll keep it this way for now but we will have to fix this before
	//	release.
	//EVXA::endTester(m_szTesterName.c_str(), ""); 

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
			if (field.compare("DEVICE") == 0){ m_MIR.PartTyp = value; continue; }
			if (field.compare("LOTID") == 0){ m_MIR.LotId = value; continue; }
			if (field.compare("PRODUCTID") == 0){ m_MIR.FamlyId = value; continue; }
			if (field.compare("TEMPERATURE") == 0){ m_MIR.TestTmp = value; continue; }
			if (field.compare("PROBERHANDLERID") == 0){ m_SDR.HandId = value; continue; }
			if (field.compare("BOARDID") == 0){ m_SDR.LoadId = value; continue; }

			// we're looking for jobfile field only, ignore the rest (for now)
			if (field.compare(JOBFILE) != 0) continue;

			m_Log << "'" << field << "' : '"  << value << "'" << CUtil::CLog::endl;

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

------------------------------------------------------------------------------------------ */
void CApp::setSTDF()
{
	// make sure we have valid evxa object
	if (!m_pProgCtrl) return;

	m_pProgCtrl->setLotInformation(EVX_LotLotID, m_MIR.LotId.c_str());
	m_pProgCtrl->setLotInformation(EVX_LotOperator, m_MIR.OperNam.c_str());
	m_pProgCtrl->setLotInformation(EVX_LotDevice, m_MIR.PartTyp.c_str());
	m_pProgCtrl->setLotInformation(EVX_LotProductID, m_MIR.FamlyId.c_str());
	m_pProgCtrl->setLotInformation(EVX_LotTestTemp, m_MIR.TestTmp.c_str());

	m_pProgCtrl->setLotInformation(EVX_LotProberHandlerID, m_SDR.HandId.c_str());
	m_pProgCtrl->setLotInformation(EVX_LotLoadBrdId, m_SDR.LoadId.c_str());
}

#if 0
		// strip the path from value and check if this exist. if not, move to next line
		//size_t n = value.find_last_of('/', std::string::npos);
		//std::string path = n == std::string::npos? "" : value.substr(0, n);
		

		// now check if value refers to file that exists. if not, move to next line
		std::string prog = value.substr(n + 1);

		m_Log << "path: '" << path << "'" << CUtil::CLog::endl;
		m_Log << "prog: '" << prog << "'" << CUtil::CLog::endl;

#endif
