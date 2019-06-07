#include <sstream>
#include <utility.h>
#include <tester.h>  
#include <fd.h>
#include <notify.h>
#include <app.h>


int main(int argc, char **argv)  
{
	CUtil::CLog::silent = true; // logging silent by default
	CApp App(argc, argv);
	return 0;
}


