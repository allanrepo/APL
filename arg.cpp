#include <arg.h>

const std::string CArg::m_strEmpty("");
 
/* ------------------------------------------------------------------------------------------
check if this arg matches the given name, with option for partial match
------------------------------------------------------------------------------------------ */
bool CArg::is(const std::string& name, bool bPartialMatch)
{
	if (!bPartialMatch) return m_strName.compare(name) == 0? true : false; 

	// if returns 0, a match occured on first character of value
	return (m_strName.find(name) == 0)? true : false;
} 

/* ------------------------------------------------------------------------------------------
add a reference option. reference options must have unique values.
------------------------------------------------------------------------------------------ */
bool CArg::addChild(CArg* pChild)
{
	if (!pChild) return false;

	// arg must have value that does not exist in list yet before it can be added
	for (unsigned int i = 0; i < m_Children.size(); i++)
	{
		if (m_Children[i]->m_strName.compare(pChild->m_strName) == 0) return false;
	}

	// add to end if not yet
	m_Children.push_back(pChild);
	pChild->m_pParent = this;
	return true;
}

/* ------------------------------------------------------------------------------------------
list all  option found that matches the given name. 
has option search for partial match
------------------------------------------------------------------------------------------ */
void CArg::findChildren(const std::string& name, std::vector< CArg* >& v, bool bPartialMatch) const
{
	v.clear();
	for (unsigned int i = 0; i < m_Children.size(); i++)
	{
		if (m_Children[i]->is(name, bPartialMatch)) v.push_back( m_Children[i] );
	}
}

/* ------------------------------------------------------------------------------------------
find option with the given name in the list. if more than one match is found, 
return null. has option to search for partial match, default being exact match
------------------------------------------------------------------------------------------ */
CArg* CArg::getChild(const std::string& name, bool bPartialMatch) const
{
	std::vector< CArg* > v;
	findChildren(name, v, bPartialMatch);
	return v.size() == 1? v[0] : 0;
}





