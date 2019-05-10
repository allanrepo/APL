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
	CHandleNotify NotifyFD(*this, m_szMonitorPath );
	m_FileDescMgr.add( NotifyFD );

	// run the application loop
	while(1) 
	{
		// connect to tester. set it such that it will keep trying to connect forever
		//connect(m_szTesterName, 1, -1);

		// get file descriptor of tester state notification and add to our fd manager		
		//CAppFileDesc StateNotifyFileDesc(&CApp::onStateNotificationResponse, *this, m_pState? m_pState->getSocketId(): -1);
		//m_FileDescMgr.add( StateNotifyFileDesc );
		
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

	// flag used to request for tester reconnect, false by default
	m_bReconnect = false;
}

/* ------------------------------------------------------------------------------------------

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

	int nTry = 30;
	while (nTry--)
	{ 		
		m_Log <<"Parsing '" << ssFullPathMonitorName.str() << "' now..." << CUtil::CLog::endl;
		if (parse(ssFullPathMonitorName.str())) break; 
		sleep(1);
	}

	// do we have the 'PROGRAM' field and its value from lotinfo.txt?
	if (m_szProgramFullPathName.empty()) 
	{
		m_Log << ssFullPathMonitorName.str() << " file received but didn't find a value program path." << CUtil::CLog::endl;
	}
	else
	{
		// try to load program 
		m_Log << "loading " << m_szProgramFullPathName << "..." << CUtil::CLog::endl;
		// load(m_szProgramFullPathName);
		launch(m_szTesterName, m_szProgramFullPathName, true);

	}

	// delete the lotinfo.txt
	unlink(ssFullPathMonitorName.str().c_str());
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
launches unison (OICu or OpTool)
------------------------------------------------------------------------------------------ */
bool CApp::launch(const std::string& tester, const std::string& program, bool bProd)
{
	// kill the tester first. we don't want another tester object running while we launch our own
	// ** 	FIX THIS: problem is the app doesn't wait until end tester is done. we we are sending launch command
	//	in the middle of tester killing. we'll keep it this way for now but we will have to fix this before
	//	release.
	EVXA::endTester(m_szTesterName.c_str(), "");

	std::stringstream ssCmd;
	ssCmd << "launcher -nodisplay " << (bProd? "-prod " : "") << (program.empty()? "": "-load ") << (program.empty()? "" : program) << " -T " << m_szTesterName;
	m_Log << "CMD: " << ssCmd.str() << CUtil::CLog::endl;
	system(ssCmd.str().c_str());	

	// connect to tester. set it such that it will keep trying to connect forever
	connect(m_szTesterName, 1, -1);

	// get file descriptor of tester state notification and add to our fd manager		
	CAppFileDesc StateNotifyFileDesc(&CApp::onStateNotificationResponse, *this, m_pState? m_pState->getSocketId(): -1);
	//m_FileDescMgr.add( StateNotifyFileDesc );

	return true;
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

	// for debugging purpose, let's dump the contents of the file
	m_Log << "---------------------------------------------------------------------" << CUtil::CLog::endl;
	m_Log << "Content extracted from " << name << "." << CUtil::CLog::endl;
	m_Log << "It's file size is " << s.length() << ". Contents: " << CUtil::CLog::endl;
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


#if 0
		// strip the path from value and check if this exist. if not, move to next line
		//size_t n = value.find_last_of('/', std::string::npos);
		//std::string path = n == std::string::npos? "" : value.substr(0, n);
		

		// now check if value refers to file that exists. if not, move to next line
		std::string prog = value.substr(n + 1);

		m_Log << "path: '" << path << "'" << CUtil::CLog::endl;
		m_Log << "prog: '" << prog << "'" << CUtil::CLog::endl;

#endif
