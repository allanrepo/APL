#include <fd.h>



/* ------------------------------------------------------------------------------------------
destructor
------------------------------------------------------------------------------------------ */
CFileDescriptorManager::~CFileDescriptorManager()
{
	// destroy all file descriptors
	while (!m_FDs.empty())
	{
		delete m_FDs.back();
		m_FDs.pop_back();
	}
}


/* ------------------------------------------------------------------------------------------
add file descriptor at the back of the list
------------------------------------------------------------------------------------------ */
bool CFileDescriptorManager::add( CFileDescriptor& fd )
{
	m_FDs.push_front( &fd );
	m_Log << "file descriptor '" << (*m_FDs.begin())->get() << "' added." << CUtil::CLog::endl;
	return true;
}

/* ------------------------------------------------------------------------------------------
remove file descriptor in the list
------------------------------------------------------------------------------------------ */
void CFileDescriptorManager::remove( CFileDescriptor& fd )
{
	m_FDs.remove( &fd );
}

/* ------------------------------------------------------------------------------------------
execute select with all file descriptors in the list set
------------------------------------------------------------------------------------------ */
void CFileDescriptorManager::select(unsigned long ms)
{
	struct timeval to;
	to.tv_sec = (unsigned long) ms / 1000; // convert to second
	to.tv_usec = (ms % 1000) * 1000; // convert to usec

	// setup fd_set 
	fd_set fds;
	FD_ZERO(&fds);
	int nMaxFD = 0;
	
	// add file descriptors to fd_set
	for (std::list< CFileDescriptor* >::iterator it = m_FDs.begin(); it != m_FDs.end(); it++)
	{
		int fd = (*it)->get();
		FD_SET(fd, &fds);	
		nMaxFD = fd > nMaxFD? fd : nMaxFD;
		//m_Log << "file descriptor '" << fd << "' is set. maxFD is now " << nMaxFD << CUtil::CLog::endl;
	}

	// if timeout is not specified, we'll be blocking here and wait for any file descriptor ready to be read.
	fd_set ready = fds;
	if (to.tv_sec == 0 && to.tv_usec == 0)
	{
		if( ::select(nMaxFD + 1, &ready, NULL, NULL, NULL) < 0 ) perror("select() failed ");
	}
	// otherwise wait for timeout
	else
	{	
		int n = ::select(nMaxFD + 1, &ready, NULL, NULL, &to);
		if (n < 0) perror("select() failed ");
		if (n == 0) return;
	}

	// if you reach this pt, there's an FD that needs to be read, let's find it
	for (std::list< CFileDescriptor* >::iterator it = m_FDs.begin(); it != m_FDs.end(); it++)
	{
		int fd = (*it)->get();
		if (!FD_ISSET(fd, &ready)) continue;
		else (*it)->onSelect();
	}
}
