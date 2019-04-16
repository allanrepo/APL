#include <cex.h>
#include <cmd.h>


/* ------------------------------------------------------------------------------------------
constructor 
------------------------------------------------------------------------------------------ */
CCex::CCex(): m_Tester(CTester::instance()), m_Log(CTester::instance().m_Log), m_Debug(CTester::instance().m_Debug)
{  
	// debug log is immediate and disabled by default 
	m_Debug.immediate = true;
	m_Debug.enable = false;
	m_Debug.silent = true;
  
	// logger is always enabled and immediate 
	m_Log.immediate = true;
	m_Log.enable = true;    
	m_Log.silent = false; 
 
	// create -t[ester] option
	addChild( &CTester::instance() );

	// create -h[elp] option
	CArg* pHelp = new CHelp();
	addChild( pHelp );

	// create -c[ommand] option
	CArg* pCmd = new CCmd(); 
	addChild( pCmd );

	// add options 
	addChild( new CArg("-debug") );
	addChild( new CArg("-dm") );
	addChild( new CArg("-timeout") );
	addChild( new CArg("-head") );
	addChild( new CArg("-hd") );
	addChild( new CArg("-syntax_check") );
	addChild( new CArg("-version") );	
	
	// let's initialize <tester> to default name
	std::stringstream ss;
	getUserName().empty()? (ss << "sim") : (ss << getUserName() << "_sim");
	getChild("-tester")->setValue(ss.str()); 
	
	// let's set tester connection enabled by default 
	getChild("-tester")->getChild("connect")->setValue("ok");
}

/* ------------------------------------------------------------------------------------------
add command objects. 
------------------------------------------------------------------------------------------ */
void CCex::addCmd( CArg* pCmd )
{
	getChild("-command")->addChild( pCmd );
}

/* ------------------------------------------------------------------------------------------
get user name
------------------------------------------------------------------------------------------ */
const std::string CCex::getUserName() const
{ 
	uid_t uid = getuid();
	char buf_passw[1024];    
	struct passwd password;
	struct passwd *passwd_info;

	getpwuid_r(uid, &password, buf_passw, 1024, &passwd_info);
	return std::string(passwd_info->pw_name);
}

/* ------------------------------------------------------------------------------------------
scan command line args
------------------------------------------------------------------------------------------ */
bool CCex::scan(int argc, char **argv) 
{
	// store all args to list
	std::list< std::string > v;
	for (int i = 1; i < argc; i++) v.push_back( argv[i] );

	// parse command line arguments
	if (!scan(v)) return false;

	return true;
}

/* ------------------------------------------------------------------------------------------
scan command line arguments
------------------------------------------------------------------------------------------ */
bool CCex::scan(std::list< std::string >& Args)
{
	for (std::list< std::string >::iterator it = Args.begin(); it != Args.end(); it++)
	{ 
		std::string arg( (*it) );

		// handle '-t' exception. it's a special case as per unison doc, in which it always refer 
		// to '-tester' even if it's ambiguous to other opts such as '-timeout'.
		if (arg.compare("-t") == 0) arg = "-tester"; 	

		// list all options that partially match this arg
		std::vector< CArg* > v;
		findChildren(arg, v, true);

		// is it ambiguous?
		if (v.size() > 1)
		{
			m_Log << "CEX Error: CEX arguments: Ambiguous option '" << arg << "' choices are: ";
			for (unsigned int i = 0; i < v.size(); i++) m_Log << "'" << v[i]->name() << "', ";
			m_Log << CUtil::CLog::endl;
			return false;
		}   
  
		// did we not find a match?
		if (!v.size())
		{ 
			m_Log << "CEX Error: CEX arguments: Bad argument to cex '" << arg << "'" << CUtil::CLog::endl;
			return false;
		}

		// or did we find a unique match?

		// is it -t[ester]?
		if (v[0]->is("-tester"))
		{
			// expect another argument after this as <tester>
			it++;
			if (it == Args.end())
			{
				m_Log << "CEX Error: CEX arguments: -tester option found but tester name missing." << CUtil::CLog::endl;
				return false;
			}
			else
			{
				v[0]->setValue( *it );
				v[0]->getChild("connect")->setValue("ok");
				continue;
			}
		}

		if (v[0]->is("-help"))
		{
			v[0]->setValue("ok");
			continue;
		}		

		if (v[0]->is("-debug"))
		{
			v[0]->setValue("ok");
			continue;
		}

		// is it -c[ommand]?
		if (v[0]->is("-command"))
		{
			std::list< std::string > c;
			c.splice(c.begin(), Args, ++it, Args.end() );
			v[0]->setValue(""); // clear it's value before we proceed
			return v[0]->scan(c);			
		}	
	}

	return true;
}


/* ------------------------------------------------------------------------------------------
 
------------------------------------------------------------------------------------------ */
bool CCex::exec()
{
	// are we in debug mode?
	if (getChild("-debug")->has("ok"))
	{
		m_Debug.enable = true;
		m_Debug.silent = false;
	}

	// if general help is enabled, print it and do nothing
	if (getChild("-help")->has("ok"))
	{
		getChild("-help")->exec();
		return true;
	}

	// try to connect to tester
	if (getChild("-tester")->getChild("connect")->has("ok") )
	{
		if (!m_Tester.connect( getChild("-tester")->getValue() )) return false;
	}

	// are we executing command?
	if (!getChild("-command")->getValue().empty())
	{
		std::string cmd = getChild("-command")->getValue();
		CArg* pCmd = getChild("-command")->getChild( cmd );

		if (pCmd) pCmd->exec();
		else loop();
	}
	// if not, let's get into loop
	else loop();

	m_Tester.disconnect();
	return true;
}



/* ------------------------------------------------------------------------------------------
 
------------------------------------------------------------------------------------------ */
void CCex::loop()
{
	while(true)
	{
		std::string s;
		std::cout << "cex>";
		std::cin >> s;

		// parse entry string and store each words in list
		std::list< std::string > v;

		// clear the -c[ommand] 
		getChild("-command")->setValue("");

		// pass it to -c[ommand] to find the <command>
		// returns true if no error is found
		if ( !getChild("-command")->scan(v) ) 
		{
			continue;
		}

		CArg* pCmd = getChild("-command");
		pCmd = pCmd->getChild( pCmd->getValue() );
		if (pCmd) pCmd->exec();
	}
}


