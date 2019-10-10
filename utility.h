#ifndef __UTILITY__
#define __UTILITY__

// std C++ includes for string and file stuff
#include <string>
#include <sstream>
#include <iostream>
#include <cstdio>
#include <list>
#include <vector>
#include <fstream>

// linux specific include for access()
#include <unistd.h>
#include <sys/types.h>

// linux specific include for dir
#include <dirent.h>


namespace CUtil
{
	// standard class version
	class CLog
	{
	private:
		std::stringstream m_stream;
		static std::string m_file;

	public:
		CLog(){}
		virtual ~CLog(){} 
		
		CLog(const CLog&){} // copy constructor                     
		//const CLog& operator=(const CLog&){} // operator '='
	public:
		// properties
		static bool immediate;
		static bool enable;
		static bool silent;

		void file(const std::string& file){ m_file = file; }
		const std::string& file(){ return m_file; }
		void clear(){ m_stream.str(std::string()); }
		void flush()
		{ 
			if (!silent) std::cout << m_stream.str(); 
			if (m_file.size())
			{
				std::fstream fs;
				fs.open(m_file.c_str(), std::fstream::out | std::fstream::app);
				if (fs.is_open()) fs << m_stream.str();
				fs.close();
			}
			clear(); 
		}

		template <class T>
		CLog& operator << (const T& s)
		{
			if (enable)
			{
				if(immediate)
				{
					if (!silent) std::cout << s;

					if (m_file.size())
					{
						std::fstream fs;
						fs.open(m_file.c_str(), std::fstream::out | std::fstream::app);
						if (fs.is_open()) fs << s;
						fs.close();
					}
				}
				else m_stream << s;
			}
			return *this;
		}

		static std::ostream& endl(std::ostream &o)
		{
			o << std::endl;	
			return o;
		}
	};

	long toLong(const std::string& num);
	bool isNumber(const std::string& n);
	bool isInteger(const std::string& n);
	char toUpper(const char c);
	const std::string removeLeadingTrailingSpace(const std::string& str);
	const std::string toUpper(const std::string& str);
	int getFirstPIDByName( const std::string& name, bool bSkipSelf = false);

	bool renameFile(const std::string& szOldFileNamePath, const std::string& szNewFileNamePath, int nAttempt = 5, int nDelaySecond = 1);
	bool removeFile(const std::string& szFileNamePath, int nAttempt = 5, int nDelaySecond = 1);
	bool openFile(const std::string& file, std::fstream& fs, std::ios_base::openmode mode, int nAttempt = 5, int nDelaySec = 1);
	bool isFileExist(const std::string& szFile, int nAttempt = 1, int nDelaySec = 0);
};


#endif
