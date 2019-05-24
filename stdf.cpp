#include <stdf.h>

MIR::MIR()
{
	clear();
}

MIR::MIR(const MIR& ot)
{
	LotId = ot.LotId;
	CmodCod = ot.CmodCod;
	FlowId = ot.FlowId;
	DsgnRev = ot.DsgnRev;
	DateCod = ot.DateCod;
	OperFrq = ot.OperFrq;
	OperNam = ot.OperNam;
	NodeNam = ot.NodeNam;
	PartTyp = ot.PartTyp;
	EngId = ot.EngId;
	TestTmp = ot.TestTmp;
	FacilId = ot.FacilId;
	FloorId = ot.FloorId;
	StatNum = ot.StatNum;
	ProcId = ot.ProcId;
	ModeCod = ot.ModeCod;
	FamlyId = ot.FamlyId;	
	PkgTyp = ot.PkgTyp;
	SblotId = ot.SblotId;
	JobNam = ot.JobNam;
	SetupId = ot.SetupId;
	JobRev = ot.JobRev;
	ExecTyp = ot.ExecTyp;
	ExecVer = ot.ExecVer;
	AuxFile = ot.AuxFile;
	RtstCod = ot.RtstCod;
	TestCod = ot.TestCod;
	UserText = ot.UserText;
	RomCod = ot.RomCod;
	SerlNum = ot.SerlNum;
	SpecNam = ot.SpecNam;
	TstrTyp = ot.TstrTyp;
	SuprNam = ot.SuprNam;
	SpecVer = ot.SpecVer;
	ProtCod = ot.ProtCod;
}

MIR& MIR::operator=(const MIR& ot)
{
	LotId = ot.LotId;
	CmodCod = ot.CmodCod;
	FlowId = ot.FlowId;
	DsgnRev = ot.DsgnRev;
	DateCod = ot.DateCod;
	OperFrq = ot.OperFrq;
	OperNam = ot.OperNam;
	NodeNam = ot.NodeNam;
	PartTyp = ot.PartTyp;
	EngId = ot.EngId;
	TestTmp = ot.TestTmp;
	FacilId = ot.FacilId;
	FloorId = ot.FloorId;
	StatNum = ot.StatNum;
	ProcId = ot.ProcId;
	ModeCod = ot.ModeCod;
	FamlyId = ot.FamlyId;	
	PkgTyp = ot.PkgTyp;
	SblotId = ot.SblotId;
	JobNam = ot.JobNam;
	SetupId = ot.SetupId;
	JobRev = ot.JobRev;
	ExecTyp = ot.ExecTyp;
	ExecVer = ot.ExecVer;
	AuxFile = ot.AuxFile;
	RtstCod = ot.RtstCod;
	TestCod = ot.TestCod;
	UserText = ot.UserText;
	RomCod = ot.RomCod;
	SerlNum = ot.SerlNum;
	SpecNam = ot.SpecNam;
	TstrTyp = ot.TstrTyp;
	SuprNam = ot.SuprNam;
	SpecVer = ot.SpecVer;
	ProtCod = ot.ProtCod;
	return *this;
}

MIR::~MIR()
{
}

void MIR::clear()
{
	LotId.clear();
	CmodCod.clear();
	DsgnRev.clear();
	DateCod.clear();
	OperFrq.clear();
	OperNam.clear();
	NodeNam.clear();
	PartTyp.clear();
	EngId.clear();
	TestTmp.clear();
	FacilId.clear();
	FloorId.clear();
	StatNum.clear();
	ProcId.clear();
	ModeCod.clear();
	FamlyId.clear();
	PkgTyp.clear();
	SblotId.clear();
	JobNam.clear();
	JobRev.clear();
	ExecTyp.clear();
	ExecVer.clear();
	AuxFile.clear();
	RtstCod.clear();
	TestCod.clear();
	SetupId.clear();
	UserText.clear();
	RomCod.clear();
	SerlNum.clear();
	SpecNam.clear();
	TstrTyp.clear();
	SuprNam.clear();
	SpecVer.clear();
	ProtCod.clear();
}

SDR::SDR() 
{
	clear();
}

SDR::SDR(const SDR& ot)
{
	HandTyp = ot.HandTyp; 
	CardId = ot.CardId;
	LoadId = ot.LoadId;
	HandId = ot.HandId;  
	DibTyp = ot.DibTyp;
	CableId = ot.CableId;
	ContTyp = ot.ContTyp;
	LoadTyp = ot.LoadTyp;
	LaserTyp = ot.LaserTyp;
	LaserId = ot.LaserId;
	ExtrTyp = ot.ExtrTyp;
	ExtrId = ot.ExtrId;
	ContId = ot.ContId;
	DibId = ot.DibId;
	CardTyp = ot.CardTyp;
	CableTyp = ot.CableTyp; 
}

SDR& SDR::operator=(const SDR& ot)
{
	HandTyp = ot.HandTyp; 
	CardId = ot.CardId;
	LoadId = ot.LoadId;
	HandId = ot.HandId;  
	DibTyp = ot.DibTyp;
	CableId = ot.CableId;
	ContTyp = ot.ContTyp;
	LoadTyp = ot.LoadTyp;
	LaserTyp = ot.LaserTyp;
	LaserId = ot.LaserId;
	ExtrTyp = ot.ExtrTyp;
	ExtrId = ot.ExtrId;
	ContId = ot.ContId;
	DibId = ot.DibId;
	CardTyp = ot.CardTyp;
	CableTyp = ot.CableTyp;
	return *this;
}

SDR::~SDR() 
{
}

void SDR::clear()
{
	HandTyp.clear(); 
	CardId.clear();
	LoadId.clear();
	HandId.clear();
	DibTyp.clear();
	CableId.clear();
	ContTyp.clear();
	LoadTyp.clear();
	LaserTyp.clear();
	LaserId.clear();
	ExtrTyp.clear();
	ExtrId.clear();
	ContId.clear();
	DibId.clear();
	CardTyp.clear();
	CableTyp.clear();
}

