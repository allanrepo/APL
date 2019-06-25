#ifndef __STATE__
#define __STATE__

#include <list>
#include <string>
#include <utility.h>
#include <sys/time.h>

//------------------------------------------------------------------------------------------------------
// class declarations
//------------------------------------------------------------------------------------------------------
class CStateManager;
class CSequence;
class CState;
class CTask;

//------------------------------------------------------------------------------------------------------
// task class
//------------------------------------------------------------------------------------------------------
class CTask
{
protected:
	std::string m_szName;

	long m_nDelayMS;
	bool m_bEnabled;
	bool m_bLoop;

	CUtil::CLog m_Log;
	
public:
	CTask(const std::string& name = "", long nDelayMS = 0, bool bEnabled = true, bool bLoop = false)
	{
		m_nDelayMS = nDelayMS;
		m_szName = name;
		m_bEnabled = bEnabled;
		m_bLoop = bLoop;		
	}
	
	// virtual functions
	virtual void run(){} //{ m_Log << "Task: " << m_szName << CUtil::CLog::endl; }
	virtual void load(){ enable(); }
	virtual void unload(){}

	// manage state
	void enable(){ m_bEnabled = true; }
	void disable(){ m_bEnabled = false; }

	const std::string& getName() const { return m_szName; }

	// let these class access the privates
	friend class CSequence;
	friend class CState;
};

//------------------------------------------------------------------------------------------------------
// state class
//------------------------------------------------------------------------------------------------------
class CState
{
protected:
	std::string m_szName;
	std::list< CTask* > m_Tasks;
	bool m_bEnabled;
	CUtil::CLog m_Log;

public:
	CState(const std::string& name);
	virtual ~CState();

	virtual void run();
	virtual void load();
	virtual void unload();

	// manage state
	void enable(){ m_bEnabled = true; }
	void disable(){ m_bEnabled = false; }

	void add( CTask* pTask );

	const std::string& getName() const { return m_szName; }

	friend class CStateManager;
};

//------------------------------------------------------------------------------------------------------
// state manager class
//------------------------------------------------------------------------------------------------------
class CStateManager
{
public:

protected:
	CState* m_pNext;
	CState* m_pState;

public:
	// constructors
	CStateManager();
	virtual ~CStateManager();

	// virtual functions
	virtual void run();
	virtual void load();
	virtual void unload();

	// fixed functions specific to this class
	void set( CState* pState );
	void clear();
	void halt(){ if (m_pState) m_pState->disable(); }

	const std::string getCurrState() const { return m_pState? m_pState->getName() : ""; }
};

/*------------------------------------------------------------------------------------------------------
sequence
-	is a task
-	can contain several task to be executed in sequence
-	task are executed based on elapsed time, expects tasks to have delay time as attribute
-	does not need its own delay time. will execute in state instantly
-	cannot be looped. it will be dangerous. imagine queueing a single task with no delay?
	the delta time between state loop will never decrease. the task will keep running repeatedly
	within sequence loop. 
-	execution must always end at the last task on queue. delta time between state loop must
	not carry over next loop, or the app will end up locked in this class' while loop
------------------------------------------------------------------------------------------------------*/
class CSequence: public CTask
{
protected:
	std::list< CTask* > m_Tasks;
	std::list< CTask* >::iterator m_currTask;

	struct timeval m_prev;
	bool m_bFirst;

public:
	CSequence(const std::string& name = "", bool bLoop = false);
	virtual ~CSequence();

	// virtual functions
	virtual void run();
	virtual void load();
	virtual void unload();

	void queue(CTask* pTask);
};

#endif
