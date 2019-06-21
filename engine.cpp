#include <engine.h>

CState::CState(const std::string& name)
{
	m_szName = name;
}

CState::~CState()
{
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

CTask::CTask(const std::string& name): CState(name)
{
}

CTask::~CTask()
{
}

CSequence::CSequence(const std::string& name): CTask(name)
{
}

CSequence::~CSequence()
{
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


