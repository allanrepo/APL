#include "fademo_hooks.h"
#include <iostream>


#define _macroToStr(s) _defToStr(s)
#define _defToStr(s) #s

#ifndef UH_NAME
#define UH_NAME "Unison FAPROC User Hooks"
#endif

#ifndef UH_VERSION_NUMBER
#define UH_VERSION_NUMBER 1
#endif

#ifndef UH_VERSION_STRING
#define UH_VERSION_STRING _macroToStr(UH_VERSION_NUMBER)
#endif

ISG_REGISTER_USER_HOOKS register_user_hooks(CfaModuleInterface& cCtrl, std::string& errStr, FA_LONG& errCode);
ISG_DEREGISTER_USER_HOOKS deregister_user_hooks(CFademoHooks *theseHooks);


ISG_REGISTER_USER_HOOKS register_user_hooks(CfaModuleInterface& cCtrl, std::string& errStr, FA_LONG& errCode)
{
	CFademoHooks *theseHooks = new CFademoHooks(cCtrl, errStr, errCode, UH_NAME, UH_VERSION_STRING, UH_VERSION_NUMBER);
	return theseHooks;
}

ISG_DEREGISTER_USER_HOOKS deregister_user_hooks(CFademoHooks *theseHooks)
{
    if (NULL == theseHooks) return false;

    delete theseHooks;
    theseHooks = NULL;
    return true;
}


// The following is only to allow code development on the windows platform.
//

#if (defined(WIN32) || defined(_WINDOWS))

#ifndef CURI_PLATFORM_USE_WINDOWS_H

#include <afxdllx.h>

static AFX_EXTENSION_MODULE FAPROC_PRODUCTION_HOOKS_DLL = { NULL, NULL };

extern "C" int APIENTRY
DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	// Remove this if you use lpReserved
	UNREFERENCED_PARAMETER(lpReserved);

	if (dwReason == DLL_PROCESS_ATTACH)
	{
		TRACE0("FAPROC_PRODUCTION_HOOKS.DLL Initializing!\n");
		
		// Extension DLL one-time initialization
		if (!AfxInitExtensionModule(FAPROC_PRODUCTION_HOOKS_DLL, hInstance))
			return 0;

		// Insert this DLL into the resource chain
		// NOTE: If this Extension DLL is being implicitly linked to by
		//  an MFC Regular DLL (such as an ActiveX Control)
		//  instead of an MFC application, then you will want to
		//  remove this line from DllMain and put it in a separate
		//  function exported from this Extension DLL.  The Regular DLL
		//  that uses this Extension DLL should then explicitly call that
		//  function to initialize this Extension DLL.  Otherwise,
		//  the CDynLinkLibrary object will not be attached to the
		//  Regular DLL's resource chain, and serious problems will
		//  result.

		new CDynLinkLibrary(FAPROC_PRODUCTION_HOOKS_DLL);
	}
	else if (dwReason == DLL_PROCESS_DETACH)
	{
		TRACE0("FAPROC_PRODUCTION_HOOKS.DLL Terminating!\n");
		// Terminate the library before destructors are called
		AfxTermExtensionModule(FAPROC_PRODUCTION_HOOKS_DLL);
	}
	return 1;   // ok
}


#else

BOOL WINAPI DllMain( HINSTANCE hDllInst, 
                       DWORD ul_reason_for_call, 
                       LPVOID lpReserved )
{
	bool ok = true;
	bool isThread = true;

    switch( ul_reason_for_call ) 
	{
		case DLL_PROCESS_ATTACH:
			isThread = false;
			break;

		case DLL_THREAD_ATTACH:
			break;
		case DLL_PROCESS_DETACH:
			isThread = false;
			break;

		case DLL_THREAD_DETACH:
			break;

		default:
			break;
    }
	return (ok ? TRUE : FALSE);
}


#endif

#endif
