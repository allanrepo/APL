#include <stdf.h>

/* ------------------------------------------------------------------------------------------
quickly opens STDF file and search for TSR's backwards. store all TSR found in list
------------------------------------------------------------------------------------------ */
/*
bool readTSRs(const std::string& file, std::list< APL::TSR* > TSRs)
{
 
	return true;
}
*/

/* ------------------------------------------------------------------------------------------
quickly opens STDF file and find MIR. reads its contents
------------------------------------------------------------------------------------------ */
bool APL::CSTDF::readMIR(const std::string& file, APL::MIR& mir)
{
	// safely open file
	std::fstream fs;
	if (!CUtil::openFile(file.c_str(), fs, std::fstream::in | std::fstream::binary)) return false;

	// remember file end. we need it to check later
	fs.seekg(0, fs.end);
	std::streampos fend = fs.tellg();
	fs.seekg(0, fs.beg);

	bool bFirst = true;
	bool bRslt = true;
	while(1)
	{		
		//std::streampos curr = fs.tellg();

		// read header
		header hd;
		fs.read((char*) &hd, sizeof(header));	

		// let's check if we hit the EOF while reading. bail if yes
		if (fs.eof())
		{
			bRslt = false;
			break;
		}

		//std::cout << "[" <<  std::hex<< curr << "]: '" << std::dec << hd.len << "'" << std::endl;
		//std::cout << "[" <<  std::hex<< ((int)curr + 2) << "]: '" << std::dec << (int)hd.typ << "'" << std::endl;
		//std::cout << "[" <<  std::hex<< ((int)curr + 3) << "]: '" << std::dec << (int)hd.sub << "'" << std::endl;

		// first record read. we expect it to be FAR. is it?
		if (bFirst)
		{
			if (hd.typ != 0){ bRslt = false; break; }
			if (hd.sub != 10){ bRslt = false; break; }
			bFirst = false;
			fs.seekg(hd.len, fs.cur);
			continue;
		}
		
		// if this is not the first record, it must be ATR or MIR. anything else is flagged as error
		
		// is this ATR?
		if (hd.typ == 0 && hd.sub == 20)
		{
			fs.seekg(hd.len, fs.cur);
			continue;
		}

		// if this is not ATR, it must be MIR. anything else is flagged as error
		if (hd.typ != 1 || hd.sub != 10){ bRslt = false; break; }

		// MIR data size must not go beyond file size
		if ((long)fs.tellg() + hd.len > fend){ bRslt = false; break; }

		// if we reach this point, we found a valid MIR
		mir.read(fs, hd.len);
		break;
	}

	fs.close();
	return bRslt;
}

/* ------------------------------------------------------------------------------------------
get file size of STDF file in bytes by reading it
------------------------------------------------------------------------------------------ */
bool APL::CSTDF::getFileSize(const std::string& file, unsigned long& size)
{
	// safely open file
	std::fstream fs;
	if (!CUtil::openFile(file.c_str(), fs, std::fstream::in | std::fstream::binary)) return false;

	fs.seekg(0, fs.beg);
	std::streampos fsize = fs.tellg();
	fs.seekg(0, fs.end);
	fsize = fs.tellg() - fsize;
	size = fsize;
//	std::cout << "file size: " << fsize << std::endl;	
//	std::cout << "file begin: " << fbeg << std::endl;	
//	std::cout << "file end: " << fend << std::endl;	
	return true;
}

/* ------------------------------------------------------------------------------------------
 
------------------------------------------------------------------------------------------ */
bool APL::CSTDF::readMRR(const std::string& file, APL::MRR& mrr)
{
	// safely open file
	std::fstream fs;
	if (!CUtil::openFile(file.c_str(), fs, std::fstream::in | std::fstream::binary)) return false;

	// before doing anything, let's remember beginning and end file position
	fs.seekg(0, fs.beg);
	std::streampos fbeg = fs.tellg();
	fs.seekg(0, fs.end);
	std::streampos fend = fs.tellg();
	
	// MRR len size range from 4 to 517 so we search between those 
	bool bRslt = false;
	for (unsigned int i = 4; i <= 517; i++)
	{
		// calculate seek shift from end for this current iteration
		int shift = -4 - i;

		// move to file position based on current shift
		fs.seekg(shift, fs.end);

		// if we shift way past file begin, then something is wrong. bail out
		if (fs.tellg() < fbeg)
		{
			m_Log << "error: shifting way past file begin." << CUtil::CLog::endl;
			bRslt = false;
			break;	
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
		if ( (long)fs.tellg() + hd.len > fend) continue;

		mrr.read(fs, hd.len);		
		
		// if we reach this point, we have a winner
		bRslt = true;
		break;
	}

	fs.close();
	return bRslt;
}

/* ------------------------------------------------------------------------------------------
 
------------------------------------------------------------------------------------------ */
bool APL::CRecord::readUnsignedInteger( std::fstream& fs, long len, unsigned int& out, unsigned short& curr )
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

/* ------------------------------------------------------------------------------------------
 
------------------------------------------------------------------------------------------ */
bool APL::CRecord::readChar( std::fstream& fs, long len, char& out, unsigned short& curr )
{
	if (len - curr < 1) return false;

	// check if file is valid
	if (!fs.is_open()) return false;

	fs.read((char*)&out, sizeof(char));
	curr += 1;
	return true;
}

/* ------------------------------------------------------------------------------------------
 
------------------------------------------------------------------------------------------ */
bool APL::CRecord::readUnsignedChar( std::fstream& fs, long len, unsigned char& out, unsigned short& curr )
{
	if (len - curr < 1) return false;

	// check if file is valid
	if (!fs.is_open()) return false;

	fs.read((char*)&out, sizeof(unsigned char));
	curr += 1;
	return true;
}

/* ------------------------------------------------------------------------------------------
 
------------------------------------------------------------------------------------------ */
bool APL::CRecord::readUnsignedShort( std::fstream& fs, long len, unsigned short& out, unsigned short& curr )
{
	if (len - curr < 2) return false;

	// check if file is valid
	if (!fs.is_open()) return false;

	fs.read((char*)&out, sizeof(unsigned short));
	curr += 2;
	return true;
}


/* ------------------------------------------------------------------------------------------
 
------------------------------------------------------------------------------------------ */
bool APL::CRecord::readVariableLengthString( std::fstream& fs, long len,  std::string& out, unsigned short& curr )
{
	long nMax = len - curr;
	long r = 0;
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

bool APL::MRR::read( std::fstream& fs, const unsigned short len )
{
	// check if file is valid
	if (!fs.is_open()) return false;

	// first 4 bytes are required so if len < 4 then something is wrong
	if (len < 4){ return false; }

	// count how many bytes are read so far
	unsigned short nRead = 0;
	
	if (!readUnsignedInteger(fs, len, FINISH_T, nRead)) return false; // get FINISH_T	
	if (!readChar(fs, len, DISP_COD, nRead)) return true; // get DISP_COD	
	if (!readVariableLengthString(fs, len, USER_DESC, nRead)) return true; // get USER_DESC	
	if (!readVariableLengthString(fs, len, EXC_DESC, nRead)) return true; // get EXC_DESC

	return true;
}

void APL::MRR::clear()
{
}

void APL::MRR::print()
{
	time_t t = (unsigned long)FINISH_T;
	tm* plt = gmtime((const time_t*)&t); 

	m_Log << "FINISH_T: '" << FINISH_T << "'" << "("  << (plt->tm_year + 1900) << "/" << (plt->tm_mon + 1) << "/" << (plt->tm_mday) << " " << plt->tm_hour << ":" << plt->tm_min << ":" << plt->tm_sec << ")" << CUtil::CLog::endl;


	m_Log << "DST: " << (plt->tm_isdst? "in effect" : "not in effect" )<< CUtil::CLog::endl;
	m_Log << "DISP_COD: '" << DISP_COD << "'" << CUtil::CLog::endl;
	m_Log << "USER_DESC: \"" << USER_DESC << "\"" << CUtil::CLog::endl;
	m_Log << "EXC_DESC: \"" << EXC_DESC << "\"" << CUtil::CLog::endl;

//	time(&t);
//	plt = localtime
}

void APL::MIR::clear()
{
}

bool APL::MIR::read( std::fstream& fs, const unsigned short len )
{
	// check if file is valid
	if (!fs.is_open()) return false;

	// count how many bytes are read so far
	unsigned short nRead = 0;
	
	if (!readUnsignedInteger(fs, len, SETUP_T, nRead)) return false; // get SETUP_T
	if (!readUnsignedInteger(fs, len, START_T, nRead)) return false; // get START_T
	if (!readUnsignedChar(fs, len, STAT_NUM, nRead)) return false; // get STAT_NUM
	if (!readChar(fs, len, MODE_COD, nRead)) return true;
	if (!readChar(fs, len, RTST_COD, nRead)) return true;
	if (!readChar(fs, len, PROT_COD, nRead)) return true;
	if (!readUnsignedShort(fs, len, BURN_TIM, nRead)) return true;
	if (!readChar(fs, len, CMOD_COD, nRead)) return true;
	if (!readVariableLengthString(fs, len, LOT_ID, nRead)) return true;
	if (!readVariableLengthString(fs, len, PART_TYP, nRead)) return true;
	if (!readVariableLengthString(fs, len, NODE_NAM, nRead)) return true;
	if (!readVariableLengthString(fs, len, TSTR_TYP, nRead)) return true;
	if (!readVariableLengthString(fs, len, JOB_NAM, nRead)) return true;
	if (!readVariableLengthString(fs, len, JOB_REV, nRead)) return true;
	if (!readVariableLengthString(fs, len, SBLOT_ID, nRead)) return true;
	if (!readVariableLengthString(fs, len, OPER_NAM, nRead)) return true;
	if (!readVariableLengthString(fs, len, EXEC_TYP, nRead)) return true;
	if (!readVariableLengthString(fs, len, EXEC_VER, nRead)) return true;
	if (!readVariableLengthString(fs, len, TEST_COD, nRead)) return true;
	if (!readVariableLengthString(fs, len, TST_TEMP, nRead)) return true;
	if (!readVariableLengthString(fs, len, USER_TXT, nRead)) return true;
	if (!readVariableLengthString(fs, len, AUX_FILE, nRead)) return true;
	if (!readVariableLengthString(fs, len, PKG_TYP, nRead)) return true;
	if (!readVariableLengthString(fs, len, FAMLY_ID, nRead)) return true;
	if (!readVariableLengthString(fs, len, DATE_COD, nRead)) return true;
	if (!readVariableLengthString(fs, len, FACIL_ID, nRead)) return true;
	if (!readVariableLengthString(fs, len, FLOOR_ID, nRead)) return true;
	if (!readVariableLengthString(fs, len, PROC_ID, nRead)) return true;
	if (!readVariableLengthString(fs, len, OPER_FRQ, nRead)) return true;
	if (!readVariableLengthString(fs, len, SPEC_NAM, nRead)) return true;
	if (!readVariableLengthString(fs, len, SPEC_VER, nRead)) return true;
	if (!readVariableLengthString(fs, len, FLOW_ID, nRead)) return true;
	if (!readVariableLengthString(fs, len, SETUP_ID, nRead)) return true;
	if (!readVariableLengthString(fs, len, DSGN_REV, nRead)) return true;
	if (!readVariableLengthString(fs, len, ENG_ID, nRead)) return true;
	if (!readVariableLengthString(fs, len, ROM_COD, nRead)) return true;
	if (!readVariableLengthString(fs, len, SERL_NUM, nRead)) return true;
	if (!readVariableLengthString(fs, len, SUPR_NAM, nRead)) return true;

	return true;
}

void APL::MIR::print()
{
	m_Log << "SETUP_T: '" << SETUP_T << "'" << CUtil::CLog::endl;
	m_Log << "START_T: '" << START_T << "'" << CUtil::CLog::endl;
	m_Log << "STAT_NUM: '" << (int)STAT_NUM << "'" << CUtil::CLog::endl;
	m_Log << "MODE_COD: '" << (char)MODE_COD << "'" << CUtil::CLog::endl;
	m_Log << "RTST_COD: '" << (char)RTST_COD << "'" << CUtil::CLog::endl;
	m_Log << "PROT_COD: '" << (char)PROT_COD << "'" << CUtil::CLog::endl;
	m_Log << "BURN_TIM: '" << (char)BURN_TIM << "'" << CUtil::CLog::endl;
	m_Log << "CMOD_COD: '" << (char)CMOD_COD << "'" << CUtil::CLog::endl;
	m_Log << "LOT_ID: \"" << LOT_ID << "\"" << CUtil::CLog::endl;
	m_Log << "PART_TYP: \"" << PART_TYP << "\"" << CUtil::CLog::endl;
	m_Log << "NODE_NAM: \"" << NODE_NAM << "\"" << CUtil::CLog::endl;
	m_Log << "TSTR_TYP: \"" << TSTR_TYP << "\"" << CUtil::CLog::endl;
	m_Log << "JOB_NAM: \"" << JOB_NAM << "\"" << CUtil::CLog::endl;
	m_Log << "JOB_REV: \"" << JOB_REV << "\"" << CUtil::CLog::endl;
	m_Log << "SBLOT_ID: \"" << SBLOT_ID << "\"" << CUtil::CLog::endl;
	m_Log << "OPER_NAM: \"" << OPER_NAM << "\"" << CUtil::CLog::endl;
	m_Log << "EXEC_TYP: \"" << EXEC_TYP << "\"" << CUtil::CLog::endl;
	m_Log << "EXEC_VER: \"" << EXEC_VER << "\"" << CUtil::CLog::endl;
	m_Log << "TEST_COD: \"" << TEST_COD << "\"" << CUtil::CLog::endl;
	m_Log << "TST_TEMP: \"" << TST_TEMP << "\"" << CUtil::CLog::endl;
	m_Log << "USER_TXT: \"" << USER_TXT << "\"" << CUtil::CLog::endl;
	m_Log << "AUX_FILE: \"" << AUX_FILE << "\"" << CUtil::CLog::endl;
	m_Log << "PKG_TYP: \"" << PKG_TYP << "\"" << CUtil::CLog::endl;
	m_Log << "FAMLY_ID: \"" << FAMLY_ID << "\"" << CUtil::CLog::endl;
	m_Log << "DATE_COD: \"" << DATE_COD << "\"" << CUtil::CLog::endl;
	m_Log << "FACIL_ID: \"" << FACIL_ID << "\"" << CUtil::CLog::endl;
	m_Log << "FLOOR_ID: \"" << FLOOR_ID << "\"" << CUtil::CLog::endl;
	m_Log << "PROC_ID: \"" << PROC_ID << "\"" << CUtil::CLog::endl;
	m_Log << "OPER_FRQ: \"" << OPER_FRQ << "\"" << CUtil::CLog::endl;
	m_Log << "SPEC_NAM: \"" << SPEC_NAM << "\"" << CUtil::CLog::endl;
	m_Log << "SPEC_VER: \"" << SPEC_VER << "\"" << CUtil::CLog::endl;
	m_Log << "FLOW_ID: \"" << FLOW_ID << "\"" << CUtil::CLog::endl;
	m_Log << "SETUP_ID: \"" << SETUP_ID << "\"" << CUtil::CLog::endl;
	m_Log << "DSGN_REV: \"" << DSGN_REV << "\"" << CUtil::CLog::endl;
	m_Log << "ENG_ID: \"" << ENG_ID << "\"" << CUtil::CLog::endl;
	m_Log << "ROM_COD: \"" << ROM_COD << "\"" << CUtil::CLog::endl;
	m_Log << "SERL_NUM: \"" << SERL_NUM << "\"" << CUtil::CLog::endl;
	m_Log << "SUPR_NAM: \"" << SUPR_NAM << "\"" << CUtil::CLog::endl;
}





APL::CField::CField()
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



