RELEASE NOTES:

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
