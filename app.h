/* ---------------------------------------------------------------------------------------------------------------------
version beta.1.20190607 release notes:
-	now, setSTDF() is called on State Notification's PROGRAM_LOAD event. previously the main app loop checks if 
	program is loaded and only then it calls setSTDF(). this is because i am concerned that app might miss
	PROGRAM_LOAD event because the app connects to tester at the same time it launches OICu with load program 
	at the same time. if program loads faster befure app can connect to tester, it might miss PROGRAM_LOAD 
	event. however, if i check isProgramLoaded() during app loop, evxa returns true even before program is 
	actually loaded, thereby causing it to setSTDF() too soon. this can cause issue intermitently. also, it is
	observed that app is able to connect to tester fast enough before program gets loaded.
-	now there's a possibility that setting STDF through setLotInformation() might fail. if it does, i added
	a mechanism to reconnect tester and setLotInformation() again.

version beta.1.20190530 release notes: 
-	fixed a bug where APL crashes if config.xml set <binning> disabled
-	added an attribute in <STDF state="true"> where state=true enables lotinfo fields to be set to 
	unison's STDF. this is disabled by default now.
-	setting up file to save log (if enabled) is now actively updated through the app loop. this is to
	ensure the file name changes when time stamp (next day) occurs.
-	some variables are now moved into CONFIG structure for more elegant coding design

version beta.1.20190529 release notes:

-	added a new feature following Amkor Bin Solution specificatios Revision 1.3 where APL sends a string 
	containing bin results for all sites tested every test cycle (EOT) to Amkor's remote host via UDP
	-	this feature is disabled by default. this can be enabled through APL's XML config file
	-	IP, port, and socket type can be specified in APL's XML config file as well. Amkor specifies
		socket type to be UDP but APL can be configured to use TCP-IP through APL's XML config file.
	-	bin type to be send (hard or soft) can also be specified in APL's XML config file

-	because there are so many settings to set now, using command line arguments is too much therefore
	APL can now be configured via XML config file. a separate documentation will be provided to describe
	in detail all the parameters that can be set.
	- 	launching APL now is a simpe as >apl -config /home/user/test/config.xml where /home/user/test
		is the path where the config file is located and apl.xml is the name of the config file.
	- 	note that the tag names such as <Field> or <BinType> are case sensitve. but the values they
		contain are not. so <BinType>hard</BinType> has same value as <BinType>HARD</BinType>
	
-	in production, APL will be running in the background, therefore it's logs will not be visible. there
	is now an option to store in into a file. the file and the path to save it can be set in APL XML
	config file

TO-DO list
-	fd manage's add() function implementation is dangerous. it adds new fd directly which you can call in one 
	an fd object's process while that same fd object is being processed by fd manager in it's select() loop.
	bad idea. this will cause instant crash. must fix this by making add() to queue for adding new fd's 
	and peform the actual add only outside of select() loop
-	put a god damn mutex in some of the system calls to synchronize with the app!!!
-	need to mutex logger class. not critical, low priority, but must be done for a more elegant code.
	

--------------------------------------------------------------------------------------------------------------------- */

#ifndef __APP__
#define __APP__
#include <fd.h>
#include <tester.h>
#include <notify.h>
#include <pwd.h>
#include <evxa/EVXA.hxx>
#include <stdf.h>
#include <socket.h>
#include <xml.h>
#include <event.h>

/* ------------------------------------------------------------------------------------------
constants
------------------------------------------------------------------------------------------ */
#define DELIMITER ':'
#define JOBFILE "JOBFILE"
#define VERSION "beta.1.0.20190607"
#define DEVELOPER "allan asis / allan.asis@gmail.com"
#define MAXCONNECT 20
#define KILLAPPCMD "kill.app.sh"
#define KILLTESTERCMD "kill.tester.sh"


/* ------------------------------------------------------------------------------------------
declarations
------------------------------------------------------------------------------------------ */
class CApp;
class CAppFileDesc;
class CMonitorFileDesc;
class CAppEvent;

/* ------------------------------------------------------------------------------------------
event class inherited specifically to allow CApp class methods to be executed 
at onTimeOut.
------------------------------------------------------------------------------------------ */
class CAppEvent: public CEventManager::CEvent
{
protected:
	CApp& m_App;
	void (CApp::* m_onTimeOutPtr)(CEvent*);
	CUtil::CLog m_Log;
public:
	CAppEvent(CApp& app, void (CApp::* p)(CEvent*) = 0, long nTimeOut = 0):
	CEvent(nTimeOut), m_App(app)
	{
		m_onTimeOutPtr = p;
	}
	virtual ~CAppEvent(){}

	// overwritten to allow it to execute a method from CApp class
	virtual void onTimeOut(long n)
	{
		if (m_onTimeOutPtr) (m_App.*m_onTimeOutPtr)(this);
	}
};

/* ------------------------------------------------------------------------------------------
app class. this is where stuff happens. 
inherited CTester class so as to make it a lot easier to access EVXA objects
------------------------------------------------------------------------------------------ */
class CApp: public CTester
{
protected:
	struct CONFIG
	{
		enum APL_TEST_TYPE
		{
			APL_WAFER,
			APL_FINAL,
			APL_MANUAL
		};

		// launch parameters
		bool 		bProd;

		// binning parameters
		bool 		bSendBin;
		bool 		bUseHardBin;
		APL_TEST_TYPE 	nTestType;
		std::string 	IP;	
		int 		nPort;
		int 		nSocketType;

		// logging parameters
		std::string 	szLogPath;
		bool 		bLogToFile;

		// STDF parameters
		bool 		bSendInfo;

		// lotinfo file parameters
		std::string 	szLotInfoFileName;
		std::string 	szLotInfoFilePath;

		CONFIG()
		{
			bProd = true;
			bSendInfo = false;
			bSendBin = false;
			bUseHardBin = false;
			nTestType = APL_FINAL;		
			nPort = 54000;
			IP = "127.0.0.1";
			szLogPath = "/tmp";	
			bLogToFile = false;
			nSocketType = SOCK_STREAM;
			szLotInfoFileName = "lotinfo.txt";
			szLotInfoFilePath = "/tmp";
		}
	};

protected:
	CFileDescriptorManager m_FileDescMgr;
	bool m_bReconnect;

	// config
	CONFIG m_CONFIG;

	// parameters
	std::string m_szProgramFullPathName;
	std::string m_szTesterName;
	std::string m_szConfigFullPathName;

	// file descriptor for inotify to monitor path where lotinfo.txt will be sent
	CMonitorFileDesc* m_pMonitorFileDesc;

	// STDF field holders
	MIR m_MIR;
	SDR m_SDR;
	bool setSTDF();
	bool m_bSTDF;
	bool setLotInformation(const EVX_LOTINFO_TYPE type, const std::string& field, const std::string& label, bool bForce = false);

	// we set flag to allw app to send STDF again after reconnecting to tester. this is necessary in situations where sending STDF fails 
	// after trying to do it after PROGRAM_LOAD
	bool m_bSTDFAftReconnect;

	// initialize variabls, reset stuff
	void init();

	void initLogger(bool bEnable = false);

	// utility function that acquire linux login username
	const std::string getUserName() const;
	
	// parse xml config file and extract this software's settings
	bool config(const std::string& config);

	// event manager
	CEventManager m_EventMgr;

	// event objects
	CEventManager::CEvent* m_pLaunchOICu;
	CEventManager::CEvent* m_pConnect;
	CEventManager::CEvent* m_pSetLotInfo;

public:
	CApp(int argc, char **argv);
	virtual ~CApp();

	bool scan(int argc, char **argv);
 
	// even handlers for evxa response
	void onStateNotificationResponse(int fd);
	void onEvxioResponse(int fd);
	void onErrorResponse(int fd);

	// process the incoming file from the monitored path
	void onReceiveFile(const std::string& name);
	
	// parse the incoming file from the monitored path
	bool parse(const std::string& name);
	bool getFieldValuePair(const std::string& line, const char delimiter, std::string& field, std::string& value);

	// event handler for state notification program change
	virtual void onProgramChange(const EVX_PROGRAM_STATE state, const std::string& msg);

	// event handler for state notification EOT
	virtual void onEndOfTest(const int array_size, int site[], int serial[], int sw_bin[], int hw_bin[], int pass[], EVXA_ULONG dsp_status = 0);

	void onLaunchOICU(CEventManager::CEvent* p = 0);
	void onConnect(CEventManager::CEvent* p = 0);
	void onSetLotInfo(CEventManager::CEvent* p = 0);
};


/* ------------------------------------------------------------------------------------------
file desc class specifically for CApp to handle state notification and evxio
------------------------------------------------------------------------------------------ */
class CAppFileDesc: public CFileDescriptorManager::CFileDescriptor
{
protected:
	CApp& m_App;
	void (CApp::* m_onSelectPtr)(int);

public:
	CAppFileDesc(CApp& app, void (CApp::* p)(int) = 0, int fd = -1): m_App(app)
	{
		m_onSelectPtr = p;
		set(fd);
	}	
	virtual void onSelect()
	{ 
		if (m_onSelectPtr) (m_App.*m_onSelectPtr)(m_fd);	
	}
};



/* ------------------------------------------------------------------------------------------
inherit notify file descriptor class and customize event handlers
------------------------------------------------------------------------------------------ */
class CMonitorFileDesc: public CNotifyFileDescriptor
{
protected:
	CApp& m_App;
public:
	CMonitorFileDesc(CApp& app, const std::string& path, unsigned short mask = IN_MODIFY | IN_CREATE | IN_DELETE | IN_MOVED_TO | IN_MOVED_FROM)
	: CNotifyFileDescriptor(path, mask), m_App(app){}

	virtual	void onFileCreate( const std::string& name ){ m_Log << " onFileCreate" << CUtil::CLog::endl; m_App.onReceiveFile(name); }	
	virtual	void onFileMoveTo( const std::string& name ){ m_Log << " onFileMoveTo" << CUtil::CLog::endl; m_App.onReceiveFile(name); }
	virtual	void onFileModify( const std::string& name ){ m_Log << " onFileModify" << CUtil::CLog::endl; m_App.onReceiveFile(name); }
};




#endif
