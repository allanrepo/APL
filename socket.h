#ifndef __SOCKET__
#define __SOCKET__

#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <sstream>
#include <stdio.h>
#include <utility.h>

#define SOCKET_IP_DEFAULT "127.0.0.1"
#define SOCKET_PORT_DEFAULT 54000 // default listenting port used if not specified in command line 
#define MAXBYTEMESSAGE 4096

/* ------------------------------------------------------------------------------------------
client class
------------------------------------------------------------------------------------------ */
class CClient
{
private:
	int m_fdSocket;
	fd_set m_fds;
	int m_nMaxFD;
	CUtil::CLog m_Log;
public:
	CClient();
	virtual ~CClient();

	bool connect(const std::string& ip = SOCKET_IP_DEFAULT , unsigned short port = SOCKET_PORT_DEFAULT, int nType = SOCK_STREAM);
	bool send(const std::string& in);
	void disconnect();
};


#endif
