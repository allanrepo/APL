#ifndef __STATE__
#define __STATE__

#include <list>
#include <string>

//------------------------------------------------------------------------------------------------------
// state manager class
//------------------------------------------------------------------------------------------------------
class CStateManager
{
public:
	class CTime
	{
	private:
	public:
		CTime();
		virtual ~CTime();
		virtual void run();
	};

public:
	//------------------------------------------------------------------------------------------------------
	// state class
	//------------------------------------------------------------------------------------------------------
	class CState
	{
	protected:
		std::string m_szName;
	public:
		CState(const std::string& name);
		virtual ~CState();

		virtual void run();
		virtual void load();
		virtual void unload();

		friend class CStateManager;
	};

protected:
	CState* m_pNext;
	CState* m_pState;
	bool m_bNext;

public:
	// constructors
	CStateManager();
	virtual ~CStateManager();

	// virtual functions
	virtual void run();
	virtual void load();
	virtual void unload();

	// fixed functions specific to this class
	void set( CState* pState );
	void clear();
};

#endif
