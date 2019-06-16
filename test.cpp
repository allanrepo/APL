#include <app1.h>
#include <utility.h>


int main(int argc, char **argv)  
{
	CUtil::CLog::silent = false;
	CUtil::CLog Log;

	CApp App(argc, argv);

	Log << "This is test exec!" << CUtil::CLog::endl;
	return 0;
}



