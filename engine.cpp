#include <engine.h>

int CRef::s_nInstance = 0;

CState::CState(const std::string& name)
{
	m_szName = name;
}

CState::~CState()
{
	unload();
}

void CState::load()
{ 
}

/* ------------------------------------------------------------------------------------------

------------------------------------------------------------------------------------------ */
void CState::run()
{ 
	// let's snapshot time now
	struct timeval now;
	gettimeofday(&now, NULL);

	for (std::list< CTask* >::iterator it = m_Tasks.begin(); it != m_Tasks.end(); it++)
	{			
		// is this a valid task?
		CTask* pTask = *it;
		if ( !pTask ) continue;

		// is this task enabled?
		if (!pTask->enable()) continue;

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

		if ( nTimeMS >= pTask->m_nDelayMS )
		{
			// calculate number of times this interval happened within this elapsed time
			long nStep = pTask->m_nDelayMS > 0? nTimeMS / pTask->m_nDelayMS : 1;

			// execute task
			pTask->run();

			// update this event's timer
			nTimeMS = pTask->m_nDelayMS * nStep;
			pTask->m_prev.tv_sec += (nTimeMS / 1000);
			pTask->m_prev.tv_usec += (nTimeMS % 1000) * 1000;

			// disable if we're not looping this task
			if (!pTask->loop()) pTask->enable(false);
		}

	}		
}

/* ------------------------------------------------------------------------------------------
remove and delete all task objects
------------------------------------------------------------------------------------------ */
void CState::unload()
{
	while ( m_Tasks.size() )
	{
		CTask* p = m_Tasks.front();
		m_Tasks.pop_front();
		if (p) delete p;
	}
}


/* ------------------------------------------------------------------------------------------

------------------------------------------------------------------------------------------ */
CTask* CState::get(const std::string& name)
{
	for (std::list< CTask* >::iterator it = m_Tasks.begin(); it != m_Tasks.end(); it++)
	{			
		// is this a valid task?
		CTask* pTask = *it;
		if ( !pTask ) continue;

		if (pTask->getName().compare(name) == 0)
		{
			return pTask;
		}		
	}	

	return 0;
}


/* ------------------------------------------------------------------------------------------
constructor
------------------------------------------------------------------------------------------ */
CStateManager::CStateManager(const std::string& name): CState(name)
{
	m_pNext = 0;
	m_pState = 0;
}

CStateManager::~CStateManager()
{
}

/* ------------------------------------------------------------------------------------------
task constructor
------------------------------------------------------------------------------------------ */
CTask::CTask(const std::string& name, long nDelayMS, bool bEnable, bool bLoop)
{
	m_szName = name;
	m_nDelayMS = nDelayMS;
	m_bEnabled = bEnable;
	m_bLoop = bLoop;
	m_bFirst = true;
}

CTask::~CTask()
{
}

/* ------------------------------------------------------------------------------------------

------------------------------------------------------------------------------------------ */
CSequence::CSequence(const std::string& name, bool bEnable, bool bLoop)
:CTask(name, 0, bEnable, bLoop)
{
	m_currTask = m_Tasks.begin();			
}

/* ------------------------------------------------------------------------------------------

------------------------------------------------------------------------------------------ */
CSequence::~CSequence()
{
	while ( m_Tasks.size() )
	{
		CTask* p = m_Tasks.front();
		m_Tasks.pop_front();
		if (p) delete p;
	}
}

/* ------------------------------------------------------------------------------------------

------------------------------------------------------------------------------------------ */
void CSequence::queue(CTask* pTask)
{
	if (!pTask) return;

	m_Tasks.push_back(pTask);

	// if current task still points to end of list, let's make it point at the beginning now 
	if ( m_currTask == m_Tasks.end() ) m_currTask = m_Tasks.begin();
}

/* ------------------------------------------------------------------------------------------
sequentially execute tasks the sequence contain based on elapsed time
------------------------------------------------------------------------------------------ */
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

	// execute all asks starting at the current task iterator
	long nSpentMS = 0;
	while ( m_currTask != m_Tasks.end() )
	{
		// skip disabled tasks and let enabled tasks eat up the delta time
		if (!(*m_currTask)->enable()) 
		{
			m_currTask++;
			continue;
		}

		// is the elapsed time for this loop enough to execut current task based on its delay?
		if ( nTimeMS >= (*m_currTask)->getDelay() )
		{
			(*m_currTask)->run();

			if (!(*m_currTask)->loop()) (*m_currTask)->enable(false);

			// slice off delay from this task 
			nTimeMS -= (*m_currTask)->getDelay();
			
			// accumulate used elapsed time. we use this later to increment actual used elapsed time
			nSpentMS += (*m_currTask)->getDelay();			

			// move to next task
			m_currTask++;
		}
		else break;
	}

	// increment our sequence snapshot time with the actual used elapsed time
	m_prev.tv_sec += (nSpentMS / 1000);
	m_prev.tv_usec += (nSpentMS % 1000) * 1000;

	// if we're to loop and we already are at the end of the task list, let's loop back to beginning of list
	if (m_currTask == m_Tasks.end() && loop())
	{ 
		m_currTask = m_Tasks.begin(); 

		// if we reach end of sequence, we throw any remaining elapsed time. 
		m_prev.tv_sec = now.tv_sec;
		m_prev.tv_usec = now.tv_usec;
	}
	
}

/* ------------------------------------------------------------------------------------------
set the new state to replace current one
------------------------------------------------------------------------------------------ */
void CStateManager::set( CState* pState )
{
	m_pNext = pState;
}

/* ------------------------------------------------------------------------------------------
state manager's loop function
------------------------------------------------------------------------------------------ */
void CStateManager::run()
{
	while(1)
	{
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



