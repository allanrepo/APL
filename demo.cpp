#include <sstream>
#include <utility.h>
#include <tester.h>  

void doThis()
{
	CUtil::CLog Log;
	Log << "hello event function" << CUtil::CLog::endl;
}

int main(int argc, char **argv)  
{
	CTester Tester;
	
	while(1)
	{	
		Tester.connect("localuser_sim");
		Tester.loop();
		Tester.disconnect();
	}
/*
	foo f;
	f.doThisPtr = &doThis;
	f.doThis();
	f.eventPtr = &event::doThis;

	event e;
	f.doThisEvent(e);

*/
	return 0;
}


