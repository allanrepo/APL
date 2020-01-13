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
	m_pFrame = new wxFrame((wxFrame*) NULL, wxID_ANY, "APL Commander 1.0", wxPoint(50,50), wxSize(800, 600));
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
		if (MessageBox())
		{
			if ( !send("LOADLOTINFO", &errnum))
			{
				(*m_pLog) << "Error sending message: " << strerror(errnum) << "\n";
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

bool CxAPL::MessageBox()
{
	// create dialog box
	wxString t("Received Lotinfo File! Press \"Proceed\" to UNLOAD current test program and LOAD new one. This will also END THE LOT.\nPress \"Cancel\" to ignore this message");
	wxLoadProgramDialog ProgramLoad("WARNING", t, wxSize(800,600));
	ProgramLoad.Show(true);

	// return dialog box' return code
	if (ProgramLoad.ShowModal() == wxID_OK)
	{
		wxLoadProgramDialog Confirm("Please Confirm Again.", "Are you Really Sure?", wxSize(400,300));
		Confirm.Show(true);
		if (Confirm.ShowModal() == wxID_OK) return true;
		else return false;
	}
	else return false;
}


// ---------------------------------------------------------------------------------
// customized dialog box for asking operator if they want to ptoceed in processing
// incoming lotinfo.txt file as it warns them that current test program loaded
// will be unloaded and current lot under test will end
// -	'bConfirm' flag can be set to TRUE so it pops a dialog box to ask operator
//	for confirmation to really proceed
// ---------------------------------------------------------------------------------
wxLoadProgramDialog::wxLoadProgramDialog(const wxString &title, const wxString& msg, const wxSize& size)
:wxDialog(NULL, wxID_ANY, title, wxDefaultPosition, size, wxDEFAULT_DIALOG_STYLE|wxSTAY_ON_TOP)
{
	// create our message fonts
	wxFont font(32, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);

	// create sizer that will contain the message. make this vertical but doesn't really matter as we will only put 1 static test control
	wxBoxSizer *wxTopSizer = new wxBoxSizer(wxHORIZONTAL);

	// create sizer that will contain buttons. make this horizontal
	wxBoxSizer *wxBtmSizer = new wxBoxSizer(wxHORIZONTAL);

	// create sizer that will contain the button and message sizers
	wxBoxSizer *wxDialogSizer = new wxBoxSizer(wxVERTICAL);

	// create static text widget and add its content
	wxStaticText *wxMessage = new wxStaticText(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE_HORIZONTAL);
	wxMessage->SetFont(font);
	wxMessage->SetLabel(msg);
	wxMessage->Wrap(size.GetWidth());

	// add static text widget to its sizer
	// proportion doesn't matter. setting to either 0 or 1 will have same result
	// wxALIGN_CENTER forces the widget to align vertically, as this sizer is horizontal orientation
  	wxTopSizer->Add(wxMessage, 0,  wxALIGN_CENTER);

	// create "OK" button and bind it's event handler
	wxButton *wxBtnOK = new wxButton(this, wxID_OK, wxT("Proceed"), wxDefaultPosition, wxDefaultSize);
	wxBtnOK->SetFont(font);

	// create "Cancel" button and set its ID as wxID_CANCEL. this button's parent (the dialog widget) will recognize it 
	// as "end modal" when it's pressed and dialog box closes so we don't need to bind event handler 
	wxButton *wxBtnCancel = new wxButton(this, wxID_CANCEL, wxT("Cancel"), wxDefaultPosition, wxDefaultSize);
	wxBtnCancel->SetFont(font);

	// add input buttons to its sizer. proportion is both set to 1 so they both use the biggest size
	// wxLeft, 50 on cancel button is to give space of 50px between OK and cancel button
  	wxBtmSizer->Add(wxBtnOK, 0);
 	wxBtmSizer->Add(wxBtnCancel, 1, wxLEFT, 20);

	// add button and message sizers in main sizer and set it to this widget
	wxDialogSizer->Add(wxTopSizer, 1, wxALIGN_CENTER); // proportion = 1 to fill whole row, wxALIGN_CENTER to put the contents of top sizer at center horizontally as dialog size is vertical
  	wxDialogSizer->Add(wxBtmSizer, 0, wxALIGN_CENTER | wxTOP | wxBOTTOM, 10);
	SetSizer(wxDialogSizer);

	// make sure this widget is centered and remember its center position so we will keep it in the center through OnMove event handler
	Centre();
	GetPosition(&m_wxPosition.x, &m_wxPosition.y);

	// bind event handler for this dialog's drag/move so we can catch it it prevent this dialog from being moved. 
	// we want it to stay at center of screen
	Bind(wxEVT_MOVE, &wxLoadProgramDialog::OnMove, this);
}


void wxLoadProgramDialog::OnMove( wxMoveEvent &evt )
{
	Move( m_wxPosition.x, m_wxPosition.y);
	evt.Skip();
}

