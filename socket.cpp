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
bool CClient::connect(const std::string& ip, unsigned short port, int nType)
{
	/* ---------------------------------------------------------------------------------------------------------
	create a socket that we'll use to listen to an incoming connection request.
	-	AF_INET means we are using IPv4 protocols as communication domain. 
	-	SOCK_STREAM is set as type. it means the socket uses TCP or connection oriented protocol
	-	returns this socket's file descriptor if successful
	--------------------------------------------------------------------------------------------------------- */
	m_fdSocket = socket(AF_INET, nType, 0);	
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


/* ------------------------------------------------------------------------------------------

------------------------------------------------------------------------------------------ */
void CClient::listen(unsigned long ms)
{

}

/* ------------------------------------------------------------------------------------------

------------------------------------------------------------------------------------------ */
CClientFileDescriptor::CClientFileDescriptor(const std::string& ip, unsigned short port, int type)
:m_szIP(ip), m_nPort(port), m_nType(type)
{
	 m_fd = -1;
}

CClientFileDescriptor::~CClientFileDescriptor()
{
}

/* ------------------------------------------------------------------------------------------

------------------------------------------------------------------------------------------ */
bool CClientFileDescriptor::connect()
{
	/* ---------------------------------------------------------------------------------------------------------
	create a socket that we'll use to listen to an incoming connection request.
	-	AF_INET means we are using IPv4 protocols as communication domain. 
	-	SOCK_STREAM is set as type. it means the socket uses TCP or connection oriented protocol
	-	returns this socket's file descriptor if successful
	--------------------------------------------------------------------------------------------------------- */
	m_fd = socket(AF_INET, m_nType, 0);	
	if (m_fd == -1)
	{
		m_Log << "ERROR: Failed to create socket!" << CUtil::CLog::endl;
		return false;
	}

	/* ---------------------------------------------------------------------------------------------------------
	setup port info for the server we want to connect to and try to connect to it
	--------------------------------------------------------------------------------------------------------- */
	sockaddr_in addrClient; 
	addrClient.sin_family = AF_INET;
	addrClient.sin_port = htons(m_nPort);
	if (inet_pton(AF_INET, m_szIP.c_str(), &addrClient.sin_addr) <= 0)
	{
		m_Log << "ERROR: Invalid adress!" << CUtil::CLog::endl;
		return false;
	}

	if (::connect(m_fd, (sockaddr*)&addrClient, sizeof(addrClient)) < 0)
	{
		m_Log << "ERROR: No connection!" << CUtil::CLog::endl;
		return false;
	}
	m_Log << "Successfully connected to server at port: " << ntohs(addrClient.sin_port) << CUtil::CLog::endl;

	return true;
}

/* ------------------------------------------------------------------------------------------

------------------------------------------------------------------------------------------ */
bool CClientFileDescriptor::send(const std::string& msg)
{
	::send(m_fd, msg.c_str(), msg.length(), 0);
	return true;
}

/* ------------------------------------------------------------------------------------------

------------------------------------------------------------------------------------------ */
void CClientFileDescriptor::onSelect()
{
	// bail out if bad fd
	if (m_fd < 0) return;

	// message is received from server. must be a broadcast message. let's print it.
	char buf[MAXBYTEMESSAGE];
	int nBytesReceived = recv(m_fd, buf, MAXBYTEMESSAGE, 0);

	// this client is exiting. fd must be invalid now so we set to -1
	if (nBytesReceived <= 0)
	{
		m_Log << "Server stopped..." << CUtil::CLog::endl;
		m_fd = -1;
		return;
	}

	m_Log << std::string(buf, 0, nBytesReceived)  << CUtil::CLog::endl;				
}

/* ------------------------------------------------------------------------------------------

------------------------------------------------------------------------------------------ */
CClientFileDescriptorUDP::CClientFileDescriptorUDP()
{
	m_szIP = SOCKET_IP_DEFAULT;
	m_nPort = SOCKET_PORT_DEFAULT; 
	m_fd = -1;
}

/* ------------------------------------------------------------------------------------------

------------------------------------------------------------------------------------------ */
bool CClientFileDescriptorUDP::connect(const std::string& ip, unsigned short port)
{
	m_szIP = ip;
	m_nPort = port;

	m_fd = socket(AF_INET, SOCK_DGRAM, 0);	
	if (m_fd == -1)
	{
		m_Log << "ERROR: Failed to create socket!" << CUtil::CLog::endl;
		return false;
	}

	// Filling server information 
	memset(&m_addrServ, 0, sizeof(m_addrServ)); 
	m_addrServ.sin_family = AF_INET; 
	m_addrServ.sin_port = htons(m_nPort); 
	m_addrServ.sin_addr.s_addr = INADDR_ANY;  
}

/* ------------------------------------------------------------------------------------------

------------------------------------------------------------------------------------------ */
CClientFileDescriptorUDP::~CClientFileDescriptorUDP()
{
	close(m_fd);
}

/* ------------------------------------------------------------------------------------------

------------------------------------------------------------------------------------------ */
void CClientFileDescriptorUDP::onSelect()
{
	// bail out if bad fd
	if (m_fd < 0) return;

	int len; 
	char buf[MAXBYTEMESSAGE];
	int nBytesReceived = recvfrom(m_fd, buf, MAXBYTEMESSAGE,  MSG_WAITALL, (struct sockaddr *) &m_addrServ, (socklen_t*) &len); 
	buf[nBytesReceived] = '\0'; 
	
	// pass the received message to handler
	onRecv( std::string(buf, 0, nBytesReceived) );
}

/* ------------------------------------------------------------------------------------------

------------------------------------------------------------------------------------------ */
bool CClientFileDescriptorUDP::send(const std::string& msg)
{
	::sendto(m_fd, msg.c_str(), msg.length(), MSG_DONTWAIT, (const struct sockaddr *) &m_addrServ,  sizeof(m_addrServ)); 
	return true;
}

/* ------------------------------------------------------------------------------------------

------------------------------------------------------------------------------------------ */
CServerUDP::CServerUDP(const std::string& ip, unsigned short port)
:m_szIP(ip), m_nPort(port)
{
	m_fd = -1;
	memset(&m_addrClient, 0, sizeof(m_addrClient));
	memset(&m_addrServ, 0, sizeof(m_addrServ)); 
}

/* ------------------------------------------------------------------------------------------

------------------------------------------------------------------------------------------ */
bool CServerUDP::connect(const std::string& ip, unsigned short port)
{
	m_szIP = ip;
	m_nPort = port;
	m_fd = socket(AF_INET, SOCK_DGRAM, 0);	
	if (m_fd == -1)
	{
		std::stringstream ss;
		onError("ERROR: Failed to create socket!");
		m_Log << "ERROR: Failed to create socket!" << CUtil::CLog::endl;
		return false;
	}

	// Filling server information 
	memset(&m_addrServ, 0, sizeof(m_addrServ)); 
	m_addrServ.sin_family = AF_INET; 
	m_addrServ.sin_port = htons(m_nPort); 
	m_addrServ.sin_addr.s_addr = INADDR_ANY;  

	// Bind the socket with the server address 
	if (bind(m_fd, (sockaddr*)&m_addrServ, sizeof(m_addrServ)) == -1)
	{
		onError("ERROR: Failed to bind to port");
		m_Log << "ERROR: Failed to bind to port: " << m_nPort << "!" << CUtil::CLog::endl; 
		return false;
	}

	return true;
}

/* ------------------------------------------------------------------------------------------

------------------------------------------------------------------------------------------ */
bool CServerUDP::send(const std::string& msg, int* pErrNum)
{
	if ( sendto(m_fd, msg.c_str(), msg.length(), MSG_DONTWAIT, (const struct sockaddr *) &m_addrClient,  sizeof(m_addrClient)) == -1)
	{
		if (pErrNum) *pErrNum = errno;
		return false;
	} 
	else return true;
}

/* ------------------------------------------------------------------------------------------
** IMPORTANT!!! len must be initialized with sizeof(m_addrClient) for sendto() to work
------------------------------------------------------------------------------------------ */
void CServerUDP::listen(unsigned long ms)
{
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(fileno(stdin), &fds);
	FD_SET(m_fd, &fds);
	int nMaxFD = m_fd;

	struct timeval to;
	to.tv_sec = (unsigned long) ms / 1000; // convert to second
	to.tv_usec = (ms % 1000) * 1000; // convert to usec

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

	// check all file descriptors and find the one that is ready
	for (int i = 0; i <= nMaxFD; i++)
	{
		// only if this is the socket ready for reading
		if (!FD_ISSET(i, &ready)) continue;

		// is it the listening socket?
		if (i == m_fd)
		{
			char buffer[MAXBYTEMESSAGE];
			memset(&m_addrClient, 0, sizeof(m_addrClient));		
			int len = sizeof(m_addrClient); /* IMPORTANT INITIALIZATION!! */
			int n = recvfrom(m_fd, (char *)buffer, MAXBYTEMESSAGE,  MSG_WAITALL, ( struct sockaddr *) &m_addrClient, (socklen_t*)&len); 
			buffer[n] = '\0'; 
			std::stringstream ss;
			ss << buffer;
			onRecv(ss.str());
	
		}
	}
}





