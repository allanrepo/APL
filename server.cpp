// Server side implementation of UDP client-server model 
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 

#include <utility.h>  
#include <string.h>
#include <sstream>
#include <iostream>

#define PORT    54000 
#define MAXLINE 1024 
  
// Driver code 
int main(int argc, char **argv)  
{ 

	// let's make sure this is the only instance of this application that runs

	// trim the name of '/' and path, we just want name
	std::string name = argv[0];
	size_t pos = name.rfind('/');
	if (pos != std::string::npos) name = name.substr(pos + 1); 

	// let's check if same app is already running, kill it.
	// note that we are skipping our own PID so we don't kill ourselves
	int pid = CUtil::getFirstPIDByName(name, true);
	while ( pid != -1 )
	{
		std::stringstream ssCmd;
		ssCmd << "kill -9 " << pid;
		std::cout << ssCmd.str() << std::endl;
		system(ssCmd.str().c_str());	

		// after killing, let's keep finding. 
		pid = CUtil::getFirstPIDByName(name,true );
	}


    int sockfd; 
    char buffer[MAXLINE]; 
    const char *hello = "Hello from server"; 
    struct sockaddr_in servaddr, cliaddr; 
      
    // Creating socket file descriptor 
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 
      
    memset(&servaddr, 0, sizeof(servaddr)); 
    memset(&cliaddr, 0, sizeof(cliaddr)); 
      
    // Filling server information 
    servaddr.sin_family    = AF_INET; // IPv4 
    servaddr.sin_addr.s_addr = INADDR_ANY; 
    servaddr.sin_port = htons(PORT); 
      
    // Bind the socket with the server address 
    if ( bind(sockfd, (const struct sockaddr *)&servaddr,  
            sizeof(servaddr)) < 0 ) 
    { 
        perror("bind failed"); 
        exit(EXIT_FAILURE); 
    } 

	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(fileno(stdin), &fds);
	FD_SET(sockfd, &fds);
	int nMaxFD = sockfd;

	bool go = true;
	while(go)
	{
		// we'll be blocking here and wait for any file descriptor ready to be read.
		fd_set ready = fds;
//		int n = select(nMaxFD + 1, &ready, NULL, NULL, NULL);

		struct timeval to;
		to.tv_sec = 1;
		to.tv_usec = 0;

		int n = select(nMaxFD + 1, &ready, NULL, NULL, &to);
//		std::cout << "server idle "<< std::endl;
		if (n == 0) continue;
      
		// check all file descriptors and find the one that is ready
		for (int i = 0; i <= nMaxFD; i++)
		{
			// only if this is the socket ready for reading
			if (!FD_ISSET(i, &ready)) continue;
/*
			// message is being typed on stdin. let's get it and send to server
			if (i == fileno(stdin))
			{
				std::string s;
				std::getline(std::cin, s);		
//				int n, len; 
				std::cout << s << std::endl;
				//sendto(sockfd, s.c_str(), s.length(), MSG_DONTWAIT, (const struct sockaddr *) &cliaddr, sizeof(cliaddr)); 
				continue;
			}	
*/
			// is it the listening socket?
			if (i == sockfd)
			{
				int len, n; 
				//memset(&cliaddr, 0, sizeof(cliaddr));
				n = recvfrom(sockfd, (char *)buffer, MAXLINE,  MSG_WAITALL, ( struct sockaddr *) &cliaddr, (socklen_t*)&len); 
				buffer[n] = '\0'; 
				printf("Client : %s\n", buffer); 
				std::stringstream ss;
				ss << "SERVER: " << buffer;
				sendto(sockfd, ss.str().c_str(), ss.str().length(), MSG_CONFIRM, (const struct sockaddr *) &cliaddr, sizeof(cliaddr)); 
			}
		}
	}

} 
