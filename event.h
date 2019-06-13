#ifndef __EVENT__
#define __EVENT__

#include <utility.h>
#include <sys/time.h>

class CEventManager
{
public:
	class CEvent
	{
	public:
		enum STATE
		{
			REMOVE,
			PAUSE,
			EXEC
		};
	protected:
		struct timeval m_prev;
		bool m_bFirst;
		long m_nTimeOutMS;
		STATE m_state;

	public:
		virtual void onTimeOut(long n = 0){}

		CEvent(long n): m_nTimeOutMS(n)
		{
			m_bFirst = true;
			m_state = EXEC;			
		}
		virtual ~CEvent(){}

		STATE getState(){ return m_state; }
		void setState( STATE s ){ m_state = s; }

		friend class CEventManager;
	};

protected:
	std::list< CEvent* > m_Events;
	std::list< CEvent* > m_Queue;

	bool m_bFirst;
	struct timeval m_prev;

	CUtil::CLog m_Log;

public:
	CEventManager();

	virtual ~CEventManager();

	void update();

	void add(CEvent* pEvent, bool bImmediate = false);
	void remove(CEvent* pEvent);
};


#endif
