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

	struct timeval m_prev;
	bool m_bFirst;
	long m_nDelayMS;
	bool m_bEnabled;
	bool m_bLoop;

	CUtil::CLog m_Log;
	
public:
	CTask(const std::string& name = "", long nDelayMS = 0, bool bLoop = false)
	{
		m_nDelayMS = nDelayMS;
		m_szName = name;
		m_bEnabled = true;
		m_bFirst = true;
		m_bLoop = bLoop;		
	}
	
	// virtual functions
	virtual void run(){} //{ m_Log << "Task: " << m_szName << CUtil::CLog::endl; }
	virtual void load(){}
	virtual void unload(){}

	// manage state
	void enable(){ m_bEnabled = true; }
	void disable(){ m_bEnabled = false; }

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
	CUtil::CLog m_Log;
public:
	CState(const std::string& name);
	virtual ~CState();

	virtual void run();
	virtual void load();
	virtual void unload();

	void add( CTask* pTask );

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
	bool m_bNext;

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
};

/*------------------------------------------------------------------------------------------------------
sequence
-	is a task
-	can contain several task to be executed in sequence
-	task are executed based on elapsed time
------------------------------------------------------------------------------------------------------*/
class CSequence: public CTask
{
protected:
	std::list< CTask* > m_Tasks;
	std::list< CTask* >::iterator m_currTask;

	struct timeval m_prev;
	bool m_bFirst;
	bool m_bLoop;

public:
	CSequence();
	virtual ~CSequence();

	// virtual functions
	virtual void run();
	virtual void load();
	virtual void unload();

	void queue(CTask* pTask);
};

#endif
