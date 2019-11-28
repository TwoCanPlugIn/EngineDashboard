Engine Dashboard plug-in for OpenCPN
====================================

The Engine Dashboard Plugin displays engine parameters, fluid levels and battery status in OpenCPN. It accepts NMEA 0183 RPM (Revolutions), RSA (Rudder Sensor Angle) and XDR (Transducer Measurement) sentences as its input. For sailors with NMEA2000® engine and tank sensors, the latest release of the TwoCan plugin, verion 1.6 can convert the appropriate messages from NMEA2000® networks to their NMEA 0183 equivalents which can then be displayed by the Engine Dashboard.

The Engine Dashboard displays the following data:

Engine RPM, Oil Pressure, Coolant Temperature, Engine Hours & Alternator Voltage for either single or dual engine vessels.

Fluid levels for Fuel, Water, Oil, Live Well, Grey and Black Waste.

Battery status (voltage and current) for the Start and House batteries.

and Rudder Angle.

The engine dashboard consumes a subset of NMEA 0183 sentences.
For engine speed it may use either RPM or XDR sentences.
For Rudder Angle it uses RSA sentences.

For XDR entences, the following are used:

| Measurement	Transducer |	Type	|	Measurement Unit |	Transducer Name<sup>1</sup>|
|-----------------------|------|------------------|-----------------|
|Engine RPM | T | R (RPM)| MAIN, PORT or STBD|
|Oil pressure	| P |	P (Pascals)	| MAIN, PORT or STBD|
|Water Temperature	| C |		C (Celsius)		| MAIN, PORT or STBD|
|Alternator Voltage |	U |	V (Volts)	| MAIN, PORT or STBD|
|Engine Hours |	G  | H (Hours)<sup>2</sup>	| MAIN, PORT or STBD|
|Fluid Levels	| V  |	P (Percent)<sup>3</sup>	 |	FUEL, H2O, OIL, LIVE, GREY, BLK|
|Battery Voltage | U  | V (Volts) | STRT (Start or Main), HOUS (House or Auxilliary)|
|Battery Current | U  | A (Amps)<sup>4</sup> | STRT (Start or Main), HOUS (House or Auxilliary)|

1. These names are hardcoded in the Engine Dashboard and mate with the output produced by the TwoCan plugin. If user's receive data from other sources with different transducer names then the NMEA Converter plugin could be used to modify these fields.
2. The use of 'H' to indicate hours is a customised use of the generic transducer type.
3. Note this deviates from the the standard volume measurement unit which is 'M' cubic metres.
4. Note this extends the usage of the "U" (Voltage Transducer) to include current measured in Amps.

Examples of NMEA 0183 XDR sentences that may be used by the engine plugin are:

$IIXDR,P,158300.00,P,MAIN,C,23.11,C,MAIN,U,13.86,V,MAIN&ast;6A

$IIXDR,T,804.50,R,MAIN&ast;54

$IIXDR,G,1.16,H,MAIN&ast;52


There are a few features yet to be implemented in this version of the Engine Dashboard:
1. It supports only a single rudder display.
2. While the preferences dialog allows selection of Pressure units (Pascal or PSI) and Temperature (Celsius or Fahrenheit), the display is only changed the next time OpenCPN is started,

The rationale for developing yet another dashboard was the following:
1. The existing dashboard plugin has limitation of how many inputs/controls it can support. Therefore to add these engine displays would have meant deleting some of the other dashboard controls such as position, depth, speed. 
2. On the other hand the tactics_dashboard eliminates the above limitation, however adding engine displays to the tactics plugin may be of little interest and perhaps confusing to motor boat sailors. 
3. I also felt that it would be easier and simpler for me to release a simple dashboard purely to display engine & tank data and which would possibly be less confusing for end users. 

Unfortunately development of both this new version of the TwoCan plugin and Engine Dashboard was undertaken independently and without the knowledge of the work being done on the new version of the tactics dashboard plugin which also adds some support for engine controls. I have yet to look at the new version of the tactics dashboard to see if it could be used instead of the engine dashboard plugin. I apologise for any confusion this may cause to end users.

Known bugs
----------
On Linux (such as Ubuntu or Raspberry Pi) with OpenCPN v5.0, if the dashboard is in a horizontal orientation, OpenCPN crashes when adding or deleting an instrument to the dashboard. The workaround is to add or delete instruments to the dashboard when it is in a vertical orientation, or to add or delete instruments when the dashboard is not visible. (Note the same bug exists in the built-in dashboard and probably also in the tactics-dashboard)

Obtaining the source code
-------------------------

git clone https://github.com/TwoCanPlugIn/EngineDashboard.git


Build Environment
-----------------

Both Windows and Linux are currently supported.

This plugin builds outside of the OpenCPN source tree

For both Windows and Linux, refer to the OpenCPN developer manual for details regarding other requirements such as git, cmake and wxWidgets.

For Windows you must place opencpn.lib into the twocan_pi/build directory to be able to link the plugin DLL. opencpn.lib can be obtained from your local OpenCPN build, or alternatively downloaded from http://sourceforge.net/projects/opencpnplugins/files/opencpn_lib/

Build Commands
--------------
 mkdir engine_dashboard_pi/build

 cd engine_dashboard_pi/build

 cmake ..

 cmake --build . --config debug

  or

 cmake --build . --config release

Creating an installation package
--------------------------------
 cmake --build . --config release --target package

  or

 cpack

Installation
------------

Windows: Run the resulting packagename.exe installation package
Linux: Install the resulting installation package with the appropriate Package Manager.
For example with Ubuntu: sudo dpkg -i packagename.arch.deb (where arch is the cpu architecture such as amd64)

Problems
--------

Please send bug reports/questions/comments to the opencpn forum or via email to twocanplugin@hotmail.com


License
-------
The plugin code is licensed under the terms of the GPL v3 or, at your convenience, a later version.

NMEA2000® is a registered trademark of the National Marine Electronics Association.
