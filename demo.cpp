#include <sstream>
#include <utility.h>
#include <cex.h>  
#include <cmd.h>
#include <xml.h> 

int main(int argc, char **argv)  
{
	CCex T;

	// add commands here
	T.addCmd(new CGetHead());
	T.addCmd(new CCexVersion());
	T.addCmd(new CGetName());
	T.addCmd(new CLoad());
	T.addCmd(new CUnload());
	T.addCmd(new CStart());
	T.addCmd(new CGetUserName());
	T.addCmd(new CProgramLoaded());
	T.addCmd(new CProgramLoadDone());
	T.addCmd(new CGetExp());
	T.addCmd(new CEvxSummary());
	T.addCmd(new CDlogMethods());
	T.addCmd(new CDlogFileFreq());
	T.addCmd(new CDlogType());
	T.addCmd(new CDlogSampleRate());
	T.addCmd(new CDlogTestID());
	T.addCmd(new CDebug());
	T.addCmd(new CExecFlow());
	T.addCmd(new CSave());
	T.addCmd(new CSaveAs());
	T.addCmd(new CRestart());
	T.addCmd(new CDFilter());
	T.addCmd(new CCexHelp());
	T.addCmd(new CList());
	//T.addCmd(new CListActiveObjects());
	T.addCmd(new CListBoards());
	T.addCmd(new CListWafers());
	T.addCmd(new CListFlows());
	T.addCmd(new CListMaps());
	T.addCmd(new CListExtIntfObjects());

	// parse command line args and execute 
	if (T.scan(argc, argv)) T.exec();

	return 0;
}


