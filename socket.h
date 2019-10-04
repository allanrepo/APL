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
#include <fd.h>

#define SOCKET_IP_DEFAULT "127.0.0.1"
#define SOCKET_PORT_DEFAULT 54000 // default listenting port used if not specified in command line 
#define MAXBYTEMESSAGE 256

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

	void listen(unsigned long ms);
};

class CClientFileDescriptor: public CFileDescriptorManager::CFileDescriptor
{
protected:
	std::string m_szIP;
	unsigned short m_nPort;
	int m_nType;
	CUtil::CLog m_Log;

public:
	CClientFileDescriptor(const std::string& ip = SOCKET_IP_DEFAULT , unsigned short port = SOCKET_PORT_DEFAULT, int nType = SOCK_STREAM);
	virtual ~CClientFileDescriptor();
	virtual void onSelect();
	bool connect();
	bool send(const std::string& msg);
};

class CClientFileDescriptorUDP: public CFileDescriptorManager::CFileDescriptor
{
protected:
	std::string m_szIP;
	unsigned short m_nPort;
	CUtil::CLog m_Log;
	struct sockaddr_in m_addrServ; 

public:
	CClientFileDescriptorUDP(const std::string& ip = SOCKET_IP_DEFAULT , unsigned short port = SOCKET_PORT_DEFAULT);
	virtual ~CClientFileDescriptorUDP();
	virtual void onSelect();
	bool send(const std::string& msg);
	bool connect();
};

#endif
