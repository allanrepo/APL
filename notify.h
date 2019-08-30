#ifndef __NOTIFY__
#define __NOTIFY__
#include <iostream>
#include <sys/inotify.h>
#include <utility.h>
#include <string>
#include <list>
#include <unistd.h>
#include <string.h>
#include <fd.h>

#define EVENT_SIZE  ( sizeof (struct inotify_event) )
#define BUF_LEN     ( 1024 * ( EVENT_SIZE + 255 ) )

/* ------------------------------------------------------------------------------------------
derived file descriptor class specifically for inotify FD
------------------------------------------------------------------------------------------ */
class CNotifyFileDescriptor: public CFileDescriptorManager::CFileDescriptor
{
protected:
	int m_wd;
	CUtil::CLog m_Log;
	std::string m_szPath;
	unsigned short m_nMask;
	bool m_bHalt;

public:
	CNotifyFileDescriptor(const std::string& path, unsigned short mask = IN_MODIFY | IN_CREATE | IN_DELETE | IN_MOVED_TO | IN_MOVED_FROM);
	virtual ~CNotifyFileDescriptor();
	virtual void onSelect();

	void start();
	void stop();
	void halt(){ m_bHalt = true; }

	// event handlers
	virtual	void onDirCreate( const std::string& name );
	virtual	void onFileCreate( const std::string& name );
	virtual	void onDirDelete( const std::string& name );
	virtual	void onFileDelete( const std::string& name );
	virtual	void onDirModify( const std::string& name );
	virtual	void onFileModify( const std::string& name );
	virtual	void onDirAccess( const std::string& name );
	virtual	void onFileAccess( const std::string& name );
	virtual	void onDirAttrChange( const std::string& name );
	virtual	void onFileAttrChange( const std::string& name );
	virtual	void onDirCloseWrite( const std::string& name );
	virtual	void onFileCloseWrite( const std::string& name );
	virtual	void onDirCloseNoWrite( const std::string& name );
	virtual	void onFileCloseNoWrite( const std::string& name );
	virtual	void onDirDeleteSelf( const std::string& name );
	virtual	void onFileDeleteSelf( const std::string& name );
	virtual	void onDirMoveTo( const std::string& name );
	virtual	void onFileMoveTo( const std::string& name );
	virtual	void onDirMoveFrom( const std::string& name );
	virtual	void onFileMoveFrom( const std::string& name );
	virtual	void onDirOpen( const std::string& name );
	virtual	void onFileOpen( const std::string& name );
};

#endif
