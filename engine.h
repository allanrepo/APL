#ifndef __ENGINE__
#define __ENGINE__
 
#include <utility.h>
#include <sys/time.h>

class CState;
class CStateManager;
class CTask;
class CSequence;
class CRef;


class CRef
{
protected:
	CUtil::CLog m_Log;
	static int s_nInstance;
public:
	CRef()
	{
		s_nInstance++;
		m_Log << "Ref Object created: " << s_nInstance << CUtil::CLog::endl;
	}

	virtual~ CRef()
	{
		s_nInstance--;
		m_Log << "Ref Object destroyed: " << s_nInstance << CUtil::CLog::endl;
	}

};

/* ------------------------------------------------------------------------------------------
state class
------------------------------------------------------------------------------------------ */
class CState: public CRef
{
protected:
	std::string m_szName;
	CUtil::CLog m_Log;
	std::list< CTask* > m_Tasks;

public:
	CState(const std::string& name = "");
	virtual~ CState();

	const std::string& getName() const { return m_szName; }

	virtual void run();
	virtual void load();
	virtual void unload();

	void add(CTask* pTask){ m_Tasks.push_back(pTask); }

	CTask* get(const std::string& name);
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

public:
	CStateManager(const std::string& name = "");
	virtual~ CStateManager();

	virtual void run();

	void set(CState* pstate);
	const CState* get()const { return m_pState; }
};

/* ------------------------------------------------------------------------------------------
task class
------------------------------------------------------------------------------------------ */
class CTask: public CRef
{
protected:
	std::string m_szName;
	bool m_bEnabled;
	bool m_bLoop;
	long m_nDelayMS;

	struct timeval m_prev;
	bool m_bFirst;

	CUtil::CLog m_Log;

public:
	CTask(const std::string& name = "", long nDelayMS = 0, bool bEnable = true, bool bLoop = true);
	virtual~ CTask();

	virtual void run(){}
	virtual void load(){ m_bFirst = true; }

	void loop(bool b){ m_bLoop = b; }
	bool loop(){ return m_bLoop; }

	void enable(bool b){ m_bEnabled = b; }
	bool enable(){ return m_bEnabled; }

	long getDelay(){ return m_nDelayMS; }

	const std::string& getName() const { return m_szName; }

	friend class CState;
};

/* ------------------------------------------------------------------------------------------
sequence class
------------------------------------------------------------------------------------------ */
class CSequence: public CTask
{
protected:
	std::list< CTask* > m_Tasks;
	std::list< CTask* >::iterator m_currTask;

public:
	CSequence(const std::string& name = "", bool bEnable = true, bool bLoop = true);
	virtual~ CSequence();

	virtual void run();

	void queue(CTask* pTask);
};


#endif
