#include <app1.h>
#include <utility.h>
#include <engine.h>
/*
action item:
-	when destroying CSequence, must destroy all its child tasks too
-	should we execute killtester state if we time-out on launch?


*/

int main(int argc, char **argv)  
{
	CUtil::CLog::silent = false;
	CUtil::CLog Log;

	CApp App(argc, argv);

	Log << "This is test exec!" << CUtil::CLog::endl;

	return 0;

#if 0
	std::list< std::string > strs;

	std::list< std::string >::iterator t;// = strs.begin();

	if (t == strs.end()) Log << "same" << CUtil::CLog::endl;

	
	CStateManager Mgr;

	CState state("test");

	Mgr.set(&state);

	CTask task0 ("0");
	CTask task1 ("1", 431);
	CTask task2 ("2", 2640);
	CTask task3 ("3", 0);

	CSequence seq;
	seq.queue(&task1);
	seq.queue(&task2);
	seq.queue(&task3);
	
//	state.add(&task0);
//	state.add(&task1);
	state.add(&seq);

	Mgr.run();
	
	return 0;

#endif
}



