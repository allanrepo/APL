RELEASE NOTES:

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
