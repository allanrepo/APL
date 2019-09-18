#ifndef __TESTER__
#define __TESTER__

#include <evxa/ProgramControl.hxx>
#include <evxa/StateNotification.hxx>
#include <evxa/EvxioStreamsClient.hxx>
#include <utility.h>
#include <unistd.h>


#define SAFE_DELETE(p){ delete(p); p = 0; }
/* ------------------------------------------------------------------------------------------
class declarations
------------------------------------------------------------------------------------------ */
class CTester;
class CStateNotification;
class CEvxioStreamClient;

class CEvxioStreamClient: public EvxioStreamsClient
{
public:
  	CEvxioStreamClient(char *strTesterName, int nHead):EvxioStreamsClient(strTesterName, nHead) {}
	virtual ~CEvxioStreamClient(){}
};
 
/* ------------------------------------------------------------------------------------------
tester class 
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

	// check if tester is ready
	bool isReady()
	{
		if (!m_pTester) return false;
		return m_pTester->isTesterReady(m_nHead);
	}

	// load program
	bool load(const std::string& name, bool bDisplay = false);
	bool unload(bool bWait = true, int nWait = 0);

	// event handler for state notification program change
	virtual void onProgramChange(const EVX_PROGRAM_STATE state, const std::string& msg){}

	// event handler for state notification EOT
	virtual void onEndOfTest(const int array_size, int site[], int serial[], int sw_bin[], int hw_bin[], int pass[], EVXA_ULONG dsp_status = 0){}
	virtual void onLotChange(const EVX_LOT_STATE state, const std::string& szLotId){}
	virtual void onWaferChange(const EVX_WAFER_STATE state, const std::string& szWaferId){}

	// let our state notification class access tester's members
	friend class CStateNotification;

	// utility functions
	bool isProgramLoaded();
	const std::string getProgramFullPath();
	const std::string getProgramName();
};


/* ------------------------------------------------------------------------------------------
state notification class 
------------------------------------------------------------------------------------------ */
class CStateNotification: public StateNotification
{
protected:
	CTester& m_Tester;
public:
  	CStateNotification(CTester& tester):StateNotification(*tester.m_pTestHead), m_Tester(tester){}
	virtual ~CStateNotification(){}
	virtual void gemStateChange(const bool linkState, const bool controlState, const bool spoolState, const char *text_msg){}
	virtual void programChange(const EVX_PROGRAM_STATE state, const char *text_msg){ m_Tester.onProgramChange(state, text_msg); }
	virtual void gemTerminalMessage(const char *pcMessage){}
	virtual void programRunDone(const int array_size, int site[], int serial[], int sw_bin[], int hw_bin[], int pass[], EVXA_ULONG dsp_status = 0)
	{
		m_Tester.onEndOfTest(array_size, site, serial, sw_bin, hw_bin, pass, dsp_status);
	}
	virtual void lotChange(const EVX_LOT_STATE state, const char *lot_id){ m_Tester.onLotChange(state, lot_id); }
	virtual void waferChange(const EVX_WAFER_STATE state, const char *wafer_id){ m_Tester.onWaferChange(state, wafer_id); }

};
#endif
