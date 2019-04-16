#ifndef __ARG__
#define __ARG__
/*
design consideration:

on partial match search
-	note that partial match strictly compare for matches at first character
-	e.g. hello on hello_world is OK but world on hello_world is NOT OK

on reference option values
-	must be unique. because it doesn't make sense to have multiple options 
	of the same value. 

on given option values
-	options from arg command list can have duplicates since we can't and shouldn't control
	what arguments are used by applications. this list's job is only to remember them.
-	there are 2 methods to check for existence of a given opt - isOpt() and hasOpt()
	isOpt() checks if an option of a specific index in the list matches the given value
	hasOpt() finds an option that matches the given value.
	both methods search with exact match 
*/

#include <string>
#include <vector>
#include <list>

class CArg
{
private:
	// name of this arg
	std::string m_strName;

	// value of this arg
	std::string m_strValue;
	
	// list of arg options acceptable to this arg
	std::vector< CArg* > m_Children;

	CArg* m_pParent;

public:
	// constructs with option to specify value
	CArg(const std::string strName = ""){ m_strName = strName; };
	virtual ~CArg(){};

	// access this arg's name
	const std::string& name() const { return m_strName; } 	

	// check if this arg matches the given name, with option for partial match
	bool is(const std::string& name, bool bPartialMatch = false );

	// access this arg's parent
	CArg* parent(){ return m_pParent; }

	/*------------------------------------------------------------------------------
	methods for managing options
	------------------------------------------------------------------------------*/

	// add an option. options must have unique names. 
	bool addChild(CArg* pChild);

	// return number of options
	unsigned int getNumChildren() const { return m_Children.size(); }

	// list all Reference option found that matches the given name. has option search for partial match
	void findChildren(const std::string& name, std::vector< CArg* >& v, bool bPartialMatch = false) const;

	// find option with the given name in the list. if more than one match is found, return null.
	// there's an option to search for partial match, default being exact match
	CArg* getChild(const std::string& name, bool bPartialMatch = false) const;

	// get child based on index in the children list 
	CArg* getChild(unsigned int n){ return n >= m_Children.size()? 0 : m_Children[n]; }

	/*------------------------------------------------------------------------------
	methods for managing value
	------------------------------------------------------------------------------*/

	// add a given option for this arg
	void setValue(const std::string& value){ m_strValue = value; }

	// clear this arg's value
	void clearValue(){ m_strValue.clear(); }

	// returns the option of specified index. if index is invalid, returns empty string
	const std::string& getValue() const { return m_strValue; }

	// checks if this arg contains value that exactly matches the given string. 
	bool has(const std::string& value) const { return m_strValue.compare(value) == 0? true : false; }

	static const std::string m_strEmpty;

	/*------------------------------------------------------------------------------
	can be defined by derived class to specify what the arg option can do
	------------------------------------------------------------------------------*/
	virtual bool exec(){ return true; }
	virtual bool scan(std::list< std::string >& Args){ return true; }
};


#endif
