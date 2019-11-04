#ifndef __APP__
#define __APP__

#include <utility.h>
#include <state.h>
#include <fd.h>
#include <notify.h>
#include <pwd.h>
#include <sys/time.h>
#include <tester.h>
#include <xml.h>
#include <stdf.h>
#include <socket.h>
#include <list>

/* ------------------------------------------------------------------------------------------
constants
------------------------------------------------------------------------------------------ */
#define DELIMITER ':'
#define VERSION "beta.2.5.20191030"
#define DEVELOPER "allan asis / allan.asis@gmail.com"
#define MAXCONNECT 20
#define KILLAPPCMD "kill.app.sh"
#define KILLTESTERCMD "kill.tester.sh"
#define POPUPSERVER ".widget"


/* ------------------------------------------------------------------------------------------
class declarations
------------------------------------------------------------------------------------------ */
class CApp;
class CAppTask;
class CAppState;
class CMonitorFileDesc;
class CAppFileDesc;
class CMap;
class CAppClientUDPFileDesc;

class CApp: public CTester
{
protected:
	class CFieldValuePair
	{
	protected:
		struct FIELDVALUE
		{
			std::string field;
			std::string value;
		};

		std::vector< FIELDVALUE > m_fvs;

	public:
		CFieldValuePair(){}
		virtual ~CFieldValuePair(){ m_fvs.clear(); }
		void clear(){ m_fvs.clear(); }

		void add(const std::string& field, const std::string& value)
		{
			FIELDVALUE fv;
			fv.field = field;
			fv.value = value;
			m_fvs.push_back(fv);	
		}

		const std::string getValue(const std::string& field)
		{
			for(unsigned int i = 0; i < m_fvs.size(); i++)
			{
				if (m_fvs[i].field.compare(field) == 0) return m_fvs[i].value;
			}
			return std::string();
		}

		const std::string getField(const std::string& value)
		{
			for(unsigned int i = 0; i < m_fvs.size(); i++)
			{
				if (m_fvs[i].value.compare(value) == 0) return m_fvs[i].field;
			}
			return std::string();
		}

		bool setValue(const std::string& field, const std::string& value, bool bAdd = false)
		{
			for(unsigned int i = 0; i < m_fvs.size(); i++)
			{
				if (m_fvs[i].field.compare(field) == 0)
				{
					m_fvs[i].value = value;
					return true;
				}
			}
			if (bAdd)
			{
				add(field, value);
				return true;
			}
			return false;
		}
	};

	struct CONFIG
	{
		struct STEP
		{
			std::string szStep;
			std::string szFlowId;
			std::string szCmodCod;
			int nRtstCod;
		};

		enum APL_TEST_TYPE
		{
			APL_WAFER,
			APL_FINAL,
			APL_MANUAL
		};

		// STDF field labels 
		CFieldValuePair mir;
		CFieldValuePair sdr;
		CFieldValuePair fields;

		// resources
		CUtil::CLog m_Log;

		// step parameters
		bool bStep;
		std::vector< STEP > steps;

		// launch parameters
		bool 		bProd;
		bool		bKillTesterOnLaunch;
		int		nRelaunchTimeOutMS;
		int		nRelaunchAttempt;
		int		nEndLotTimeOutMS;
		int		nUnloadProgTimeOutMS;
		int		nKillTesterTimeOutMS;
		int		nFAModuleTimeOutMS;
		bool		bForceLoad;

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
		std::string 	szSTDFPath;	
		bool		bZipSTDF;
		std::string	szZipSTDFCmd;
		std::string	szZipSTDFExt;
		bool		bRenameSTDF;
		std::string	szRenameSTDFFormat;
		bool 		bTimeStampIsEndLot;

		// FAModule parameters
		bool		bFAModule;

		// lotinfo file parameters
		std::string 	szLotInfoFileName;
		std::string 	szLotInfoFilePath;
		bool		bDeleteLotInfo;
		bool 		bSendInfo;

		// summary appending feature
		bool 		bSummary;
		std::string	szSummaryPath;

		// GUI interface server parameters
		bool		bPopupServer;
		std::string 	szServerIP;
		int		nServerPort;

		// NewLotConfig.xml parameters
		std::string	szNewLotConfigFile;
		std::string	szNewLotConfigPath;

		// customer/factory parameters
		std::string 	szTestSite;
		std::string	szSupplier;

		// clearing CONFIG doesn't empty parameters, it just sets them to default
		void clear()
		{
			bProd = true;
			bForceLoad = false;
			bKillTesterOnLaunch = false;
			nRelaunchTimeOutMS = 120000;
			nRelaunchAttempt = 3;
			nEndLotTimeOutMS = 30000;
			nUnloadProgTimeOutMS = 30000;
			nKillTesterTimeOutMS = 30000;
			nFAModuleTimeOutMS = 30000;
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
			bSummary = false;
			szSummaryPath = "/tmp";
			bStep = false;
			bDeleteLotInfo = false;
			steps.clear();
			bPopupServer = true;
			szServerIP = "127.0.0.1";
			nServerPort = 54000;
			szNewLotConfigFile = "NewLotUnison";
			szNewLotConfigPath = "$LTX_UPROD_PATH/newlot-config";
			szSTDFPath = "/tmp";
			szTestSite = "cohu";
			szSupplier = "cohu";
			bZipSTDF = false;
			szZipSTDFCmd = "/usr/bin/gzip";
			szZipSTDFExt = ".gz";
			bTimeStampIsEndLot = true; 
			bRenameSTDF = false;
			szRenameSTDFFormat = "QUALCOMM";		
			bFAModule = false;
			mir.clear();
			sdr.clear();
			fields.clear();
			
			// add and set default field/value for MIR fields						
			mir.add("SETUP_T", "SETUP_T");
			mir.add("START_T", "START_T");
			mir.add("STAT_NUM", "STAT_NUM");
			mir.add("MODE_COD", "TESTMODE");
			mir.add("RTST_COD", "LOTSTATE");
			mir.add("PROT_COD", "PROTECTIONCODE");
			mir.add("BURN_TIM", "BURN_TIM");
			mir.add("CMOD_COD", "COMMANDMODE");
			mir.add("LOT_ID", "LOTID");
			mir.add("PART_TYP", "DEVICE");
			mir.add("NODE_NAM", "NODE_NAM");
			mir.add("TSTR_TYP", "TESTERTYPE");
			mir.add("JOB_NAM", "JOB_NAM");
			mir.add("JOB_REV", "FILENAMEREV");
			mir.add("SBLOT_ID", "SUBLOTID");
			mir.add("OPER_NAM", "OPERATOR");
			mir.add("EXEC_TYP", "SYSTEMNAME");
			mir.add("EXEC_VER", "TARGETNAME");
			mir.add("TEST_COD", "TESTPHASE");
			mir.add("TST_TEMP", "TEMPERATURE");
			mir.add("USER_TXT", "USERTEXT");
			mir.add("AUX_FILE", "AUXDATAFILE");
			mir.add("PKG_TYP", "PACKAGE");
			mir.add("FAMLY_ID", "PRODUCTID");
			mir.add("DATE_COD", "DATECODE");
			mir.add("FACIL_ID", "TESTFACILITY");
			mir.add("FLOOR_ID", "TESTFLOOR");
			mir.add("PROC_ID", "FABRICATIONID");
			mir.add("OPER_FRQ", "OPERFREQ");
			mir.add("SPEC_NAM", "TESTSPECNAME");
			mir.add("SPEC_VER", "TESTSPECREV");
			mir.add("FLOW_ID", "ACTIVEFLOWNAME");
			mir.add("SETUP_ID", "TESTSETUP");
			mir.add("DSGN_REV", "DESIGNREV");
			mir.add("ENG_ID", "ENGRLOTID");
			mir.add("ROM_COD", "ROMCODE");
			mir.add("SERL_NUM", "TESTERSERNUM");
			mir.add("SUPR_NAM", "SUPERVISOR");	

			// add and set default field/value for SDR fields						
			sdr.add("HAND_TYP", "PROBERHANDLERTYPE");
			sdr.add("CARD_ID", "CARDID");
			sdr.add("CARD_TYP", "CARDTYPE");
			sdr.add("LOAD_ID", "BOARDID");
			sdr.add("HAND_ID", "PROBERHANDLERID");
			sdr.add("DIB_TYP", "DIBTYPE");
			sdr.add("CABL_ID", "CABLEID");	
			sdr.add("CONT_TYP", "CONTACTORTYPE");	
			sdr.add("LOAD_TYP", "BOARDTYPE");	
			sdr.add("CONT_ID", "CONTACTORID");	
			sdr.add("LASR_TYP", "LASERTYPE");	
			sdr.add("LASR_ID", "LASERID");	
			sdr.add("EXTR_TYP", "EXTRAEQUIPMENTTYPE");	
			sdr.add("EXTR_ID", "EXTRAEQUIPMENTID");	
			sdr.add("DIB_ID", "DIBID");		
			sdr.add("CABL_TYP", "CABLETYPE");

			// add the generic field/value pairs
			fields.add("JOBFILE", "JOBFILE");
			fields.add("CUSTOMER", "CUSTOMER");
			fields.add("DEVICENICKNAME", "DEVICENICKNAME");
		}


		// constructor
		CONFIG()
		{
			clear();
		}

		// too lazy to code this, so just prevent copying...
		CONFIG(const CONFIG& p){}		
		CONFIG& operator=(const CONFIG& p){}

		// handle config file parsing
		bool parse(const std::string& file);		
		void print();
	};

	struct LOTINFO
	{
		std::string szProgramFullPathName;
		std::string szStep;
		std::string szLotId;
		std::string szDevice;
		std::string szCustomer;
		std::string szDeviceNickName;

		MIR mir;
		SDR sdr;		
		LOTINFO()
		{
			clear();
		}
		
		LOTINFO(const LOTINFO& p)
		{
			szProgramFullPathName = p.szProgramFullPathName;
			szStep = p.szStep;
			szLotId = p.szLotId;
			szDevice = p.szDevice;
			mir = p.mir;
			sdr = p.sdr;
			szCustomer = p.szCustomer;
			szDeviceNickName = p.szDeviceNickName;			
		}
		
		LOTINFO& operator=(const LOTINFO& p)
		{
			szProgramFullPathName = p.szProgramFullPathName;
			szStep = p.szStep;
			szLotId = p.szLotId;
			szDevice = p.szDevice;
			mir = p.mir;
			sdr = p.sdr;
			szCustomer = p.szCustomer;
			szDeviceNickName = p.szDeviceNickName;			
		}

		void clear()
		{
			szProgramFullPathName = "";
			szStep = "";
			szLotId = "";
			szDevice = "";
			mir.clear();
			sdr.clear();
			szCustomer = "";
			szDeviceNickName = "";			
		}
	};	

	class CMap
	{
	private:
		struct COORD
		{
			int X;
			std::list< int > Y;

			COORD(int x): X(x){}
			virtual ~COORD()
			{
				Y.clear();
			}

			bool add(int y)
			{
				if (has(y)) return false;
				else Y.push_back(y);
			}

			bool has(int y)
			{
				for (std::list< int >::iterator it = Y.begin(); it != Y.end(); it++)
				{
					if (y == *it) return true;
				}
				return false;
			}

		};
		
		std::list< COORD* > m_xy;
	public:
		CMap(){}
		virtual ~CMap(){ clear(); }	

		void clear()
		{
			for (std::list< COORD* >::iterator it = m_xy.begin(); it != m_xy.end(); it++)
			{
				if (*it) delete (*it);
			}
			m_xy.clear();
		}	

		bool add( int x, int y )
		{
			for (std::list<COORD* >::iterator it = m_xy.begin(); it != m_xy.end(); it++)
			{
				if (x == (*it)->X)
				{ 
					if ((*it)->has(y)) return false; 
					else
					{
						(*it)->add(y);
						return true;
					}
				}
			}
			COORD* xy = new COORD(x);
			xy->add(y);
			m_xy.push_back(xy);
			return true;
		}

		bool has( int x, int y )
		{
			for (std::list<COORD* >::iterator it = m_xy.begin(); it != m_xy.end(); it++)
			{
				if (x == (*it)->X){ if ((*it)->has(y)) return true; }
			}
			return false;
		}
	};

protected:

	// lotinfo data
	LOTINFO m_lotinfo;
	
	// resources
	CUtil::CLog m_Log;

	// count how many launch attempts we made
	long m_nLaunchAttempt;

	CMap m_map;

	// parameters
	CONFIG m_CONFIG;
	std::string m_szConfigFullPathName;
	std::string m_szTesterName;
	std::string m_szName;
	bool m_bDebug;

	// file descriptor engine
	CFileDescriptorManager m_FileDescMgr;
	CMonitorFileDesc* m_pMonitorFileDesc; 
	CMonitorFileDesc* m_pSummaryFileDesc; 
	CFileDescriptorManager::CFileDescriptor* m_pStateNotificationFileDesc;
	CClientFileDescriptorUDP* m_pClientFileDesc;
	CMonitorFileDesc* m_pSTDFFileDesc; 

	// state machine
	CStateManager m_StateMgr;

	// state objects
	CState* m_pStateOnInit;
	CState* m_pStateOnIdle;
	CState* m_pStateOnEndLot;
	CState* m_pStateOnUnloadProg;
	CState* m_pStateOnKillTester;
	CState* m_pStateOnLaunch;
	CState* m_pSendLotInfo;

	// state functions (load)
	void onInitLoadState(CState&);
	void onIdleLoadState(CState&);
	void onEndLotLoadState(CState&);
	void onKillTesterLoadState(CState&);
	void onUnloadProgLoadState(CState&);
	void onLaunchLoadState(CState&);
	void onSetLotInfoLoadState(CState&);

	// state functions (unload)
	void onIdleUnloadState(CState&);
	void onEndLotUnloadState(CState&);
	void onUnloadProgUnloadState(CState&);
	void onLaunchUnloadState(CState&);

	// tasks/events
	void init(CTask&);
	void setLogFile(CTask&);
	void connect(CTask&);
	void select(CTask&);
	void listen(CTask&);
	void endLot(CTask&);
	void timeOutEndLot(CTask&);
	void unloadProg(CTask&);
	void timeOutUnloadProg(CTask&);
	void killTester(CTask&);
	void isTesterDead(CTask&);
	void launch(CTask&);
	void timeOutLaunch(CTask&);
	void timeOutKillTester(CTask&);
	void sendLotInfoToFAModule(CTask&);
	void timeOutSendLotInfoToFAModule(CTask&);
	void setSTDF(CTask&);
	void updateConfig(CTask&);

	// functions executed by file descriptor handlers
	void onReceiveFile(const std::string& name);	
	void onStateNotificationResponse(int fd);
	void onSummaryFile(const std::string& name);
	void onReceiveFromServer(const std::string& msg);
	void onWatchSTDF(const std::string& name);
	
	
	// utility functions. purpose are obvious in their function name and arguments
	bool scan(int argc, char **argv);
	const std::string getUserName() const;
	bool setLotInformation(const EVX_LOTINFO_TYPE type, const std::string& field, const std::string& label, bool bForce = false);
	bool setLotInfo(); 

	// functions to parse lotinfo.txt file 
	void processLotInfoFile(const std::string& name);	
	bool parse(const std::string& name);
	bool getFieldValuePair(const std::string& line, const char delimiter, std::string& field, std::string& value);
	bool updateLotinfoFile(const std::string& name);

public:
	CApp(int argc, char **argv);
	virtual ~CApp();

	void setState(CState& state){ m_StateMgr.set(&state); }

	// event handler for state notification
	virtual void onLotChange(const EVX_LOT_STATE state, const std::string& szLotId);
	virtual void onWaferChange(const EVX_WAFER_STATE state, const std::string& szWaferId);
	virtual void onProgramChange(const EVX_PROGRAM_STATE state, const std::string& msg);
	virtual void onEndOfTest(const int array_size, int site[], int serial[], int sw_bin[], int hw_bin[], int pass[], EVXA_ULONG dsp_status = 0);
};

/* ------------------------------------------------------------------------------------------
derived state class for CApp; built-in load and unload handlers for state object
------------------------------------------------------------------------------------------ */
class CAppState: public CState
{
protected:
	CApp& m_App;
	void (CApp::* m_pLoad)(CState&);
	void (CApp::* m_pUnload)(CState&);
	
public:
	CAppState(CApp& app, const std::string& name = "", void (CApp::* load)(CState&) = 0, void (CApp::* unload)(CState&) = 0)
	:CState(name), m_App(app)
	{
		m_pLoad = load;
		m_pUnload = unload;
	}
	virtual ~CAppState(){}

	virtual void load()
	{ 
		m_Log << "loading state: " << getName() << CUtil::CLog::endl;
		if (m_pLoad) (m_App.*m_pLoad)(*this); 
	}
	virtual void unload()
	{
		m_Log << "unloading state: " << getName() << CUtil::CLog::endl; 
		if (m_pUnload) (m_App.*m_pUnload)(*this); 
		CState::unload(); 
	}
};

/* ------------------------------------------------------------------------------------------
derived task class for CApp; executes CApp's function loaded to it
------------------------------------------------------------------------------------------ */
class CAppTask: public CTask
{
protected:
	CApp& m_App;
	void (CApp::* m_pRun)(CTask&);

public:
	CAppTask(CApp& app, void (CApp::* p)(CTask&) = 0, const std::string& name = "", long nDelayMS = 0, bool bEnable = true, bool bLoop = true)
	:CTask(name, nDelayMS, bEnable, bLoop), m_App(app)
	{
		m_pRun = p;		
	}
	
	virtual void run(){ if (m_pRun) (m_App.*m_pRun)(*this); }
};

/* ------------------------------------------------------------------------------------------
derived task class used for quick switching states
------------------------------------------------------------------------------------------ */
class CSwitchTask: public CTask
{
protected:
	CApp& m_App;
	CState& m_state;
public:
	CSwitchTask(CApp& app, CState& state, long nDelayMS = 0, bool bEnable = true)
	:CTask(state.getName(), nDelayMS, bEnable, false), m_App(app), m_state(state){}

	virtual void run(){ m_App.setState(m_state); }
	
};


/* ------------------------------------------------------------------------------------------
inherit notify file descriptor class and customize event handlers
------------------------------------------------------------------------------------------ */
class CMonitorFileDesc: public CNotifyFileDescriptor
{
protected:
	CApp& m_App;
	void (CApp::* m_pOnReceiveFile)(const std::string& name);

public:
	CMonitorFileDesc(CApp& app, void (CApp::* p)(const std::string&), const std::string& path, unsigned short mask = IN_MODIFY | IN_CREATE | IN_DELETE | IN_MOVED_TO | IN_MOVED_FROM)
	: CNotifyFileDescriptor(path, mask), m_App(app){ m_pOnReceiveFile = p; }

	virtual	void onFileCreate( const std::string& name ){ if (m_pOnReceiveFile) (m_App.*m_pOnReceiveFile)(name); }	
	virtual	void onFileMoveTo( const std::string& name ){ if (m_pOnReceiveFile) (m_App.*m_pOnReceiveFile)(name); }
	virtual	void onFileModify( const std::string& name ){ if (m_pOnReceiveFile) (m_App.*m_pOnReceiveFile)(name); }
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

------------------------------------------------------------------------------------------ */
class CAppClientUDPFileDesc: public CClientFileDescriptorUDP
{
protected:
	CApp& m_App;
	void (CApp::* m_onRecvPtr)(const std::string&);
public:
	CAppClientUDPFileDesc(CApp& app, void (CApp::* p)(const std::string&) = 0)
	: m_App(app)
	{
		m_onRecvPtr = p;
	}

	virtual void onRecv(const std::string& msg)
	{ 
		if (m_onRecvPtr) (m_App.*m_onRecvPtr)(msg);	
	}
};


#endif
