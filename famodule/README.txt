1. compile it using any of the below command:
>gmake
>gmake CFG=Release
>gmake CFG=Debug

if CFG=Release, the compiled library will be at ./Release subdirectory of the source code directory
if CFG=Debug, the compiled library will be at ./Debug subdirectory of the source code directory
	
2. modify CURI config /opt/ateTools/curi/unison/config/curi_conf.xml
- go to <Config Configuration_ID="DefaultConfiguration"> tag in curi_conf.xml and it should look like the ones below:

<CURI_CONF>
    <Config Configuration_ID="DefaultConfiguration">
        <ConfigEnv Equipment_Path="/opt/ateTools/curi/unison/lib" Communications_Path="/opt/ateTools/curi/unison/lib" User_Path="/home/localuser/Desktop/famodule/Release"/>
        <CommunicationsList>
            <GPIB_IF>
                <Settings>
                    <stringSetting token="IbcAUTOPOLL" value="0"/>
                </Settings>
            </GPIB_IF>
            <RS232_IF/>
            <TTL_IF/>
            <TTL_IF DriverID="ASL PCI PORT" AccessName="ASL_XP_TTL" Library="curi_asl_ttl" />
            <TTL_IF DriverID="ASL AT PORT" AccessName="ASL_NT_TTL" Library="curi_asl_ttl" />
            <TCPIP_IF Port="65000"/>
            <USB_IF/>
        </CommunicationsList>
    </Config>

    <UserLibrary AccessName="Demo famodule" Library="fademo_hooks">
        <Settings>
	    <stringSetting token="Module Type" value="FAPROC Module"/>
	    <stringSetting token="Priority" value="50"/>
        </Settings>
    </UserLibrary>

- <configEnv> tag should have attribute User_Path= <famodule library path>, in above example, the so file is located at "/home/localuser/Desktop/famodule/Release"
- <UserLibrary> tag must have attribute Library= <library file name with no extension>. in above example, the library file it will look for at <User_Path> directory is "fademo_hooks.so"
- the 2 <stringSetting> tags within <UserLibrary><Settings> are fixed and should not be changed. therefore both must always be there and their attributes must be set as the above example

