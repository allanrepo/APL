// *************************************************************************************
//  Module      : QCOM_Datalog.cpp
//  Description : LTX-Credence standard ASCII and STDFV4 Datalog methods
//
//  Copyright (C) LTX-Credence Corporation 2009.  All rights reserved.
// *************************************************************************************

#include "QCOM_Datalog.h"
#include <Unison.h>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <vector>
#include <cstring>
#include <cstdlib>

using namespace std;
// Note to anyone making additions to the list of formatters
// Make sure the formatters have unique first characters.  Code below has a shortcut that
// only compares the first character of the formatter name for performance reasons.
static const char *formats[] = {"ASCII", "STDFV4", 0};
const int ASCII_INDEX = 0;		// must match formats array above
const int STDFV4_INDEX = 1;		// must match formats array above
const int TNSize = 10;
const int VASize = 13;
const int PGSize = 21;
const int TDSize = 50;

const int UnitSize = 8;
const int DefaultFieldWidth = 13;
const StringS DefaultPassString = " P ";

const int IntegerPartWidthScaled   = 6;
const int IntegerPartWidthUnscaled = 10;

DATALOG_METHOD_CLASS(QCOM_Datalog);

QCOM_Datalog::
QCOM_Datalog() : 
	DatalogMethod(formats),
	SummaryNeeded(false),
	LastFormatEvent(DatalogMethod::StartTestBlock),
	VerbosePins(),
	PerSiteSummary(),
	EnableVerbose(),
	AppendPinName(),
	UnitAutoscaling(),
	ASCIIDatalogInColumns(),
	EnableDebug(),
	EnhancedFuncChars(),
	EnableScan2007(),
	EnableFullOpt(),
	NumTestsExecuted(0),
	FieldWidth(DefaultFieldWidth),
	PassString(DefaultPassString),
	FinishTime(UTL_VOID)
{
	RegisterAttribute(PerSiteSummary, "PerSiteSummary", true);
	RegisterAttribute(EnableVerbose, "EnableVerbose", false);
	RegisterAttribute(ASCIIDatalogInColumns, "ASCIIDatalogInColumns", false);
	RegisterAttribute(EnableDebug, "EnableDebugText", false);
	RegisterAttribute(EnhancedFuncChars, "EnhancedFunctionalChars", false);
	RegisterAttribute(EnableScan2007, "EnableScan2007", false);
	RegisterAttribute(AppendPinName, "AppendPinName", true);
	RegisterAttribute(UnitAutoscaling, "UnitAutoscaling", false);
	RegisterAttribute(ASCIIOptimizeForUnscaledValues, "ASCIIOptimizeForUnscaledValues", false);
//	RegisterAttribute(EnableFullOpt, "EnableFullOptimization", false);

	RegisterEvent(GetSystemEventName(DatalogMethod::StartOfTest), &QCOM_Datalog::StartOfTest);
	RegisterEvent(GetSystemEventName(DatalogMethod::EndOfTest), &QCOM_Datalog::EndOfTest);
	RegisterEvent(GetSystemEventName(DatalogMethod::ProgramLoad), &QCOM_Datalog::ProgramLoad);
	RegisterEvent(GetSystemEventName(DatalogMethod::ProgramUnload), &QCOM_Datalog::ProgramUnload);
	RegisterEvent(GetSystemEventName(DatalogMethod::ProgramReset), &QCOM_Datalog::ProgramReset);
	RegisterEvent(GetSystemEventName(DatalogMethod::Summary), &QCOM_Datalog::Summary);
	RegisterEvent(GetSystemEventName(DatalogMethod::StartOfWafer), &QCOM_Datalog::StartOfWafer);
	RegisterEvent(GetSystemEventName(DatalogMethod::EndOfWafer), &QCOM_Datalog::EndOfWafer);
	RegisterEvent(GetSystemEventName(DatalogMethod::StartOfLot), &QCOM_Datalog::StartOfLot);
	RegisterEvent(GetSystemEventName(DatalogMethod::EndOfLot), &QCOM_Datalog::EndOfLot);
	RegisterEvent(GetSystemEventName(DatalogMethod::StartTestNode), &QCOM_Datalog::StartTestNode);
	RegisterEvent(GetSystemEventName(DatalogMethod::StartTestBlock), &QCOM_Datalog::StartTestBlock);
	RegisterEvent(GetSystemEventName(DatalogMethod::ParametricTest), &QCOM_Datalog::ParametricTest);
	RegisterEvent(GetSystemEventName(DatalogMethod::ParametricTestArray), &QCOM_Datalog::ParametricTestArray);
	RegisterEvent(GetSystemEventName(DatalogMethod::FunctionalTest), &QCOM_Datalog::FunctionalTest);
	RegisterEvent(GetSystemEventName(DatalogMethod::ScanTest), &QCOM_Datalog::ScanTest);
	RegisterEvent(GetSystemEventName(DatalogMethod::Text), &QCOM_Datalog::Text);
	RegisterEvent(GetSystemEventName(DatalogMethod::Generic), &QCOM_Datalog::Generic);
}

QCOM_Datalog::
~QCOM_Datalog()
{
}

bool QCOM_Datalog::
GetSummaryNeeded() const
{
	return SummaryNeeded;
}

// ***************************************************************************** 
// ***************************************************************************** 
// QCOM_DatalogData
// The following class is an extension for the DatalogData that contains common collection
// members used for the QCOM_DatalogMethod data.
class QCOM_DatalogData : public DatalogData {
public:
	QCOM_DatalogData(DatalogMethod::SystemEvents event, QCOM_Datalog &parent);
	virtual ~QCOM_DatalogData() = 0;

	// public methods for reading the members
	bool GetSummaryBySite() const;
	bool GetVerboseEnable() const;
	bool GetDebugEnable() const;
	bool GetAppendPinName() const;
	bool GetUnitAutoscaling() const;
	bool GetASCIIDatalogInColumns() const;
	bool GetEnableFullOpt() const;
	bool GetEnhancedChars() const;
	bool GetScanEnable() const;
	bool GetASCIIOptimizeForUnscaledValues() const;
	const FloatS &GetDlogTime() const;
	const DatalogMethod::SystemEvents GetEvent() const;
        const DatalogMethod::SystemEvents GetLastFormatEvent() const;
	void SetLastFormatEvent();
	PinML &GetVerbosePins();
	void ResetNumTestsExecuted();
	void IncNumTestsExecuted();
	unsigned int GetNumTestsExecuted(SITE site) const;
	void SetFinishTime();
	const FloatS &GetFinishTime() const;
	IntS GetFieldWidth();
        void SetFieldWidth(IntS width);
        StringS GetPassString();
	void SetPassString(StringS pass);
        int  GetIntegerPartWidth() const;

protected:
	// The members will be collected upon construction
	FloatS DlogTime;				// Time a collection
	DatalogMethod::SystemEvents Event;		// Event type
	QCOM_Datalog *Parent;				// My parent object

	void SetSummaryNeeded(bool is_needed);
        void FormatTestDescription(StringS &str, const StringS &user_desc) const;
	STDFV4Stream GetSTDFV4Stream(bool make_private) const;
private:
	QCOM_DatalogData();				// disable default constructor
	QCOM_DatalogData(const QCOM_DatalogData &);	// disable copy
	QCOM_DatalogData &operator=(const QCOM_DatalogData &);	// disable copy
};

QCOM_DatalogData::
QCOM_DatalogData(DatalogMethod::SystemEvents event, QCOM_Datalog &parent) : 
	DatalogData(), 
	DlogTime(RunTime.GetCurrentLocalTime()),
	Event(event),
	Parent(&parent)
{
}

QCOM_DatalogData::
~QCOM_DatalogData()
{
}

bool QCOM_DatalogData::
GetVerboseEnable() const
{
	if (Parent != NULL)
		return Parent -> EnableVerbose.GetValue();
	return false;
}

bool QCOM_DatalogData::
GetDebugEnable() const
{
	if (Parent != NULL)
		return Parent -> EnableDebug.GetValue();
	return false;
}

bool QCOM_DatalogData::
GetAppendPinName() const
{
	if (Parent != NULL)
		return Parent -> AppendPinName.GetValue();
	return true;
}

bool QCOM_DatalogData::
GetUnitAutoscaling() const
{
	if (Parent != NULL)
		return Parent -> UnitAutoscaling.GetValue();
	return false;
}

bool QCOM_DatalogData::
GetASCIIDatalogInColumns() const
{
	if (Parent != NULL)
		return Parent -> ASCIIDatalogInColumns.GetValue();
	return true;
}

bool QCOM_DatalogData::
GetSummaryBySite() const
{
	if (Parent != NULL)
		return Parent -> PerSiteSummary.GetValue();
	return false;
}

bool QCOM_DatalogData::
GetEnableFullOpt() const
{
//	if (Parent != NULL)
//		return Parent -> EnableFullOpt.GetValue();
	return false;
}

bool QCOM_DatalogData::
GetEnhancedChars() const
{
	if (Parent != NULL)
		return Parent -> EnhancedFuncChars.GetValue();
	return false;
}

bool QCOM_DatalogData::
GetScanEnable() const
{
	if (Parent != NULL)
		return Parent -> EnableScan2007.GetValue();
	return false;
}

bool QCOM_DatalogData::
GetASCIIOptimizeForUnscaledValues() const
{
	if (Parent != NULL)
		return Parent -> ASCIIOptimizeForUnscaledValues.GetValue();
	return false;
}

const FloatS &QCOM_DatalogData::
GetDlogTime() const
{
	return DlogTime;
}

const DatalogMethod::SystemEvents QCOM_DatalogData::
GetEvent() const
{
	return Event;
}

void QCOM_DatalogData::
SetSummaryNeeded(bool is_needed)
{
	if (Parent != NULL)
		Parent -> SummaryNeeded = is_needed;
}

IntS QCOM_DatalogData::
GetFieldWidth()
{
	return (Parent != NULL) ? Parent -> FieldWidth : DefaultFieldWidth;
}

void QCOM_DatalogData::
SetFieldWidth(IntS width)
{
	if (Parent != NULL)
		Parent -> FieldWidth = width;
}

StringS QCOM_DatalogData::
GetPassString()
{
	return (Parent != NULL) ? Parent -> PassString : DefaultPassString;
}

void QCOM_DatalogData::
SetPassString(StringS pass)
{
	if (Parent != NULL)
		Parent -> PassString = pass;
}

const DatalogMethod::SystemEvents QCOM_DatalogData::
GetLastFormatEvent() const
{
	return (Parent != NULL) ? Parent -> LastFormatEvent : DatalogMethod::StartTestNode;
}

void QCOM_DatalogData::
SetLastFormatEvent()
{
	if (Parent != NULL)
		Parent -> LastFormatEvent = Event;
}

PinML &QCOM_DatalogData::
GetVerbosePins()
{
	if (Parent != NULL)
		return Parent -> VerbosePins;
	return UTL_VOID;
}

void QCOM_DatalogData::
ResetNumTestsExecuted()
{
	if (Parent != NULL)
		Parent -> NumTestsExecuted = 0;
}

void QCOM_DatalogData::
IncNumTestsExecuted()
{
	if (Parent != NULL) {
		for (SiteIter s1 = GetDlogSites().Begin(); !s1.End(); ++s1)
			++(Parent -> NumTestsExecuted[*s1]);
	}
}

unsigned int QCOM_DatalogData::
GetNumTestsExecuted(SITE site) const
{
	if (Parent != NULL)
		return Parent -> NumTestsExecuted[site];
	return 0;
}

void QCOM_DatalogData::
SetFinishTime()
{
	if (Parent != NULL)
		Parent -> FinishTime = DlogTime;
}

const FloatS &QCOM_DatalogData::
GetFinishTime() const
{
	if (Parent != NULL)
		return Parent -> FinishTime;
	return UTL_VOID;
}

STDFV4Stream QCOM_DatalogData::
GetSTDFV4Stream(bool make_private) const
{
	if (Parent != NULL)
		return Parent -> GetSTDFV4Stream(make_private);
	return STDFV4Stream::NullSTDFV4Stream;
}

void QCOM_DatalogData::
FormatTestDescription(StringS &str, const  StringS &user_info) const
{
	if (user_info.Valid() && (user_info.Length() > 0)) {
		str = user_info;
		return;
	}
	if (Parent != NULL) {
		const StringS &TN = GetTestName();
		if (TN.Valid() && (TN.Length() > 0)) {
			str = TN;
			const StringS &BN = GetBlockName();
			if (BN.Valid() && (BN.Length() > 0)) {
				str += "/";
				str += BN;
			}
			return;
		}
	}
	str = user_info;
}

int QCOM_DatalogData::GetIntegerPartWidth() const
{
    return (GetASCIIOptimizeForUnscaledValues() ? IntegerPartWidthUnscaled : IntegerPartWidthScaled);
}

// ***************************************************************************** 
// StartOfTest

class StartOfTestData : public QCOM_DatalogData {
public:
	StartOfTestData(QCOM_Datalog &);
	~StartOfTestData();

	virtual void Format(const char *format, bool fail_only_mode, std::ostream &output);
private:
	Sites SelSites;
	StringS TesterType;
	void FormatASCII(bool fail_only_mode, std::ostream &output);
	void FormatSTDFV4(bool fail_only_mode, std::ostream &output);
};

StartOfTestData::
StartOfTestData(QCOM_Datalog &parent) : 
	QCOM_DatalogData(DatalogMethod::StartOfTest, parent),
	SelSites(SelectedSites),
	TesterType(SYS.GetTestHeadType())
{
	ResetNumTestsExecuted();
}

StartOfTestData::
~StartOfTestData()
{
}

void StartOfTestData::
Format(const char *format, bool fail_only_mode, std::ostream &output)
{
	if (format != NULL) {
		if (format[0] == formats[ASCII_INDEX][0])
			FormatASCII(fail_only_mode, output);
		else if (format[0] == formats[STDFV4_INDEX][0])
			FormatSTDFV4(fail_only_mode, output);
		SetLastFormatEvent();
	}
}

void StartOfTestData::
FormatASCII(bool fail_only_mode, std::ostream &output)
{
	output << endl << endl;
}

static char GetCode(const char *field)
{
	if (field != NULL) {
		StringS str = TestProg.GetLotInfo(field);
		if (str.Length() > 0)
			return str[0];
	}
	return ' ';
}

void StartOfTestData::
FormatSTDFV4(bool fail_only_mode, std::ostream &output)
{
	STDFV4Stream STDF = GetSTDFV4Stream(false);
	if (STDF.Valid()) {
		STDFV4_PIR PIR;
		for (SiteIter s1 = SelSites.Begin(); !s1.End(); ++s1) {
			PIR.SetInfo(*s1);
			STDF.Write(PIR);
		}
	}
}

DatalogData *QCOM_Datalog::
StartOfTest(const DatalogBaseUserData *)
{
	SummaryNeeded = true;
	return new StartOfTestData(*this);
}

// ***************************************************************************** 
// EndOfTest

class EndOfTestData : public QCOM_DatalogData {
public:
	EndOfTestData(QCOM_Datalog &);
	~EndOfTestData();

	virtual void Format(const char *format, bool fail_only_mode, std::ostream &output);
private:
	EndOfTestStruct EOT;
	bool Valid;
	Sites SelSites;
	void FormatASCII(bool fail_only_mode, std::ostream &output);
	void FormatSTDFV4(bool fail_only_mode, std::ostream &output);
};

EndOfTestData::
EndOfTestData(QCOM_Datalog &parent) : 
	QCOM_DatalogData(DatalogMethod::EndOfTest, parent),
	Valid(false),
	EOT(),
	SelSites(SelectedSites)
{
	Valid = RunTime.GetEndOfTestData(EOT);
	SetFinishTime();
}

EndOfTestData::
~EndOfTestData()
{
}

void EndOfTestData::
Format(const char *format, bool fail_only_mode, std::ostream &output)
{
	if (format != NULL) {
		if (format[0] == formats[ASCII_INDEX][0])
			FormatASCII(fail_only_mode, output);
		else if (format[0] == formats[STDFV4_INDEX][0])
			FormatSTDFV4(fail_only_mode, output);
		SetLastFormatEvent();
	}
}

static bool IsSelectedSite(SITE site)
// Checks to see if the site passed in is in the SelectedSites list for this run
{
	for (SiteIter s1 = SelectedSites.Begin(); !s1.End(); ++s1) {
		if (*s1 == site) return true;
	}
	return false;
}

static void OutputBorder(std::ostream &output, int len, int space)
{
	char buff[1024];
	memset(buff, '-', len);
	buff[len] = '\0';
	output << buff;
	if (space > 0)
	output << setw(space) << " ";
}

void EndOfTestData::
FormatASCII(bool fail_only_mode, std::ostream &output)
{
	output << endl;
	if (GetASCIIDatalogInColumns()) {
		// This section for column-oriented output
		const int field_width = GetFieldWidth();

		OutputBorder(output, 12, 0);
		OutputBorder(output, field_width, 2);
		// iterate over LoadedSites to make sure of a consistent number of columns in the output
		for (SiteIter s1 = LoadedSites.Begin(); !s1.End(); ++s1) {
			OutputBorder(output, field_width+4, 2);
		}
		output << endl;

		output << setw(12 + field_width) << "Device Results" << setw(2) << " ";
		for (SiteIter s1 = LoadedSites.Begin(); !s1.End(); ++s1) {
			output << left << "Site_" << setw(4) << left << s1.GetValue() << setw(field_width+4-9) << left << " " << setw(2) << " ";
		}
		if (EOT.Retest)
			output << "RETEST";

		output << endl;
		
		OutputBorder(output, 12, 0);
		OutputBorder(output, field_width, 2);
		for (SiteIter s1 = LoadedSites.Begin(); !s1.End(); ++s1) {
			OutputBorder(output, field_width+4, 2);
		}
		output << endl;

		output << setw(12+field_width) << left << " Pass/Fail" << setw(2) << " ";
		for (SiteIter s1 = LoadedSites.Begin(); !s1.End(); ++s1) {
			StringS PF = " ";
			if (IsSelectedSite(*s1)) {
				PF = (EOT.Results[*s1] == true) ? "PASS " : "*FAIL*";
			}
			output << setw(field_width+3) << right << PF << setw(3) << " ";
		}
		output << endl;

		output << setw(12+field_width) << left << " Bin Name" << setw(2) << " ";
		for (SiteIter s1 = LoadedSites.Begin(); !s1.End(); ++s1) {
			if (IsSelectedSite(*s1)) {
				StringS bin_text = EOT.BinNames[*s1];
				output << setw(field_width+4) << left << bin_text.Substring(0,field_width+4) << setw(2) << " ";
			} else
				output << setw(field_width+4) << " " << setw(2) << " ";
		}
		output << endl;

		output << setw(12+field_width) << left << " Serial Number" << setw(2) << " ";
		for (SiteIter s1 = LoadedSites.Begin(); !s1.End(); ++s1) {
			if (IsSelectedSite(*s1)) {
				output << setw(field_width+2) << right << EOT.SerialNumbers[*s1] << setw(4) << " ";
			} else
				output << setw(field_width+4) << " " << setw(2) << " ";
		}
		output << endl;

		if (EOT.XCoord[SelectedSites.Begin().GetValue()] > UTL_NO_WAFER_COORD) {
			output << setw(12+field_width) << left << " Wafer X-Coordinate" << setw(2) << " ";
			for (SiteIter s1 = LoadedSites.Begin(); !s1.End(); ++s1) {
				if (IsSelectedSite(*s1)) {
					output << setw(field_width+2) << right << EOT.XCoord[*s1] << setw(4) << " ";
				} else
					output << setw(field_width+4) << " " << setw(2) << " ";
			}
			output << endl;

			output << setw(12+field_width) << left << " Wafer Y-coordinate" << setw(2) << " ";
			for (SiteIter s1 = LoadedSites.Begin(); !s1.End(); ++s1) {
				if (IsSelectedSite(*s1)) {
					output << setw(field_width+2) << right << EOT.YCoord[*s1] << setw(4) << " ";
				} else
					output << setw(field_width+4) << " " << setw(2) << " ";
			}
			output << endl;
		}

		output << setw(12+field_width) << left << " Software Bin Number" << setw(2) << " ";
		for (SiteIter s1 = LoadedSites.Begin(); !s1.End(); ++s1) {
			if (IsSelectedSite(*s1)) {
				int sw_bin = EOT.SoftwareBinNumbers[*s1];
				if (sw_bin < 0)
					output << setw(field_width+2) << right << "Not Binned" << setw(4) << " ";
				else
					output << setw(field_width+2) << right << sw_bin << setw(4) << " ";
			} else
				output << setw(field_width+4) << " " << setw(2) << " ";
		}
		output << endl;

		output << setw(12+field_width) << left << " Hardware Bin Number" << setw(2) << " ";
		for (SiteIter s1 = LoadedSites.Begin(); !s1.End(); ++s1) {
			if (IsSelectedSite(*s1)) {
				output << setw(field_width+2) << right << EOT.HardwareBinNumbers[*s1] << setw(4) << " ";
			} else
				output << setw(field_width+4) << " " << setw(2) << " ";
		}
		output << endl;

		output << setw(12+field_width) << left << " Test Time" << setw(2) << " ";
		for (SiteIter s1 = LoadedSites.Begin(); !s1.End(); ++s1) {
			if (IsSelectedSite(*s1)) {
				output << setw(field_width+1) << fixed << setprecision(6) << right << EOT.TestTimes[*s1] << "s" << setw(4) << " ";
			} else
				output << setw(field_width+4) << " " << setw(2) << " ";
		}
		output << endl;
		output << setw(12+field_width) << left << " Total Tests Executed" << setw(2) << " ";
		for (SiteIter s1 = LoadedSites.Begin(); !s1.End(); ++s1) {
			if (IsSelectedSite(*s1)) {
				output << setw(field_width+2) << right << GetNumTestsExecuted(*s1) << setw(4) << " ";
			} else
				output << setw(field_width+4) << " " << setw(2) << " ";
		}
		output << endl;

		output << setw(12+field_width) << left << " Part Description" << setw(2) << " ";
		for (SiteIter s1 = LoadedSites.Begin(); !s1.End(); ++s1) {
			if (IsSelectedSite(*s1)) {
				StringS part_text = EOT.PartTexts[*s1];
				output << setw(field_width+4) << left << part_text.Substring(0,field_width+4) << setw(2) << " ";
			} else
				output << setw(field_width+4) << " " << setw(2) << " ";
		}
		output << endl;

		OutputBorder(output, 12, 0);
		OutputBorder(output, field_width, 2);
		for (SiteIter s1 = LoadedSites.Begin(); !s1.End(); ++s1) {
			OutputBorder(output, field_width+4, 2);
		}
		output << endl;

	} else {
		// This section for row-oriented output
		output << endl;
		output << "  Site  Device ID       X Coord  Y Coord   P/F  SW Bin No.  HW Bin No.  Test Time      Test Count  Status  Device Description" << endl;
		output << "  ----  ---------       -------  -------   ---  ----------  ----------  -------------  ----------  ------  ------------------" << endl;
		for (SiteIter s1 = SelSites.Begin(); !s1.End(); ++s1) {
			output << "  " << fixed << setw(4) << right << *s1 << "  " << setw(9) << EOT.SerialNumbers[*s1] << "       ";
			if (EOT.XCoord[*s1] > UTL_NO_WAFER_COORD)
				output << fixed << setw(7) << right << EOT.XCoord[*s1] << "  " << setw(7) << right << EOT.YCoord[*s1] << "   ";
			else
				output << "                   ";
			StringS PF = "   ";
			if (EOT.Results[*s1] != UTL_VOID)
				PF = (EOT.Results[*s1] == true) ? " P " : " F ";
			if (EOT.SoftwareBinNumbers[*s1] >= 0)
				output << PF << "  " << fixed << setw(10) << right << EOT.SoftwareBinNumbers[*s1] << "  " << setw(10) << right << EOT.HardwareBinNumbers[*s1] << "  ";
			else
				output << PF << "              " << fixed << setw(10) << right << EOT.HardwareBinNumbers[*s1] << "  ";
			if (EOT.TestTimes[*s1] != UTL_VOID)
				output << setw(12) << fixed << setprecision(6) << right << EOT.TestTimes[*s1] << "s" << "  ";
			else
				output << setw(10) << right << " " << "  ";
			output << fixed << setw(10) << right << GetNumTestsExecuted(*s1) << "  ";
			if (EOT.Retest)
				output << "RETEST";
			else
				output << "      ";
			output << "  " << left << EOT.PartTexts[*s1] << endl;
		}
	}
}

void EndOfTestData::
FormatSTDFV4(bool fail_only_mode, std::ostream &output)
{
	STDFV4Stream STDF = GetSTDFV4Stream(false);
	if (STDF.Valid()) {
	 	STDFV4_PRR PRR;
		StringS SNStr;
		for (SiteIter s1 = SelSites.Begin(); !s1.End(); ++s1) {
			PRR.SetResult(EOT.Results[*s1] == UTL_VOID ? false : true, EOT.Results[*s1], EOT.Retest ? STDFV4_PRR::REPLACE : STDFV4_PRR::NEW_PART, *s1);
			PRR.SetInfo(EOT.OverallTestTime, GetNumTestsExecuted(*s1), EOT.HardwareBinNumbers[*s1], EOT.SoftwareBinNumbers[*s1],
	                            EOT.SerialNumbers[*s1].GetText(), EOT.PartTexts[*s1], EOT.XCoord[*s1], EOT.YCoord[*s1]);
			STDF.Write(PRR);
		}
	}
}

DatalogData *QCOM_Datalog::
EndOfTest(const DatalogBaseUserData *)
{
	SummaryNeeded = true;
	return new EndOfTestData(*this);
}

// ***************************************************************************** 
// ProgramLoad

DatalogData *QCOM_Datalog::
ProgramLoad(const DatalogBaseUserData *)
{
	return NULL;		// Not used  by QCOM_Datalog
}

// ***************************************************************************** 
// ProgramUnload

DatalogData *QCOM_Datalog::
ProgramUnload(const DatalogBaseUserData *)
{
	if (SummaryNeeded)		// insure my summary has been processed
		DoAction(GetSystemEventName(DatalogMethod::Summary));
	return NULL;
}

// ***************************************************************************** 
// ProgramReset

class ProgramResetData : public QCOM_DatalogData {
public:
	ProgramResetData(QCOM_Datalog &);
	~ProgramResetData();

	virtual void Format(const char *format, bool fail_only_mode, std::ostream &output);
private:
	EndOfTestStruct EOT;
	bool Valid;
	Sites SelSites;
	void FormatASCII(bool fail_only_mode, std::ostream &output);
	void FormatSTDFV4(bool fail_only_mode, std::ostream &output);
};

ProgramResetData::
ProgramResetData(QCOM_Datalog &parent) : 
	QCOM_DatalogData(DatalogMethod::ProgramReset, parent),
	Valid(false),
	EOT(),
	SelSites(SelectedSites)
{
	Valid = RunTime.GetEndOfTestData(EOT);
	SetFinishTime();
}

ProgramResetData::
~ProgramResetData()
{
}

void ProgramResetData::
Format(const char *format, bool fail_only_mode, std::ostream &output)
{
	if (format != NULL) {
		if (format[0] == formats[ASCII_INDEX][0])
			FormatASCII(fail_only_mode, output);
		else if (format[0] == formats[STDFV4_INDEX][0])
			FormatSTDFV4(fail_only_mode, output);
		SetLastFormatEvent();
	}
}

void ProgramResetData::
FormatASCII(bool fail_only_mode, std::ostream &output)
{
	output << endl << endl << "*** Test program RESET was executed! ***";
	output << endl << endl << "Device Results:" << endl;
	output << "  Site  Device ID       X Coord  Y Coord   P/F  SW Bin No.  HW Bin No.  Test Count  Status  Device Description" << endl;
	output << "  ----  ---------       -------  -------   ---  ----------  ----------  ----------  ------  ------------------" << endl;
	for (SiteIter s1 = SelSites.Begin(); !s1.End(); ++s1) {
		output << "  " << fixed << setw(4) << right << *s1 << "  " << setw(9) << EOT.SerialNumbers[*s1] << "       ";
		if (EOT.XCoord[*s1] > UTL_NO_WAFER_COORD)
			output << fixed << setw(7) << right << EOT.XCoord[*s1] << "  " << setw(7) << right << EOT.YCoord[*s1] << "   ";
		else
			output << "                   ";
		StringS PF = "   ";
		if (EOT.Results[*s1] != UTL_VOID)
			PF = (EOT.Results[*s1] == true) ? " P " : " F ";
		if (EOT.SoftwareBinNumbers[*s1] >= 0)
			output << PF << "  " << fixed << setw(10) << right << EOT.SoftwareBinNumbers[*s1] << "  " << setw(10) << right << EOT.HardwareBinNumbers[*s1] << "  ";
		else
			output << PF << "              " << fixed << setw(10) << right << EOT.HardwareBinNumbers[*s1] << "  ";
		output << fixed << setw(10) << right << GetNumTestsExecuted(*s1) << "  ";
		if (EOT.Retest)
			output << "RETEST";
		else
			output << "      ";
		output << "  " << left << EOT.PartTexts[*s1] << endl;
	}
}

void ProgramResetData::
FormatSTDFV4(bool fail_only_mode, std::ostream &output)
{
	STDFV4Stream STDF = GetSTDFV4Stream(false);
	if (STDF.Valid()) {
		STDFV4_PRR PRR;
		StringS SNStr;
		for (SiteIter s1 = SelSites.Begin(); !s1.End(); ++s1) {
			// force bad result
			PRR.SetResult(false, EOT.Results[*s1], EOT.Retest ? STDFV4_PRR::REPLACE : STDFV4_PRR::NEW_PART, *s1);
			PRR.SetInfo(	EOT.TestTimes[*s1], GetNumTestsExecuted(*s1), EOT.HardwareBinNumbers[*s1], EOT.SoftwareBinNumbers[*s1],
					EOT.SerialNumbers[*s1].GetText(), EOT.PartTexts[*s1], EOT.XCoord[*s1], EOT.YCoord[*s1]);
			STDF.Write(PRR);
		}
	}
}

DatalogData *QCOM_Datalog::
ProgramReset(const DatalogBaseUserData *)
{
	SummaryNeeded = true;
	return new ProgramResetData(*this);
}

// ***************************************************************************** 
// Summary

class SummaryData : public QCOM_DatalogData {
public:
	SummaryData(QCOM_Datalog &, bool is_final = false, bool file_closing_after_summary = false);
	~SummaryData();

	virtual void Format(const char *format, bool fail_only_mode, std::ostream &output);
	void BinASCII(SITE site) const;
private:
	bool IsFinalSummary;
        bool FileClosingAfterSummary;
	bool Valid;
	bool TSRValid;
	// Lot Info
	StringS FileName;
	StringS ProgName;
	StringS UserName;
	StringS DUTName;
	StringS LotID;
	StringS SublotID;
	StringS LotStat;
	StringS LotType;
	StringS LotDesc;
	StringS ProdID;
	StringS WaferID;
	StringS FabID;
	StringS LotStart;
	StringS Shift;
	StringS Operator;
	StringS TesterName;
	StringS FlowName;
	StringS DIBName;
    StringS LimitTableName;
	StringS CurLocalTime;
	StringS CurGMTime;
	StringS PHName;
	StringS SumHdr;
	StringS TestMode;
	// Bin Info
	BinInfoArrayStruct BinInfo;
	HWBinInfoArrayStruct HWBinInfo;
	// Pass/Fail totals
	BinCountStruct Passes;
	BinCountStruct Fails;
	TSRInfoStruct TSRInfo;
	

	void FormatASCII(bool fail_only_mode, std::ostream &output);
	void FormatSTDFV4(bool fail_only_mode, std::ostream &output);
};

SummaryData::
SummaryData(QCOM_Datalog &parent, bool is_final, bool file_closing_after_summary) : 
	QCOM_DatalogData(DatalogMethod::Summary, parent),
	IsFinalSummary(is_final),
        FileClosingAfterSummary(file_closing_after_summary),
	Valid(false),
	BinInfo(),
	HWBinInfo(),
	Passes(),
	Fails(),
	FileName(),
	ProgName(),
	UserName(),
	DUTName(),
	LotID(),
	SublotID(),
	LotStat(),
	LotType(),
	LotDesc(),
	ProdID(),
	WaferID(),
	FabID(),
	LotStart(),
	Shift(),
	Operator(),
	TesterName(),
	FlowName(),
	DIBName(),
    LimitTableName(),
	CurLocalTime(),
	CurGMTime(),
	PHName(),
	SumHdr(),
	TestMode()
{
	Valid = RunTime.GetBinInfo(BinInfo, Passes, Fails);
 	if (Valid) {
		(void) RunTime.GetBinInfo(HWBinInfo);
		FileName = TestProg.GetLotInfo("TestProgFileName");
		ProgName = TestProg.GetLotInfo("ProgramName");
		UserName = TestProg.GetLotInfo("UserName");
		DUTName = TestProg.GetLotInfo("DeviceName");
		LotID = TestProg.GetLotInfo("LotID");
		SublotID = TestProg.GetLotInfo("SubLotID");
		LotStat = TestProg.GetLotInfo("LotStatus");
		LotType = TestProg.GetLotInfo("LotType");
		LotDesc = TestProg.GetLotInfo("LotDescription");
		ProdID = TestProg.GetLotInfo("ProductID");
		WaferID = TestProg.GetLotInfo("WaferID");
		FabID = TestProg.GetLotInfo("FabID");
		LotStart = TestProg.GetLotInfo("LotStartTime");
		Operator = TestProg.GetLotInfo("OperatorID");
		TesterName = TestProg.GetLotInfo("TesterName");
		FlowName = TestProg.GetLotInfo("ActiveFlow");
		DIBName = TestProg.GetLotInfo("ActiveLoadBoard");
        LimitTableName = TestProg.GetActiveLimitTable().GetName();
		PHName = TestProg.GetLotInfo("ProberHander");
		SumHdr = TestProg.GetLotInfo("SummaryHeader");
		CurLocalTime = TestProg.GetCurrentLocalTime();
		CurGMTime = TestProg.GetCurrentGMTime();

                TestMode = TestProg.GetLotInfo("TestMode");
                if (TestMode.Length() == 0)
                        TestMode = (RunTime.GetCurrentExecutionMode() == ILQA_EXECUTION ? "QA" : "Production");
	}
 	RunTime.GetTSRInformation(TSRInfo);
	TSRValid = (TSRInfo.TestNum.GetSize() > 0) ? true : false;
}

SummaryData::
~SummaryData()
{
}

void SummaryData::
Format(const char *format, bool fail_only_mode, std::ostream &output)
{
	if (format != NULL) {
		if (format[0] == formats[ASCII_INDEX][0])
			FormatASCII(fail_only_mode, output);
		else if (format[0] == formats[STDFV4_INDEX][0])
			FormatSTDFV4(fail_only_mode, output);
		SetLastFormatEvent();
	}
}

static void OutputSummaryBinHeader(std::ostream &output)
{
}

void SummaryData::
FormatASCII(bool fail_only_mode, std::ostream &output)
{
	bool SummaryBySite = GetSummaryBySite();
	if (IsFinalSummary)
		output << endl << right << setw(50) << "FINAL SUMMARY" << endl << endl;
	else
		output << endl << right << setw(50) << "SUBLOT SUMMARY" << endl << endl;
	output << left << setw(18) << "FILE NAME" << ": " << FileName << endl;
	output << left << setw(18) << "PROGRAM NAME" << ": " << ProgName << endl;
	output << left << setw(18) << "USER NAME" << ": " << UserName << endl;
	output << left << setw(18) << "DEVICE NAME" << ": " << DUTName << endl;
	output << left << setw(18) << "LOT ID" << ": " << LotID << endl;
	output << left << setw(18) << "SUBLOT ID" << ": " << SublotID << endl;
	output << left << setw(18) << "LOT STATUS" << ": " << LotStat << endl;
	output << left << setw(18) << "LOT TYPE" << ": " << LotType << endl;
	output << left << setw(18) << "LOT DESCRIPTION" << ": " << LotDesc << endl;
	output << left << setw(18) << "PRODUCT ID" << ": " << ProdID << endl;
	output << left << setw(18) << "WAFER ID" << ": " << WaferID << endl;
	output << left << setw(18) << "FAB ID" << ": " << FabID << endl;
	output << left << setw(18) << "LOT START TIME" << ": " << LotStart << endl;
	output << left << setw(18) << "SHIFT" << ": " << Shift << endl;
	output << left << setw(18) << "OPERATOR ID" << ": " << Operator << endl;
	output << left << setw(18) << "TESTER NAME" << ": " << TesterName << endl;
	output << left << setw(18) << "ACTIVE FLOW" << ": " << FlowName << endl;
	output << left << setw(18) << "ACTIVE LOADBOARD" << ": " << DIBName << endl;
    output << left << setw(18) << "ACTIVE LIMITTABLE" << ": " << LimitTableName << endl;
	output << left << setw(18) << "LOCAL TIME" << ": " << CurLocalTime << endl;
	output << left << setw(18) << "GM TIME" << ": " << CurGMTime << endl;
	output << left << setw(18) << "PROBER/HANDLER" << ": " << PHName << endl;
	output << left << setw(18) << "HEADER" << ": " << SumHdr << endl;
        output << left << setw(18) << "MODE" << ": " << TestMode << endl;
	output << endl ;
						// output TSR summary if enabled
	if (TSRValid) {
		int num_recs = TSRInfo.TestNum.GetSize();
						// output the TSR header
		output << "Test Result Summary:" << endl;
		output << setw(TNSize) << left << "Test_No." << "  Site  Type  Executions  Failures    % Passed  Test_Description" << endl;
		OutputBorder(output, TNSize, 2);		// Test number
		OutputBorder(output, 4, 2);			// Site
		OutputBorder(output, 4, 2);			// Type
		OutputBorder(output, 10, 2);			// Executions
		OutputBorder(output, 10, 2);			// Failures
		OutputBorder(output, 8, 2);			// % Passed
		OutputBorder(output, TDSize, 0);		// Test Description
		output << endl;
					// output the TSR information
		int ii = 0;
		if (SummaryBySite) {
			for (SiteIter s1 = LoadedSites.Begin(); !s1.End(); ++s1) {
				SITE site = *s1;
				for (ii = 0; ii < num_recs; ii++) {
					output << setw(TNSize) << right << dec << TSRInfo.TestNum[ii] << "  ";
					output << setw(4) << right << site << "  ";
					output << "  " << TSRInfo.TestType[ii][0] << "   ";
					output << setw(10) << right << dec << TSRInfo.NumTested[site][ii] << "  ";
					output << setw(10) << right << dec << TSRInfo.NumFails[site][ii] << "  ";
					double PC = TSRInfo.PassPercent[site][ii] * 100.0;
					output << setw(7) << fixed << setprecision(2) << PC << "%  ";
					StringS test_text = TSRInfo.TestText[ii];
					
					if (GetAppendPinName()) DatalogData::AppendPinNameToTestText(TSRInfo.PinName[ii], test_text);
					    
					output << left << test_text << endl;
				}
			}
		}
		for (ii = 0; ii < num_recs; ii++) {
			unsigned int NTested = 0;
			unsigned int NFails = 0;
			SITE first_site = NO_SITES;
			for (SiteIter s1 = LoadedSites.Begin(); !s1.End(); ++s1) {
				if (first_site == NO_SITES)
					first_site = *s1;
				NTested += TSRInfo.NumTested[*s1][ii];
				NFails += TSRInfo.NumFails[*s1][ii];
			}
			output << setw(TNSize) << right << dec << TSRInfo.TestNum[ii] << "  ";
			output << setw(4) << right << "All" << "  ";
			output << "  " << TSRInfo.TestType[ii][0] << "   ";
			output << setw(10) << right << dec << NTested << "  ";
			output << setw(10) << right << dec << NFails << "  ";
			double PC = (NTested > 0) ? ((double(NTested) - double(NFails)) / double(NTested)) * 100.0 : 0.0;
			output << setw(7) << fixed << setprecision(2) << PC << "%  ";
			StringS test_text = TSRInfo.TestText[ii];
			
			if (GetAppendPinName()) DatalogData::AppendPinNameToTestText(TSRInfo.PinName[ii], test_text);
			
			output << left << test_text << endl;
		}
		output << endl;
	}
						// Bin summary
	output << "Bin Summary:" << endl;
	output << "Site  " << left << setw(4+2) << "P/F" << setw(5+2) << "SWBin" << setw(5+2) << "HWBin" << setw(34+2) << "Bin Name" << setw(10+2) << "Count" << setw(10) << "Percent" << endl;
	OutputBorder(output, 4, 2);
	OutputBorder(output, 4, 2);
	OutputBorder(output, 5, 2);
	OutputBorder(output, 5, 2);
	OutputBorder(output, 34, 2);
	OutputBorder(output, 10, 2);
	OutputBorder(output, 10, 0);
	output << endl;
						// output per site Bin counts
	if (SummaryBySite) {
		for (SiteIter s1 = LoadedSites.Begin(); !s1.End(); ++s1) {
			SITE site = *s1;
			int site_total = (IsFinalSummary) ? (Passes.FinalSiteCount[site] + Fails.FinalSiteCount[site]) : (Passes.SiteCount[site] + Fails.SiteCount[site]);
			if (site_total > 0) {
				for (int ii = 0; ii < BinInfo.NumBins; ii++) {
					unsigned int bcount = (IsFinalSummary) ? BinInfo.FinalSiteCount[site][ii] : BinInfo.SiteCount[site][ii];
					if (bcount > 0) {
						output << right << setw(4) << site << "  ";
						output << left << setw(4) << BinInfo.Description[ii] << "  ";
						output << right << setw(5) << BinInfo.SWBinNumber[ii] << "  ";
						output << right << setw(5) << BinInfo.HWBinNumber[ii] << "  ";
						output << left << setw(34) << BinInfo.BinName[ii] << "  ";
						output << right << setw(10) << bcount << "  ";
						double PC = (double(bcount) / double(site_total)) * 100.0;
						output << right << setw(9) << fixed << setprecision(3) << PC << "%" << endl;
					}
				}
				OutputBorder(output, 78, 0);
				output << endl << endl;
			}
		}
	}
						// output total Bin counts
	int total = (IsFinalSummary) ? (Passes.FinalCount + Fails.FinalCount) : (Passes.Count + Fails.Count);
	for (int ii = 0; ii < BinInfo.NumBins; ii++) {
		if (BinInfo.BinName[ii].Length() > 0) {
			unsigned int bcount = (IsFinalSummary) ? BinInfo.FinalCount[ii] : BinInfo.Count[ii];
			output << " ALL  ";
			output << left << setw(4) << BinInfo.Description[ii] << "  ";
			output << right << setw(5) << BinInfo.SWBinNumber[ii] << "  ";
			output << right << setw(5) << BinInfo.HWBinNumber[ii] << "  ";
			output << left << setw(34) << BinInfo.BinName[ii] << "  ";
			output << right << setw(10) << bcount << "  ";
			double PC = ((total > 0) && (bcount > 0)) ? (double(bcount) / double(total)) * 100.0 : 0.0;
			output << right << setw(9) << fixed << setprecision(3) << PC << "%" << endl;
		}
	}
	OutputBorder(output, 78, 0);
	output << endl << endl;

	output << "Hardware Bin Summary:" << endl;
	output << "Site  " << left << setw(4+2) << "P/F" << setw(5+2) << "HWBin" << setw(34+2) << "HW Bin Name" << setw(10+2) << "Count" << setw(10) << "Percent" << endl;
	OutputBorder(output, 4, 2);
	OutputBorder(output, 4, 2);
	OutputBorder(output, 5, 2);
	OutputBorder(output, 34, 2);
	OutputBorder(output, 10, 2);
	OutputBorder(output, 10, 0);
	output << endl;
						// output per site Bin counts
	if (SummaryBySite) {
		for (SiteIter s1 = LoadedSites.Begin(); !s1.End(); ++s1) {
			SITE site = *s1;
			int site_total = (IsFinalSummary) ? (Passes.FinalSiteCount[site] + Fails.FinalSiteCount[site]) : (Passes.SiteCount[site] + Fails.SiteCount[site]);
			if (site_total > 0) {
				for (int ii = 0; ii < HWBinInfo.NumBins; ii++) {
					unsigned int bcount = (IsFinalSummary) ? HWBinInfo.FinalSiteCount[site][ii] : HWBinInfo.SiteCount[site][ii];
					if (bcount > 0) {
						output << right << setw(4) << site << "  ";
						output << left << setw(4) << HWBinInfo.Description[ii] << "  ";
						output << right << setw(5) << HWBinInfo.BinNumber[ii] << "  ";
						output << left << setw(34) << HWBinInfo.BinName[ii] << "  ";
						output << right << setw(10) << bcount << "  ";
						double PC = (double(bcount) / double(site_total)) * 100.0;
						output << right << setw(9) << fixed << setprecision(3) << PC << "%" << endl;
					}
				}
				OutputBorder(output, 78, 0);
				output << endl << endl;
			}
		}
	}
						// output total Bin counts
	total = (IsFinalSummary) ? (Passes.FinalCount + Fails.FinalCount) : (Passes.Count + Fails.Count);
	for (int ii = 0; ii < HWBinInfo.NumBins; ii++) {
		if (HWBinInfo.BinNumber[ii] >= 0) {
			unsigned int bcount = (IsFinalSummary) ? HWBinInfo.FinalCount[ii] : HWBinInfo.Count[ii];
			output << " ALL  ";
			output << left << setw(4) << HWBinInfo.Description[ii] << "  ";
			output << right << setw(5) << HWBinInfo.BinNumber[ii] << "  ";
			output << left << setw(34) << HWBinInfo.BinName[ii] << "  ";
			output << right << setw(10) << bcount << "  ";
			double PC = ((total > 0) && (bcount > 0)) ? (double(bcount) / double(total)) * 100.0 : 0.0;
			output << right << setw(9) << fixed << setprecision(3) << PC << "%" << endl;
		}
	}
	OutputBorder(output, 78, 0);
	output << endl << endl;

	output << "Device Count Summary:" << endl;
	output << setw(4) << "Site" << "  " << setw(14) << "Devices Tested" << "  " << setw(14) << "Devices Passed" << "  " << setw(10) << "Percent" << endl;
	OutputBorder(output, 4, 2);
	OutputBorder(output, 14, 2);
	OutputBorder(output, 14, 2);
	OutputBorder(output, 10, 0);
	output << endl;
	if (SummaryBySite) {
		for (SiteIter s1 = LoadedSites.Begin(); !s1.End(); ++s1) {
			SITE site = *s1;
			int site_total = (IsFinalSummary) ? (Passes.FinalSiteCount[site] + Fails.FinalSiteCount[site]) : (Passes.SiteCount[site] + Fails.SiteCount[site]);
			if (site_total > 0) {
				int site_npass = (IsFinalSummary) ? Passes.FinalSiteCount[site] : Passes.SiteCount[site];
				double SPP = ((site_total > 0) && (site_npass > 0)) ? (double(site_npass) / double(site_total)) * 100.0 : 0.0;
				output << right << setw(4) << site << "  " << right << setw(14) << site_total << "  " << right << setw(14) << site_npass << right << setw(9) << fixed << setprecision(3) << SPP << "%" << endl;
			}
		}
	}
	int npass = (IsFinalSummary) ? Passes.FinalCount : Passes.Count;
	double PP = ((total > 0) && (npass > 0)) ? (double(npass) / double(total)) * 100.0 : 0.0;
	output << " ALL  " << right << setw(14) << total << "  " << right << setw(14) << npass << right << setw(9) << fixed << setprecision(3) << PP << "%" << endl;
}

void SummaryData::
FormatSTDFV4(bool fail_only_mode, std::ostream &output)
{
	if (FileClosingAfterSummary == false) return;

	STDFV4Stream STDF = GetSTDFV4Stream(false);
	if (STDF.Valid()) {
		bool SummaryBySite = GetSummaryBySite();
		int num_sites = LoadedSites.GetNumSites();
		if (TSRValid) {
			// Write TSR record
			STDFV4_TSR TSR;
			int num_recs = TSRInfo.TestNum.GetSize();
			vector<unsigned int> num_tested(num_recs, 0);
			vector<unsigned int> num_fails(num_recs, 0);
			vector<double> min_val(num_recs, 1e100);
			vector<double> max_val(num_recs, -1e100);
			vector<double> sums(num_recs, 0.0);
			vector<double> squares(num_recs, 0.0);
			int ii = 0;
			for (SiteIter s1 = LoadedSites.Begin(); !s1.End(); ++s1) {
				SITE site = *s1;
				for (ii = 0; ii < num_recs; ii++) {
					if (TSRInfo.NumTested[site][ii] > 0) {
						num_tested[ii] += TSRInfo.NumTested[site][ii];
						num_fails[ii] += TSRInfo.NumFails[site][ii];
						if (TSRInfo.MinValue[site][ii] < min_val[ii])
							min_val[ii] = TSRInfo.MinValue[site][ii];
						if (TSRInfo.MaxValue[site][ii] > max_val[ii])
							max_val[ii] = TSRInfo.MaxValue[site][ii];
						sums[ii] += TSRInfo.Sums[site][ii];
						squares[ii] += TSRInfo.SumOfSquares[site][ii];
						if (SummaryBySite && (num_sites > 1)) {
							TSR.Reset();
							TSR.SetContext(1, site);
							StringS test_text = TSRInfo.TestText[ii];
			
							if (GetAppendPinName()) DatalogData::AppendPinNameToTestText(TSRInfo.PinName[ii], test_text);
			
							TSR.SetInfo(TSRInfo.TestNum[ii], TSRInfo.TestType[ii][0], UTL_VOID, test_text);
							TSR.SetCounts(TSRInfo.NumTested[site][ii], TSRInfo.NumFails[site][ii]);
							TSR.SetStats(TSRInfo.MinValue[site][ii], TSRInfo.MaxValue[site][ii], TSRInfo.Sums[site][ii], TSRInfo.SumOfSquares[site][ii]);
							STDF.Write(TSR);
						}
					}
				}
			}
			for (ii = 0; ii < num_recs; ii++) {
				if (num_tested[ii] > 0) {
					TSR.Reset();
					TSR.SetContext();
					StringS test_text = TSRInfo.TestText[ii];
					
					if (GetAppendPinName()) DatalogData::AppendPinNameToTestText(TSRInfo.PinName[ii], test_text);
	
					TSR.SetInfo(TSRInfo.TestNum[ii], TSRInfo.TestType[ii][0], UTL_VOID, test_text);
					TSR.SetCounts(num_tested[ii], num_fails[ii]);
					TSR.SetStats(min_val[ii], max_val[ii], sums[ii], squares[ii]);
					STDF.Write(TSR);
				}
			}
		}
		// Write HBR record
		int bn = 0;
		STDFV4_HBR HBR;
		if (SummaryBySite) {
			for (SiteIter s1 = LoadedSites.Begin(); !s1.End(); ++s1) {
				for (bn = 0; bn < HWBinInfo.NumBins; bn++) {
					if (HWBinInfo.BinName[bn].Length() > 0)
						HBR.SetInfo(HWBinInfo.BinNumber[bn], IsFinalSummary ? HWBinInfo.FinalSiteCount[*s1][bn] : HWBinInfo.SiteCount[*s1][bn], HWBinInfo.Description[bn][0], HWBinInfo.BinName[bn], 1, *s1);
					else
						HBR.SetInfo(HWBinInfo.BinNumber[bn], IsFinalSummary ? HWBinInfo.FinalSiteCount[*s1][bn] : HWBinInfo.SiteCount[*s1][bn], HWBinInfo.Description[bn][0], UTL_VOID, 1, *s1);
					STDF.Write(HBR);
				}
			}
		}
		for (bn = 0; bn < HWBinInfo.NumBins; bn++) {
			if (HWBinInfo.BinName[bn].Length() > 0)
				HBR.SetInfo(HWBinInfo.BinNumber[bn], IsFinalSummary ? HWBinInfo.FinalCount[bn] : HWBinInfo.Count[bn], HWBinInfo.Description[bn][0], HWBinInfo.BinName[bn]);
			else
				HBR.SetInfo(HWBinInfo.BinNumber[bn], IsFinalSummary ? HWBinInfo.FinalCount[bn] : HWBinInfo.Count[bn], HWBinInfo.Description[bn][0], UTL_VOID);
			STDF.Write(HBR);
		}
		// Write SBR record
		STDFV4_SBR SBR;
		if (SummaryBySite) {
			for (SiteIter s1 = LoadedSites.Begin(); !s1.End(); ++s1) {
				for (bn = 0; bn < BinInfo.NumBins; bn++) {
					SBR.SetInfo(BinInfo.SWBinNumber[bn], IsFinalSummary ? BinInfo.FinalSiteCount[*s1][bn] : BinInfo.SiteCount[*s1][bn], BinInfo.Description[bn][0], BinInfo.BinName[bn], 1, *s1);
					STDF.Write(SBR);
				}
			}
		}
		for (bn = 0; bn < BinInfo.NumBins; bn++) {
			SBR.SetInfo(BinInfo.SWBinNumber[bn], IsFinalSummary ? BinInfo.FinalCount[bn] : BinInfo.Count[bn], BinInfo.Description[bn][0], BinInfo.BinName[bn]);
			STDF.Write(SBR);
		}
		// Write PCR record
		STDFV4_PCR PCR;
		if (SummaryBySite) {
			for (SiteIter s1 = LoadedSites.Begin(); !s1.End(); ++s1) {
				PCR.SetInfo(IsFinalSummary ? Passes.FinalSiteCount[*s1] + Fails.FinalSiteCount[*s1] : Passes.SiteCount[*s1] + Fails.SiteCount[*s1], IsFinalSummary ? Passes.FinalSiteCount[*s1] : Passes.SiteCount[*s1], UTL_VOID, UTL_VOID, UTL_VOID, 1, *s1);
				STDF.Write(PCR);
			}
		}
		PCR.SetInfo(IsFinalSummary ? Passes.FinalCount + Fails.FinalCount : Passes.Count + Fails.Count, IsFinalSummary ? Passes.FinalCount : Passes.Count);
		STDF.Write(PCR);
		// Write MRR record
		STDFV4_MRR MRR;
		StringS disp_code = TestProg.GetLotInfo("DispCode");
		MRR.SetInfo(GetFinishTime(), (disp_code.Length() > 0) ? disp_code[0] : ' ', TestProg.GetLotInfo("LotDescription"), TestProg.GetLotInfo("ExecDescription"));
		STDF.Write(MRR);
	}
}

DatalogData *QCOM_Datalog::
Summary(const DatalogBaseUserData *udata)
{
	const DatalogSummaryInfo *sdata = dynamic_cast<const DatalogSummaryInfo *>(udata);
	SummaryNeeded = false;
	bool DoFinal = (sdata != NULL) ? (sdata -> GetPartialSummary() ? false : true) : false;
        bool FileClosingAfterSummary = sdata ? sdata->GetFileClosingAfterSummary() : false;
	return new SummaryData(*this, DoFinal, FileClosingAfterSummary);
}

// ***************************************************************************** 
// StartOfWafer

class StartOfWaferData : public QCOM_DatalogData {
public:
	StartOfWaferData(QCOM_Datalog &);
	~StartOfWaferData();

	virtual void Format(const char *format, bool fail_only_mode, std::ostream &output);
private:
	bool Valid;
	WaferMap WMap;
	StringS WaferID;

	void FormatASCII(bool fail_only_mode, std::ostream &output);
	void FormatSTDFV4(bool fail_only_mode, std::ostream &output);
};

StartOfWaferData::
StartOfWaferData(QCOM_Datalog &parent) : 
	QCOM_DatalogData(DatalogMethod::StartOfWafer, parent),
	Valid(false),
	WMap(TestProg.GetActiveWaferMap()),
	WaferID(TestProg.GetLotInfo("WaferID"))
{
	Valid = (WMap.Valid() && (WaferID.Length() > 0));
}

StartOfWaferData::
~StartOfWaferData()
{
}

void StartOfWaferData::
Format(const char *format, bool fail_only_mode, std::ostream &output)
{
	if (format != NULL) {
		if (format[0] == formats[ASCII_INDEX][0])
			FormatASCII(fail_only_mode, output);
		else if (format[0] == formats[STDFV4_INDEX][0])
			FormatSTDFV4(fail_only_mode, output);
		SetLastFormatEvent();
	}
}

static const StringS FormatTime(const FloatS &time)
{
	if (time != UTL_VOID) {
		char buff[8192];
		time_t wtime = time;
		struct tm tm_var;
		asctime_r(gmtime_r(&wtime, &tm_var), buff);
		return buff;
	}
	return "";
}

static const StringS SafeString(const StringS &str)
{
	if (str != UTL_VOID)
		return str;
	return "";
}

void StartOfWaferData::
FormatASCII(bool fail_only_mode, std::ostream &output)
{
	if (Valid == true) {
		StringS MapName = WMap.GetName();
		if (MapName.Length() > 0)
			output << "Start of Wafer: WaferID " << WaferID << " - Mapping to " << MapName << endl << endl;
		else
			output << "Start of Wafer: WaferID " << WaferID << endl << endl;
	}
}

static char GetDirectionChar(const WaferDirectionS &dir)
{
    switch(dir) {
	case WAFER_DIR_LEFT:	return 'L';
	case WAFER_DIR_RIGHT:	return 'R';
	case WAFER_DIR_TOP: 	return 'U';
	case WAFER_DIR_BOTTOM:	return 'D';
	default:		break;
    }
    return ' ';
}

void StartOfWaferData::
FormatSTDFV4(bool fail_only_mode, std::ostream &output)
{
	STDFV4Stream STDF = GetSTDFV4Stream(false);
	if (STDF.Valid()) {
		if (Valid) {
			if (STDF.NeedWaferSetup()) {
				// Write WCR
				STDFV4_WCR WCR;
				FloatS w_size = WMap.GetWaferSize();
				FloatS w_height = WMap.GetWaferHeight();
				FloatS w_width = WMap.GetWaferWidth();
				const StringS sunits = w_size.GetUnits();
				unsigned int w_units = 0;				// Unknown units
				if (sunits.Length() > 0) {
					if (sunits == "Inch")
						w_units = 1;				// inch units
					else if (sunits == "Meter") {
						FloatS max = MATH.Max(MATH.Max(w_size, w_height), w_width);
						if (max >= 0.02) {
							if (w_size != UTL_VOID) w_size *= 1e2;
							if (w_height != UTL_VOID) w_height *= 1e2;
							if (w_width != UTL_VOID) w_width *= 1e2;
							w_units = 2;			// centimeter units
						}
						else {
							if (w_size != UTL_VOID) w_size *= 1e3;
							if (w_height != UTL_VOID) w_height *= 1e3;
							if (w_width != UTL_VOID) w_width *= 1e3;
							w_units = 3;			// millimeter units
						}
					}
				}
				char flat = GetDirectionChar(WMap.GetOrientation());
				IntS center_x = WMap.GetFirstDieXCoord();
				IntS center_y = WMap.GetFirstDieYCoord();
				char inc_x = GetDirectionChar(WMap.GetXDirection());
				char inc_y = GetDirectionChar(WMap.GetYDirection());
				if (((inc_x == 'U') || (inc_x == 'D')) && ((inc_y == 'L') || (inc_y == 'R'))) {
					char temp = inc_x;
					inc_x = inc_y;
					inc_y = temp;
				}
				WCR.SetInfo(w_units, w_size, w_height, w_width, flat, inc_x, inc_y, center_x, center_y);
				STDF.Write(WCR);
			}
			STDFV4_WIR WIR;
			WIR.SetInfo(DlogTime, WaferID);
			STDF.Write(WIR);
		}
	}
}

DatalogData *QCOM_Datalog::
StartOfWafer(const DatalogBaseUserData *)
{
	return new StartOfWaferData(*this);
}

// ***************************************************************************** 
// EndOfWafer

class EndOfWaferData : public QCOM_DatalogData {
public:
	EndOfWaferData(QCOM_Datalog &);
	~EndOfWaferData();

	virtual void Format(const char *format, bool fail_only_mode, std::ostream &output);
private:
	bool Valid;
	WaferInfoStruct WaferInfo;

	void FormatASCII(bool fail_only_mode, std::ostream &output);
	void FormatSTDFV4(bool fail_only_mode, std::ostream &output);
};

EndOfWaferData::
EndOfWaferData(QCOM_Datalog &parent) : 
	QCOM_DatalogData(DatalogMethod::EndOfWafer, parent),
	Valid(false),
	WaferInfo()
{
	Valid = RunTime.GetWaferInfo(WaferInfo);
}

EndOfWaferData::
~EndOfWaferData()
{
}

void EndOfWaferData::
Format(const char *format, bool fail_only_mode, std::ostream &output)
{
	if (format != NULL) {
		if (format[0] == formats[ASCII_INDEX][0])
			FormatASCII(fail_only_mode, output);
		else if (format[0] == formats[STDFV4_INDEX][0])
			FormatSTDFV4(fail_only_mode, output);
		SetLastFormatEvent();
	}
}

void EndOfWaferData::
FormatASCII(bool fail_only_mode, std::ostream &output)
{
	output << endl << "Wafer Results:" << endl;
	output << left << setw(25) << "  Finish Time:" << FormatTime(GetFinishTime()) << endl;
	output << left << setw(25) << "  Devices Tested:" << WaferInfo.NumTested << endl;
	output << left << setw(25) << "  Passed Devices:" << WaferInfo.NumPasses << endl;
	output << left << setw(25) << "  Retested Devices:" << WaferInfo.NumRetested << endl;
	output << left << setw(25) << "  Wafer ID:" << SafeString(WaferInfo.WaferID) << endl;
	output << left << setw(25) << "  Fab Wafer ID:" << SafeString(TestProg.GetLotInfo("FabWaferID")) << endl;
	output << left << setw(25) << "  Wafer Frame ID:" << SafeString(TestProg.GetLotInfo("FabWaferFrame")) << endl;
	output << left << setw(25) << "  Wafer Mask ID:" << SafeString(TestProg.GetLotInfo("WaferMask")) << endl;
	output << left << setw(25) << "  User Description:" << SafeString(TestProg.GetLotInfo("WaferUserDesc")) << endl;
	output << left << setw(25) << "  Exec Description:" << SafeString(TestProg.GetLotInfo("ExecDescription")) << endl << endl;
}

void EndOfWaferData::
FormatSTDFV4(bool fail_only_mode, std::ostream &output)
{
	STDFV4Stream STDF = GetSTDFV4Stream(false);
	if (STDF.Valid()) {
		STDFV4_WRR WRR;
		WRR.SetCounts(WaferInfo.NumTested, WaferInfo.NumPasses, UTL_VOID, WaferInfo.NumRetested);
		WRR.SetIDs(WaferInfo.WaferID, TestProg.GetLotInfo("FabWaferID"), TestProg.GetLotInfo("FabWaferFrame"), TestProg.GetLotInfo("WaferMask"));
		WRR.SetInfo(GetFinishTime(), TestProg.GetLotInfo("WaferUserDesc"), TestProg.GetLotInfo("ExecDescription"));
		STDF.Write(WRR);
	}
}

DatalogData *QCOM_Datalog::
EndOfWafer(const DatalogBaseUserData *)
{
	return new EndOfWaferData(*this);
}

// ***************************************************************************** 
// StartOfLot

class StartOfLotData : public QCOM_DatalogData {
public:
	StartOfLotData(QCOM_Datalog &);
	~StartOfLotData();

	virtual void Format(const char *format, bool fail_only_mode, std::ostream &output);
private:
	Sites SelSites;
	StringS TesterType;
	void FormatASCII(bool fail_only_mode, std::ostream &output);
	void FormatSTDFV4(bool fail_only_mode, std::ostream &output);
};

StartOfLotData::
StartOfLotData(QCOM_Datalog &parent) : 
	QCOM_DatalogData(DatalogMethod::StartOfLot, parent),
	SelSites(SelectedSites),
	TesterType(SYS.GetTestHeadType())
{
	ResetNumTestsExecuted();
}

StartOfLotData::
~StartOfLotData()
{
}

void StartOfLotData::
Format(const char *format, bool fail_only_mode, std::ostream &output)
{
	if (format != NULL) {
		if (format[0] == formats[ASCII_INDEX][0])
			FormatASCII(fail_only_mode, output);
		else if (format[0] == formats[STDFV4_INDEX][0])
			FormatSTDFV4(fail_only_mode, output);
		SetLastFormatEvent();
	}
}

void StartOfLotData::
FormatASCII(bool fail_only_mode, std::ostream &output)
{
	StringS LotID = TestProg.GetLotInfo("LotID");
	StringS DevName = TestProg.GetLotInfo("DeviceName");
	output << endl << "Start of Lot";
	if (LotID.Length() > 0)
		output << " - LotID: " << LotID;
	if (DevName.Length() > 0)
		output << " - Device Name: " << DevName;
	output << endl << endl;
}

void StartOfLotData::
FormatSTDFV4(bool fail_only_mode, std::ostream &output)
{
	STDFV4Stream STDF = GetSTDFV4Stream(false);
	if (STDF.Valid()) {
		if (STDF.NeedFileSetup()) {
					// Setup optimization upon open of the file
			STDF.SetOptimization(GetEnableFullOpt() ? STDFV4Stream::FullOptimization : STDFV4Stream::STDFOptimization);
			// Write FAR
			STDF.Write(STDFV4_FAR());
			// Wrtie MIR
			// No ATR
			if (GetScanEnable())
				STDF.Write(STDFV4_VUR(STDFV4_VUR::V4_SCAN));
			STDFV4_MIR MIR;
			// Write MIR
			MIR.SetInfo(GetDlogTime(), GetDlogTime());
                        char testmode = GetCode("TestMode");
                        if (testmode == ' ')
                            testmode = (RunTime.GetCurrentExecutionMode() == ILQA_EXECUTION ? 'Q' : 'P');
			MIR.SetCodes(testmode, GetCode("LotStatus"), GetCode("ProtectionCode"), GetCode("CommandMode"));
			if (TestProg.GetLotInfo("BurnInTime").Length() > 0)
				MIR.SetBurnInTime(FloatS(atoi((const char *)TestProg.GetLotInfo("BurnInTime"))));
			MIR.SetField(STDFV4_MIR::LOT_ID, TestProg.GetLotInfo("LotID"));
			MIR.SetField(STDFV4_MIR::PART_TYPE, TestProg.GetLotInfo("DeviceName"));
			MIR.SetField(STDFV4_MIR::NODE_NAME, TestProg.GetLotInfo("TesterName"));
			StringS TType = TestProg.GetLotInfo("TesterType");
			if ((TType.Length() > 0) && (TType != "Fusion"))
				MIR.SetField(STDFV4_MIR::TESTER_TYPE, TType);
			else 
				MIR.SetField(STDFV4_MIR::TESTER_TYPE, TesterType);
			MIR.SetField(STDFV4_MIR::JOB_NAME, TestProg.GetLotInfo("ProgramName"));
			MIR.SetField(STDFV4_MIR::JOB_REVISION, TestProg.GetLotInfo("FileNameRev"));
			MIR.SetField(STDFV4_MIR::SUBLOT_ID, TestProg.GetLotInfo("SubLotID"));
			MIR.SetField(STDFV4_MIR::OPERATOR_NAME, TestProg.GetLotInfo("OperatorID"));
			StringS SysName = TestProg.GetLotInfo("SystemName");
			if ((SysName.Length() > 0) && (SysName != "enVision"))
				MIR.SetField(STDFV4_MIR::EXEC_TYPE, SysName);
			else
				MIR.SetField(STDFV4_MIR::EXEC_TYPE, "Unison");
			MIR.SetField(STDFV4_MIR::EXEC_VERSION, TestProg.GetLotInfo("TargetName"));
			MIR.SetField(STDFV4_MIR::TEST_CODE, TestProg.GetLotInfo("TestPhase"));
			MIR.SetField(STDFV4_MIR::TEST_TEMP, TestProg.GetLotInfo("TestTemp"));
			MIR.SetField(STDFV4_MIR::USER_TEXT, TestProg.GetLotInfo("UserText"));
			MIR.SetField(STDFV4_MIR::AUX_FILE, TestProg.GetLotInfo("AuxDataFile"));
			MIR.SetField(STDFV4_MIR::PACKAGE_TYPE, TestProg.GetLotInfo("Package"));
			MIR.SetField(STDFV4_MIR::FAMILY_ID, TestProg.GetLotInfo("ProductID"));
			MIR.SetField(STDFV4_MIR::DATE_CODE, TestProg.GetLotInfo("DateCode"));
			MIR.SetField(STDFV4_MIR::FACILITY_ID, TestProg.GetLotInfo("TestFacility"));
			MIR.SetField(STDFV4_MIR::FLOOR_ID, TestProg.GetLotInfo("TestFloor"));
			MIR.SetField(STDFV4_MIR::PROCESS_ID, TestProg.GetLotInfo("FabID"));
			MIR.SetField(STDFV4_MIR::OPERATION_FREQ, TestProg.GetLotInfo("OperFreq"));
			MIR.SetField(STDFV4_MIR::SPEC_NAME, TestProg.GetLotInfo("TestSpecName"));
			MIR.SetField(STDFV4_MIR::SPEC_VERSION, TestProg.GetLotInfo("TestSpecRev"));
			MIR.SetField(STDFV4_MIR::FLOW_ID, TestProg.GetLotInfo("ActiveFlow"));
			MIR.SetField(STDFV4_MIR::SETUP_ID, TestProg.GetLotInfo("TestSetup"));
			MIR.SetField(STDFV4_MIR::DESIGN_REV, TestProg.GetLotInfo("DesignRevision"));
			MIR.SetField(STDFV4_MIR::ENG_LOT_ID, TestProg.GetLotInfo("EngineeringLotID"));
			MIR.SetField(STDFV4_MIR::ROM_CODE, TestProg.GetLotInfo("ROMCode"));
			MIR.SetField(STDFV4_MIR::TESTER_SN, TestProg.GetLotInfo("TesterSerNum"));
			MIR.SetField(STDFV4_MIR::SUPERVISOR, TestProg.GetLotInfo("Supervisor"));
			STDF.Write(MIR);
			StringS RetestStr = TestProg.GetLotInfo("LotStatus");
			if (RetestStr == "Retest") {
				// Optionally write RDR
				STDFV4_RDR RDR;
				RDR.SetBins();
				STDF.Write(RDR);
			}
			// Write SDR
			STDFV4_SDR SDR;
			SDR.SetSiteInfo(1, LoadedSites);
			SDR.SetField(STDFV4_SDR::HANDLER, TestProg.GetLotInfo("HandlerType"), TestProg.GetLotInfo("PHID"));
			SDR.SetField(STDFV4_SDR::PROBE_CARD, TestProg.GetLotInfo("CardType"), TestProg.GetLotInfo("CardID"));
			SDR.SetField(STDFV4_SDR::LOAD_BOARD, TestProg.GetLotInfo("LoadBoardType"), TestProg.GetLotInfo("LoadBoardID"));
			SDR.SetField(STDFV4_SDR::DIB_BOARD, TestProg.GetLotInfo("DIBType"), TestProg.GetLotInfo("ActiveLoadBoard"));
			SDR.SetField(STDFV4_SDR::CABLE, TestProg.GetLotInfo("IFCableType"), TestProg.GetLotInfo("IFCableID"));
			SDR.SetField(STDFV4_SDR::CONTACTOR, TestProg.GetLotInfo("ContactorType"), TestProg.GetLotInfo("ContactorID"));
			SDR.SetField(STDFV4_SDR::LASER, TestProg.GetLotInfo("LaserType"), TestProg.GetLotInfo("LaserID"));
			SDR.SetField(STDFV4_SDR::EXTRA_EQUIP, TestProg.GetLotInfo("ExtEquipmentType"), TestProg.GetLotInfo("ExtEquipmentID"));
			STDF.Write(SDR);
			// Write DTRs (For Galaxy above 256 pins)
			STDF.SetSiteConfiguration(LoadedSites);
			// Write PMR
			STDF.UnisonPinMap();						// use system software generation routine
			// Write PGR
			STDF.UnisonPinGroups();						// use system software generation routine
 		}
	}
}

DatalogData *QCOM_Datalog::
StartOfLot(const DatalogBaseUserData *)
{
	SummaryNeeded = true;
	return new StartOfLotData(*this);
}

// ***************************************************************************** 
// EndOfLot

DatalogData *QCOM_Datalog::
EndOfLot(const DatalogBaseUserData *)
{
	return NULL;			// Processed as summary
}

// ***************************************************************************** 
// StartTestNode

class StartTestNodeData : public QCOM_DatalogData {
public:
	StartTestNodeData(QCOM_Datalog &);
	~StartTestNodeData();

	virtual void Format(const char *format, bool fail_only_mode, std::ostream &output);
private:
};

StartTestNodeData::
StartTestNodeData(QCOM_Datalog &parent) :
	QCOM_DatalogData(DatalogMethod::StartTestNode, parent)
{
}

StartTestNodeData::
~StartTestNodeData()
{
}

void StartTestNodeData::
Format(const char *format, bool fail_only_mode, std::ostream &output)
{
	// Suppress the per-node header if in column mode for a neater output
	if (!GetASCIIDatalogInColumns()) SetLastFormatEvent();
}

DatalogData *QCOM_Datalog::
StartTestNode(const DatalogBaseUserData *)
{
	return new StartTestNodeData(*this);
}

// ***************************************************************************** 
// StartTestBlock

DatalogData *QCOM_Datalog::
StartTestBlock(const DatalogBaseUserData *)
{
	return NULL;				// not used
}

// ***************************************************************************** 
// ParametricTest

class ParametricTestData : public QCOM_DatalogData {
public:
	ParametricTestData(QCOM_Datalog &, const DatalogParametric &pdata);
	~ParametricTestData();

	virtual void Format(const char *format, bool fail_only_mode, std::ostream &output);
private:
	DatalogParametric PData;

	void FormatASCII(bool fail_only_mode, std::ostream &output);
	void FormatSTDFV4(bool fail_only_mode, std::ostream &output);
};

ParametricTestData::
ParametricTestData(QCOM_Datalog &parent, const DatalogParametric &pdata) : 
	QCOM_DatalogData(DatalogMethod::ParametricTest, parent),
	PData(pdata)
{
	IncNumTestsExecuted();
}

ParametricTestData::
~ParametricTestData()
{
}

void ParametricTestData::
Format(const char *format, bool fail_only_mode, std::ostream &output)
{
	if (format != NULL) {
		if (format[0] == formats[ASCII_INDEX][0])
			FormatASCII(fail_only_mode, output);
		else if (format[0] == formats[STDFV4_INDEX][0])
			FormatSTDFV4(fail_only_mode, output);
		SetLastFormatEvent();
	}
}

static void OutputParametricHeader(std::ostream &output, int field_width, bool columns, bool separate_units)
{
	// omit_pin_name will be hooked to an options.cfg setting in a later release 
	const bool omit_pin_name = false;
	output << endl;

	if (columns) {
		// This section for column-oriented output
		output << setw(TNSize) << left << "Test_No." << setw(2) << " ";
		output << setw(field_width) << left << "Minimum" << setw(2) << " ";
		for (SiteIter s1=LoadedSites.Begin();!s1.End();++s1) {
			output << left << "Site_" << setw(4) << left << s1.GetValue() << setw(field_width+4-9) << left << " " << setw(2) << " ";
		}
		output << setw(field_width) << left << "Maximum" << setw(2) << " ";
		output << setw(UnitSize) << left << "Units" << setw(2) << " ";
		if (!omit_pin_name) {
			output << setw(PGSize) << left << "Pin_Name" << setw(2) << " ";
		}
		output << "Test_Description" << endl;
	
		OutputBorder(output, TNSize, 2);        // TestID
		OutputBorder(output, field_width, 2);        // Min
		for (SiteIter s1=LoadedSites.Begin();!s1.End();++s1)
			OutputBorder(output, field_width+4, 2);          // Meas
		OutputBorder(output, field_width, 2);                // Max
		OutputBorder(output, UnitSize, 2);                // Units
		if (!omit_pin_name) {
			OutputBorder(output, PGSize, 2);        // Pins
		}
		OutputBorder(output, TDSize, 2);        // Description
	} else {
		// This section for row-oriented output
		output << setw(TNSize) << left << "Test_No." << setw(2) << " ";
		output << setw(3) << "P/F" << "  ";
		output << setw(4) << "Site" << "  ";
		output << setw(field_width) << left << "Minimum" << setw(2) << " ";
		output << setw(field_width) << left << "Measured" << setw(2) << " ";
		output << setw(field_width) << left << "Maximum" << setw(2) << " ";
		if (separate_units) {
			output << setw(UnitSize) << left << "Units" << setw(2) << " ";
		}
		if (!omit_pin_name) {
			output << setw(PGSize) << left << "Pin_Name" << setw(2) << " ";
		}
		output << "Test_Description" << endl;
	
		OutputBorder(output, TNSize, 2);        // TestID
		OutputBorder(output, 3, 2);        // P/F
		OutputBorder(output, 4, 2);        // Site
		OutputBorder(output, field_width, 2);        // Min
		OutputBorder(output, field_width, 2);                // Meas
		OutputBorder(output, field_width, 2);                // Max
		if (separate_units) {
			OutputBorder(output, UnitSize, 2);                // Units
		}
		if (!omit_pin_name) {
			OutputBorder(output, PGSize, 2);        // Pins
		}
		OutputBorder(output, TDSize, 2);        // Description
	}
	output << endl;
}

static void PrintValue(std::ostream &output, SV_TYPE type, const BasicVar &Val, const StringS &units, int width, double scale, bool suppress_units, int int_part_width)
{
	if (Val.Valid()) {
		StringS str;
		int unit_width = units.Length();
		int val_width = width;
		if (!suppress_units) val_width -= unit_width;
		if (DatalogBaseUserData::FormatSVData(str, type, Val, val_width, scale, int_part_width)) {
			if (suppress_units) {
				output << right << setw(val_width) << str << left << setw(2) << " ";
			} else {
				output << right << setw(val_width) << str << left << setw(unit_width) << units << setw(2) << " ";
			}
		} else {
			output << setw(width + 2) << " ";
		}
	}
	else
		output << setw(width + 2) << " ";
}

static void OutputParametricSiteASCII(	std::ostream &output, 
					SITE site, 
					const UnsignedS &TestID, 
					TM_RESULT Res,
					const int field_width,
					const double scale,
					const BasicVar &TV, 
					const BasicVar &LL, 
					const BasicVar &HL,
					const bool separate_units,
					const StringS &units, 
					const PinML &pins, 
					const StringS &comment, 
					const StringS pass_string,
					const int int_part_width)
{
	const bool omit_pin_name = false;

	output << setw(TNSize) << right << dec << TestID << setw(2) << " ";
	StringS PF = (Res == TM_PASS) ? pass_string : (Res == TM_FAIL) ? "*F*" : "   ";
	output << setw(3) << PF << setw(2) << " ";
	output << setw(4) << right << site << setw(2) << " ";
	SV_TYPE var_type = TV.Valid() ? TV.GetType() : LL.Valid() ? LL.GetType() : HL.GetType();
	PrintValue(output, var_type, LL, units, field_width, scale, separate_units, int_part_width);
	PrintValue(output, var_type, TV, units, field_width, scale, separate_units, int_part_width);
	PrintValue(output, var_type, HL, units, field_width, scale, separate_units, int_part_width);
	if (separate_units) {
		output << setw(8) << left << units << setw(2) << " ";
	}
	if (!omit_pin_name) {
		StringS str;
		DatalogBaseUserData::FormatPins(str, pins, PGSize);
		output << setw(PGSize) << left << str << setw(2) << " ";
	}
	output << left << comment;
	output << endl;
}

static void OutputParametricLineStartASCII(     std::ostream &output,
										const UnsignedS &TestID,
										const int field_width,
										double scale,
										const BasicVar &TV,
										const BasicVar &LL,
										const BasicVar &HL,
										const StringS &units,
										const int int_part_width)
{
	// This is called in column mode to print the first part of the test data
	output << setw(TNSize) << right << dec << TestID << left << setw(2) << " ";
	SV_TYPE var_type = TV.Valid() ? TV.GetType() : LL.Valid() ? LL.GetType() : HL.GetType();
	PrintValue(output, var_type, LL, units, field_width, scale, true, int_part_width);
}

static void OutputParametricLineEndASCII(   std::ostream &output,
											const int field_width,
											double limit_scale,
											const BasicVar &TV,
											const BasicVar &LL,
											const BasicVar &HL,
											const StringS &units,
											const PinML &pins,
											const StringS &comment,
											const int int_part_width)
{
	// This is called in column mode to complete the line of test data
	const bool omit_pin_name = false;

	SV_TYPE var_type = TV.Valid() ? TV.GetType() : LL.Valid() ? LL.GetType() : HL.GetType();
	PrintValue(output, var_type, HL, units, field_width, limit_scale, true, int_part_width);
	output << setw(UnitSize) << left << units << setw(2) << " ";
	if (!omit_pin_name) {
		StringS str;
		DatalogBaseUserData::FormatPins(str, pins, PGSize);
		output << setw(PGSize) << left << str << setw(2) << " ";
	}
	output << left << comment;
	output << endl;
}

void ParametricTestData::
FormatASCII(bool fail_only_mode, std::ostream &output)
{
	int field_width = GetFieldWidth();
	int int_part_width = GetIntegerPartWidth();

	if ((GetLastFormatEvent() != DatalogMethod::ParametricTest) && (GetLastFormatEvent() != DatalogMethod::ParametricTestArray))
		OutputParametricHeader(output, field_width, GetASCIIDatalogInColumns(), false);

	// store first and last tested site for later
	Sites fsites = GetDlogSites();
	SITE  first_site = LoadedSites.Begin().GetValue();
	SITE  last_site =  LoadedSites.GetLargestSite();

	const TMResultM &Res = PData.GetResult();
	if (fail_only_mode)
		(void)fsites.DisableFailingSites(Res.Equal(TM_FAIL));	// This removes anything that is not a fail due to Equal
	const StringS &units = PData.GetUnits();
	StringS real_units, str, tdesc;
	StringS testText = PData.GetComment();

	if (GetAppendPinName()) DatalogData::AppendPinNameToTestText(PData.GetPins(), testText);
		
	FormatTestDescription(tdesc, testText);
	// scale gets set to the inverse of the unit multiplier, eg if unit = mA then scale = 1e3
	// real_units gets set to the base unit of units, with the multiplier removed, e.g. if unit = mA then real_units = A
	double scale = PData.CalculateUnitScale(units, real_units);
	// if no known unit found and autoscaling is not on then set scale to 1.0
	if ( scale == 0.0 && !GetUnitAutoscaling() ) scale = 1.0;

	if (GetASCIIDatalogInColumns()) {
		// this section for column-oriented output
		// iterator for tested sites
		SiteIter tested = fsites.Begin();
		// get limits and result from first datalogged site, necessary compromise for column output
		SITE limit_site = fsites.Begin().GetValue();
		const BasicVar &TV_first = PData.GetBaseSData(DatalogParametric::Test,      limit_site);
		const BasicVar &LL       = PData.GetBaseSData(DatalogParametric::LowLimit,  limit_site);
		const BasicVar &HL       = PData.GetBaseSData(DatalogParametric::HighLimit, limit_site);
		// from the first datalogged site:
		// limit_scale gets set to the inverse of the unit multiplier, eg if unit = mA then scale = 1e3
		// limit_units gets set to the engineering unit that covers the max of the value, the low limit and the high limit
		StringS limit_units = units;
		double limit_scale = (scale != 0.0) ? scale : PData.CalculateAutoRangeUnitScale(units, limit_units, TV_first, LL, HL);

		for (SiteIter s1 = LoadedSites.Begin(); !s1.End(); ++s1) {
			SITE site = *s1;
			if (site == first_site) {
				OutputParametricLineStartASCII(output, PData.GetTestID(), field_width, limit_scale, TV_first, LL, HL, limit_units, int_part_width);
			}
			if (site == *tested) {
				const BasicVar &TV = PData.GetBaseSData(DatalogParametric::Test, *s1);
				double real_scale = (scale != 0.0) ? scale : PData.CalculateAutoRangeUnitScale(units, real_units, TV, LL, HL);
				StringS PF = (Res[site] == TM_PASS) ? GetPassString() : (Res[site] == TM_FAIL) ? "*F*" : "   ";
				output << PF << " ";
				SV_TYPE var_type = TV.Valid() ? TV.GetType() : LL.Valid() ? LL.GetType() : HL.GetType();
				PrintValue(output, var_type, TV, real_units, field_width, real_scale, true, int_part_width);
				if (!tested.End())
					++tested;
			} else {
				output << "    " << setw(field_width) << " " << "  ";
			}
			if (site == last_site) {
				OutputParametricLineEndASCII(output, field_width, limit_scale, TV_first, LL, HL, limit_units, PData.GetPins(), tdesc, int_part_width);
			}
		}
	} else {
		// this section for row-oriented output
		for (SiteIter s1 = fsites.Begin(); !s1.End(); ++s1) {
			SITE site = *s1;
			const BasicVar &TV = PData.GetBaseSData(DatalogParametric::Test, site);
			const BasicVar &LL = PData.GetBaseSData(DatalogParametric::LowLimit, site);
			const BasicVar &HL = PData.GetBaseSData(DatalogParametric::HighLimit, site);
			double real_scale = (scale != 0.0) ? scale : PData.CalculateAutoRangeUnitScale(units, real_units, TV, LL, HL);
			OutputParametricSiteASCII(output, site, PData.GetTestID(), Res[site], field_width, real_scale, TV, LL, HL, false, real_units, PData.GetPins(), tdesc, GetPassString(), int_part_width);
		}
	}
}

static const char *GetDefaultFormat(const BasicVar &var)
{
    if ((var.GetType() == SV_INT) || (var.GetType() == SV_UINT))
	return "%9.0f";
    return "%9.3f";
}

void ParametricTestData::
FormatSTDFV4(bool fail_only_mode, std::ostream &output)
{
	STDFV4Stream STDF = GetSTDFV4Stream(false);
	if (STDF.Valid()) {
		STDFV4_PTR PTR;
		Sites fsites = GetDlogSites();
		const TMResultM &Res = PData.GetResult();
		if (fail_only_mode)
			(void)fsites.DisableFailingSites(Res.Equal(TM_FAIL));	// This removes anything that is not a fail due to Equal
		const StringS &units = PData.GetUnits();
		StringS real_units, str, tdesc;
		StringS testText = PData.GetComment();

		if (GetAppendPinName()) DatalogData::AppendPinNameToTestText(PData.GetPins(), testText);
		
		FormatTestDescription(tdesc, testText);
                double scale = PData.CalculateBaseUnitScale(units, real_units);
                if ( scale == 0.0 && !GetUnitAutoscaling() ) scale = 1.0;
		PTR.SetInfo(PData.GetTestID(), tdesc);
		PTR.SetUnits(real_units);
		for (SiteIter s1 = fsites.Begin(); !s1.End(); ++s1) {
			SITE site = *s1;
			const BasicVar &TV = PData.GetBaseSData(DatalogParametric::Test, site);
			if (TV != UTL_VOID) {
				const BasicVar &LL = PData.GetBaseSData(DatalogParametric::LowLimit, site);
				const BasicVar &HL = PData.GetBaseSData(DatalogParametric::HighLimit, site);
				double real_scale = (scale != 0.0) ? scale : PData.CalculateAutoRangeUnitScale(units, real_units, TV, LL, HL);
				if (real_scale != 0.0)
					real_scale = 1.0 / real_scale;			// STDF routine wants value, not multiplier
				const char *fmt = GetDefaultFormat(TV);
				PTR.SetContext(site);
				PTR.SetResult(Res[site], TV, real_scale, fmt);
				PTR.SetLimit(STDFV4_PTR::PTR_LO_LIMIT, LL, real_scale, fmt);
				PTR.SetLimit(STDFV4_PTR::PTR_HI_LIMIT, HL, real_scale, fmt);
				STDF.Write(PTR);
			}
		}
	}
}

DatalogData *QCOM_Datalog::
ParametricTest(const DatalogBaseUserData *udata)
{
	const DatalogParametric *pdata = dynamic_cast<const DatalogParametric *>(udata);
	if (pdata != NULL) {
		SummaryNeeded = true;
		return new ParametricTestData(*this, *pdata);
 	}
	return NULL;
}

// ***************************************************************************** 
// ParametricTestArray

class ParametricTestDataArray : public QCOM_DatalogData {
public:
	ParametricTestDataArray(QCOM_Datalog &, const DatalogParametricArray &);
	~ParametricTestDataArray();

	virtual void Format(const char *format, bool fail_only_mode, std::ostream &output);
private:
	DatalogParametricArray PData;

	void FormatASCII(bool fail_only_mode, std::ostream &output);
	void FormatSTDFV4(bool fail_only_mode, std::ostream &output);
};

ParametricTestDataArray::
ParametricTestDataArray(QCOM_Datalog &parent, const DatalogParametricArray &pdata) : 
	QCOM_DatalogData(DatalogMethod::ParametricTestArray, parent),
	PData(pdata)
{
	IncNumTestsExecuted();
}

ParametricTestDataArray::
~ParametricTestDataArray()
{
}

void ParametricTestDataArray::
Format(const char *format, bool fail_only_mode, std::ostream &output)
{
	if (format != NULL) {
		if (format[0] == formats[ASCII_INDEX][0])
			FormatASCII(fail_only_mode, output);
		else if (format[0] == formats[STDFV4_INDEX][0])
			FormatSTDFV4(fail_only_mode, output);
		SetLastFormatEvent();
	}
}

void ParametricTestDataArray::
FormatASCII(bool fail_only_mode, std::ostream &output)
{
	int field_width = GetFieldWidth();
	int int_part_width = GetIntegerPartWidth();

	if ((GetLastFormatEvent() != DatalogMethod::ParametricTest) && (GetLastFormatEvent() != DatalogMethod::ParametricTestArray))
		OutputParametricHeader(output, field_width, GetASCIIDatalogInColumns(), false);
	const TMResultM1D &Res1D = PData.GetResults();
	const Sites &dlog_sites = GetDlogSites();
	const StringS &units = PData.GetUnits();
	StringS real_units, str, tdesc;
	FormatTestDescription(tdesc, PData.GetComment());
	// scale gets set to the inverse of the unit multiplier, eg if unit = mA then scale = 1e3
	// real_units gets set to the base unit of units, with the multiplier removed, e.g. if unit = mA then real_units = A
	double scale = PData.CalculateUnitScale(units, real_units);
	if ( scale == 0.0 && !GetUnitAutoscaling() ) scale = 1.0;
	const PinML &Pins = PData.GetPins();
	int npins = Pins.GetNumPins();
	int nvalues = PData.GetNumValues(DatalogParametricArray::Test);

	if (GetASCIIDatalogInColumns()) {
		// this section for column-oriented output
		unsigned int test_id = PData.GetTestID();
		SITE first_site = LoadedSites.Begin().GetValue();
		SITE limit_site = dlog_sites.Begin().GetValue();
		SITE last_site = LoadedSites.GetLargestSite();
		for (int ii = 0; ii < nvalues; ++ii) {
			TMResultM ResM = Res1D[ii];
			BasicVar TV_first, LL, HL;
			// get limits and result from first datalogged site, necessary compromise for column output
			PData.StuffSData(TV_first, DatalogParametricArray::Test,      ii, limit_site);
			PData.StuffSData(LL,       DatalogParametricArray::LowLimit,  ii, limit_site);
			PData.StuffSData(HL,       DatalogParametricArray::HighLimit, ii, limit_site);
			// from the first datalogged site:
			// limit_scale gets set to the inverse of the unit multiplier, eg if unit = mA then scale = 1e3
			// limit_units gets set to the engineering unit that covers the max of the value, the low limit and the high limit
			StringS limit_units = units;
			double limit_scale = (scale != 0.0) ? scale : PData.CalculateAutoRangeUnitScale(units, limit_units, TV_first, LL, HL);
	
			if (!(ResM==TM_PASS) || !fail_only_mode) {
				SiteIter tested = dlog_sites.Begin();
				for (SiteIter s1 = LoadedSites.Begin(); !s1.End(); ++s1) {
					SITE site = *s1;
					if (site == first_site) {
						OutputParametricLineStartASCII(output, PData.GetTestID(), field_width, limit_scale, TV_first, LL, HL, limit_units, int_part_width);
					}
					if (site == *tested) {
						BasicVar TV;
						PData.StuffSData(TV, DatalogParametricArray::Test, ii, site);
						double real_scale = (scale != 0.0) ? scale : PData.CalculateAutoRangeUnitScale(units, real_units, TV, LL, HL);
						if (!(ResM[site]==TM_PASS) || !fail_only_mode) {
							StringS PF = (ResM[site] == TM_PASS) ? GetPassString() : (ResM[site] == TM_FAIL) ? "*F*" : "   ";
							output << PF << " ";
							SV_TYPE var_type = TV.Valid() ? TV.GetType() : LL.Valid() ? LL.GetType() : HL.GetType();
							PrintValue(output, var_type, TV, real_units, field_width, real_scale, true, int_part_width);
						} else {
							output << "    " << setw(field_width) << " " << "  ";
						}
						if (!tested.End())
							++tested;
					} else {
						output << "    " << setw(field_width) << " " << "  ";
					}
					if (site == last_site) {
						OutputParametricLineEndASCII(output, field_width, limit_scale, TV_first, LL, HL, limit_units, (ii < npins ? Pins[ii] : PinML(UTL_VOID)), tdesc, int_part_width);
					}
				}
			}
		}
	} else {
		// this section for row-oriented output
		for (SiteIter s1 = dlog_sites.Begin(); !s1.End(); ++s1) {
			SITE site = *s1;
			const TMResultS1D &Res = Res1D[site];
			unsigned int test_id = PData.GetTestID();
			for (int ii = 0; ii < nvalues; ++ii) {
				if (fail_only_mode && (Res[ii] != TM_FAIL))
					continue;
				BasicVar TV, LL, HL;
				PData.StuffSData(TV, DatalogParametricArray::Test, ii, site);
				PData.StuffSData(LL, DatalogParametricArray::LowLimit, ii, site);
				PData.StuffSData(HL, DatalogParametricArray::HighLimit, ii, site);
				double real_scale = (scale != 0.0) ? scale : PData.CalculateAutoRangeUnitScale(units, real_units, TV, LL, HL);
				OutputParametricSiteASCII(output, site, test_id, Res[ii], field_width, real_scale, TV, LL, HL, false, real_units, (ii < npins ? Pins[ii] : PinML(UTL_VOID)), tdesc, GetPassString(), int_part_width);
			}
		}
	}
}

static bool PerPinLimits(const BasicVar &LL, const BasicVar &HL)
{
	if ((LL != UTL_VOID) && (LL.GetConfig() == SV_ARRAY_S1D)) {
		switch(LL.GetType()) {
		case SV_FLOAT:
			{
				const FloatS1D &sv = LL.GetFloatS1D();
				if ((sv.GetSize() > 1) && ((sv == sv[0]) == false))
					return true;
			}
			break;
		case SV_INT:
			{
				const IntS1D &sv = LL.GetIntS1D();
				if ((sv.GetSize() > 1) && ((sv == sv[0]) == false))
					return true;
			}
			break;
		case SV_UINT:
			{
				const UnsignedS1D &sv = LL.GetUnsignedS1D();
				if ((sv.GetSize() > 1) && ((sv == sv[0]) == false))
					return true;
			}
			break;
		default:
			break;
		}
	}
	if ((HL != UTL_VOID) && (HL.GetConfig() == SV_ARRAY_S1D)) {
		switch(HL.GetType()) {
		case SV_FLOAT:
			{
				const FloatS1D &sv = HL.GetFloatS1D();
				if ((sv.GetSize() > 1) && ((sv == sv[0]) == false))
					return true;
			}
			break;
		case SV_INT:
			{
				const IntS1D &sv = HL.GetIntS1D();
				if ((sv.GetSize() > 1) && ((sv == sv[0]) == false))
					return true;
			}
			break;
		case SV_UINT:
			{
				const UnsignedS1D &sv = HL.GetUnsignedS1D();
				if ((sv.GetSize() > 1) && ((sv == sv[0]) == false))
					return true;
			}
			break;
		default:
			break;
		}
	}
	return false;
}

static int GetArrayLength(const BasicVar &TV)
{
	if ((TV != UTL_VOID) && (TV.GetConfig() == SV_ARRAY_S1D)) {
		switch(TV.GetType()) {
		case SV_FLOAT:
			{
				const FloatS1D &sv = TV.GetFloatS1D();
				return sv.GetSize();
			}
			break;
		case SV_INT:
			{
				const IntS1D &sv = TV.GetIntS1D();
				return sv.GetSize();
			}
			break;
		case SV_UINT:
			{
				const UnsignedS1D &sv = TV.GetUnsignedS1D();
				return sv.GetSize();
			}
			break;
		default:
			break;
		}
	}
	return 0;
}

void ParametricTestDataArray::
FormatSTDFV4(bool fail_only_mode, std::ostream &output)
{
	STDFV4Stream STDF = GetSTDFV4Stream(false);
	if (STDF.Valid()) {
		STDFV4_MPR MPR;
		Sites fsites = GetDlogSites();
		const TMResultM1D &Res1D = PData.GetResults();
		const Sites &dlog_sites = GetDlogSites();
		const StringS &units = PData.GetUnits();
		StringS real_units, str, tdesc;
		FormatTestDescription(tdesc, PData.GetComment());
                double scale = PData.CalculateBaseUnitScale(units, real_units);
                if ( scale == 0.0 && !GetUnitAutoscaling() ) scale = 1.0;
		MPR.SetInfo(PData.GetTestID(), tdesc);
		MPR.SetUnits(real_units);
		for (SiteIter s1 = dlog_sites.Begin(); !s1.End(); ++s1) {
			SITE site = *s1;
			const BasicVar &TV = PData.GetBaseS1DData(DatalogParametricArray::Test, site);
			const BasicVar &LL = PData.GetBaseS1DData(DatalogParametricArray::LowLimit, site);
			const BasicVar &HL = PData.GetBaseS1DData(DatalogParametricArray::HighLimit, site);
			const char *fmt = GetDefaultFormat(TV);
                        double real_scale = (scale != 0.0) ? scale : PData.CalculateAutoRangeUnitScale(real_units, str, TV, LL, HL);
			if (real_scale != 0.0)
				real_scale = 1.0 / real_scale;				// STDF routine wants value, not multiplier
			if (PerPinLimits(LL, HL)) {					// Implement as an array of PTRs
				const PinML &pins = PData.GetPins();
				int num_vals = GetArrayLength(TV);
				int num_low = GetArrayLength(LL);
				int num_high = GetArrayLength(HL);
				int num_pins = pins.GetNumPins();
				STDFV4_PTR PTR;
				PTR.SetInfo(PData.GetTestID(), tdesc);
				PTR.SetUnits(real_units);
				PTR.SetContext(site);
				BasicVar BV;
				for (int ii = 0; ii < num_vals; ii++) {
					PData.StuffSData(BV, DatalogParametricArray::Test, ii, site);
					if (BV.Valid()) {
						PTR.SetResult(Res1D[site][ii], BV, real_scale, fmt);
						BV = UTL_VOID;
						if (num_low > 0)
							PData.StuffSData(BV, DatalogParametricArray::LowLimit, (num_low == 1) ? 0 : ii, site);		
						PTR.SetLimit(STDFV4_PTR::PTR_LO_LIMIT, BV, real_scale, fmt);
						BV = UTL_VOID;
						if (num_high > 0)
							PData.StuffSData(BV, DatalogParametricArray::HighLimit, (num_high == 1) ? 0 : ii, site);		
						PTR.SetLimit(STDFV4_PTR::PTR_HI_LIMIT, BV, real_scale, fmt);
						STDF.Write(PTR);
					}
				}
			}
			else {
				MPR.SetContext(site);
				MPR.SetResult(PData.GetPins(), Res1D[site], TV, true, real_scale, fmt);
				MPR.SetLimit(STDFV4_MPR::MPR_LO_LIMIT, LL, real_scale, fmt);
				MPR.SetLimit(STDFV4_MPR::MPR_HI_LIMIT, HL, real_scale, fmt);
				STDF.Write(MPR);
			}
		}
	}
}

DatalogData *QCOM_Datalog::
ParametricTestArray(const DatalogBaseUserData *udata)
{
	const DatalogParametricArray *pdata = dynamic_cast<const DatalogParametricArray *>(udata);
	if (pdata != NULL) {
		SummaryNeeded = true;
		return new ParametricTestDataArray(*this, *pdata);
 	}
	return NULL;
}

// ***************************************************************************** 
// FunctionalTest

class FunctionalTestData : public QCOM_DatalogData {
public:
	FunctionalTestData(QCOM_Datalog &, const DatalogFunctional &);
	~FunctionalTestData();

	virtual void Format(const char *format, bool fail_only_mode, std::ostream &output);
        const DatalogFunctional &GetFuncData() const;
        const IntS &GetMaxNumFails() const;
        const DigitalPatternInfoStruct &GetPatInfo() const;
        const DigitalPatternPinStruct &GetPatPinInfo() const;
private:
	DatalogFunctional FData;			// data passed in from Functional BIF
	IntS MaxNumFails;				// requested maximum number of fails, user can cause extra collection
	DigitalPatternInfoStruct PatInfo;		// data collected from DIGITAL driver
	DigitalPatternPinStruct PatPinInfo;		// data collected from DIGITAL driver
	void FormatASCII(bool fail_only_mode, std::ostream &output);
	void FormatSTDFV4(bool fail_only_mode, std::ostream &output);
};

FunctionalTestData::
FunctionalTestData(QCOM_Datalog &parent, const DatalogFunctional &fdata) : 
	QCOM_DatalogData(DatalogMethod::FunctionalTest, parent),
	FData(fdata),
	MaxNumFails(TestProg.GetNumberOfFunctionalFails()),
	PatInfo(DIGITAL.GetPatternInfo()),
	PatPinInfo(DIGITAL.GetPatternPinInfo())
{
	IncNumTestsExecuted();
}

FunctionalTestData::
~FunctionalTestData()
{
}

const DatalogFunctional &FunctionalTestData::
GetFuncData() const
{
	return FData;
}

const IntS &FunctionalTestData::
GetMaxNumFails() const
{
	return MaxNumFails;
}

const DigitalPatternInfoStruct &FunctionalTestData::
GetPatInfo() const
{
	return PatInfo;
}

const DigitalPatternPinStruct &FunctionalTestData::
GetPatPinInfo() const
{
	return PatPinInfo;
}

void FunctionalTestData::
Format(const char *format, bool fail_only_mode, std::ostream &output)
{
	if ((format != NULL) && (PatInfo.NumRecords != 0)) {
		if (format[0] == formats[ASCII_INDEX][0])
			FormatASCII(fail_only_mode, output);
		else if (format[0] == formats[STDFV4_INDEX][0])
			FormatSTDFV4(fail_only_mode, output);
		SetLastFormatEvent();
	}
}

const int CCSize = 10;
int FPSize = 43;
const int SCSize = 14;
int BHSize = (          TNSize + 	// TestID
			2 + 		// Space
			3 + 		// P/F
			2 + 		// Space
			4 + 		// Site
			2 + 		// Space
			FPSize + 	// Pattern
			2 + 		// Space
			CCSize + 	// Count
			2 + 		// Space
			SCSize + 	// Scan
			2 + 		// Space
			TDSize + 	// Description
			2 + 		// Space
			4 + 		// # fail pins
			2);		// Space

static int CalcMaxPinLength(const PinML &pins, StringS1D  &header)
{
	int npins = pins.GetNumPins();
	if (npins > 0) {
		vector<int> pin_lens(npins, 0);
		vector<string> pin_names(npins, "");
		int ii = 0, max_len = 0;
		for (int ii = 0; ii < npins; ii++) {
			string pin_name = static_cast<const char *>(pins[ii].GetName());
			if (!pin_name.empty()) {
				int len = pin_name.length();
				pin_names[ii].swap(pin_name);
				pin_lens[ii] = len;
				if (len > max_len)
					max_len = len;
			}
		}
		if (max_len > 0) {
			header.Resize(max_len + 1);
			char buff[16384];
			for (int hd = 0; hd < max_len; hd++) {
				for (ii = 0; ii < npins; ii++) {
					if (hd < pin_lens[ii])
						buff[ii] = pin_names[ii][pin_lens[ii] - hd - 1];
					else
						buff[ii] = ' ';
				}
				buff[npins] = '\0';
				header[max_len - hd - 1] = buff;
			}
			memset(buff, '-', npins);
			buff[npins] = '\0';
			header[max_len] = buff;
			return max_len + 1;
		}
	}
	return 0;
}

void ShowPinHeaderRow(std::ostream &output, int index, int space, const StringS1D &header, bool extra = false)
{
	if (extra) {
		if (space >= 6)
			output << setw(space - 6) << " ";
		output << "fail  ";
	}
	else if (space > 0)
		output << setw(space) << " ";
	if ((index >= 0) && (index < header.GetSize(0)))
		output << left << header[index];
}

static void OutputFunctionalHeader(std::ostream &output, const PinML &pins)
{
       	output << endl;
	StringS1D PinHeader;
	int MaxPinLen = (pins.Valid()) ? CalcMaxPinLength(pins, PinHeader) : 0;
	int PinOnlyRows = MaxPinLen - 2;	// one for header, one for header borders
	int ii = 0;
	for (ii = 0; ii < PinOnlyRows; ii++) {
		ShowPinHeaderRow(output, ii, BHSize, PinHeader, (ii == (PinOnlyRows - 1)) ? true : false);
		output << endl;
	}
        output << setw(TNSize) << left << "Test_No." << "  P/F  Site  ";
	output << setw(FPSize) << left << "Pattern" << "  ";
	output << setw(CCSize) << left << "Count" << "  ";
	output << setw(SCSize) << left << "ScanVec:Bit" << "  ";
        output << setw(TDSize) << left << "Test_Description" << "  ";
	if (MaxPinLen > 1) {
		output << "pins  ";
		ShowPinHeaderRow(output, ii++, 0, PinHeader);
	}
	output << endl;
        OutputBorder(output, TNSize, 2);	// TestID
        OutputBorder(output, 3, 2);		// P/F
        OutputBorder(output, 4, 2);		// Site
        OutputBorder(output, FPSize, 2);	// Pattern
        OutputBorder(output, CCSize, 2);	// Count
        OutputBorder(output, SCSize, 2);	// Scan
        OutputBorder(output, TDSize, 2);	// Description
	if (MaxPinLen > 0) {
        	OutputBorder(output, 4, 2);	// # fail pins
		ShowPinHeaderRow(output, ii++, 0, PinHeader);
	}
        output << endl;
}

static void FormatPatternAddr(StringS &str, const Object &Pat, unsigned int Offs)
{
	if (Pat.Valid()) {
		OBJECT_TYPE type = Pat.GetType();
		if ((type == OBJ_PATTERN) || (type == OBJ_PATTERN_BURST)) {
			if (Offs > 0) {
        			stringstream strm;
				strm << Pat.GetName() << "+" << Offs;
        			str = strm.str().c_str();
			}
			else
				str = Pat.GetName();
			return;
		}
	}
}

static void FormatPatternAddr(StringS &str, const StringS &PatName, unsigned int Offs)
{
	if (Offs > 0) {
       		stringstream strm;
		strm << PatName << "+" << Offs;
       		str = strm.str().c_str();
	}
	else
		str = PatName;
}


static void FormatScanInfo(StringS &str, int scan_reg, int scan_bit)
{
	if ((scan_reg >= 0) && (scan_bit >= 0)) {
        	stringstream strm;
		strm << scan_reg << ":" << scan_bit;
        	str = strm.str().c_str();
		return;
	}
	str = "";
}

static bool CheckPatternNameSize(const ObjectM1D &Patterns, const UTL::Sites &DlogSites)
{
	int LongestNameLength = 0;
	// Get the size once, M1D is the same size across all sites
	int NumPatterns = Patterns.GetSize();
	for (SiteIter s1 = DlogSites.Begin(); !s1.End(); ++s1) {
		SITE site = *s1;
		// Index the site dimension once for higher performance
		ObjectS1D s1d = Patterns[site];
		for (int ii = 0; ii < NumPatterns; ++ii) {
			int NameLength = s1d[ii].GetName().Length();
			if (NameLength > LongestNameLength)
				LongestNameLength = NameLength;
		}
	}
	// Add a buffer of 8 characters for formatting
	LongestNameLength += 8;
	if (LongestNameLength > FPSize) {
		BHSize -= FPSize;
		FPSize = LongestNameLength;
		BHSize += FPSize;
		return true;
	}
	return false;
}

void FunctionalTestData::
FormatASCII(bool fail_only_mode, std::ostream &output)
{
	bool ShowVerbose = GetVerboseEnable();
	if (ShowVerbose && (PatPinInfo.NumRecords == 0))
		ShowVerbose = false;					// collection did not get pin information
	Sites fsites = GetDlogSites();
	const TMResultM &Res = FData.GetResult();
	if (fail_only_mode)
		(void)fsites.DisableFailingSites(Res.Equal(TM_FAIL));	// This removes anything that is not a fail due to Equal
	PinML &VerbosePins = GetVerbosePins();
	bool ShowHeaderOnce = false;
	if (GetLastFormatEvent() != DatalogMethod::FunctionalTest) {
		VerbosePins = UTL_VOID;					// make sure a new header is output
		ShowHeaderOnce = true;
	}
	if (CheckPatternNameSize(PatInfo.PatternObject, fsites))
		ShowHeaderOnce = true;
	StringS str, tdesc;
	FormatTestDescription(tdesc, FData.GetComment());
	if (ShowVerbose && (tdesc.Length() > TDSize))
	    tdesc.Erase(TDSize, tdesc.Length() - TDSize);
	int nheader_pins = (ShowVerbose) ? PatPinInfo.HeaderPins.GetSize() : 0;
	bool enhanced_chars = GetEnhancedChars();
	StringS alt_enhanced_chars;
	if (enhanced_chars) {
		StringS temp;
		if (TestProg.GetConfigVariableType("datalog", "enhanced_char_set") == "string")
			if (TestProg.GetConfigVariableValue("datalog", "enhanced_char_set", temp))
				alt_enhanced_chars = temp;
	}
	for (SiteIter s1 = fsites.Begin(); !s1.End(); ++s1) {
		SITE site = *s1;
		int nrecs = (PatInfo.NumRecords[site] < MaxNumFails) ? (int)PatInfo.NumRecords[site] : (Res[site] == TM_FAIL) ? (int)MaxNumFails : 1;
		for (int fn = 0; fn < nrecs; fn++) {
			PinML HPins = PatPinInfo.Pins;
			if (ShowVerbose && (Res[site] == TM_FAIL)) {
					// the PatPinInfo record contains a list of pin groups in the HeaderPins variable with the first index
					// containing the PatternSetup pins (same as the Pins variable). The HeaderPinIndex variable is a per
					// record, per site index into the HeaderPins array.
				PinML newPins;
                                PatPinInfo.StuffHeaderPins(site, fn, newPins);
				if (newPins.HasSameOrderAndPins(VerbosePins) == false) {
					VerbosePins = newPins;
					OutputFunctionalHeader(output, VerbosePins);
				}
			}
			else if (ShowHeaderOnce)
				OutputFunctionalHeader(output, UTL_VOID);		// header without pin header
			ShowHeaderOnce = false;
			output << setw(TNSize) << right << dec << FData.GetTestID() << "  ";
			StringS PF = (Res[site] == TM_PASS) ? GetPassString() : (Res[site] == TM_FAIL) ? "*F*" : "   ";
			output << PF << "  ";
			output << setw(4) << right << site << "  ";
			StringS str;
			FormatPatternAddr(str, PatInfo.PatternObject[site][fn], PatInfo.VecOffset[site][fn]);
			output << setw(FPSize) << left << str << "  ";
			if (PatInfo.Count[site][fn] != (unsigned) -1)
			    output << setw(CCSize) << right << PatInfo.Count[site][fn] << "  ";
			else
			    output << setw(CCSize) << right << "unknown" << "  ";
			FormatScanInfo(str, PatInfo.ScanRegister[site][fn], PatInfo.ScanBit[site][fn]);
			output << setw(SCSize) << left << str << "  ";
			if (fn > 0)
				output << setw(TDSize) << " ";
			else
				output << setw(TDSize) << left << tdesc;
			if (ShowVerbose && (Res[site] == TM_FAIL)) {
				output << "  " << setw(4) << right << PatPinInfo.FailingPinsCount[site][fn] << "  ";
				if (enhanced_chars) {
					if (alt_enhanced_chars.Length() > 0) {
						if (PatPinInfo.StuffComplexString(str, site, fn, alt_enhanced_chars) == false)
							str = PatPinInfo.DatalogChar[site][fn];
					}
					else if (PatPinInfo.StuffComplexString(str, site, fn) == false)
							str = PatPinInfo.DatalogChar[site][fn];
					output << left << str;
				}
				else {
					if (PatPinInfo.StuffPassFailString(str, site, fn, '.', 'F') == false)
						str = "";
					output << left << str;
				}
			}
			output << endl;
		}
	}
}

static struct {
	char DLChar;
	STDFV4_FTR::FTR_RetState State;
	STDFV4_FTR::FTR_RetState EnhancedState;
} FTRRetStates[] = {
	{ '.',	STDFV4_FTR::RET_UN,      STDFV4_FTR::RET_UN },
	{ 'F',	STDFV4_FTR::RET_FAIL_MB, STDFV4_FTR::RET_FAIL_MB },		// This is what it was before so I did not change it
	{ 'L',	STDFV4_FTR::RET_FAIL_MB, STDFV4_FTR::RET_FAIL_LO },
	{ 'H',	STDFV4_FTR::RET_FAIL_MB, STDFV4_FTR::RET_FAIL_HI },
	{ 'M',	STDFV4_FTR::RET_FAIL_MB, STDFV4_FTR::RET_FAIL_MB },
	{ 'V',	STDFV4_FTR::RET_FAIL_MB, STDFV4_FTR::RET_FAIL_GL }
};

void FunctionalTestData::
FormatSTDFV4(bool fail_only_mode, std::ostream &output)
{
	STDFV4Stream STDF = GetSTDFV4Stream(false);
	if (STDF.Valid()) {
		static std::vector<STDFV4_FTR::FTR_RetState> FTRRegLU;
		static std::vector<STDFV4_FTR::FTR_RetState> FTRRegLUEnhanced;
		static std::vector<STDFV4_FTR::FTR_ProgState> FTRProgLU;
		if (FTRRegLU.size() == 0) {
			int num_char = 0;
			int ii = 0;
			for (ii = 0; ii < sizeof(FTRRetStates) / sizeof(FTRRetStates[0]); ii++)
				if (FTRRetStates[ii].DLChar >= num_char)
					num_char = (int)FTRRetStates[ii].DLChar + 1;
			FTRRegLU.resize(num_char, STDFV4_FTR::NO_RET_STATE);
			for (ii = 0; ii < sizeof(FTRRetStates) / sizeof(FTRRetStates[0]); ii++)
				FTRRegLU[FTRRetStates[ii].DLChar] = FTRRetStates[ii].State;
		}
		if (FTRRegLUEnhanced.size() == 0) {
			int num_char = 0;
			int ii = 0;
			for (ii = 0; ii < sizeof(FTRRetStates) / sizeof(FTRRetStates[0]); ii++)
				if (FTRRetStates[ii].DLChar >= num_char)
					num_char = (int)FTRRetStates[ii].DLChar + 1;
			FTRRegLUEnhanced.resize(num_char, STDFV4_FTR::NO_RET_STATE);
			for (ii = 0; ii < sizeof(FTRRetStates) / sizeof(FTRRetStates[0]); ii++)
				FTRRegLUEnhanced[FTRRetStates[ii].DLChar] = FTRRetStates[ii].EnhancedState;
		}
		Sites fsites = GetDlogSites();
		const TMResultM &Res = FData.GetResult();
		if (fail_only_mode)
			(void)fsites.DisableFailingSites(Res.Equal(TM_FAIL));	// This removes anything that is not a fail due to Equal
		StringS str, tdesc;
		FormatTestDescription(tdesc, FData.GetComment());
		bool ShowVerbose = GetVerboseEnable();
	        bool enhanced_chars = GetEnhancedChars();
		STDFV4_FTR FTR;
		FTR.SetInfo(FData.GetTestID(), tdesc);
		for (SiteIter s1 = fsites.Begin(); !s1.End(); ++s1) {
			SITE site = *s1;
			int nrecs = (PatInfo.NumRecords[site] < MaxNumFails) ? (int)PatInfo.NumRecords[site] : (Res[site] == TM_FAIL) ? (int)MaxNumFails : 1;
			for (int rec = 0; rec < nrecs; rec++) {
				FTR.SetContext(site);
				FTR.SetFTRInfo(site, rec, PatInfo, PatPinInfo, enhanced_chars ? FTRRegLUEnhanced : FTRRegLU, FTRProgLU, ShowVerbose, ShowVerbose, false);
				STDF.Write(FTR);
			}
		}
	}
}

DatalogData *QCOM_Datalog::
FunctionalTest(const DatalogBaseUserData *udata)
{
	const DatalogFunctional *fdata = dynamic_cast<const DatalogFunctional *>(udata);
	if (fdata != NULL) {
		SummaryNeeded = true;
		return new FunctionalTestData(*this, *fdata);
 	}
	return NULL;
}

// *****************************************************************************// Text
// Scan Test

class ScanTestData : public QCOM_DatalogData {
public:
	ScanTestData(QCOM_Datalog &, const DatalogFunctional &);
	~ScanTestData();

	virtual void Format(const char *format, bool fail_only_mode, std::ostream &output);
	const DatalogFunctional &GetFuncData() const;
	const IntS &GetMaxNumFails() const;
	const DigitalPatternInfoStruct &GetPatInfo() const;
	const DigitalPatternPinStruct &GetPatPinInfo() const;
private:
	DatalogFunctional FData;			// data passed in from Functional BIF
	IntS MaxNumFails;				// requested maximum number of fails, user can cause extra collection
	DigitalScanInfoStruct ScanInfo;			// data collected from DIGITAL driver
	void FormatASCII(bool fail_only_mode, std::ostream &output);
	void FormatSTDFV4(bool fail_only_mode, std::ostream &output);
};

ScanTestData::
ScanTestData(QCOM_Datalog &parent, const DatalogFunctional &fdata) : 
	QCOM_DatalogData(DatalogMethod::ScanTest, parent),
	FData(fdata),
	MaxNumFails(TestProg.GetNumberOfScanFails()),
	ScanInfo(DIGITAL.GetScanInfo())
{
	IncNumTestsExecuted();
}

ScanTestData::
~ScanTestData()
{
}

void ScanTestData::
Format(const char *format, bool fail_only_mode, std::ostream &output)
{
        if (format != NULL) {
                if (format[0] == formats[ASCII_INDEX][0])
                        FormatASCII(fail_only_mode, output);
                else if (format[0] == formats[STDFV4_INDEX][0])
                        FormatSTDFV4(fail_only_mode, output);
                SetLastFormatEvent();
        }
}

const int EXSize = 3;

static void CheckPatternNameSize(const StringS1D &PatternNames)
{
	int LongestNameLength = 0;
	int NumPatterns = PatternNames.GetSize();
	for (int ii = 0; ii < NumPatterns; ++ii) {
		int NameLength = PatternNames[ii].Length();
		if (NameLength > LongestNameLength)
			LongestNameLength = NameLength;
	}
	// Add a buffer of 8 characters for formatting
	LongestNameLength += 8;
	if (LongestNameLength > FPSize) {
		BHSize -= FPSize;
		FPSize = LongestNameLength;
		BHSize += FPSize;
	}
}

void ScanTestData::
FormatASCII(bool fail_only_mode, std::ostream &output)
{
	Sites fsites = GetDlogSites();
	const TMResultM &Res = FData.GetResult();
	StringS tdesc;
	FormatTestDescription(tdesc, FData.GetComment());
	bool need_header = true;
	CheckPatternNameSize(ScanInfo.Patterns);
	for (SiteIter s1 = fsites.Begin(); !s1.End(); ++s1) {
                SITE site = *s1;
		if (need_header) {
			output << endl;
        		output << setw(TNSize) << left << "Test_No." << "  P/F  Site  ";
			output << setw(CCSize) << left << "Count" << "  ";
			output << setw(PGSize) << left << "Pin_Name" << "  ";
			output << setw(EXSize) << left << "Exp" << "  ";
			output << setw(FPSize) << left << "Pattern" << "  ";
			output << setw(SCSize) << left << "ScanVec:Bit" << "  ";
        		output << setw(TDSize) << left << "Test_Description" << "  ";
			output << endl;
        		OutputBorder(output, TNSize, 2);	// TestID
        		OutputBorder(output, 3, 2);		// P/F
        		OutputBorder(output, 4, 2);		// Site
        		OutputBorder(output, CCSize, 2);	// Count
			OutputBorder(output, PGSize, 2);	// Pin
        		OutputBorder(output, EXSize, 2);	// Expect
        		OutputBorder(output, FPSize, 2);	// Pattern
        		OutputBorder(output, SCSize, 2);	// Scan
        		OutputBorder(output, TDSize, 2);	// Description
			output << endl;
			need_header = false;
		}
		if (ScanInfo.NumRecords[site] > 0) {
			int pat_index = 0;
			unsigned int pat_end = ScanInfo.PatternCounts[0];
			for (int ii = 0; ii < ScanInfo.NumRecords[site]; ii++) {
				unsigned int cycle = ScanInfo.FailCount[site][ii];
				if (cycle > pat_end) {
					for (int zz = pat_index + 1; zz < ScanInfo.Patterns.GetSize(); zz++) {
						pat_index++;
						pat_end += ScanInfo.PatternCounts[zz];
						if (cycle <= pat_end)
							break;
					}
				}
				output << setw(TNSize) << right << dec << FData.GetTestID() << "  ";
				StringS PF = "*F*";
				output << PF << "  ";
				output << setw(4) << right << site << "  ";
				output << setw(CCSize) << right << cycle << "  ";
				output << setw(PGSize) << left << ScanInfo.Pins[ScanInfo.FailPin[site][ii]] << "  ";
				output << ScanInfo.ExpectAlias[site][ii] << "    ";
                                StringS str;
                                FormatPatternAddr(str, ScanInfo.Patterns[pat_index], ScanInfo.PatternVec[site][ii]);
				output << setw(FPSize) << left << str << "  ";
				str.Erase();
				FormatScanInfo(str, ScanInfo.ScanRegister[site][ii], ScanInfo.ScanBit[site][ii]);
				output << setw(SCSize) << left << str << "  ";
				if (ii == 0)
					output << setw(TDSize) << left << tdesc;
				output << endl;
			}
		}
		else {
			int npats = ScanInfo.Patterns.GetSize();
			UTL::Pattern end_pat = (npats > 0) ? ScanInfo.Patterns[npats - 1] : UTL::Pattern();
			int vec = (end_pat.Valid()) ? end_pat.GetNumberOfVectors() - 1 : -1;
			output << setw(TNSize) << right << dec << FData.GetTestID() << "  ";
			StringS PF = " P ";
			output << PF << "  ";
			output << setw(4) << right << site << "  ";
			output << setw(CCSize) << right << ScanInfo.BurstCount << "  ";
			output << setw(PGSize) << " " << "  ";
			output << " " << " " << "   ";
                        StringS str;
			FormatPatternAddr(str, end_pat, vec);
			output << setw(FPSize) << left << str << "  ";
			output << setw(SCSize) << " " << "  ";
			output << setw(TDSize) << left << tdesc;
			output << endl;
		}
	}
}

void ScanTestData::
FormatSTDFV4(bool fail_only_mode, std::ostream &output)
{
	STDFV4Stream STDF = GetSTDFV4Stream(false);
	if (STDF.Valid()) {
		Sites fsites = GetDlogSites();
		StringS tdesc;
		FormatTestDescription(tdesc, FData.GetComment());
		int psr_ref = STDFV4_PSR::FindTestSequence(ScanInfo.BurstName);
		STDFV4_PSR PSR;		// Always write PSR, the stream will decide when not to write it out
		PSR.SetTestSequence(ScanInfo.BurstName);
		PSR.SetPatternInfo(ScanInfo.Patterns, ScanInfo.PatternCounts);
		STDF.Write(PSR);
		for (SiteIter s1 = fsites.Begin(); !s1.End(); ++s1) {
                	SITE site = *s1;
			STDFV4_STR STR;
			STR.SetContext(FData.GetTestID(), site, tdesc);
			STR.SetSTRInfo(psr_ref, ScanInfo);
			STDF.Write(STR);
		}
	}
}

DatalogData *QCOM_Datalog::
ScanTest(const DatalogBaseUserData *udata)
{
	const DatalogFunctional *fdata = dynamic_cast<const DatalogFunctional *>(udata);
	if ((fdata != NULL) && (EnableScan2007.GetValue() == true) && DIGITAL.GetScanInfoAvailable()) {
		SummaryNeeded = true;
		return new ScanTestData(*this, *fdata);
 	}
	return NULL;
}

// ***************************************************************************** 
// Text


class TextData : public QCOM_DatalogData {
public:
	TextData(QCOM_Datalog &, const DatalogText &);
	~TextData();

	virtual void Format(const char *format, bool fail_only_mode, std::ostream &output);
private:
	DatalogText TData;

	void FormatASCII(bool fail_only_mode, std::ostream &output);
	void FormatSTDFV4(bool fail_only_mode, std::ostream &output);
};

TextData::
TextData(QCOM_Datalog &parent, const DatalogText &tdata) : 
	QCOM_DatalogData(DatalogMethod::Text, parent),
	TData(tdata)
{
}

TextData::
~TextData()
{
}

void TextData::
Format(const char *format, bool fail_only_mode, std::ostream &output)
{
	if (format != NULL) {
		if (format[0] == formats[ASCII_INDEX][0])
			FormatASCII(fail_only_mode, output);
		else if (format[0] == formats[STDFV4_INDEX][0])
			FormatSTDFV4(fail_only_mode, output);
		SetLastFormatEvent();
	}
}

void TextData::
FormatASCII(bool fail_only_mode, std::ostream &output)
{
	if (fail_only_mode == false) {
		if (TData.GetIsDebug() == false) {
			if (GetLastFormatEvent() != DatalogMethod::Text)
                		output << endl;
			output << TData.GetText() << endl;
		}
		else if (GetDebugEnable()) {
			if (GetLastFormatEvent() != DatalogMethod::Text)
                		output << endl;
			output << "DEBUG TEXT: " << TData.GetText() << endl;
		}
	}
}

void TextData::
FormatSTDFV4(bool fail_only_mode, std::ostream &output)
{
	STDFV4Stream STDF = GetSTDFV4Stream(false);
	if (STDF.Valid() && (TData.GetIsDebug() == false)) {
		STDFV4_DTR DTR;
		DTR.SetText(TData.GetText());
		STDF.Write(DTR);
	}
}

DatalogData *QCOM_Datalog::
Text(const DatalogBaseUserData *udata)
{
	const DatalogText *tdata = dynamic_cast<const DatalogText *>(udata);
	if (tdata != NULL) {
		SummaryNeeded = true;
		return new TextData(*this, *tdata);
 	}
	return NULL;
}

// ***************************************************************************** 
// Generic

class GenericData : public QCOM_DatalogData {
public:
	GenericData(QCOM_Datalog &, const DatalogGeneric &);
	~GenericData();

	virtual void Format(const char *format, bool fail_only_mode, std::ostream &output);
private:
	DatalogGeneric GData;

	void FormatASCII(bool fail_only_mode, std::ostream &output);
	void FormatSTDFV4(bool fail_only_mode, std::ostream &output);
};

GenericData::
GenericData(QCOM_Datalog &parent, const DatalogGeneric &gdata) : 
	QCOM_DatalogData(DatalogMethod::Generic, parent),
	GData(gdata)
{
}

GenericData::
~GenericData()
{
}

void GenericData::
Format(const char *format, bool fail_only_mode, std::ostream &output)
{
	if (format != NULL) {
		if (format[0] == formats[ASCII_INDEX][0])
			FormatASCII(fail_only_mode, output);
		else if (format[0] == formats[STDFV4_INDEX][0])
			FormatSTDFV4(fail_only_mode, output);
		SetLastFormatEvent();
	}
}

const int GSSize = (	TNSize + 	// TestID
			2 + 		// Space
			3 + 		// P/F
			2);		// Space
const int GDSize = 64;
const int GDAllSize = 1024;

static void DisplaySGenData(const BasicVar &Val, std::ostream &output, int index)
{
	SV_TYPE type = Val.GetType();
	StringS units, str;
	double scale = 1.0;
	if (type == SV_FLOAT)
		scale = DatalogBaseUserData::CalculateAutoRangeUnitScale(units, Val);
	output << setw(TNSize) << right << index << "       ";
	switch(type) {
		case SV_FLOAT:
			DatalogBaseUserData::FormatSVData(str, Val, VASize, scale);
			output << setw(VASize) << right << str << units << endl;
			break;
		case SV_INT:
		case SV_UINT:
		case SV_STRING:
		case SV_ENUM:
		case SV_BOOL:
			DatalogBaseUserData::FormatSVData(str, Val, GDAllSize, scale);
			output << left << str << endl;
			break;
		default:
			output << left << "** Unsupported variable type found in array **" << endl;
			break;
	}
}

static void DisplayMGenData(const BasicVar &Val, std::ostream &output, SITE site, int index)
{
	SV_TYPE type = Val.GetType();
	switch(type) {
		case SV_FLOAT: {
				const FloatM FV = Val.GetFloatM();
				if (FV.Valid())
					DisplaySGenData(FV[site], output, index);
			}
			break;
		case SV_INT: {
				const IntM FV = Val.GetIntM();
				if (FV.Valid())
					DisplaySGenData(FV[site], output, index);
			}
			break;
		case SV_UINT: {
				const UnsignedM FV = Val.GetUnsignedM();
				if (FV.Valid())
					DisplaySGenData(FV[site], output, index);
			}
			break;
		case SV_STRING: {
				const StringM SV = Val.GetStringM();
				if (SV.Valid())
					DisplaySGenData(SV[site], output, index);
			}
			break;
		case SV_ENUM: {
				const BasicEnumM EV = Val.GetEnumM();
				if (EV.Valid())
					DisplaySGenData(EV[site], output, index);
			}
			break;
		case SV_BOOL: {
				const BoolM BV = Val.GetBoolM();
				if (BV.Valid())
					DisplaySGenData(BV[site], output, index);
			}
			break;
		case SV_PIN: {
				const PinM Pin = Val.GetPinM();
				if (Pin.Valid()) {
					StringS str = Pin.GetName();
					output << setw(TNSize) << right << index << "       " << left << str << endl;
				}
			}
			break;
		default:
			DisplaySGenData(Val, output, index);			// Will process error message
			break;
	}
}

static void DisplayS1DGenData(const BasicVar &Val, std::ostream &output, int index, int site = 0, bool first_site = false)
{
	SV_TYPE type = Val.GetType();
	switch(type) {
		case SV_FLOAT: {
				StringS units, str;
				double scale = 1.0;
				scale = DatalogBaseUserData::CalculateAutoRangeUnitScale(units, Val);
				int Len = DatalogBaseUserData::GetNumberOfElements(Val);
				for (int ii = 0; ii < Len; ii++) {
					if (ii == 0)
						output << setw(TNSize) << right << index << "       ";
					else
						output << setw(GSSize) << " ";
					DatalogBaseUserData::FormatSVData(str, Val, ii, VASize, scale);
					output << setw(VASize) << right << str << units << endl;
				}
			}
			break;
		case SV_INT:
		case SV_UINT:
		case SV_STRING:
		case SV_ENUM:
		case SV_BOOL: {
				StringS units, str;
				double scale = 1.0;
				int Len = DatalogBaseUserData::GetNumberOfElements(Val);
				for (int ii = 0; ii < Len; ii++) {
					if (ii == 0)
						output << setw(TNSize) << right << index << "       ";
					else
						output << setw(GSSize) << " ";
					DatalogBaseUserData::FormatSVData(str, Val, ii, GDAllSize, scale);
					output << left << str << endl;
				}
			}
			break;
		default:	
			DisplaySGenData(Val, output, index);		// Will process error message
			break;
	}
}

static void DisplayM1DGenData(const BasicVar &Val, std::ostream &output, SITE site, int index)
{
	SV_TYPE type = Val.GetType();
	switch(type) {
		case SV_FLOAT: {
				const FloatM1D FV = Val.GetFloatM1D();
				if (FV.Valid())
					DisplayS1DGenData(FV[site], output, index);
			}
			break;
		case SV_INT: {
				const IntM1D FV = Val.GetIntM1D();
				if (FV.Valid())
					DisplayS1DGenData(FV[site], output, index);
			}
			break;
		case SV_UINT: {
				const UnsignedM1D FV = Val.GetUnsignedM1D();
				if (FV.Valid())
					DisplayS1DGenData(FV[site], output, index);
			}
			break;
		case SV_STRING: {
				const StringM1D SV = Val.GetStringM1D();
				if (SV.Valid())
					DisplayS1DGenData(SV[site], output, index);
			}
			break;
		case SV_ENUM: {
				const BasicEnumM1D EV = Val.GetEnumM1D();
				if (EV.Valid())
					DisplayS1DGenData(EV[site], output, index);
			}
			break;
		case SV_BOOL: {
				const BoolM1D BV = Val.GetBoolM1D();
				if (BV.Valid())
					DisplayS1DGenData(BV[site], output, index);
			}
			break;
		default:
			DisplaySGenData(Val, output, index);			// Will process error message
			break;
	}
}

static void DisplaySLGenData(const BasicVar &Val, std::ostream &output, int index)
{
	SV_TYPE type = Val.GetType();
	switch(type) {
		case SV_FLOAT: {
				StringS units, str;
				double scale = 1.0;
				if (type == SV_FLOAT)
					scale = DatalogBaseUserData::CalculateAutoRangeUnitScale(units, Val);
				int Len = DatalogBaseUserData::GetNumberOfElements(Val);
				for (int ii = 0; ii < Len; ii++) {
					if (ii == 0)
						output << setw(TNSize) << right << index << "       ";
					else
						output << setw(GSSize) << " ";
					DatalogBaseUserData::FormatSVData(str, Val, ii, VASize, scale);
					output << setw(VASize) << right << str << units << endl;
				}
			}
			break;
		case SV_INT:
		case SV_UINT:
		case SV_STRING:
		case SV_ENUM:
		case SV_BOOL: {
				StringS units, str;
				double scale = 1.0;
				int Len = DatalogBaseUserData::GetNumberOfElements(Val);
				for (int ii = 0; ii < Len; ii++) {
					if (ii == 0)
						output << setw(TNSize) << right << index << "       ";
					else
						output << setw(GSSize) << " ";
					DatalogBaseUserData::FormatSVData(str, Val, ii, GDAllSize, scale);
					output << left << str << endl;
				}
			}
			break;
		default:
			DisplaySGenData(Val, output, index);		// Will process error message
			break;
	}
}

static void DisplayMLGenData(const BasicVar &Val, std::ostream &output, SITE site, int index)
{
	SV_TYPE type = Val.GetType();
	switch(type) {
		case SV_FLOAT: {
				const FloatML FV = Val.GetFloatML();
				if (FV.Valid())
					DisplaySLGenData(FV[site], output, index);
			}
			break;
		case SV_INT: {
				const IntML FV = Val.GetIntML();
				if (FV.Valid())
					DisplaySLGenData(FV[site], output, index);
			}
			break;
		case SV_UINT: {
				const UnsignedML FV = Val.GetUnsignedML();
				if (FV.Valid())
					DisplaySLGenData(FV[site], output, index);
			}
			break;
		case SV_STRING: {
				const StringML SV = Val.GetStringML();
				if (SV.Valid())
					DisplaySLGenData(SV[site], output, index);
			}
			break;
		case SV_ENUM: {
				const BasicEnumML EV = Val.GetEnumML();
				if (EV.Valid())
					DisplaySLGenData(EV[site], output, index);
			}
			break;
		case SV_PIN: {
				const PinML Pins = Val.GetPinML();
				StringS str;
				DatalogBaseUserData::FormatPins(str, Pins, GDSize);
				output << setw(TNSize) << right << index << "       " << left << str << endl;
			}
			break;
		default:
			DisplaySGenData(Val, output, index);			// Will process error message
			break;
	}
}

static void DisplayArrOfGenData(const BasicVar &Val, std::ostream &output, SITE site, int &index)
{
	const ArrayOfBasicVar Arr = Val.GetArrayOfBasicVar();
	int size = (Arr.Valid()) ? Arr.GetSize() : 0;
	for (int ii = 0; ii < size; ii++, index++) {
		if (Arr[ii].Valid()) {
			SV_CONFIG Cfg = Arr[ii].GetConfig();
			switch(Cfg) {
			 	case SV_SCALAR_S:
					DisplaySGenData(Arr[ii], output, index);
					break;
				case SV_SCALAR_M:
					DisplayMGenData(Arr[ii], output, site, index);
					break;
				case SV_ARRAY_S1D:
					DisplayS1DGenData(Arr[ii], output, index);
					break;
				case SV_ARRAY_M1D:
					DisplayM1DGenData(Arr[ii], output, site, index);
					break;
				case SV_LIST_S:
					DisplaySLGenData(Arr[ii], output, index);
					break;
				case SV_LIST_M:
					DisplayMLGenData(Arr[ii], output, site, index);
					break;
				default:
					if (Arr[ii].GetType() == SV_ARRAY_OF)
						DisplayArrOfGenData(Arr[ii], output, site, index);
					else
						output << setw(GSSize + 6) << " " << left << "QCOM_Datalog::Generic - unsupported variable configuration found in array at index " << index << "." << endl;
					break;
			}
		}
	}
}

static bool GDR_HasMultisiteValues(const Sites &sites, const BasicVar &Val)
{
	if (sites.GetNumSites() < 2)
		return false;
	const ArrayOfBasicVar Arr = Val.GetArrayOfBasicVar();
	int size = (Arr.Valid()) ? Arr.GetSize() : 0;
	for (int ii = 0; ii < size; ii++) {
		if (Arr[ii].Valid()) {
			SV_CONFIG Cfg = Arr[ii].GetConfig();
			switch(Cfg) {
				case SV_SCALAR_M:
				case SV_ARRAY_M1D:
				case SV_LIST_M:
					return true;
				case SV_LIST_S:
				case SV_ARRAY_S1D:
			 	case SV_SCALAR_S:
					break;
				default:
					if (Arr[ii].GetType() == SV_ARRAY_OF)
						if (GDR_HasMultisiteValues(sites, Arr[ii]))
							return true;
					break;
			}
		}
	}
	return false;
}

void GenericData::
FormatASCII(bool fail_only_mode, std::ostream &output)
{
	const ArrayOfBasicVar &Arr = GData.GetData();
	int size = Arr.GetSize();
	if ((fail_only_mode == false) && (size > 0)) {
		const Sites &dlog_sites = GetDlogSites();
		if (GDR_HasMultisiteValues(dlog_sites, Arr)) {
			for (SiteIter s1 = dlog_sites.Begin(); !s1.End(); ++s1) {
				SITE site = s1.GetValue();
				output << setw(TNSize) << "Index" << setw(2+3+2) << " " << "Generic Data for site " << site << endl;
				OutputBorder(output, TNSize, 2+3+2);
				OutputBorder(output, GDSize, 0);
				output << endl;
				int index = 0;
				DisplayArrOfGenData(Arr, output, site, index);
			}
		}
		else {
			output << setw(TNSize) << "Index" << setw(2+3+2) << " " << "Generic Data" << endl;
			OutputBorder(output, TNSize, 2+3+2);
			OutputBorder(output, GDSize, 0);
			output << endl;
			int index = 0;
			DisplayArrOfGenData(Arr, output, dlog_sites.Begin().GetValue(), index);
		}
	}
}

void GenericData::
FormatSTDFV4(bool fail_only_mode, std::ostream &output)
{
	STDFV4Stream STDF = GetSTDFV4Stream(false);
	if (STDF.Valid()) {
		const ArrayOfBasicVar &Arr = GData.GetData();
		int size = Arr.GetSize();
		if (size > 0) {
			STDFV4_GDR GDR;
			BoolM res = GDR.SetGenericData(Arr);
			if (res == true) 			// all sites fit into the GDR
				STDF.Write(GDR);
			else if (res == false) {		// all site will not fit
				const Sites &dlog_sites = GetDlogSites();
				if (GDR_HasMultisiteValues(dlog_sites, Arr))
					ERR.ReportError(ERR_GENERIC_ADVISORY, "QCOM_Datalog::Generic: All site data was longer than STDFv4 limit for GDR records.", Arr, NO_SITES, UTL_VOID);
				else
					ERR.ReportError(ERR_GENERIC_ADVISORY, "QCOM_Datalog::Generic: Data was longer than STDFv4 limit for GDR records.", Arr, NO_SITES, UTL_VOID);
			}
			else {					// some sites fit
				const Sites &dlog_sites = GetDlogSites();
				Sites good_sites;
				for (SiteIter s1 = dlog_sites.Begin(); !s1.End(); ++s1)
					if (res[*s1])
						good_sites += *s1;
					else
						ERR.ReportError(ERR_GENERIC_ADVISORY, "QCOM_Datalog::Generic: Site data was longer than STDFv4 limit for GDR records.", Arr, *s1, UTL_VOID);
				Sites save_sites = ActiveSites;
				RunTime.SetActiveSites(good_sites);
				STDF.Write(GDR);
				RunTime.SetActiveSites(save_sites);
			}
		}
	}
}

DatalogData *QCOM_Datalog::
Generic(const DatalogBaseUserData *udata)
{
	const DatalogGeneric *gdata = dynamic_cast<const DatalogGeneric *>(udata);
	if (gdata != NULL) {
		SummaryNeeded = true;
		return new GenericData(*this, *gdata);
 	}
	return NULL;
}

