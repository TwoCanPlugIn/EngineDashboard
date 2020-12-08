Engine Dashboard plug-in for OpenCPN
====================================

The Engine Dashboard Plugin displays engine parameters, fluid levels and battery status in OpenCPN. It accepts NMEA 0183 RPM (Revolutions), RSA (Rudder Sensor Angle) and XDR (Transducer Measurement) sentences as its input. For sailors with NMEA2000® engine and tank sensors, any release of the TwoCan plugin beyond verion 1.6 can convert the appropriate messages from NMEA2000® networks to their NMEA 0183 equivalents which can then be displayed by the Engine Dashboard.

The Engine Dashboard displays the following data:

Engine RPM, Oil Pressure, Coolant Temperature, Engine Hours & Alternator Voltage for either single or dual engine vessels.

Fluid levels for Fuel, Water, Oil, Live Well, Grey and Black Waste.

Battery status (voltage and current) for the Start and House batteries.

and Rudder Angle.

The engine dashboard consumes a subset of NMEA 0183 sentences.
For engine speed it may use either RPM or XDR sentences.
For Rudder Angle it uses RSA sentences.

For XDR entences, the Engine Dashboard supports several transducer naming formats;
those generated by the TwoCan plugin, NMEA 183 v4.11 (as used by Actisense gateways), ShipModul & Maretron.

TwoCan Transducer Naming:
| Measurement	Transducer |	Type	|	Measurement Unit |	Transducer Name<sup>1</sup>|
|-----------------------|------|------------------|-----------------|
|Engine RPM | T | R (RPM)| MAIN, PORT or STBD|
|Oil pressure	| P |	P (Pascals)	| MAIN, PORT or STBD|
|Water Temperature	| C |		C (Celsius)		| MAIN, PORT or STBD|
|Alternator Voltage |	U |	V (Volts)	| MAIN, PORT or STBD|
|Engine Hours |	G  | H (Hours)<sup>2</sup>	| MAIN, PORT or STBD|
|Fluid Levels	| V  |	P (Percent)	 |	FUEL, H2O, OIL, LIVE, GREY, BLK|
|Battery Voltage | U  | V (Volts) | STRT (Start or Main), HOUS (House or Auxilliary)|
|Battery Current | U  | A (Amps)<sup>3</sup> | STRT (Start or Main), HOUS (House or Auxilliary)|

1. These transducer names are generated by the TwoCan plugin.
2. The use of 'H' to indicate hours is a customised use of the generic transducer type.
3. Note this extends the usage of the "U" (Voltage Transducer) to include current measured in Amps.

NMEA 183 v4.11 Transducer Naming<sup>1</sup>:
| Measurement	Transducer |	Type	|	Measurement Unit |	Transducer Name|
|-----------------------|------|------------------|-----------------|
|Engine RPM | T | R (RPM)| ENGINE#0, ENGINE#1|
|Oil pressure	| P |	P (Pascals)	| ENGINE#0, ENGINE#1|
|Water Temperature	| C |		C (Celsius)		| ENGINE#0, ENGINE#1|
|Alternator Voltage |	U |	V (Volts)	| ALTERNATOR#0, ALTERNATOR#1|
|Engine Hours |	G  | null | ENGINE#0, ENGINE#1|
|Fluid Levels	| V  |	P (Percent) | FUEL#0, FRESHWATER#0, OIL#0, LIVEWELLWATER#0, WASTEWATER#0, BLACKWATER#0|
|Fluid Levels	| E  |	P (Percent) | FUEL#0, FRESHWATER#0, OIL#0, LIVEWELLWATER#0, WASTEWATER#0, BLACKWATER#0|
|Battery Voltage | U  | V (Volts) | BATTERY#0, BATTERY#1<sup>2</sup> |
|Battery Current | I  | A (Amps) | BATTERY#0, BATTERY#1 |
1. The plugin preference "Dual Engine Vessel" determines whether ENGINE#0 refers to the main or port engine.
2. BATTERY#0 refers to the start battery bank, BATTERY#1 refers to the house battery bank.

ShipModul or Maretron Transducer Naming<sup>1</sup>:
| Measurement	Transducer |	Type	|	Measurement Unit |	Transducer Name|
|-----------------------|------|------------------|-----------------|
|Engine RPM | T | R (RPM)| ENGINE0, ENGINE1|
|Oil pressure	| P |	P (Pascals)	| ENGOILP0, ENGOILP1|
|Water Temperature	| C |		C (Celsius)		| ENGTEMP0, ENGTEMP1|
|Alternator Voltage |	U |	V (Volts)	| ALTVOLT0, ALTVOLT1|
|Engine Hours |	G  | null | ENGHRS0, ENGHRS1|
|Fluid Levels	| V  |	P (Percent) | FUEL0, FRESHWATER0, OIL0, LIVEWELL0, WASTEWATER0, BLACKWATER0|
|Fluid Levels	| E  |	P (Percent) | FUEL0, FRESHWATER0, OIL0, LIVEWELL0, WASTEWATER0, BLACKWATER0|
|Battery Voltage | U  | V (Volts) | BATVOLT0, BATVOLT1 <sup>2</sup> |
|Battery Current | I  | A (Amps) | BATCURR0, BATCURR1 |
1. The plugin preference "Dual Engine Vessel" determines whether ENGINE0, ENGOILP0, ENGTEMP0, ENGHRS0 ALTVOLT0 refers to the main or port engine.
2. BATVOLT0, BATCURR0 refers to the start battery bank, BATVOLT1, BATCURR1 refers to the house battery bank.


Examples of NMEA 0183 XDR sentences that may be used by the engine plugin are:
TwoCan transducer naming:
$IIXDR,P,158300.00,P,MAIN,C,23.11,C,MAIN,U,13.86,V,MAIN&ast;6A
$IIXDR,T,804.50,R,MAIN&ast;54
$IIXDR,G,1.16,H,MAIN&ast;52

NMEA 183 v4.11 transducer naming
$IIXDR,P,140000.00,P,ENGINE#1,C,90.0,C,ENGINE#1,U,12.00,V,ALTERNATOR#1&ast;46
$IIXDR,T,890.50,R,ENGINE#1&ast;4E
$IIXDR,U,12.50,V,BATTERY#0,I,4.5,A,BATTERY#0&ast;42
$IIXDR,G,1.16,,ENGINE#0,G,200.5,,ENGINE#1&ast;7E
$IIXDR,E,40.00,P,FUEL#0,E,80.00,P,FRESHWATER#0&ast;47


ShipModul transducer naming:
$ERXDR,P,140000.00,P,ENGOILP1,C,90.0,C,ENGTEMP1,U,12.00,V,ALTVOLT1&ast;34
$ERRPM,E,1,2400.00,,A&ast;69
$ERXDR,U,12.50,V,BATVOLT0,I,4.5,A,BATCURR0&ast;42
$ERXDR,G,1.16,,ENGHRS0,G,200.5,,ENGHRS1&ast;69
$ERXDR,E,40.00,P,FUEL0,E,80.00,P,FRESHWATER0&ast;*50



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

Windows, Linux and Mac OSX are currently supported.

This plugin builds outside of the OpenCPN source tree

For all platforms, refer to the OpenCPN developer manual for details regarding other requirements such as git, cmake and wxWidgets.

For Windows you must place opencpn.lib into the engine\_dashboard\_pi/build directory to be able to link the plugin DLL. opencpn.lib can be obtained from your local OpenCPN build, or alternatively downloaded from http://sourceforge.net/projects/opencpnplugins/files/opencpn_lib/

Build Commands
--------------
 mkdir enginedashboard/build

 cd enginedashboard/build

 cmake ..

 cmake --build . --config debug

  or

 cmake --build . --config release

Creating an installation package
--------------------------------
 cmake --build . --config release --target package

  or

 cpack

 or for Mac OSX

  make create-pkg

Installation
------------

Windows: Run the resulting packagename.exe installation package
Linux: Install the resulting installation package with the appropriate Package Manager.
For example with Ubuntu: sudo dpkg -i packagename.arch.deb (where arch is the cpu architecture such as amd64)
Mac OSX: Install the resulting installation package.

Since version 1.2 of the Engine Dashboard, it supports the Plugin Manager feature that was included in OpenCPN v5.2. The Engine Dashboard may now be directly installed from the Plugin Manager. 

Problems
--------

Please send bug reports/questions/comments to the opencpn forum or via email to twocanplugin@hotmail.com

License
-------
The plugin code is licensed under the terms of the GPL v3 or, at your convenience, a later version.

NMEA2000® is a registered trademark of the National Marine Electronics Association.
