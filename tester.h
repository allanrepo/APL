#ifndef __TESTER__
#define __TESTER__

#include <evxa/ProgramControl.hxx>
#include <evxa/StateNotification.hxx>
#include <evxa/EvxioStreamsClient.hxx>
#include <utility.h>
#include <unistd.h>
#include <arg.h>

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
class CTester: public CArg
{
private:
	// make these private to turn this class into singleton	
	CTester(); // private so we can't instantiate	
	virtual ~CTester(); // private so we can't destroy
	CTester(const CTester&){} // make copy constructor private so we can't copy                    
	const CTester& operator=(const CTester&){} // make = operator private so we can't copy

	// tester objects
  	TesterConnection *m_pTester;
  	TestheadConnection *m_pConn;
	ProgramControl *m_pProgCtrl;
  	CStateNotification *m_pState;
  	CEvxioStreamClient *m_pEvxio;
	int m_nHead;

public:
	// loggers
	CUtil::CLog m_Log;
	CUtil::CLog m_Debug;

public:
	// you can't instantiate yourself so this is how u get a reference to the singleton object
	static CTester& instance()
	{
		static CTester inst;
		return inst;
	}

	// tester functions
	bool connect(const std::string& strTesterName, int nAttempts = 1);
	void disconnect();
	void loop();

	// accessors to tester objects
	TesterConnection* Tester(){ return m_pTester; }
	TestheadConnection* TestHead(){ return m_pConn; }
	ProgramControl* ProgCtrl(){ return m_pProgCtrl; }
	CStateNotification* StateNotify(){ return m_pState; }
	CEvxioStreamClient* StreamClient(){ return m_pEvxio; }

	// tester properties
	int getHead(){ return m_nHead; }
};


ProgramControl* const ProgCtrl();



#endif
