#ifndef __TESTER__
#define __TESTER__

#include <evxa/ProgramControl.hxx>
#include <evxa/StateNotification.hxx>
#include <evxa/EvxioStreamsClient.hxx>
#include <utility.h>
#include <unistd.h>

#define SAFE_DELETE(p){ delete(p); p = 0; }

class CStateNotification: public StateNotification
{
public:
  	CStateNotification(const TestheadConnection &thc):StateNotification(thc){}
	virtual ~CStateNotification(){}
	virtual void gemStateChange(const bool linkState, const bool controlState, const bool spoolState, const char *text_msg){}
	virtual void programChange(const EVX_PROGRAM_STATE state, const char *text_msg){}
	virtual void gemTerminalMessage(const char *pcMessage){}
};


class CEvxioStreamClient: public EvxioStreamsClient
{
public:
  	CEvxioStreamClient(char *strTesterName, int nHead):EvxioStreamsClient(strTesterName, nHead) {}
	virtual ~CEvxioStreamClient(){}
};
 
/* ------------------------------------------------------------------------------------------
tester class 
-	singleton class. we only need 1 tester for all.
-	contains loggers. this class being singleton, allows all other classes
	to use this loggers exclusively. they just have to make reference to them
-	2 loggers. 1 for common print display, another for debug display
------------------------------------------------------------------------------------------ */
class CTester
{
protected:
	// tester objects
  	TesterConnection *m_pTester;
  	TestheadConnection *m_pTestHead;
	ProgramControl *m_pProgCtrl;
  	CStateNotification *m_pState;
  	CEvxioStreamClient *m_pEvxio;
	int m_nHead;	

	// logger
	CUtil::CLog m_Log;

public:
	// constructors, operator
	CTester(); 
	virtual ~CTester(); 
	CTester(const CTester&){}                    
	const CTester& operator=(const CTester&){} 

	// tester functions
	bool connect(const std::string& strTesterName, int nSleep = 0, int nAttempts = 1);
	void disconnect();
	void loop();

};

class event
{
public:
	void doThis()
	{
		CUtil::CLog Log;
		Log << "hello event class" << CUtil::CLog::endl;
	}
};



class foo
{


public:
	void (event::* eventPtr)();
	void (*doThisPtr)();

	foo()
	{
		doThisPtr = 0;
		eventPtr = 0;
	}
	virtual ~ foo(){}
	
	void doThisEvent(event& e)
	{
		if (!eventPtr) return;
		else (e.*eventPtr)();
	}

	void doThis()
	{
		if (!doThisPtr) return;
		else doThisPtr();
	}

};

#endif
