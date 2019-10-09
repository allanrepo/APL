#ifndef __STDF__
#define __STDF__

#include <tester.h>
#include <evxa/EVXA.hxx>
#include <utility.h>

/* ------------------------------------------------------------------------------------------
FYI, this class is just an idea for now. 
-	i'm thinking of a generic class that represents an STDF field where it has 
	functions to set value to Unison and also ensures the values are within 
	accepted/valid values
- 	a field also has reference to NewLotConfig.xml so it knows which <entry> it will 
	take the acceptable values
-	a field has reference to specific EVX_LOTINFO_TYPE so it knows which LOTINFO in
	Unison it will set
------------------------------------------------------------------------------------------ */
namespace APLSTDF
{
	class CField
	{
	protected:
		EVX_LOTINFO_TYPE m_evxType;
		std::string m_szNewLotInfoLabel;
		std::list< std::string > m_validValues;

	public:
		CField();
		virtual ~CField() = 0;

		void pushValidValues(const std::string& val);
		bool set(const std::string& val, bool bVerify = false);
		bool sendToUnison();
	};

	class CStdf
	{
	public:
		struct header
		{
			unsigned short len;
			unsigned char typ;
			unsigned char sub;	
		};

	public:
		CStdf(){}
		virtual ~CStdf(){}
	
		bool readMRR(const std::string& file);
	};

	class CRecord
	{
	protected:
		CUtil::CLog m_Log;
	public:
		CRecord(){}
		virtual ~CRecord(){}
		bool readVariableLengthString( std::ifstream& fs, unsigned long nMax,  std::string& out );
		virtual void print() = 0;
	};

	class MRR: public CRecord
	{
	public:
		unsigned int	FINISH_T;
		char 		DISP_COD;
		std::string	USER_DESC;
		std::string	EXC_DESC;

	public:
		MRR(){}
		virtual ~MRR(){}
		virtual bool read( std::ifstream& fs, const unsigned short len );
		virtual void print();		
	};	
};

/* ------------------------------------------------------------------------------------------
MIR class for STDF
------------------------------------------------------------------------------------------ */
class MIR
{
public:
	MIR();
	MIR(const MIR& ot);
	MIR& operator=(const MIR& ot);
	~MIR();

	void clear();

	std::string LotId;	// TestProgData.LotId
	std::string CmodCod;	// TestProgData.CommandMode
	std::string FlowId;	// TestProgData.ActiveFlowName
	std::string DsgnRev;	// TestProgData.DesignRev
	std::string DateCod;	// TestProgData.DateCode
	std::string OperFrq;	// TestProgData.OperFreq
	std::string OperNam;	// TestProgData.Operator
	std::string NodeNam;	// TestProgData.TcName
	std::string PartTyp;	// TestProgData.Device
	std::string EngId;	// TestProgData.EngrLotId
	std::string TstTemp;	// TestProgData.TestTemp
	std::string FacilId;	// TestProgData.TestFacility
	std::string FloorId;	// TestProgData.TestFloor
	std::string StatNum;	// TestProgData.Head (int) 	//// exists in TestProgData class (read only) but cannot be set in setLotInformation()
	std::string ProcId;	// TestProgData.FabId
	std::string ModeCod;	// TestProgData.TestMode
	std::string FamlyId;	// TestProgData.ProductId
	std::string PkgTyp;	// TestProgData.Package
	std::string SblotId;	// TestProgData.SublotId
	std::string JobNam;	// TestProgData.ObjName        //// exists in TestProgData class (read only) but cannot be set in setLotInformation()
	std::string SetupId;	// TestProgData.TestSetup
	std::string JobRev;	// TestProgData.FileNameRev
	std::string ExecTyp;	// TestProgData.SystemName
	std::string ExecVer;	// TestProgData.TargetName
	std::string AuxFile;	// TestProgData.AuxDataFile
	std::string RtstCod;	// TestProgData.LotStatusTest  //// exists in TestProgData class (read only) but cannot be set in setLotInformation()
	std::string TestCod;	// TestProgData.TestPhase
	std::string UserTxt;	// TestProgData.UserText
	std::string RomCod;	// TestProgData.RomCode
	std::string SerlNum;	// TestProgData.TesterSerNum
	std::string SpecNam;	// TestProgData.TestSpecName
	std::string TstrTyp;	// TestProgData.TesterType
	std::string SuprNam;	// TestProgData.Supervisor
	std::string SpecVer;	// TestProgData.TestSpecRev
	std::string ProtCod;	// TestProgData.ProtectionCode
	std::string BurnTim;	// TestProgData.LotBurnInTime
	std::string LotStatus;  // TestProgData.LotStatusTest 

};

/* ------------------------------------------------------------------------------------------
SDR class for STDF
------------------------------------------------------------------------------------------ */
class SDR
{
public:
	SDR();
	SDR(const SDR& ot);
	SDR& operator=(const SDR& ot);
	~SDR();

	void clear();

	std::string HandTyp;   	// TestProgData.HandlerType
	std::string CardId;	// TestProgData.CardId
	std::string LoadId;	// TestProgData.LoadboardId
	std::string HandId;	// TestProgData.PHID
	std::string DibTyp;	// TestProgData.DIBType
	std::string CableId;	// TestProgData.IfCableId
	std::string ContTyp;	// TestProgData.ContactorType
	std::string LoadTyp;	// TestProgData.LoadBrdType
	std::string ContId;	// TestProgData.ContactorId
	std::string LasrTyp;	// TestProgData.LaserType
	std::string LasrId;	// TestProgData.LasterId
	std::string ExtrTyp;	// TestProgData.ExtEquipType
	std::string ExtrId;	// TestProgData.ExtEquipId
	std::string DibId;	// TestProgData.ActiveLoadBrdName
	std::string CardTyp;	// TestProgData.CardType
	std::string CableTyp;	// TestProgData.LotIfCableType
};



#endif
