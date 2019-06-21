#include <state.h>

/* ------------------------------------------------------------------------------------------
constructor
------------------------------------------------------------------------------------------ */
CState::CState(const std::string& name)
{
	m_szName = name;
	m_bEnabled = true;
}


/* ------------------------------------------------------------------------------------------
destructor
------------------------------------------------------------------------------------------ */
CState::~CState()
{
}

/* ------------------------------------------------------------------------------------------
execute all the tasks the state has
------------------------------------------------------------------------------------------ */
void CState::run()
{
	for (std::list< CTask* >::iterator it = m_Tasks.begin(); it != m_Tasks.end(); it++)
	{			
		// if this state is disabled, we skip. 
		if (!m_bEnabled) continue;

		// is this a valid task?
		CTask* pTask = *it;
		if ( !pTask ) continue;

		// is this task enabled?
		if (!pTask->m_bEnabled) continue;

		// execute task
		pTask->run();

		// disable if we're not looping this task
		if (!pTask->m_bLoop) pTask->disable();
	}	
}

/* ------------------------------------------------------------------------------------------
add to list of stacks for this state
------------------------------------------------------------------------------------------ */
void CState::add( CTask* pTask )
{
	if (pTask) m_Tasks.push_back(pTask);
}

/* ------------------------------------------------------------------------------------------
when loading state, all tasks are enabled and its timer reset
------------------------------------------------------------------------------------------ */
void CState::load()
{
	// ensure state is enabled on load
	enable();

	// enable all tasks
	for (std::list< CTask* >::iterator it = m_Tasks.begin(); it != m_Tasks.end(); it++)
	{
		// is this a valid event?
		CTask* pTask = *it;
		if ( !pTask ) continue;

		// load task as well
		pTask->load();
	}
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

/* ------------------------------------------------------------------------------------------
constructor
-	set current task iterator to .end() since tasks list is empty now
------------------------------------------------------------------------------------------ */
CSequence::CSequence(const std::string& name, bool bLoop): CTask(name, 0, bLoop)
{
	m_currTask = m_Tasks.begin();
	m_bFirst = true;
}

CSequence::~CSequence()
{
	m_Tasks.clear();
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
		if (!(*m_currTask)->m_bEnabled) 
		{
			m_currTask++;
			continue;
		}

		// is the elapsed time for this loop enough to execut current task based on its delay?
		if ( nTimeMS >= (*m_currTask)->m_nDelayMS )
		{
			(*m_currTask)->run();

			if (!(*m_currTask)->m_bLoop) (*m_currTask)->disable();

			// slice off delay from this task 
			nTimeMS -= (*m_currTask)->m_nDelayMS;
			
			// accumulate used elapsed time. we use this later to increment actual used elapsed time
			nSpentMS += (*m_currTask)->m_nDelayMS;			

			// move to next task
			m_currTask++;
		}
		// increment our sequence snapshot time with the actual used elapsed time
		else
		{
			m_prev.tv_sec += (nSpentMS / 1000);
			m_prev.tv_usec += (nSpentMS % 1000) * 1000;
			break;
		}
	}
}

/* ------------------------------------------------------------------------------------------
when loading/reloading sequence, enable and reset its timer
------------------------------------------------------------------------------------------ */
void CSequence::load()
{
	enable();
	m_bFirst = true;
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

