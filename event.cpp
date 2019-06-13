#include <event.h>


/* ------------------------------------------------------------------------------------------
constructor
------------------------------------------------------------------------------------------ */
CEventManager::CEventManager()
{
	m_bFirst = true;
}

/* ------------------------------------------------------------------------------------------
destructor 
------------------------------------------------------------------------------------------ */
CEventManager::~CEventManager()
{
}

/* ------------------------------------------------------------------------------------------
adding a new event 
-	events in the list must always be unique. existing ones will be removed before
	adding new one 
------------------------------------------------------------------------------------------ */
void CEventManager::add(CEvent* pEvent, bool bImmediate)
{
	// if it's not valid, don't bother
	if (!pEvent) return;

	// if an event identical to this one already exists in the list, let's remove it.
	// we always replace the existing one with the new one.
	for (std::list< CEvent* >::iterator it = m_Events.begin(); it != m_Events.end(); it++)
	{
		// we guarantee that there's always only one event like this. if we find one, remove it and move on
		if (*it == pEvent){ remove(*it); break; }
	}
	
	// if this event is also already in the queue, we remove it from queue instantly 
	// we add this back later in this function
	for (std::list< CEvent* >::iterator it = m_Queue.begin(); it != m_Queue.end(); it++)
	{
		// we guarantee there's only one event like this so we move on as soon as we find one
		if ( *it == pEvent ){ m_Queue.erase(it); break; }
	}

	// set it's state to executable
	pEvent->m_state = CEvent::EXEC;

	// if immediate, this event starts now so we snapshot time now for its prev
	// otherwise, we just set first flag so first time event manager updates this event,
	// it will be its start.
	if (bImmediate)
	{ 
		gettimeofday(&pEvent->m_prev, NULL); 
		pEvent->m_bFirst = false;
	}
	else pEvent->m_bFirst = true;


	// we added it on queue. let event manager properly add it in actual event list 
	m_Queue.push_back(pEvent);
}

/* ------------------------------------------------------------------------------------------
remove event from list
------------------------------------------------------------------------------------------ */
void CEventManager::remove(CEvent* pEvent)
{
	// if it's not valid, don't bother
	if (!pEvent) return;

	// we don't immediately remove event from list. we just tell event manager to 
	// remove it. event manager will then remove it safely
	pEvent->m_state = CEvent::REMOVE;
}

/* ------------------------------------------------------------------------------------------
 
------------------------------------------------------------------------------------------ */
void CEventManager::update()
{
	// before we update the event list with elapsed time let's check if there are new ones to add?
	if (m_Queue.size())
	{ 
		m_Log << m_Queue.size() << " event" << (m_Queue.size()>1? "s":"") << " added. ";
		m_Events.splice(m_Events.end(), m_Queue); 
		m_Log << "there are now " << m_Events.size() << " active ";
		m_Log << "and " << m_Queue.size() << " left on queue." << CUtil::CLog::endl;
	}

	// let's snapshot time now
	struct timeval now;
	gettimeofday(&now, NULL);

	// this is to monitor the overall elapsed time per loop. events don't really care about this
	// we only do this for debug purposes
#if 1
	if (m_bFirst)
	{
		m_bFirst = false;
		m_prev.tv_sec = now.tv_sec;
		m_prev.tv_usec = now.tv_usec;		
	}

	// get the elapsed time between now and last update()
	long dwTimeMS = (((long)now.tv_sec - (long)m_prev.tv_sec ) * 1000);
	dwTimeMS += (((long)now.tv_usec - (long)m_prev.tv_usec) / 1000);
//	m_Log << "time: " << (dwTimeMS / 1) << " ms" << CUtil::CLog::endl;

	// remember this time's snapshot
	m_prev.tv_sec = now.tv_sec;
	m_prev.tv_usec = now.tv_usec;
#endif

	// loop through all events and increment their elapsed time: event.elapsed = now - event.prev
	for (std::list< CEvent* >::iterator it = m_Events.begin(); it != m_Events.end(); it++)
	{
		// is this a valid event?
		CEvent* pEvent = *it;
		if ( !pEvent ) continue;

		// is this event about to get removed?
		if (pEvent->m_state == CEvent::REMOVE) continue;

		// first time?
		if (pEvent->m_bFirst)
		{
			pEvent->m_bFirst = false;
			pEvent->m_prev.tv_sec = now.tv_sec;
			pEvent->m_prev.tv_usec = now.tv_usec;
		} 

		// calculate actual delta time (in milliseconds) for this event
		long nTimeMS = (((long)now.tv_sec - (long)pEvent->m_prev.tv_sec ) * 1000);
		nTimeMS += (((long)now.tv_usec - (long)pEvent->m_prev.tv_usec) / 1000);		
		
		// if elapsed time is already greater than this event's time-out, let's execute this event
		if ( nTimeMS >= pEvent->m_nTimeOutMS )
		{
			// calculate number of times this interval happened within this elapsed time
			long nStep = pEvent->m_nTimeOutMS > 0? nTimeMS / pEvent->m_nTimeOutMS : 1;

			// execute event
			pEvent->onTimeOut(nStep);

			// update this event's timer
			nTimeMS = pEvent->m_nTimeOutMS * nStep;
			pEvent->m_prev.tv_sec += (nTimeMS / 1000);
			pEvent->m_prev.tv_usec += (nTimeMS % 1000) * 1000;

			//m_Log << "event: " << nTimeMS << " ms, " << nStep << " steps, " << pEvent->m_nTimeOutMS << " ms." << CUtil::CLog::endl;			
		}
	}

	// are there events to remove?
	for (std::list< CEvent* >::iterator it = m_Events.begin(); it != m_Events.end(); it++)
	{
		CEvent* pEvent = (*it);
		if (pEvent->m_state == CEvent::REMOVE)
		{
			std::list< CEvent* >::iterator rm = it;
			it++;		
			m_Events.erase(rm);
			m_Log << "an event has been removed. There are now " << m_Events.size() << " active." << CUtil::CLog::endl;
		}
	}
}


