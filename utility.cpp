#include <utility.h>


bool CUtil::CLog::enable = true;
bool CUtil::CLog::immediate = true;
bool CUtil::CLog::silent = false;
std::string CUtil::CLog::m_file = "";

/*-----------------------------------------------------------------------------------------
convert a string containing a number into a long type variable
-----------------------------------------------------------------------------------------*/
long CUtil::toLong(const std::string& n)
{
	std::stringstream a(n);
	long val;
	a >> val;    
	return val;
} 

/*-----------------------------------------------------------------------------------------
check if string is a number
-----------------------------------------------------------------------------------------*/
bool CUtil::isNumber(const std::string& n)
{
	bool bDigit = false;
	bool bSign = false; 
	bool bDecimal = false;
	
	for (std::string::size_type i = 0; i < n.size(); i++)
	{
		switch( n[i] )
		{
			case '+':
			case '-':
			{
				if (bDigit) return false; // already found a numeral prior to sign
				if (i != 0) return false; // sign is not the first char
				if (bSign) return false; // found multiple sign
				bSign = true;
				break;
			}
			case '.':
			{
				if (bDecimal) return false; // found multiple decimal point
				bDecimal = true;
				bDigit = true;
				break;
			}
			default:
			{
				
				if (n[i] < '0' || n[i] > '9') return false; // any character that is not a number is found
				bDigit = true; // already found a valid digit				
			}
		}
	}

	return true;
}

/*-----------------------------------------------------------------------------------------
check if string is a number
-----------------------------------------------------------------------------------------*/
bool CUtil::isInteger(const std::string& n)
{
	bool bDigit = false;
	bool bSign = false; 
	
	for (std::string::size_type i = 0; i < n.size(); i++)
	{
		switch( n[i] )
		{
			case '+':
			case '-':
			{
				if (bDigit) return false; // already found a numeral prior to sign
				if (i != 0) return false; // sign is not the first char
				if (bSign) return false; // found multiple sign
				bSign = true;
				break;
			}
			default:
			{				
				if (n[i] < '0' || n[i] > '9') return false; // any character that is not a number is found
				bDigit = true; // already found a valid digit				
			}
		}
	}

	return true;
}

/*-----------------------------------------------------------------------------------------
convert alphabet char into upper case
-----------------------------------------------------------------------------------------*/
char CUtil::toUpper(const char c)
{
	// only deal with english alphabet
	if (c < 97 || c > 122) return c;

	// shift  bit 5 to turn it int upper case
	return c ^ 0x20;	
}

/*-----------------------------------------------------------------------------------------
convert string to upper case
-----------------------------------------------------------------------------------------*/
const std::string CUtil::toUpper(const std::string& str)
{
	std::string rslt = str;
	for (std::string::size_type i = 0; i < rslt.size(); i++)
	{ 
		rslt[i] = CUtil::toUpper(rslt[i]); 
	}

	return rslt;
}

/*-----------------------------------------------------------------------------------------
remove leading/trailing space in string
-----------------------------------------------------------------------------------------*/
const std::string CUtil::removeLeadingTrailingSpace(const std::string& str)
{
	std::string s = str; 

	// find the first non space character
	size_t pos = s.find_first_not_of(" \n\t\f\v", 0, 5);

	// if pos = npos, then this is an empty string with nothing but spaces. bail out
	if (pos == std::string::npos) return std::string();

	// remove characters before the first non space char
	s = s.substr(pos);

	// find the last non space character
	pos = s.find_last_not_of(" \n\t\f\v", std::string::npos, 5);

	return s.substr(0,pos + 1);
}

/*-----------------------------------------------------------------------------------------
check if file exist
-----------------------------------------------------------------------------------------*/
bool CUtil::isFileExist(const std::string& szFile)
{
     	return (access(szFile.c_str(), F_OK) > -1)? true : false;
} 

/*-----------------------------------------------------------------------------------------
find the PID of the first thread found with specified name
-----------------------------------------------------------------------------------------*/
int CUtil::getFirstPIDByName( const std::string& name )
{
	int pid = -1;

	// Open the /proc directory
	DIR *dp = opendir("/proc");
	if (dp != NULL)
	{
		// Enumerate all entries in directory until process found
		struct dirent *dirp;
		while (pid < 0 && (dirp = readdir(dp)))
		{
			// Skip non-numeric entries
			int id = CUtil::toLong(dirp->d_name);
			if (id > 0)
			{
				// Read contents of virtual /proc/{pid}/cmdline file
				std::string cmdPath = std::string("/proc/") + dirp->d_name + "/cmdline";
				std::ifstream cmdFile(cmdPath.c_str());
				std::string cmdLine;
				getline(cmdFile, cmdLine);
				if (!cmdLine.empty())
				{
					// Keep first cmdline item which contains the program path
					size_t pos = cmdLine.find('\0');
					if (pos != std::string::npos) cmdLine = cmdLine.substr(0, pos);

					// Keep program name only, removing the path
					pos = cmdLine.rfind('/');
					if (pos != std::string::npos) cmdLine = cmdLine.substr(pos + 1);

					// Compare against requested process name
					if (name == cmdLine) pid = id;		
				}
			}
		}
	}
	closedir(dp);
	return pid;
}




