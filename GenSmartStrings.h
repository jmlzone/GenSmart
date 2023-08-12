String dayFreq[]   = {"*", "*", "1-7, 15-21", "1-7", "1-7", "1-7"};
String monthFreq[] = {"*", "*", "*", "*", "1,4,7,10", "1"};
String days[] = {"Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"};
String freq[] ={"null","Weekly","Semi Monthly","Monthly","Quarterly","Anually"};

const char genRootHTML[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html><head>
<title>Generator Smart module (c) jmlzone 2023</title>
%STYLE_JS%
</head>
<body>
<h1>Generator Smart module (c) jmlzone 2023</h1>
%GEN_FOOTER%
%FOOTER_LINKS%
</body></html>
)rawliteral";

const char relayHTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html> <html>
<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">
%STYLE_JS%
<title>GenSmart Relay test</title>
<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}
body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;} h3 {color: #444444;margin-bottom: 50px;}
.button {display: block;width: 80px;background-color: #1abc9c;border: none;color: white;padding: 13px 30px;text-decoration: none;font-size: 25px;margin: 0px auto 35px;cursor: pointer;border-radius: 4px;}
.button-on {background-color: #1abc9c;}
.button-on:active {background-color: #16a085;}
.button-off {background-color: #34495e;}
.button-off:active {background-color: #2c3e50;}
p {font-size: 14px;color: #888;margin-bottom: 10px;}
</style>
<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}
body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;} h3 {color: #444444;margin-bottom: 50px;}
.button {display: block;width: 80px;background-color: #1abc9c;border: none;color: white;padding: 13px 30px;text-decoration: none;font-size: 25px;margin: 0px auto 35px;cursor: pointer;border-radius: 4px;}
.button-on {background-color: #1abc9c;}
.button-on:active {background-color: #16a085;}
.button-off {background-color: #34495e;}
.button-off:active {background-color: #2c3e50;}
p {font-size: 14px;color: #888;margin-bottom: 10px;}
</style>
<title>GenSmart Relay Test</title>
<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}
body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;} h3 {color: #444444;margin-bottom: 50px;}
.button {display: block;width: 80px;background-color: #1abc9c;border: none;color: white;padding: 13px 30px;text-decoration: none;font-size: 25px;margin: 0px auto 35px;cursor: pointer;border-radius: 4px;}
.button-on {background-color: #1abc9c;}
.button-on:active {background-color: #16a085;}
.button-off {background-color: #34495e;}
.button-off:active {background-color: #2c3e50;}
p {font-size: 14px;color: #888;margin-bottom: 10px;}
</style>
</head><body>
<h1>GenSmart Relay Test</h1>
<h3>%TIME% %TIMESTATUS%</h3>
<h3>Network State: %NETSTATE%</h3>
%DEFEAT%
%SET%
%AUX%
%GEN_FOOTER%
%FOOTER_LINKS%
</body></html>
)rawliteral";

const char cronHTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html> <html>
<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">
%STYLE_JS%
<title>cron status and testing</title>
<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: left;}
body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;} h3 {color: #444444;margin-bottom: 50px;}
.button {display: block;width: 160px;background-color: #1abc9c;border: none;color: white;padding: 10px 20px;text-decoration: none;font-size: 25px;margin: 0px auto 35px;cursor: pointer;border-radius: 4px;}
.button-on {background-color: #1abc9c;}
.button-on:active {background-color: #16a085;}
.button-off {background-color: #34495e;}
.button-off:active {background-color: #2c3e50;}
p {font-size: 14px;color: #888;margin-bottom: 10px;}
</style>
</head><body>
<h1>GenSmart Cron</h1>
<form action="/postCronForm" method="post">
%CRONTIMEFORM%
 <input type=submit name="save" value="Save">
 <input type=submit name="test" value="Test">
 </form>
%TIME% %TIMESTATUS%<br>
Network State: %NETSTATE%<br>
Cron entry count: %CRONCOUNT%<br>
Next Set time: %SETTIME%<br>
Next Defeat time: %DEFTIME%<br>
tmp cron entry: %TMPTIME%<br>
Next Trigger: %CRONNEXTTRIGGER%<br>
<h2>All Cron Status</h2>
%ALLCRON%
<center>
<p>Set test</p><a class="button button-on" href="/cron?setSet=test">Test Set</a>
<p>Defeat test</p><a class="button button-on" href="/cron?setDef=test">Test Defeat</a>
<p>Unset Auto</p><a class="button button-on" href="/cron?unsetAuto=test">Unset Auto</a>
<p>Simulate Power Fail</p><a class="button button-on" href="/cron?spf=test">Simulate Power Fail</a>
</center>
%GEN_FOOTER%
%FOOTER_LINKS%
</body></html>
)rawliteral";

const char powerFailSubject[] PROGMEM = R"rawliteral(Power Fail Alert)rawliteral";
const char powerFailBody[] PROGMEM = R"rawliteral(Power Lost at %TIME% Line voltage: %VACRMS%. Battery Voltage: %VBAT%.)rawliteral";
const char powerRestoreSubject[] PROGMEM = R"rawliteral(Power Restore notification)rawliteral";
const char powerRestoreBody[] PROGMEM = R"rawliteral(Power restored at %TIME% Line voltage: %VACRMS%. Battery Voltage: %VBAT%.)rawliteral";
