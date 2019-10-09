#include <stdf.h>

bool APLSTDF::CStdf::readMRR(const std::string& file, APLSTDF::MRR& mrr)
{
	// open file
	std::ifstream fs;
	fs.open(file.c_str(), std::fstream::in | std::fstream::binary);
/*
	// get file size
	std::streampos fsize = fs.tellg();
	fs.seekg(0, fs.end);
	std::streampos fend = fs.tellg();
	fsize = fs.tellg() - fsize;
	fs.seekg(0, fs.beg);
	std::streampos fbeg = fs.tellg();
	std::cout << "file size: " << fsize << std::endl;	
	std::cout << "file begin: " << fbeg << std::endl;	
	std::cout << "file end: " << fend << std::endl;	
*/
	// before doing anything, let's remember beginning file position
	fs.seekg(0, fs.beg);
	std::streampos fbeg = fs.tellg();

	// also remember file end
	fs.seekg(0, fs.end);
	std::streampos fend = fs.tellg();
	
	// MRR len size range from 5 to 517 so we search between those 
	bool bFound = false;
	for (unsigned int i = 4; i <= 517; i++)
	{
		// calculate seek shift from end for this current iteration
		int shift = -4 - i;
//		std::cout << "Curr Shift: " << shift << std::endl;

		// move to file position based on current shift
		fs.seekg(shift, fs.end);

		// remember current position shift
//		std::streampos curr = fs.tellg();

		// if we shift way past file begin, then something is wrong. bail out
		if (fs.tellg() < fbeg)
		{
			m_Log << "error: shifting way past file begin." << CUtil::CLog::endl;
			return false;
		}

		// read header
		header hd;
		fs.read((char*) &hd, sizeof(header));	

		// is this MRR?
		if (hd.typ != 1) continue;
		if (hd.sub != 20) continue;

		// MRR data size must not be less than 4 bytes
		if (hd.len < 4) continue;

		// MRR data size must not go beyond file size
		if ( (int)fs.tellg() + hd.len > fend) continue;
/*
		std::cout << "[" <<  std::hex<< curr << "]: '" << std::dec << hd.len << "'" << std::endl;
		std::cout << "[" <<  std::hex<< ((int)curr + 2) << "]: '" << std::dec << (int)hd.typ << "'" << std::endl;
		std::cout << "[" <<  std::hex<< ((int)curr + 3) << "]: '" << std::dec << (int)hd.sub << "'" << std::endl;
*/
//		MRR mrr;
		mrr.read(fs, hd.len);		
//		mrr.print();
		return 


		// if we reach this point, we have a winner
		bFound = true;
		break;
	}
	if (!bFound)
	{
		std::cout << "error: did not find MRR" << std::endl;
	}

	// close
	fs.close();

}

bool APLSTDF::CRecord::readUnsignedInteger( std::ifstream& fs, unsigned len, unsigned int& out, unsigned short& curr )
{
	// len is the size of MIR content
	// curr is the current byte position in the MIR content we intend to read U*4
	if (len - curr < 4) return false;

	// check if file is valid
	if (!fs.is_open()) return false;

	fs.read((char*)&out, sizeof(unsigned int));
	curr += 4;
	return true;
}

bool APLSTDF::CRecord::readVariableLengthString( std::ifstream& fs, unsigned long nMax,  std::string& out, unsigned short& curr )
{
	unsigned long r = 0;
	out.clear();
	if (r >= nMax) return false;

	// check if file is valid
	if (!fs.is_open()) return false;

	// read string size
	char n = 0;
	fs.read(&n, 1);
	r++;
	if (n >  0)
	{
		if (r + n > nMax) return false;

		unsigned char* p = new unsigned char[n+1]; // +1 for /0 (end-string char)
		fs.get((char*)p, n+1);
		std::stringstream ss; 
		ss << p;
		delete[] p;
		out = ss.str();
		r += n;
	}
	curr += (out.size() + 1);
	return true;
}

bool APLSTDF::MRR::read( std::ifstream& fs, const unsigned short len )
{
	// check if file is valid
	if (!fs.is_open()) return false;

	// first 4 bytes are required so if len < 4 then something is wrong
	if (len < 4){ return false; }

	// count how many bytes are read so far
	unsigned short nRead = 0;

	// get FINISH_T
	if (!readUnsignedInteger(fs, len, FINISH_T, nRead)) return false;
//	fs.read((char*)&FINISH_T, sizeof(unsigned int));
//	nRead = 4;

	// get DISP_COD
	if (len < nRead + 1) return true;
	fs.read((char*)&DISP_COD, sizeof(char));
	nRead += 1;

	// get USER_DESC
	if (!readVariableLengthString(fs, len - nRead, USER_DESC, nRead)) return true;
//	nRead += (USER_DESC.size() + 1);

	// get EXC_DESC
	if (!readVariableLengthString(fs, len - nRead, EXC_DESC, nRead)) return true;
//	nRead += (EXC_DESC.size() + 1);

	return true;
}

void APLSTDF::MRR::clear()
{
}

void APLSTDF::MRR::print()
{
	time_t t = (unsigned long)FINISH_T;
	tm* plt = gmtime((const time_t*)&t); 

	m_Log << "FINISH_T: '" << FINISH_T << "'" << "("  << (plt->tm_year + 1900) << "/" << (plt->tm_mon + 1) << "/" << (plt->tm_mday) << " " << plt->tm_hour << ":" << plt->tm_min << ":" << plt->tm_sec << ")" << CUtil::CLog::endl;


	m_Log << "DST: " << (plt->tm_isdst? "in effect" : "not in effect" )<< CUtil::CLog::endl;

// plt->monasctime(plt) << ")" <<CUtil::CLog::endl;
	m_Log << "DISP_COD: '" << DISP_COD << "'" << CUtil::CLog::endl;
	m_Log << "USER_DESC: \"" << USER_DESC << "\"" << CUtil::CLog::endl;
	m_Log << "EXC_DESC: \"" << EXC_DESC << "\"" << CUtil::CLog::endl;

//	time(&t);
//	plt = localtime
}

void APLSTDF::MIR::clear()
{
}

bool APLSTDF::MIR::read( std::ifstream& fs, const unsigned short len )
{
	// check if file is valid
	if (!fs.is_open()) return false;

	// first 9 bytes are required so if len < 9 then something is wrong
	if (len < 9){ return false; }

	// count how many bytes are read so far
	unsigned short nRead = 0;

	// get SETUP_T
	if (len < nRead + 4) return true;
	fs.read((char*)& SETUP_T, sizeof(unsigned int));
	nRead += 4;

	// get 
	if (len < nRead + 4) return true;
	fs.read((char*)& SETUP_T, sizeof(unsigned int));
	nRead += 4;

	return true;
}

void APLSTDF::MIR::print()
{
}





APLSTDF::CField::CField()
{
}

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
	TstTemp = ot.TstTemp;
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
	UserTxt = ot.UserTxt;
	RomCod = ot.RomCod;
	SerlNum = ot.SerlNum;
	SpecNam = ot.SpecNam;
	TstrTyp = ot.TstrTyp;
	SuprNam = ot.SuprNam;
	SpecVer = ot.SpecVer;
	ProtCod = ot.ProtCod;
	BurnTim = ot.BurnTim;
	LotStatus = ot.LotStatus;
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
	TstTemp = ot.TstTemp;
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
	UserTxt = ot.UserTxt;
	RomCod = ot.RomCod;
	SerlNum = ot.SerlNum;
	SpecNam = ot.SpecNam;
	TstrTyp = ot.TstrTyp;
	SuprNam = ot.SuprNam;
	SpecVer = ot.SpecVer;
	ProtCod = ot.ProtCod;
	BurnTim = ot.BurnTim;
	LotStatus = ot.LotStatus;
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
	TstTemp.clear();
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
	UserTxt.clear();
	RomCod.clear();
	SerlNum.clear();
	SpecNam.clear();
	TstrTyp.clear();
	SuprNam.clear();
	SpecVer.clear();
	ProtCod.clear();
	BurnTim.clear();
	LotStatus.clear();
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
	LasrTyp = ot.LasrTyp;
	LasrId = ot.LasrId;
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
	LasrTyp = ot.LasrTyp;
	LasrId = ot.LasrId;
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
	LasrTyp.clear();
	LasrId.clear();
	ExtrTyp.clear();
	ExtrId.clear();
	ContId.clear();
	DibId.clear();
	CardTyp.clear();
	CableTyp.clear();
}



