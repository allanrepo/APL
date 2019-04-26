#include <sstream>
#include <utility.h>
#include <tester.h>  
#include <fd.h>
#include <notify.h>
#include <app.h>


void doThis()
{
	CUtil::CLog Log;
	Log << "hello event function" << CUtil::CLog::endl;
}

int main(int argc, char **argv)  
{
#if 1
	CApp App;
	return 0;
#endif
#if 0 // test notify
	CFileDescriptorManager fdm;
	CNotifyFileDescriptor fdNotify("/home/localuser/Desktop/APL");
	CUtil::CLog Log;
	fdm.add( fdNotify );
	while (1) fdm.select();

	return 0;
#endif

	CTester Tester;
	
	while(1)
	{	
		Tester.connect("localuser_sim", 1, -1);
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


