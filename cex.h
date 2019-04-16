#ifndef __CEX__
#define __CEX__

#include <cstdio>
#include <iostream>
#include <unistd.h>
#include <pwd.h> 
#include <tester.h>  
#include <arg.h>

/* ------------------------------------------------------------------------------------------
cex class 
-	has reference to singleton class CTester
-	has reference to class CTester's loggers
------------------------------------------------------------------------------------------ */
class CCex: public CArg
{
private:
	// CTester is a singleton object
	CTester& m_Tester;

	// loggers that are referenced to CTester's members (public)
	CUtil::CLog& m_Log;
	CUtil::CLog& m_Debug;

	// get login username. private as we use it exclusively for default tester name
	const std::string getUserName() const;

public:
	CCex(); 	
	virtual ~CCex(){} 

	// store command line arguments to a list for processing
	bool scan(int argc, char **argv);

	// add commands
	void addCmd( CArg* pCmd );

	void clear( CArg* pCmd );

	// enter into loop and monitor command line entries
	void loop();

	// virtual methods from CArg to be overriden
	virtual bool exec();
	virtual bool scan(std::list< std::string >& Args);	
};



#endif

