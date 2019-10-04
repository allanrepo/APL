#include <sstream>
#include <utility.h>
#include <tester.h>  
#include <fd.h>
#include <notify.h>
#include <app.h>
#include <sys/types.h>
#include <unistd.h>


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

	// at this point, there should only be us that is running...

	CUtil::CLog::silent = false; // logging silent by default
	CApp App(argc, argv);
	return 0;
}


