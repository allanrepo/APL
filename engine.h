#ifndef __ENGINE__
#define __ENGINE__

#include <utility.h>

class CState;
class CStateManager;
class CTask;
class CSequence;

/* ------------------------------------------------------------------------------------------
state class
------------------------------------------------------------------------------------------ */
class CState
{
protected:
	std::string m_szName;
	CUtil::CLog m_Log;
public:
	CState(const std::string& name = "");
	virtual~ CState();

	const std::string& getName() const { return m_szName; }

	virtual void run(){}
	virtual void load(){}
	virtual void unload(){}
};

/* ------------------------------------------------------------------------------------------
state manager class
-	is a state
-	has reference to a state (current)
-	executes curr_state.run() in its .run() loop
------------------------------------------------------------------------------------------ */
class CStateManager: public CState
{
protected:
	CState* m_pNext;
	CState* m_pState;
	bool m_bNext;

public:
	CStateManager(const std::string& name = "");
	virtual~ CStateManager();

	virtual void run();

	void set(CState* pstate);
};

/* ------------------------------------------------------------------------------------------
task class
------------------------------------------------------------------------------------------ */
class CTask: public CState
{
protected:
public:
	CTask(const std::string& name = "");
	virtual~ CTask();
};

/* ------------------------------------------------------------------------------------------
sequence class
------------------------------------------------------------------------------------------ */
class CSequence: public CTask
{
protected:
public:
	CSequence(const std::string& name = "");
	virtual~ CSequence();
};


#endif
