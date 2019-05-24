#include <socket.h>

/* ------------------------------------------------------------------------------------------
constructor
------------------------------------------------------------------------------------------ */
CClient::CClient()
{

}

/* ------------------------------------------------------------------------------------------
destructor
------------------------------------------------------------------------------------------ */
CClient::~CClient()
{

}

/* ------------------------------------------------------------------------------------------
connect command. connects to specific IP/Port via TCP-IP
------------------------------------------------------------------------------------------ */
bool CClient::connect(const std::string& ip, unsigned short port)
{
	/* ---------------------------------------------------------------------------------------------------------
	create a socket that we'll use to listen to an incoming connection request.
	-	AF_INET means we are using IPv4 protocols as communication domain. 
	-	SOCK_STREAM is set as type. it means the socket uses TCP or connection oriented protocol
	-	returns this socket's file descriptor if successful
	--------------------------------------------------------------------------------------------------------- */
	m_fdSocket = socket(AF_INET, SOCK_STREAM, 0);	
	if (m_fdSocket == -1)
	{
		m_Log << "ERROR: Failed to create socket!" << CUtil::CLog::endl;
		return false;
	}

	/* ---------------------------------------------------------------------------------------------------------
	setup port info for the server we want to connect to and try to connect to it
	--------------------------------------------------------------------------------------------------------- */
	sockaddr_in addrClient; 
	addrClient.sin_family = AF_INET;
	addrClient.sin_port = htons(port);
	if (inet_pton(AF_INET, ip.c_str(), &addrClient.sin_addr) <= 0)
	{
		m_Log << "ERROR: Invalid adress!" << CUtil::CLog::endl;
		return false;
	}

	if (::connect(m_fdSocket, (sockaddr*)&addrClient, sizeof(addrClient)) < 0)
	{
		m_Log << "ERROR: No connection!" << CUtil::CLog::endl;
		return false;
	}
	m_Log << "Successfully connected to server at port: " << ntohs(addrClient.sin_port) << CUtil::CLog::endl;

	return true;
}

/* ------------------------------------------------------------------------------------------
send a string of bytes to server
------------------------------------------------------------------------------------------ */
bool CClient::send(const std::string& in)
{
	::send(m_fdSocket, in.c_str(), in.length(), 0);
	return true;
}

/* ------------------------------------------------------------------------------------------
disconnec from server
------------------------------------------------------------------------------------------ */
void CClient::disconnect()
{
	close(m_fdSocket);
}

#if 0
bool CClient::read(unsigned long ms)
{
	/* ---------------------------------------------------------------------------------------------------------
	create fd_set variable and store the file descriptors for listening port. the max fd value is required
	by select() so must get it
	--------------------------------------------------------------------------------------------------------- */
	fd_set fds;
	FD_ZERO(&fds);

	// set stdin
	FD_SET(fileno(stdin), &fds);
	int nMaxFD = fileno(stdin);

	// set socket fd
	FD_SET(m_fdSocket, &fds);
	nMaxFD = m_fdSocket > nMaxFD? m_fdSocket : nMaxFD;

	/* ---------------------------------------------------------------------------------------------------------
	enter into app loop and monitor all sockets' file descriptor if changes happen to any of them.
	if there's an incoming connection request, listening port's file descriptor is set. connection to 
	server will be handled accordingly. 
	if incoming message request occurs, the sending client's file descriptor is set. handle it accordingly.
	--------------------------------------------------------------------------------------------------------- */
	bool go = true;
	while(go)
	{
		struct timeval to;
		to.tv_sec = (unsigned long) ms / 1000; // convert to second
		to.tv_usec = (ms % 1000) * 1000; // convert to usec

		// if timeout is not specified, we'll be blocking here and wait for any file descriptor ready to be read.
		fd_set ready = fds;
		if (to.tv_sec == 0 && to.tv_usec == 0)
		{
			int n = select(nMaxFD + 1, &ready, NULL, NULL, NULL);
		}
		// otherwise wait for timeout
		else
		{	
			//std::cout << "time out" << std::endl;
			int n = select(nMaxFD + 1, &ready, NULL, NULL, &to);
			if (n == 0) continue;
		}

		// check all file descriptors and find the one that is ready
		for (int i = 0; i <= nMaxFD; i++)
		{
			// only if this is the socket ready for reading
			if (!FD_ISSET(i, &ready)) continue;

			// message is being typed on stdin. let's get it and send to server
			if (i == fileno(stdin))
			{
				std::string s;
				std::getline(std::cin, s);		
				::send(m_fdSocket, s.c_str(), s.length(), 0);	
				continue;
			}
			// message is received from server. must be a broadcast message. let's print it.
			else
			{
				char buf[MAXBYTEMESSAGE];
				int nBytesReceived = recv(i, buf, MAXBYTEMESSAGE, 0);

				// this client is exiting
				if (nBytesReceived <= 0)
				{
					close(i);	
					FD_CLR(i, &fds);
					std::cout << "Server stopped. Quitting..." << std::endl;
					go = false;
					break;
				}

				std::cout << std::string(buf, 0, nBytesReceived)  << std::endl;			
			}
		}
	}
}

bool CClient::read(std::string& out, unsigned long ms)
{
	/* ---------------------------------------------------------------------------------------------------------
	enter into app loop and monitor all sockets' file descriptor if changes happen to any of them.
	if there's an incoming connection request, listening port's file descriptor is set. connection to 
	server will be handled accordingly. 
	if incoming message request occurs, the sending client's file descriptor is set. handle it accordingly.
	--------------------------------------------------------------------------------------------------------- */

	struct timeval to;
	to.tv_sec = (unsigned long) ms / 1000; // convert to second
	to.tv_usec = (ms % 1000) * 1000; // convert to usec

	// if timeout is not specified, we'll be blocking here and wait for any file descriptor ready to be read.
	fd_set ready = m_fds;
	if (to.tv_sec == 0 && to.tv_usec == 0)
	{
		int n = select(m_nMaxFD + 1, &ready, NULL, NULL, NULL);
	}
	// otherwise wait for timeout
	else
	{	
		int n = select(m_nMaxFD + 1, &ready, NULL, NULL, &to);
		if (n == 0) return true;
	}

	// check all file descriptors and find the one that is ready
	for (int i = 0; i <= m_nMaxFD; i++)
	{
		// only if this is the socket ready for reading
		if (!FD_ISSET(i, &ready)) continue;

		// message is being typed on stdin. let's get it and send to server
		if (i == fileno(stdin))
		{
			continue;
		}
		// message is received from server. must be a broadcast message. let's print it.
		else
		{
			char buf[MAXBYTEMESSAGE];
			int nBytesReceived = recv(i, buf, MAXBYTEMESSAGE, 0);

			// this client is exiting
			if (nBytesReceived <= 0)
			{
				close(i);	
				FD_CLR(i, &m_fds);
				std::cout << "Server stopped. Quitting..." << std::endl;
				disconnect();
				return false;
			}
			out = std::string(buf, 0, nBytesReceived);
		}
	}
	return true;
}
#endif



CServer::CServer()
{

}

CServer::~CServer()
{

}


