#pragma once
// *************************************************************************************
//  Module      : QCOM_Datalog.h
//  Description : LTX-Credence standard ASCII and STDFV4 Datalog methods
//
//  Copyright (C) LTX-Credence Corporation 2009.  All rights reserved.
// *************************************************************************************

#include <Unison.h>
#include <DatalogMethod.h>

class QCOM_DatalogData;                    // forward reference

// The following is the main LTXC Datalog class declaration. The class is composed of:
//     A set of DatalogAttributes that compose the optional parameters for the datalogger.
//     A set of events and datalog entries to respond to.
//     A data collection class per event and entry.
//     A data formatting class per event and entry per supported format (ASCII and STDFV4).

class QCOM_Datalog : public DatalogMethod {
public:
	QCOM_Datalog();
	virtual ~QCOM_Datalog();

	/** @file
	@brief Defines @ref QCOM_DatalogAPIs "QCOM_Datalog" Default Datalog Method */
	/** @defgroup QCOM_DatalogAPIs QCOM_Datalog Default Datalog Method
	@{
	@ingroup DatalogSystem
	@par QCOM_Datalog Description

	The QCOM_Datalog method collects, formats, and displays both ASCII
	and STDFv4 datalog information. Datalog methods are used to define
	a standard datalog interface. The system software contributes a set
	of events to the Datalog methods. The @ref DLOG_BIFS contribute the application
	data events that are reported by the test program execution.
	Parametric results can be displayed in two methods:
	- row-wise -    Each site will have a new line in the ASCII datalog for each result.
	                This representation will be more readable for programs with more than
	                4 to 8 sites.
	- column-wise - Each site will have a new column in the ASCII datalog for each result.
	                This representation will be most useful for programs with fewer than
	                8 sites.  Above this site count the lines will grow too long and
	                unwieldy.
	The system does not limit the users choice by site count.  Please set this mode in a way
	that works for your application and data display needs.
	@par Supported System Datalog Events

	The System level events are called by the system software automatically upon
	operations in the system software. The Datalog method has to be enabled before
	the events will be called.

	- Start of Test -   Called upon start of a new device test or a retest.
	- End Of Test -     Called upon the end of a device test.
	- Summary -         Called upon the summary external access event and 
	                    upon an EndOfLot or EndOfWafer events.
	- EndOfLot -        Called upon the end of lot external access event.
	- EndOfWafer -      Called upon the end of wafer external access event.


	@par Supported Application Datalog Events

	The application events are called from user code or from the TestMethod code
	via the DLOG BIFs.

	- Parametric Test -             Called from @ref UTL_DLOG::Value() "DLOG.Value()" for single multisite values 
	                                or for array values with incrementing minor ID.
	- Multiple Parametric Test -    Called from @ref UTL_DLOG::Value() "DLOG.Value()" for array values with a 
	                                single minor ID.
	- Functional Test -             Called from @ref UTL_DLOG::Functional() "DLOG.Functional()".
	- Text -                        Called from @ref UTL_DLOG::Text() "DLOG.Text()" or @ref UTL_DLOG::DebugText() "DLOG.DebugText()".
	- Generic Data -                Called from @ref UTL_DLOG::Generic() "DLOG.Generic()".
	- Scan Test -                   Called from @ref UTL_DLOG::Functional() "DLOG.Functional()" if EnableScan2007 is enabled.

	@par  User Configurable Attributes

	The user can configure the QCOM_Datalog method to operate based off
	the following set of configuration attributes. These attributes are
	shown in the Datalog menu once the method has been added to a given
	datalog slot for use.

	- AppendPinName -                   If enabled, the pin name will be appended to the test
	                                    name for all single-pin parametric datalog outputs.
	- ASCIIDatalogInColumns -           If enabled, multisite ASCII parametric datalog data
	                                    will be presented in a column format with one column
	                                    per site for the measured values.  Unison allows per-site
	                                    limits though their use is not recommended.  In the column
	                                    mode of the datalogger the limits are taken from the
	                                    first tested site, as there is only one set of limits
	                                    displayed for all sites.
	- EnableDebugText -                 If enabled, all strings sent to DLOG.DebugText will
	                                    be added to the ASCII datalog stream. DebugText does
	                                    not contribute to the STDFv4 stream.
	- EnableVerbose -                   At this time setting this to true will output per 
	                                    pin information to the Functional Test output. 
	                                    Applicable to both ASCII and STDFv4 outputs. It should
                                            be noted that this feature adds additional runtime
                                            overhead to the datalog.
	- EnhancedFunctionalChars -         If enabled and EnableVerbose enabled, the LTXC specific 
                                            set of datalog characters will be collected and contributed to 
                                            the output. This feature is a generic feature used for debug. 
                                            It  will add additional runtime overhead so caution should 
                                            be used to not leave it enabled during production. The
                                            default enhanced characters are:
                                            'L' : compare low fail
                                            'H' : compare high fail
                                            'M' : compare midband fail
                                            'V' : compare valid fail
                                            The characters can be changed by specifying alternative
                                            characters in options.cfg or local_options.cfg in the
                                            datalog section: enhanced_char_set = "LHMV"
                                            Substitute your characters in place of L, H, M, V.
	- PerSiteSummary -                  If enabled, per site versions of all summary events
	                                    will be added to the output along with the overall
	                                    totals. Applicable to both ASCII and STDFv4 outputs.
	- UnitAutoscaling -                 If enabled, the unit display will automatically scale
	                                    to an appropriate engineering unit multiplier.  If
	                                    disabled then the unit chosen by the user in the
	                                    DLOG.Value statement or in the Unit field in the Limit
	                                    Structure will be used without any automatic scaling.
	- EnableScan2007 -                  If enabled, any Scan pattern execution will generate a
                                            Scan datalog record. For STDFv4 streams this will result
                                            in the addition of STR, PSR, and VUR records in the file.
                                            The number fails pin and cycle count fails will be limited
                                            by the Scan Fail Count variable in the Datalog setup menu.
	- ASCIIOptimizeForUnscaledValues -  If enabled, a larger width is used for the integer part
                                            of floating point values in ASCII output.  This wider
                                            representation is intended for cases where the user wants
                                            to use unscaled values.  It does not affect STDF output.
                                            

	@par Summary Data Collection

	The user can enable or disable collection of TSR results using the Datalog menu in
	the operator panel. If collection of TSR results is enabled the DLOG BIFs will collect
	the data and upon the Summary event, the information will be read by the Datalog method
	and processed.

	@par STDF Data Compression
	The STDF content is automatically compressed using the techniques specified in the STDFv4
	specification. That compression is applied to PTR, MPR, and FTR record types. Valid PMR
	records are put in for all sites so that the resource names are available but all records
	that refer to the PMR  refer to the first loaded site.

	@par Source Code Supplied
	
	The source code for the QCOM_Datalog is supplied in the operating system,
	in the directory $LTXHOME/unison/ltx/methods/LTXCDatalogMethods<br>
	This code can be used as a starting point for datalog customizations, if needed.

	    */
	/** @} */

	bool GetSummaryNeeded() const;

private:
	bool SummaryNeeded;                             // set to true when data is made available
	FlowNode CurrentFN;                             // Active FlowNode name
	StringS CurrentBlock;                           // Active TestBlock
	DatalogMethod::SystemEvents LastFormatEvent;    // Last formatted event type
	DatalogAttribute PerSiteSummary;                // Display per site summary information or not
	DatalogAttribute EnableVerbose;                 // Display verbose information or not
	DatalogAttribute EnableDebug;                   // Display DebugText information or not
	DatalogAttribute UnitAutoscaling;               // Autoscale units to engineering multipliers
	DatalogAttribute ASCIIDatalogInColumns;         // Autoscale units to engineering multipliers
	DatalogAttribute AppendPinName;                 // Append pin name for parametric results
	DatalogAttribute EnhancedFuncChars;             // Complex functional datalog characters
	DatalogAttribute EnableFullOpt;                 // LTXC specific optimization versus STDFV4 specified
	DatalogAttribute EnableScan2007;		// Enabled STDF V4 2007.1 Scan support
	DatalogAttribute ASCIIOptimizeForUnscaledValues;// Use larger width for integer part
	PinML VerbosePins;                              // Cache for functional verbose pin header
	UnsignedM NumTestsExecuted;                     // Number of PTR, MPR, and FTRs executed in last run
	FloatS FinishTime;                              // Time of last execution, updated at EOT
	int FieldWidth;                                 // this one is set by the QCOM_Datalog_FieldWidth OpVar if present.
	                                                // Checked and changed in StartOfTest event
	StringS PassString;                             // Stores pass string value

	QCOM_Datalog(const QCOM_Datalog &);             // disable copy
	QCOM_Datalog &operator=(const QCOM_Datalog &);  // disable copy

	// The following are the data collection methods
	DatalogData *StartOfTest(const DatalogBaseUserData *);
	DatalogData *EndOfTest(const DatalogBaseUserData *);
	DatalogData *ProgramLoad(const DatalogBaseUserData *);
	DatalogData *ProgramUnload(const DatalogBaseUserData *);
	DatalogData *ProgramReset(const DatalogBaseUserData *);
	DatalogData *Summary(const DatalogBaseUserData *);
	DatalogData *StartOfWafer(const DatalogBaseUserData *);
	DatalogData *EndOfWafer(const DatalogBaseUserData *);
	DatalogData *StartOfLot(const DatalogBaseUserData *);
	DatalogData *EndOfLot(const DatalogBaseUserData *);
	DatalogData *StartTestNode(const DatalogBaseUserData *);
	DatalogData *StartTestBlock(const DatalogBaseUserData *);
	DatalogData *ParametricTest(const DatalogBaseUserData *);
	DatalogData *ParametricTestArray(const DatalogBaseUserData *);
	DatalogData *FunctionalTest(const DatalogBaseUserData *);
	DatalogData *Text(const DatalogBaseUserData *);
	DatalogData *Generic(const DatalogBaseUserData *);
	DatalogData *ScanTest(const DatalogBaseUserData *);

	friend class QCOM_DatalogData;
};
