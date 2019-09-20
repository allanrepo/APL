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

