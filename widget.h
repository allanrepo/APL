#include <wx/wxprec.h>
#include <wx/listctrl.h>
#include <wx/statline.h> 
#include <wx/slider.h>
#include <sys/time.h>
#include <socket.h>
#include <utility.h>

#ifndef WX_PRECOMP
	#include <wx/wx.h>
	#include <wx/app.h>
#endif 

// ---------------------------------------------------------------------------------
// declarations
// ---------------------------------------------------------------------------------

// app class
class CxAPL: public wxApp, CServerUDP
{
private:
	wxTextCtrl* m_pLog;
	wxFrame* m_pFrame;
	wxTimer* m_pTimer;
	wxTimer* m_pUpdateApp;

	bool m_bFirst;
	struct timeval m_prev;
	struct timeval m_curr;
	long m_nTimer;
	long m_nFrame;
	std::string m_szOwner;
	bool m_bDebug;

	bool runThisInstanceOnly();

public:
	virtual bool OnInit();
	virtual void OnIdle(wxIdleEvent& evt);
	virtual void OnTimer(wxTimerEvent& evt);
	virtual void OnUpdateApp(wxTimerEvent& evt);

	virtual void onError(const std::string& msg);
	virtual void onRecv(const std::string&);
};


