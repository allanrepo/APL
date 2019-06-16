#include <app1.h>
#include <unistd.h>

/* ------------------------------------------------------------------------------------------

------------------------------------------------------------------------------------------ */
void CAppState::run()
{
	// snapshot time now
	struct timeval now;
	gettimeofday(&now, NULL);

	for (std::list< CTask* >::iterator it = m_Tasks.begin(); it != m_Tasks.end(); it++)
	{			
		// is this a valid event?
		CTask* pTask = *it;
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
constructor
------------------------------------------------------------------------------------------ */
CApp::CApp(int argc, char **argv)
{
	// parse command line arguments
	scan(argc, argv);

	// if tester name is not set through command line argument, assign default name
	if (m_szTesterName.empty())
	{
		std::stringstream ss;
		getUserName().empty()? (ss << "sim") : (ss << getUserName() << "_sim");
		m_szTesterName = ss.str();
	}

	// create notify file descriptor and add to fd manager. this will monitor incoming lotinfo.txt file
	m_pMonitorFileDesc = new CMonitorFileDesc(*this, &CApp::onReceiveFile, "/home/localuser/Desktop");
	m_FileDescMgr.add( *m_pMonitorFileDesc );

	// create idle state and and its tasks
	m_pStateOnIdle = new CAppState("onIdle");
	m_pStateOnIdle->add(*this, &CApp::onConnect, 1000, "onConnect"); 
	m_pStateOnIdle->add(*this, &CApp::onSelect, 0, "onSelect"); 

	// add set the first active state 
	m_StateMgr.set(m_pStateOnIdle);

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


void CApp::onConnect()
{
	m_Log << "TASK: onConnect" << CUtil::CLog::endl;
}

void CApp::onSelect()
{
	// proccess any file descriptor notification on select every second.
	m_FileDescMgr.select(20);
}

void CApp::onReceiveFile(const std::string& name)
{
	m_Log << "file: " << name << CUtil::CLog::endl;
}

