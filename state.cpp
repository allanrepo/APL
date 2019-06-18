#include <state.h>

/* ------------------------------------------------------------------------------------------
constructor
------------------------------------------------------------------------------------------ */
CState::CState(const std::string& name)
{
	m_szName = name;
}


/* ------------------------------------------------------------------------------------------
destructor
------------------------------------------------------------------------------------------ */
CState::~CState()
{
}

/* ------------------------------------------------------------------------------------------

------------------------------------------------------------------------------------------ */
void CState::run()
{
//	m_Log << "State: " << m_szName << CUtil::CLog::endl;

	for (std::list< CTask* >::iterator it = m_Tasks.begin(); it != m_Tasks.end(); it++)
	{			
		// is this a valid event?
		CTask* pTask = *it;
		if ( !pTask ) continue;

		// is this state enabled?
		if (!pTask->m_bEnabled) continue;
	
		// execute event
		pTask->run();
	}	
}

void CState::add( CTask* pTask )
{
	if (pTask) m_Tasks.push_back(pTask);
}

/* ------------------------------------------------------------------------------------------

------------------------------------------------------------------------------------------ */
void CState::load()
{
}

/* ------------------------------------------------------------------------------------------

------------------------------------------------------------------------------------------ */
void CState::unload()
{
}

/* ------------------------------------------------------------------------------------------
constructor
------------------------------------------------------------------------------------------ */
CStateManager::CStateManager()
{
	m_pNext = 0;
	m_pState = 0;
}


/* ------------------------------------------------------------------------------------------
destructor
------------------------------------------------------------------------------------------ */
CStateManager::~CStateManager()
{
}

/* ------------------------------------------------------------------------------------------
state manager's loop function
-	right now, let's handle elapsed time calculation internally. by right this 
	class should not do it because time or elapsed time may be needed by other
	objects in the application that also uses this class so a timer class must
	handle this instead. but let's do that later once we realized we need it
	so we can design the time class the best way we need it to be
------------------------------------------------------------------------------------------ */
void CStateManager::run()
{
	while(1)
	{
		// snapshot time now

		// is this the first time you run? elapsed time should be 

		// are we switching to new state? 
		if (m_pNext)
		{
			if (m_pState) m_pState->unload();
			m_pState = m_pNext;
			m_pNext = 0;
			m_pState->load();
		}

		// whatever the current state is, let's run it
		if (m_pState) m_pState->run();
	}
}

/* ------------------------------------------------------------------------------------------

------------------------------------------------------------------------------------------ */
void CStateManager::load()
{
}

/* ------------------------------------------------------------------------------------------

------------------------------------------------------------------------------------------ */
void CStateManager::unload()
{
}

/* ------------------------------------------------------------------------------------------

------------------------------------------------------------------------------------------ */
void CStateManager::set( CState* pState )
{
	m_pNext = pState;
}

/* ------------------------------------------------------------------------------------------

------------------------------------------------------------------------------------------ */
void CStateManager::clear()
{
}

CSequence::CSequence()
{
	m_currTask = m_Tasks.begin(); // actually sets it to .end() since m_Tasks is empty now
	m_bLoop = true;
	m_bFirst = true;
}

CSequence::~CSequence()
{
	m_Tasks.clear();
}

void CSequence::run()
{
	// snapshot time now
	struct timeval now;
	gettimeofday(&now, NULL);


	// first time?
	if (m_bFirst)
	{
		m_bFirst = false;
		m_prev.tv_sec = now.tv_sec;
		m_prev.tv_usec = now.tv_usec;
	} 

	// delta time is in timeval format. let's convert it into long format (milliseconds)	
	long nTimeMS = (((long)now.tv_sec - (long)m_prev.tv_sec ) * 1000);
	nTimeMS += (((long)now.tv_usec - (long)m_prev.tv_usec) / 1000);	

	long nSpentMS = 0;
	while ( m_currTask != m_Tasks.end() )
	{
		if ( nTimeMS >= (*m_currTask)->m_nDelayMS )
		{
			(*m_currTask)->run();

			m_Log << "Seq exec: " << nTimeMS << CUtil::CLog::endl;

			nTimeMS -= (*m_currTask)->m_nDelayMS;
			
			nSpentMS += (*m_currTask)->m_nDelayMS;			

			m_currTask++;

			if (m_bLoop && m_currTask == m_Tasks.end()) m_currTask = m_Tasks.begin();
		}
		else
		{
			m_prev.tv_sec += (nSpentMS / 1000);
			m_prev.tv_usec += (nSpentMS % 1000) * 1000;
			break;
		}
	}
}

void CSequence::load()
{
}

void CSequence::unload()
{
}

void CSequence::queue(CTask* pTask)
{
	if (!pTask) return;

	m_Tasks.push_back(pTask);

	// if current task still points to end of list, let's make it point at the beginning now 
	if ( m_currTask == m_Tasks.end() ) m_currTask = m_Tasks.begin();
}

