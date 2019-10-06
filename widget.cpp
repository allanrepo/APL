#include <widget.h>
#include <utility.h>

wxIMPLEMENT_APP(CxAPL);

// ---------------------------------------------------------------------------------
// utility: finds other running instance of this app and attempts to kill it
// ---------------------------------------------------------------------------------
bool CxAPL::runThisInstanceOnly()
{
	// trim the name of '/' and path, we just want name
	std::string name(wxGetApp().argv[0]);
	size_t pos = name.rfind('/');
	if (pos != std::string::npos) name = name.substr(pos + 1); 

	// let's check if same app is already running, kill it.
	// note that we are skipping our own PID so we don't kill ourselves
	sleep(1);
	int pid = CUtil::getFirstPIDByName(name, true);
	while ( pid != -1 )
	{
		std::stringstream ssCmd;
		ssCmd << "kill -9 " << pid;
		std::cout << ssCmd.str() << std::endl;
		system(ssCmd.str().c_str());	

		// after killing, let's keep finding. 
		pid = CUtil::getFirstPIDByName(name,true );
	}

	return true;
}

// ---------------------------------------------------------------------------------
// initialize app here
// ---------------------------------------------------------------------------------
bool CxAPL::OnInit()
{
	// let's not launch if we're not given required information
	if(wxGetApp().argc < 4) return false;

	// let's get the IP, port from command line arguments
	m_szIP = wxGetApp().argc < 2 ? SOCKET_IP_DEFAULT : std::string( wxGetApp().argv[1] );
	m_nPort = wxGetApp().argc < 3? SOCKET_PORT_DEFAULT : CUtil::toLong(std::string( wxGetApp().argv[2]));

	// remember its owner's name
	m_szOwner = std::string( wxGetApp().argv[3] );
	size_t pos = m_szOwner.rfind('/');
	if (pos != std::string::npos) m_szOwner = m_szOwner.substr(pos + 1); 

	// are we runing debug mode?
	m_bDebug = wxGetApp().argc >= 5? (std::string( wxGetApp().argv[4] ).compare("DEBUG") == 0? true : false ) : false;
	
	// let's make sure this is the only instance of this app that runs
	if(!runThisInstanceOnly()) return false;

	// create frame 
	m_pFrame = new wxFrame((wxFrame*) NULL, wxID_ANY, "APL Interface Server", wxPoint(50,50), wxSize(800, 600));
	m_pFrame->Show( m_bDebug? true : false );

	// create menu bar
    	wxMenuBar* pMenuBar = new wxMenuBar();
    	m_pFrame->SetMenuBar(pMenuBar);  

    	// create view menu. setup menu here
    	wxMenu* pViewMenu = new wxMenu();
   	pMenuBar->Append(pViewMenu, _T("&View"));

	// create display sizer
	wxBoxSizer *pFrameSizer = new wxBoxSizer(wxVERTICAL);

	// create display widget
	m_pLog = new wxTextCtrl(m_pFrame, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY);

	// add display widgets to sizer
	pFrameSizer->Add(m_pLog , 1, wxEXPAND, 0);	

	// set this sizer as our frame's sizer
	m_pFrame->SetSizer(pFrameSizer);

	// create invertal timer. since this app is this timer's parent, events emitted by the timer is redirected 
	// to this app so this app can bind the event handler for this timer by itself
	m_pTimer = new wxTimer( this, wxID_ANY );
	Bind(wxEVT_TIMER, &CxAPL::OnTimer, this, m_pTimer->GetId());
	m_pTimer->Start( 100 );

	// this interval timer emit event that allows this app to update its status
	m_pUpdateApp = new wxTimer( this, wxID_ANY );
	Bind(wxEVT_TIMER, &CxAPL::OnUpdateApp, this, m_pUpdateApp->GetId());
	m_pUpdateApp->Start( 1000 );

	m_bFirst = true;
	return true;
}

// ---------------------------------------------------------------------------------
// update app status in this event
// ---------------------------------------------------------------------------------
void CxAPL::OnUpdateApp(wxTimerEvent& event)
{
	// we only do below stuff if we're not on debug mode
	if (!m_bDebug)
	{
		// if owner is not running anymore, let's stop running as well
		if( CUtil::getFirstPIDByName(m_szOwner) == -1 )
		{ 
			(*m_pLog) << "Didn't find '" << m_szOwner << "' running. Exiting...\n";
			m_pFrame->Close(true); 
		}

		//is our log container full? let's clear it
		if ( m_pLog->GetNumberOfLines() > 64){ m_pLog->Clear(); }
	}

	event.Skip();
}

void CxAPL::onError(const std::string& msg)
{
	(*m_pLog) << msg << "\n";
}

// ---------------------------------------------------------------------------------
// this is where the magic happens
// ---------------------------------------------------------------------------------
void CxAPL::onRecv(const std::string& msg)
{ 
	if (m_bDebug) (*m_pLog) << "ONRECV: " << msg << "\n";

	std::string rcv = msg;
	size_t pos = rcv.find('|');
	if (pos != std::string::npos) rcv = rcv.substr(0, pos); 	

	int errnum = 0;

	if (rcv.compare("LOADLOTINFO") == 0)
	{
		if (wxMessageBox("Program Is loaded. If you Proceed, APL will RESET the Lot and Reload Program. Please Confirm.", "WARNING!! APL Receive Lotinfo File!", wxYES_NO, NULL) == wxYES)
		{
			if (wxMessageBox("Please Confirm Again.", "Are you Really Sure?", wxYES_NO, NULL) == wxYES)
			{
				if ( !send("LOADLOTINFO", &errnum))
				{
					(*m_pLog) << "Error sending message: " << strerror(errnum) << "\n";
				}
			}
		}		
	}
	else if (rcv.compare("APL_REGISTER") == 0)
	{
		if (!send("APL_ACKNOWLEDGE", &errnum))
		{
			(*m_pLog) << "Error sending message: " << strerror(errnum) << "\n";
		}
	}
}

// ---------------------------------------------------------------------------------
// this is executed every time wxTimer event occurs
// ---------------------------------------------------------------------------------
void CxAPL::OnTimer(wxTimerEvent& event)
{	
	// listen to port for any incoming message from owner
	if (m_bFirst)
	{
		(*m_pLog) << "Establishing Server at IP: " << m_szIP << " Port: " << m_nPort << "\n";
		connect(m_szIP, m_nPort);
		m_bFirst = false;
	}
	else listen(90);

	event.Skip();
} 

// ---------------------------------------------------------------------------------
// this is executed when system is idle. by default, it's executed only once when system is idle.
// to keep executing it when system is idle, call RequestMore() as we are doing here
// ---------------------------------------------------------------------------------
void CxAPL::OnIdle(wxIdleEvent& evt)
{	
	m_nFrame++;
	evt.RequestMore();
} 

