#ifndef __SERVER__
#define __SERVER__

#include <iostream>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <string>
#include <sstream>
#include <sys/select.h>
#include <fd.h>
#include <utility.h>



#define SOCKET_IP_DEFAULT "127.0.0.1"
#define SOCKET_PORT_DEFAULT 54000 // default listenting port used if not specified in command line 
#define MAXBYTEMESSAGE 4096

/* ------------------------------------------------------------------------------------------
server class
------------------------------------------------------------------------------------------ */
class CServer
{
private:
	int m_fdSocketToListenTo;
	fd_set m_FDs;
	int m_nMaxFD;
public:
	CServer(){}
	virtual ~CServer(){}


	int start(unsigned short port = SOCKET_PORT_DEFAULT, int type = SOCK_STREAM);

	void listen(unsigned long ms);
	void run();

	int send(int to, const std::string& msg);

};

#endif
