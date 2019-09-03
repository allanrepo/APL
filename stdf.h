#ifndef __STDF__
#define __STDF__

#include <tester.h>
#include <evxa/EVXA.hxx>

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
	std::string TestTmp;	// TestProgData.TestTemp
	std::string FacilId;	// TestProgData.TestFacility
	std::string FloorId;	// TestProgData.TestFloor
	std::string StatNum;	// TestProgData.Head (int)	
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
	std::string UserText;	// TestProgData.UserText
	std::string RomCod;	// TestProgData.RomCode
	std::string SerlNum;	// TestProgData.TesterSerNum
	std::string SpecNam;	// TestProgData.TestSpecName
	std::string TstrTyp;	// TestProgData.TesterType
	std::string SuprNam;	// TestProgData.Supervisor
	std::string SpecVer;	// TestProgData.TestSpecRev
	std::string ProtCod;	// TestProgData.ProtectionCode
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
	std::string LaserTyp;	// TestProgData.LaserType
	std::string LaserId;	// TestProgData.LasterId
	std::string ExtrTyp;	// TestProgData.ExtEquipType
	std::string ExtrId;	// TestProgData.ExtEquipId
	std::string DibId;	// TestProgData.ActiveLoadBrdName
	std::string CardTyp;	// TestProgData.CardType
	std::string CableTyp;	// TestProgData.LotIfCableType
};



#endif
