TO-DO List:
- 	log the status buffer from connection functions in CTester::connect(). this is helpful in finding the root cause of APL not able to connect to tester in PAx testers. 
	you will be able to see the difference in the path where APL looks for tester instance as compared to evxa_demo_program.
-	figure out the root cause why in PAx testers, APL looks for tester instance in /opt/ltx/release/ltx/FX1/testers instead of /opt/ltx/testers like ecvxa_demo_program does


RELEASE NOTES:

version beta.2.13.20200114
-	quality of life improvements
	-	the pop-up message box that warns operator before proceeding to load new program upon receiving new lotinfo.txt file is now larger and easier to see
	-	also, the pop-up message will remain at the center and top of the screen so operator will never miss it.

-	update feature
	-	bin solution for wafer test now appends WAFERID instead of LOTID


version beta.2.11.20191105
-	quality of life improvements
	- rename widget app title to "APL Commander 1.0"

-	bug fixes
	- fixed bug where APL is not reading <FAMODULE> state in config.xml, thus making it false all the time. (thanks to cedric for catching this bug)
		- also, FAMODULE is now enabled by default. 
	- when APL renames STDF (qualcomm requirement), it makes sure STDF's MIR.NODE_NAM matches the tester name before doing so. this was unintenionally disabled. now fixed. 
	- fixed setLotInformation() to write RTST_COD. changed EVX_LOT_TYPE from LotLotStatus to LotLotState

- 	update feature (STEP value setting also added CMOD_COD)
	- added another STDF field that corresponds to STEP value - MIR.CMOD_COD. it can now be added as shown below:
		<LotInfo>
			<Step state = "true">
				<Param flow_id = "FT1" rtst_cod = "0" cmod_cod = "F">FT1</Param>
				<Param flow_id = "FT1" rtst_cod = "1" cmod_cod = "F">Rt1</Param>
				<Param flow_id = "FT2" rtst_cod = "0" cmod_cod = "F">FT2</Param>
				<Param flow_id = "QT2" rtst_cod = "2" cmod_cod = "Q">2RT2</Param>
				<Param flow_id = "QT1" rtst_cod = "3" cmod_cod = "Q">RT3</Param>
			</Step>
		</LotInfo>

-	new feature (for Qualcomm, MIR.ENG_ID = MIR.LOT_ID, MIR.FACIL_ID = <Test Site> 
	- for Qualcomm customer, MIG.ENG_ID = MIR.LOT_ID while MIR.FACIL_ID must be set to test site value which can be specified in config.xml as shown below
		<Launch>
			<Param name = "Test Site">AMKKR3-S</Param>
		</Launch>
	- in order to achieve this, APL must be informed if device being tested belong to Qualcomm. There are 2 ways to do this:
		- through lotinfo.txt file CUSTOMER: QUALCOMM
		- through config.xml 
			<Launch>
				<Param name = "Customer">qualcomm</Param>
			</Launch>
	- why are there 2 options to do this? because we cannot guarantee Amkor will always fill lotinfo.txt file with CUSTOMER field.
	- the CUSTOMER field from lotinfo.txt file will always take precedence. the only time CUSTOMER field in config.xml is used is when CUSTOMER field
	  is not used in lotinfo.txt file.
	- if neither config.xml nor lotinfo.txt file specify the CUSTOMER, APL will default to none, in which case it will not set MIR.ENG_ID and MIR.FACIL_ID
	
-	update feature (enabling/disabling lotinfo.txt to FAModule)
	- feature that sends lotinfo.txt to FAModule (ST Galileo flow) which is part of uprodtool can now be disabled
		<SiteConfiguration name = "ATK3">
				<FAMODULE state = "false" />
		</SiteConfiguration>

-	new feature (setting NewLotUnison.xml)
	- previously, NewLotUnison.xml file, a file that OICU uses to customize its New Lot Input Dialog box, and path are hard coded into APL, 
	  specifically targetting Uprodtool's NewLotUnison.xml file. with APL now handling other customers too (Qualcomm), APL is updated such that 
	  you can now specify the path where it will look for the NewLotUnison.XML file. The file name can now also be specified. it can be set in
	  config.xml as shown below:
		<Launch>
			<Param name = "NewLot Config Path">/var/home/service/Desktop/Cohu_Debug/APL</Param> <!-- path to look for the file -->
			<Param name = "NewLot Config File">NewLotUnison</Param> 
		</Launch>
	- for ST, NewLotUnison.xml file name is a special case, as it uses MIR.PROC_ID as suffix. as for others (Qualcomm), it's just a standard filename.
	  to accomodate both, APL defaults to appending filename with MIR.PROC_ID suffix to meet ST format. this allows APL's latest version not to require
	  current configuration on testers running ST devices to change.
	- the path to look for NewLotUnison.xml also defaults to "$LTX_UPROD_PATH/newlot-config", which is the Uprodtool path. again, this is done to avoid
	  requiring current configuration on testers running ST devices to change.
	- if lotinfo.txt file contain field/value pair like CUSTOMER: QUALCOMM, APL will then use NewLotUnison.xml file without suffix.

-	new feature (renaming and compressing STDF format)
	- a requirement from Qualcomm is to rename STDF to match its format requirement. the date stamp they require is end-lot time stamp so the only way to do this 
	  is to rename STDF file after completion. this feature is disabled by default as other customers (ST) does not require this.
	- to enable, set in config.xml as below:
		<STDF>
			<Path>/home/localuser/synchro_sim/localuser_sim/dlog</Path> <!--path where STDF file is saved -->
			<Rename state = "true" format = "qualcomm" timestamp = "end"/> <!-- timestamp: start, end -->
			<Compress state = "true" cmd = "/usr/bin/gzip" ext = ".gz" />
		</STDF>
	- note that you can still make APL use start-lot time stamp. in the <Rename> tag, it contains "timestamp" attribute where you can set 'end' or 'start'
	- APL will only rename incoming completed STDF files in <Path> if the SRTDF's MIR.NODE_NAME matches tester name.

-	new feature (lotinfo.txt field label customization
	- lotinfo.txt file can now customize its field/value pair labels. 
	- example is MIR.LOT_ID field. if customer/amkor wants APL to set MIR.LOT_ID, it can specify any field as MIR.LOT_ID in lotinfo.txt file. 
	  customer can set something like this in lotinfo.txt file 
		- MyLotId: CUST1234
		- To tell APL that the value corresponding to field 'MyLotId" refers to MIR.LOT_ID, APL can be configured as shown below:
			<LotInfo>
				<Field>
					<Param name = "LOT_ID">MyLotId</Param>
				</Field>
			</LotInfo>
	- below is an example of several MIR and SDR fields that can be set to specific field labels
		<LotInfo>
			<Field>
				<Param name = "LOT_ID">LOTID</Param>
				<Param name = "PART_TYP">PARTTYPE</Param>
				<Param name = "OPER_NAM">OPERATORID</Param>
				<Param name = "DATE_COD">DATECODE</Param>
				<Param name = "TEST_COD">TESTCODE</Param>
				<Param name = "TST_TEMP">TEMPERATURE</Param>
				<Param name = "JOB_NAM">PROGRAM</Param>
				<Param name = "RTST_COD">RETESTCODE</Param>
				<Param name = "NODE_NAM">HOSTNAME</Param>
				<Param name = "HAND_TYP">HANDLERNAME</Param>
				<Param name = "HAND_ID">HANDLERID</Param>
				<Param name = "LOAD_ID">LOADBOARD</Param>
				<Param name = "DIB_ID">DUTBOARD</Param>
				<Param name = "CONT_ID">SOCKET</Param>
				<Param name = "JOBFILE">PROGRAMPATH</Param>
				<Param name = "CUSTOMER">CUSTOMER</Param>
				<Param name = "DUMMY">DUMBDUMB</Param>
			</Field>
		</LotInfo>
	- note above that some fields "DUMMY", "CUSTOMER", and "JOBFILE" are not STDF fields. instead they are special fields with specific use. 
		- JOBFILE refers to the program path/name APL will load in Unison
		- CUSTOMER is the customer name used for customer specific behavior such as which NewLotConfig to use, etc...
		- DUMMY is just a useless example so ignore this.
	- you are not required to set field labels for every STDF fields. APL assigns default to each of the writeable STDF field. Note tha the default
	  values are based on lotinfo.txt file used for ST in DMDx. this is to ensure that no changes are required on config.xml on testers where ST
	  devices are tested
	- the following are default field label for MIR. format is <Param name = "SETUP_T">SETUP_T</Param>
		SETUP_T: SETUP_T
		START_T: START_T
		STAT_NUM: STAT_NUM
		MODE_COD: TESTMODE
		RTST_COD: LOTSTATE
		PROT_COD: PROTECTIONCODE
		BURN_TIM: BURN_TIM
		CMOD_COD: COMMANDMODE
		LOT_ID: LOTID
		PART_TYP: DEVICE
		NODE_NAM: NODE_NAM
		TSTR_TYP: TESTERTYPE
		JOB_NAM: JOB_NAM
		JOB_REV: FILENAMEREV
		SBLOT_ID: SUBLOTID
		OPER_NAM: OPERATOR
		EXEC_TYP: SYSTEMNAME
		EXEC_VER: TARGETNAME
		TEST_COD: TESTPHASE
		TST_TEMP: TEMPERATURE
		USER_TXT: USERTEXT
		AUX_FILE: AUXDATAFILE
		PKG_TYP: PACKAGE
		FAMLY_ID: PRODUCTID
		DATE_COD: DATECODE
		FACIL_ID: TESTFACILITY
		FLOOR_ID: TESTFLOOR
		PROC_ID: FABRICATIONID
		OPER_FRQ: OPERFREQ
		SPEC_NAM: TESTSPECNAME
		SPEC_VER: TESTSPECREV
		FLOW_ID: ACTIVEFLOWNAME
		SETUP_ID: TESTSETUP
		DSGN_REV: DESIGNREV
		ENG_ID: ENGRLOTID
		ROM_COD: ROMCODE
		SERL_NUM: TESTERSERNUM
		SUPR_NAM: SUPERVISOR	
	- the following are default field label for SDR. format is <Param name = "HAND_TYP">PROBERHANDLERTYPE</Param>
		HAND_TYP: PROBERHANDLERTYPE
		CARD_ID: CARDID
		CARD_TYP: CARDTYPE
		LOAD_ID: BOARDID
		HAND_ID: PROBERHANDLERID
		DIB_TYP: DIBTYPE
		CABL_ID: CABLEID	
		CONT_TYP: CONTACTORTYPE	
		LOAD_TYP: BOARDTYPE	
		CONT_ID: CONTACTORID	
		LASR_TYP: LASERTYPE	
		LASR_ID: LASERID	
		EXTR_TYP: EXTRAEQUIPMENTTYPE	
		EXTR_ID: EXTRAEQUIPMENTID	
		DIB_ID: DIBID		
		CABL_TYP: CABLETYPE
	- the following are default field label for generic fields. format is <Param name = "JOBFILE">JOBFILE</Param>
		JOBFILE: JOBFILE
		CUSTOMER: CUSTOMER
		DEVICENICKNAME: DEVICENICKNAME
		STEP: STEP

version beta.2.5.20191006
-	bug fixes
	- fixed launch command on POPUP server to work on bash shell
	- APL will now attempt to go back to IDLE state if PROGRAM_LOAD_FAIL occurs

version beta.2.4.20191006
-	bug fixes
	- fixed a bug where APL can potentially crash when STEP feature is disabled. Credit to Cedric for finding this out and fixing it.
	- fixed display bug where max launch attempt displays load type instead of the number of launch attempt

- 	new feature (STEP value setting RTST_COD and FLOW_ID)
	- Amkor sends a lotinfo.txt file containing one of the fields called “STEP”. APL is required to and set FLOW_ID and RTST_COD values that corresponds to STEP value.
	- Amkor provided a list of STEP values and corresponding RTST_COD and FLOW_ID values. However, it’s safe to assume that the list given to us is not complete. 
	  Therefore to future proof the implementation, setting table for STEP, FLOW_ID, and RTST_COD is configurable. 
	- APL can be configured to set a table of STEP value and corresponding values for RTST_COD and FLOW_ID. it can be set in config.xml as shown below
		<LotInfo>
			<Step state = "true">
				<Param flow_id = "FT1" rtst_cod = "0">FT1</Param>
				<Param flow_id = "FT1" rtst_cod = "1">Rt1</Param>
				<Param flow_id = "FT2" rtst_cod = "0">FT2</Param>
				<Param flow_id = "FT2" rtst_cod = "2">2RT2</Param>
				<Param flow_id = "FT1" rtst_cod = "3">RT3</Param>
			</Step>
		</LotInfo>
	- APL cannot set RTST_COD and FLOW_ID by itself. STDF fields are governed by uprodtool to ensure STDF contents will always be compliant to ST specification. 
	  Any STDF field value APL wants to set can only be done by passing the lotinfo.txt file to uprodtool (via FAModule). This function already exist in APL. 
	  But APL must insert RTST_COD and FLOW_ID values in lotinfo.txt file (if there’s non yet) in order for uprodtool to consider setting them in STDF file. 
	  APL achieves this by doing the following:
		- APL parses lotinfo.txt file, gets the STEP value (if any) and take the corresponding rtst_cod and flow_id values from STEP table (config.xml)
		- APL checks lotinfo.txt file if it already contains ACTIVEFLOWNAME and RTSTCODE fields. The values these fields contain refer to flow_id and rtst_cod respectively. 
		  If the values of this fields matches corresponding values for STEP, APL will do nothing. Otherwise, it will attempt to edit lotinfo.txt file and 
		  set the correct flow_id and rtst_cod 
		- It will then proceed on passing the updated lotinfo.txt file to uprodtool
	- algorithm and test cases
		- APL ensures that possible RTS_COD values for STEP can only be between 0-255
		- STEP value is NOT CASE sensitive. APL will always store STEP values from config file as all upper case letters. 
			- if STEP value in lotinfo.txt file has lower case letters, APL will compare them to STEP table as all upper case letters
		- RTST_COD value is CASE sensitive. APL will always store FLOWID values from config file as all upper case letters.
			- if RTSTCODE value in lotinfo.txt file is valid but uses lower case letters, APL will flag it as mismatch and will edit lotinfo.txt with all upper case letters
		- If lotinfo.txt file contains invalid STEP (does not match any in the STEP table), APL will log an error and will do nothing. 
		  It will still proceed to load program (if required), and eventually move to idle state
		- If lotinfo.txt file does not contain STEP, APL will log error and do nothing. It will still proceed to load program (if required) , and eventually move to idle state
		- If APL fails to update lotinfo.txt file with correct flow_id and rtst_cod, it will log an error. It will still proceed to load program (if required), 
		  and eventually move to idle state

-	new feature (popup Messagebox on loading lotinfo.txt when program is already loaded)
	- Amkor requested that if lotinfo.txt file is received by APL but program is already loaded, APL must get confirmation first from operator to proceed processing the lotinfo.txt
	  through a pop-up Message Box.
	- When APL receives lotinfo.txt file and program is already loaded, APL will pop a Message Box asking operator to proceed. The message box will pop-up twice to ensure
	  operator is really sure to proceed (prevents accidental acceptance). This is important because current lot will be reset and if APL is on "force-load" config, current program
 	  will be unloaded
	- This feature is enabled by default and can be disabled through config file by setting <GUI> state to true
		<GUI state = "true">
			<IP>127.0.0.1</IP>
			<Port>53213</Port> 
		</GUI>
	- note that enabling/disabling and changing GUI configuration cannot take effect in real-time. changes will apply only after relaunching APL for security reasons

-	update feature
	- APL will not attempt to set MIR.BURN_TIM anymore
	- STEP feature now ensures it only appends to .sum file, ignores others e.g. .sum_open
	- STDF will not force empty value on fields not set by lotinfo.txt anymore
	- new config parameter added to enable/disable APL from deleting lotinfo.txt file after parsing it
		<LotInfo>
			<Delete>true</Delete>
		</LotInfo>		

-	quality of life improvement
	- APL runs as singleton now. When you launch it, it will kill off any other instance of APL that is currently running, ensuring there's only one APL running at a time.
	- Force Load config option setting now displayed when printing out config 
	- added utility functions to safely open files for read/write as well as renaming and removing them
	- error log with setLotInformation() is more information friendly now


version beta.2.3.20190920
-	bug fixes
	- fixed a bug where network connection always use TCP even if you set to UDP. now UDP connection works
	- fixed a bug in fetchVal() function in xml parser 
	- added '=' operator for LOTINFO class


version beta.2.3.20190918
-	update feature
	- as per amkor's request, if program is already loaded and the same as the jobfile in lotinfo.txt, APL should not relaunch unison and load program again.
	  instead, it will just update STDF fields as well as lotinfo fields.
		- this is how it should have been but due to the issue with OICU where APL cannot query if it's connected to tester or not (in fact no one can), APL always
	  	  relaunch unison and load program every time it receives lotinfo.txt file. this ensures that OICU is connected to tester when program is loaded.
	  	- but since amkor insists, we update this BUT we will not take responsibility if there is complain from amkor on situation where APL successfully loaded
		  program but operator does not see it in monitor.
		- alternatively, force launching/loading can still be enabled by setting the appropriate tag in config.xml as shown below. this flag is false by default

			<Launch>
				<Param name = "Force Load">true</Param> <!-- true if APL will always launch OICU and load program, false by default -->
			</Launch>	

	- the suffix for NewLotUnison_***.xml is now taken from lotinfo.txt field 'FABRICATIONID'
-	quality of life improvement
	- previously, if you make changes to config file, you have to relaunch APL for the changes to take effect. now, APL reads the config file every minute therefore
	  you can make changes to the config file without having to launch APL again. you only need to wait a minute for changes to take effect.


Version beta.2.2.20190911
-	update feature 
	- bin solution feature has been updated to match latest amkor's bin solution specification v1.4
		- the "FT/RT" text is now inserted in the bin solution for wafer sort as per specification
		- APL now detects if any die in a test cycle is a retest by comparing its coordinate from the previous dies tested in the lot.
-	new feature
	- a new task is added that takes all STDF (MIR and SDR for now) fields from lotinfo.txt file and sends it to unison to be set in STDF file
		- can be enabled in config.xml by setting tag <STDF state = "true" />; disabled by default
		- there are 36 writable fields in MIR but only 31 can be filled. the following fields cannot be set
			- MIR.STAT_NUM - read only, unison fills it with test head number
			- MIR.JOB_NAM - read only, unison fills it with test program object name
			- MIR.RTST_COD - read only, unison fills it with specific single character values - N (new test), R (retest), U (unknown test)
			- MIR.NODE_NAM - can be set, but when lot starts, unison overwrites it with tester name
			- MIR.BURN_TIM - there's not even an enum to use setLotInformation() to set this; using setExpression is also unsuccessful. unison sets it to '0' by default
			- note that MIR.MODE_COD, PROT_COD, CMOD_COD can be set, but unison will only take the first character in the string you set it to as its values.
			  this is expected as these fields only takes single character as per STDFV4 specification
		- there are 16 writable fields in SDR but only 13 can be filled
			- SDR.HAND_TYP - can be set, but unison will overwrite it with CURI driver name
			- SDR.HAND_ID - can be set, but unison will overwrite it with CURI object name
			- SDR.CABL_TYP - can be set, but unison will overwrite it with empty string; strange behavior, need to raise SPR
-	bug fixes
	- summary appending feature now disabled by default. enable/disable flag now working
-	quality of life improvement
	- logger filename now also contain the version of APL 

Version beta.2.1.20190902
-	new feature: APL can now monitor a specified directory for incoming sublot summary file and appends it with "Step" value taken from lotinfo.txt file
	-	to enable, set the <Summary> tag in config.xml to <Summary state = "true">. otherwise this feature is disabled by default
	-	to specify the directory path to monitor for incoming sublot summary file, specify a <Path> tag within <Summary> tag as shown below:

			<Summary state = "true">
				<Path>/home/localuser/shared</Path>
			</Summary>
-	bug fixes:
	- fixed a bug where APL goes haywire when receiving invalid lotinfo.txt file
		-	appending GDR to lotinfo.txt is done even when lotinfo.txt file is invalid. since it's invalid, APl stays in idle state
			and therefore notify object catches lotinfo.txt modify event. this causes endless loop. 
		-	GDR is now only appended after ensuring lotinfo.txt is valid and APL moved to next state
	- fixed a bug that crashes APL when it tries to remove leading/trailing spaces in field/value pair if any of them contains nothing or just spaces
	
-	improvements:
	-	added function in notify fd class called halt(). this function allows you to stop succeeding events from fd select() 
		if it sends more than one event preventing multiple triggers
	-	the halt() function is also now used by lotinfo notify object to prevent multiple triggers. 
	-	state class has new function halt(). when called, all other tasks it's supposed to execute will not be executed anymore


Version beta.2.0.20190726
-	APL now appends incoming lotinfo.txt file with the following parameters to be set in STDF file as GDR's
	-	AutomationNam
	-	AutomationVer
	-	LoaderApiNam 
	-	LoaderApiRev
-	sample config.xml accompanied in the software package has been updated to remove <STDF> params


DEBUG NOTES:

-	compile test program command line
	>MethodCompiler -force -f <una file>

