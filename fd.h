#ifndef __FD__
#define __FD__
#include <list>
#include <iostream>
#include <utility.h>
#include <sys/socket.h>

/* ------------------------------------------------------------------------------------------
this class will contain list of file descriptors and use select to monitor their
IO transactions
------------------------------------------------------------------------------------------ */
class CFileDescriptorManager
{
public:
	//class interface for file descriptor wrapper
	class CFileDescriptor
	{
	protected:
		int m_fd;
	public:
		CFileDescriptor(){}
		virtual~ CFileDescriptor(){}
		int get(){ return m_fd; }
		void set(int fd){ m_fd = fd; }
		virtual void onSelect() = 0;
	};

protected:
	std::list< CFileDescriptor* > m_FDs;
	CUtil::CLog m_Log;

	std::list< CFileDescriptor* > m_Adds;

public:
	CFileDescriptorManager(){}
	virtual ~CFileDescriptorManager();

	// manage fd list
	bool add( CFileDescriptor& fd );
	void remove( CFileDescriptor& fd );

	// run select
	void select(unsigned long ms = 0);
};

#endif
