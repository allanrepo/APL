#include <notify.h>

/* ------------------------------------------------------------------------------------------
creates the file descriptor and sets the directory path to watch
------------------------------------------------------------------------------------------ */
CNotifyFileDescriptor::CNotifyFileDescriptor(const std::string& path, unsigned short mask): m_szPath(path), m_nMask(mask)
{
	// create file descriptor for inotify
	m_fd = inotify_init();
	if ( m_fd < 0 ) perror( "inotify_init" );
	m_Log << "notify FD created with file descriptor of '" << m_fd << "'" << CUtil::CLog::endl;

	start();

/*
	// let's watch the specified path
	m_wd = inotify_add_watch( m_fd, path.c_str(), mask);
	if (m_wd == -1)
	{
		m_Log << "ERROR: Something went wrong in trying to watch '" << path << "'. Check the path again if it's valid. " << CUtil::CLog::endl;
		return;
	}	
	else m_Log << "Now watching '" << path << CUtil::CLog::endl;	
*/
}

/* ------------------------------------------------------------------------------------------

------------------------------------------------------------------------------------------ */
void CNotifyFileDescriptor::start()
{
	// let's watch the specified path
	m_wd = inotify_add_watch( m_fd, m_szPath.c_str(), m_nMask);
	if (m_wd == -1)
	{
		m_Log << "ERROR: Something went wrong in trying to watch '" << m_szPath << "'. Check the path again if it's valid. " << CUtil::CLog::endl;
		return;
	}	
	else m_Log << "Now watching '" << m_szPath << CUtil::CLog::endl;		
}

/* ------------------------------------------------------------------------------------------

------------------------------------------------------------------------------------------ */
void CNotifyFileDescriptor::stop()
{
	inotify_rm_watch( m_fd, m_wd );
	m_Log << "Stopped watching '" << m_szPath << CUtil::CLog::endl;
}

/* ------------------------------------------------------------------------------------------
stop watching the directory path
------------------------------------------------------------------------------------------ */
CNotifyFileDescriptor::~CNotifyFileDescriptor()
{
	stop();
	//inotify_rm_watch( m_fd, m_wd );
	close( m_fd );
}

/* ------------------------------------------------------------------------------------------
event handler when this file descriptor has response from select()
------------------------------------------------------------------------------------------ */
void CNotifyFileDescriptor::onSelect(bool bOnce)	
{
	// bail out if bad fd
	if (m_fd < 0) return;

	char buffer[BUF_LEN];
	int i = 0;
	memset(buffer, 0, sizeof(buffer)); 
	int length = read( m_fd, buffer, BUF_LEN );  
	if ( length < 0 ) perror( "read" );

	while ( i < length )   
	{
		struct inotify_event *event = ( struct inotify_event * ) &buffer[ i ];

		if (!event->len) continue;

		if ( event->mask & IN_CREATE ) 
		{
			if ( event->mask & IN_ISDIR ) onDirCreate( event->name );       
			else onFileCreate( event->name );
		}
		else if ( event->mask & IN_DELETE ) 
		{
			if ( event->mask & IN_ISDIR ) onDirDelete( event->name );      
			else onFileDelete( event->name );
		}
		else if ( event->mask & IN_MODIFY ) 
		{
			if ( event->mask & IN_ISDIR ) onDirModify( event->name ); 
			else onFileModify( event->name );
		}
		else if ( event->mask & IN_ACCESS ) 
		{
			if ( event->mask & IN_ISDIR ) onDirAccess( event->name ); 
			else onFileAccess( event->name );
		} 
		else if ( event->mask & IN_ATTRIB ) 
		{
			if ( event->mask & IN_ISDIR ) onDirAttrChange( event->name ); 
			else onFileAttrChange( event->name );
		}
		else if ( event->mask & IN_CLOSE_WRITE ) 
		{
			if ( event->mask & IN_ISDIR ) onDirCloseWrite( event->name ); 
			else onFileCloseWrite( event->name );
		}
		else if ( event->mask & IN_CLOSE_NOWRITE ) 
		{
			if ( event->mask & IN_ISDIR ) onDirCloseNoWrite( event->name ); 
			else onFileCloseNoWrite( event->name );
		} 
		else if ( event->mask & IN_DELETE_SELF ) 
		{
			if ( event->mask & IN_ISDIR ) onDirDeleteSelf( event->name ); 
			else onFileDeleteSelf( event->name );
		}
		else if ( event->mask & IN_MOVED_FROM ) 
		{
			if ( event->mask & IN_ISDIR ) onDirMoveFrom( event->name ); 
			else onFileMoveFrom( event->name );
		}
		else if ( event->mask & IN_MOVED_TO ) 
		{
			if ( event->mask & IN_ISDIR ) onDirMoveTo( event->name ); 
			else onFileMoveTo( event->name );
		}
		else if ( event->mask & IN_OPEN ) 
		{
			if ( event->mask & IN_ISDIR ) onDirOpen( event->name ); 
			else onFileOpen( event->name );
		}
		i += EVENT_SIZE + event->len;
	}

}

/* ------------------------------------------------------------------------------------------
event handlers place holders
------------------------------------------------------------------------------------------ */
void CNotifyFileDescriptor::onDirCreate( const std::string& name ){ m_Log << "directory '" << name << "' was created." << CUtil::CLog::endl; }
void CNotifyFileDescriptor::onFileCreate( const std::string& name ){ m_Log << "file '" << name << "' was created." << CUtil::CLog::endl; }
void CNotifyFileDescriptor::onDirDelete( const std::string& name ){ m_Log << "directory '" << name << "' was deleted." << CUtil::CLog::endl; }
void CNotifyFileDescriptor::onFileDelete( const std::string& name ){ m_Log << "file '" << name << "' was deleted." << CUtil::CLog::endl; }
void CNotifyFileDescriptor::onDirModify( const std::string& name ){ m_Log << "directory '" << name << "' was modified." << CUtil::CLog::endl; }
void CNotifyFileDescriptor::onFileModify( const std::string& name ){ m_Log<< "file '" << name << "' was modified." << CUtil::CLog::endl; }
void CNotifyFileDescriptor::onDirAccess( const std::string& name ){ m_Log << "directory '" << name << "' was accessed." << CUtil::CLog::endl; }
void CNotifyFileDescriptor::onFileAccess( const std::string& name ){ m_Log << "file '" << name << "' was accessed." << CUtil::CLog::endl; }
void CNotifyFileDescriptor::onDirAttrChange( const std::string& name ){ m_Log << "directory '" << name << "' attribute changed." << CUtil::CLog::endl; }
void CNotifyFileDescriptor::onFileAttrChange( const std::string& name ){ m_Log << "file '" << name << "' attribute changed." << CUtil::CLog::endl; }
void CNotifyFileDescriptor::onDirCloseWrite( const std::string& name ){ m_Log << "directory '" << name << "' open for writing was closed." << CUtil::CLog::endl; }
void CNotifyFileDescriptor::onFileCloseWrite( const std::string& name ){ m_Log << "file '" << name << "' open for writing was closed." << CUtil::CLog::endl; }
void CNotifyFileDescriptor::onDirCloseNoWrite( const std::string& name ){ m_Log << "directory '" << name << "' open for reading was closed." << CUtil::CLog::endl; }
void CNotifyFileDescriptor::onFileCloseNoWrite( const std::string& name ){ m_Log << "file '" << name << "' open for reading was closed." << CUtil::CLog::endl; }
void CNotifyFileDescriptor::onDirDeleteSelf( const std::string& name ){ m_Log << "watched directory '" << name << "' was deleted." << CUtil::CLog::endl; }
void CNotifyFileDescriptor::onFileDeleteSelf( const std::string& name ){ m_Log << "wathed file '" << name << "' was deleted." << CUtil::CLog::endl; }
void CNotifyFileDescriptor::onDirMoveTo( const std::string& name ){ m_Log << "directory '" << name << "' was moved to this location." << CUtil::CLog::endl; }
void CNotifyFileDescriptor::onFileMoveTo( const std::string& name ){ m_Log << "file '" << name << "' was moved to this location." << CUtil::CLog::endl; }
void CNotifyFileDescriptor::onDirMoveFrom( const std::string& name ){ m_Log << "directory '" << name << "' was moved from this location." << CUtil::CLog::endl; }
void CNotifyFileDescriptor::onFileMoveFrom( const std::string& name ){ m_Log << "file '" << name << "' was moved from this location." << CUtil::CLog::endl; }
void CNotifyFileDescriptor::onDirOpen( const std::string& name ){ m_Log << "directory '" << name << "' was opened." << CUtil::CLog::endl; }
void CNotifyFileDescriptor::onFileOpen( const std::string& name ){ m_Log << "file '" << name << "' was opened." << CUtil::CLog::endl; }

