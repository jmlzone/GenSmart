
/*----------------------------------------------------------------------------- 
  GenSmart Make a generac generator smarter by intersecting some of the 
  control signals

  Time for excercise is set by cron

  TBD:
  1) allow cron settings to be done through GUI and saved to file
  2) Send a text when power is lost and returns.
-----------------------------------------------------------------------------*/

#include <OneWire.h>
#include <DallasTemperature.h>
#include <iotfw.h>
#include "GenSmart.h"
#include "GenSmartStrings.h"
#define DEBUG 1
#define INCLUDE_PLOT 1
// GPIO where the DS18B20 is connected to
// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(ONEWIREPIN);

// Pass our oneWire reference to Dallas Temperature sensor 
DallasTemperature sensors(&oneWire);

iotfw iotfw;
extern httpsserver::HTTPSServer secureServer;
extern httpsserver::HTTPServer server;
extern httpsserver::ResourceParameters *params;
extern httpsserver::HTTPRequest * __req;
extern httpsserver::HTTPResponse * __res;

extern CronClass Cron;
float tempF;
float vref;
float vbat;
int ndx;
float tempFilt[4];
int vrefFilt[4];
int vrefRaw;
int vbatFilt[4];
int pointsA[20];
int pointsB[20];
int pointsvac[20];
float currAS[20];
float currBS[20];
float vacS[20];
float currArms;
float currBrms;
float vacrms;
int pndx;
calibration_t cal;
cront_t cront;
CronID_t setCronID;
CronID_t defCronID;
CronID_t tmpCronID;

bool defeatRelay=LOW, setRelay=LOW, auxRelay=LOW;
bool powerFail = false;
extern struct tm * timeinfo;
/* function declariations */
String genPlot(void);
String timeText(time_t *time);
void setSet(void);
void setDefeat(void);
void clearDefeat(void);
String genSmartTokHandler(const String& var);
void measureCurrent(void);
void setCronFromCront(void);
void saveCront(void);
void loadCront(void);
void setDefCron(void);
String gen_cronTimeForm(void);
void unsetAuto(void);
void releaseAuto(void);
void simulatePowerFail(void);
void restorePowerFail(void);
/* end function declariations */

void handleSensors(httpsserver::HTTPRequest * req, httpsserver::HTTPResponse * res) {
    params = req->getParams();
  __req=req;
  __res=res;

 String ptr = "<!DOCTYPE html> <html>\n"; 
  ptr +="<html>\n";
  ptr+="<head>\n";
  ptr+="<title>Sensor Data</title>\n";
  ptr+="<script>\n function autoRefresh() {\n window.location = window.location.href;\n }\n setInterval('autoRefresh()', 60000);\n</script>\n";
  ptr+="%STYLE_JS%";
  if(INCLUDE_PLOT) {
    ptr+=genPlot();
  }
  ptr+="</head>\n";
  if(INCLUDE_PLOT) {
    ptr+="<body onload=\"draw();\">\n";
  } else {
    ptr+="<body>\n";
  }
  ptr+="Temp (F): %TEMPF%\n<br>";
  ptr+="vref = %VREF%\n<br>";
  ptr+="vbat = %VBAT%\n<br>";
  ptr+="vac = %VACRMS%\n<br>";
  ptr+="Current A = %CURRA% Amps\n<br>";
  ptr+="Current B = %CURRB% Amps\n<br>";
  ptr+="Imbalance (A-B) = " + String(currArms-currBrms) + " Amps\n<br>";
  if(INCLUDE_PLOT) {
    ptr+="<canvas id=\"plot\" width=400 height=256></canvas><br>";
  }
  ptr+="<H1>Status</H1>\n";
  ptr+=genStatus();
  ptr+="%GEN_FOOTER%%FOOTER_LINKS%</body>\n";
  ptr+="</html>\n";
  sendTok((char *) ptr.c_str());  
}
void handleCal(httpsserver::HTTPRequest * req, httpsserver::HTTPResponse * res) {
    params = req->getParams();
  __req=req;
  __res=res;
  String ptr = "<!DOCTYPE html> <html>\n"; 
  ptr+="<html>\n%STYLE_JS%\n<head>\n";
  ptr+="<title>Calibration Data</title>\n";
  ptr+="<form action=\"/postCalForm\" method=\"post\">\n";
  ptr+="<label for=\"refSlope\">Reference slope</label><input type=\"number\" step=\"any\" id=\"refSlope\" name=\"refSlope\" value=\"";
  ptr+=String(cal.refSlope,8);
  ptr+="\">";
  ptr+="<label for=\"refOffset\">offset</label><input type=\"number\" step=\"any\" id=\"refOffset\" name=\"refOffset\" value=\"";
  ptr+=String(cal.refOffset,8);
  ptr+="\">";
  ptr+="<br>\n";
  ptr+="<label for=\"batSlope\">Battery slope</label><input type=\"number\" step=\"any\" id=\"batSlope\" name=\"batSlope\" value=\"";
  ptr+=String(cal.batSlope,8);
  ptr+="\">";
  ptr+="<label for=\"batOffset\">offset</label><input type=\"number\" step=\"any\" id=\"batOffset\" name=\"batOffset\" value=\"";
  ptr+=String(cal.batOffset,8);
  ptr+="\">";
  ptr+="<br>\n";
  ptr+="<label for=\"lineSlope\">Line slope</label><input type=\"number\" step=\"any\" id=\"lineSlope\" name=\"lineSlope\" value=\"";
  ptr+=String(cal.lineSlope,8);
  ptr+="\">";
  ptr+="<label for=\"lineOffset\">offset</label><input type=\"number\" step=\"any\" id=\"lineOffset\" name=\"lineOffset\" value=\"";
  ptr+=String(cal.lineOffset,8);
  ptr+="\">";
  ptr+="<br>\n";
  ptr+="<label for=\"currSlope\">Current slope</label><input type=\"number\" step=\"any\" id=\"currSlope\" name=\"currSlope\" value=\"";
  ptr+=String(cal.currSlope,8);
  ptr+="\">";
  ptr+="<label for=\"currOffset\">offset</label><input type=\"number\" step=\"any\" id=\"currOffset\" name=\"currOffset\" value=\"";
  ptr+=String(cal.currOffset,8);
  ptr+="\">";
  ptr+="<br>\n";
  ptr+="<input type=submit value=\"Test\" name=\"test\">\n";
  ptr+="<input type=submit value=Save name=\"save\">\n";
  ptr+="<input type=submit value=Cancel name=\"cancel\">\n";
  ptr+="</form>\n";
  ptr+="<p>Click the 'Save' button to store to eeprom.</p>\n";
  ptr+="<br>\n%GEN_FOOTER%%FOOTER_LINKS%";
  ptr+="</body>\n";
  ptr+="</html>\n";
  sendTok((char *) ptr.c_str());  
}
void postCalForm(httpsserver::HTTPRequest * req, httpsserver::HTTPResponse * res) {
  params = req->getParams();
  __req=req;
  __res=res;
  httpsserver::HTTPURLEncodedBodyParser parser(req);
  if (req->getMethod() != "POST") {
    String message = "Non post POST method: ";
    message += String(req->getMethod().c_str()) +"\n";
    iotfw_send(405, "text/plain", message);
  } else {
    bool b_cancel = false;
    bool b_test = false;
    bool b_save = false;
    calibration_t calTmp;
    char pvalue[80];
    char * value;
    size_t bl;
    String ptr = "<!DOCTYPE html> <html>\n";
    while (parser.nextField()) {
      std::string name_o = parser.getFieldName();
      String name = String(name_o.c_str());
      bl=parser.read((byte *)pvalue,80);
      pvalue[bl] = 0;
      value = trim(pvalue); // trim leading and trailing whitespace
      Serial.println("Name :" + String(bl) + ", " + name + " : " + String(value));
      if(name.substring(0,8).equals("refSlope")) {
        calTmp.refSlope = atof(value);
      }
      if(name.substring(0,9).equals("refOffset")) {
        calTmp.refOffset = atof(value);
      }
      if(name.substring(0,8).equals("batSlope")) {
        calTmp.batSlope = atof(value);
      }
      if(name.substring(0,9).equals("batOffset")) {
        calTmp.batOffset = atof(value);
      }
      if(name.substring(0,9).equals("lineSlope")) {
        calTmp.lineSlope = atof(value);
      }
      if(name.substring(0,10).equals("lineOffset")) {
        calTmp.lineOffset = atof(value);
      }
      if(name.substring(0,9).equals("currSlope")) {
        calTmp.currSlope = atof(value);
      }
      if(name.substring(0,10).equals("currOffset")) {
        calTmp.currOffset = atof(value);
      }
      if(name.substring(0,6).equals("cancel")) {b_cancel=true;}
      if(name.substring(0,4).equals("save"))   {b_save=true;}
      if(name.substring(0,4).equals("test"))   {b_test=true;}
    }
    if(b_cancel) {
      ptr+="<meta http-equiv=\"refresh\" content=\"0; url=http:/\" />";
    } else {
      ptr +="<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
      ptr +="<title>Post cal form</title>\n";
      ptr +="</head>\n";
      ptr +="<body>\n";
      ptr +="<h1>Post Cal Form</h1>\n";
      if(b_test || b_save) {
	cal=calTmp;
      }
      if(b_save) {
        File f = LittleFS.open("/calibration.dat", "w");
        if(f && f.write((byte*)&cal,sizeof(cal)) ) {
          f.close();
          ptr+="Saved!<br>\n";
        }
      }
    } // not cancel
    ptr +="</body>\n%FOOTER_LINKS%</html>\n";
    sendTok((char *) ptr.c_str());
  }
}
void handleRelays (httpsserver::HTTPRequest * req, httpsserver::HTTPResponse * res) {
  params = req->getParams();
  __req=req;
  __res=res;
  std::string value;
  bool hasDef = params->getQueryParameter("def",value);
  if(hasDef) {
    if(value=="off") {defeatRelay = LOW;}
    if(value=="on") {defeatRelay = HIGH;}
    digitalWrite(DEFEATRELAYPIN, defeatRelay);
  }
  bool hasSet = params->getQueryParameter("set",value);
  if(hasSet) {
    if(value=="off") {setRelay = LOW;}
    if(value=="on") {setRelay = HIGH;}
    digitalWrite(SETRELAYPIN, setRelay);
  }
  bool hasAux = params->getQueryParameter("aux",value);
  if(hasAux) {
    if(value=="off") {auxRelay = LOW;}
    if(value=="on") {auxRelay = HIGH;}
    digitalWrite(AUXRELAYPIN, auxRelay);
  }
  bool hasSetSet = params->getQueryParameter("setSet",value);
  if(hasSetSet) {setSet();}
  bool hasSetDef = params->getQueryParameter("setDef",value);
  if(hasSetDef) {setDefeat();}

sendTok((char *) relayHTML);  
}
void handleCron (httpsserver::HTTPRequest * req, httpsserver::HTTPResponse * res) {
  params = req->getParams();
  __req=req;
  __res=res;
  std::string value;
  bool hasSetSet = params->getQueryParameter("setSet",value);
  bool hasSetDef = params->getQueryParameter("setDef",value);
  bool hasUnsetAuto = params->getQueryParameter("unsetAuto",value);
  bool hasSimulatePowerFail = params->getQueryParameter("spf",value);
  // send html first so the cron can run without delay
  sendTok((char *) cronHTML);  
  if(hasSetSet) {setSet();}
  if(hasUnsetAuto) {unsetAuto();}
  if(hasSimulatePowerFail) {simulatePowerFail();}
}
void postCronForm(httpsserver::HTTPRequest * req, httpsserver::HTTPResponse * res) {
  params = req->getParams();
  __req=req;
  __res=res;
  httpsserver::HTTPURLEncodedBodyParser parser(req);
  if (req->getMethod() != "POST") {
    String message = "Non post POST method: ";
    message += String(req->getMethod().c_str()) +"\n";
    iotfw_send(405, "text/plain", message);
  } else {
    char pvalue[80];
    char * value;
    char * ep;
    size_t bl;
    bool b_save = false;
    while (parser.nextField()) {
      std::string name_o = parser.getFieldName();
      String name = String(name_o.c_str());
      bl=parser.read((byte *)pvalue,80);
      pvalue[bl] = 0;
      value = trim(pvalue); // trim leading and trailing whitespace
      Serial.println("Name :" + String(bl) + ", " + name + " : " + String(value));
      if(name.substring(0,4).equals("time")) {
	cront.hr = (uint8_t) strtol(value, &ep,10);
        cront.min = (uint8_t) strtol(ep+1, &ep,10);
      }
      if(name.substring(0,3).equals("dow")) {
        cront.dow = (uint8_t) atoi(value);
      }
      if(name.substring(0,9).equals("frequency")) {
        cront.freq = (uint8_t) atoi(value);
      }
      if(name.substring(0,4).equals("save"))   {b_save=true;}
    }
    setCronFromCront();
    if(b_save) {
      saveCront();
      unsetAuto();
    }
    sendTok((char *) cronHTML);
  }
}
void setCronFromCront(void) {
// cron 0 and 1 are the defeat and set
  Cron.free(0);
  Cron.free(1);
  String cronstr = "00 " + String(cront.min) + " " + String(cront.hr)
                         + " " + dayFreq[cront.freq] + " " + monthFreq[cront.freq]
                         + " " + String(cront.dow);
  setCronID=Cron.create((char *) cronstr.c_str(), setSet,false);
  int defmin, defhr;
  if(cront.min<5) { // defeat is 5 minutes before set
    defmin = cront.min + 55;
    defhr = cront.hr -1;
  } else {
    defmin = cront.min - 5;
    defhr = cront.hr;
  }
  cronstr = "00 " + String(defmin) + " " + String(defhr) + " * * " + String(cront.dow);
  defCronID=Cron.create((char *)cronstr.c_str(), setDefeat, false); 
}
void saveCront(void){
  File f = LittleFS.open("/genCron.dat", "w");
  if(f && f.write((byte*)&cront,sizeof(cront)) ) {
    f.close();
  }
}
void loadCront(void) {
  File f = LittleFS.open("/genCron.dat","r");
  if(!f) { // file does not exist set some ok defaults
    setDefCron();
  } else {
    int r=f.read((byte*)&cront, sizeof(cront));
    f.close();
    if(r!=sizeof(cront)) {
      setDefCron();
    }
  }
}
void setDefCron(void){
  cront.min = 0;
  cront.hr = 10;
  cront.dow = 2;
  cront.freq = 3;
}

void handleGenRoot() {
  sendTok((char *) genRootHTML);
}

void setup() {
  // Start the Serial Monitor
  Serial.begin(115200);
  // Start the DS18B20 sensor
  sensors.begin();
  pinMode(DEFEATRELAYPIN, OUTPUT);
  pinMode(SETRELAYPIN, OUTPUT);
  pinMode(AUXRELAYPIN, OUTPUT);
  digitalWrite(DEFEATRELAYPIN, defeatRelay);
  digitalWrite(SETRELAYPIN, setRelay);
  digitalWrite(AUXRELAYPIN, auxRelay);
  iotfw.begin();
  httpsserver::ResourceNode * sensorRS   = new httpsserver::ResourceNode("/sensors", "GET", &handleSensors);
  httpsserver::ResourceNode * caldataRS  = new httpsserver::ResourceNode("/caldata", "GET", &handleCal);
  httpsserver::ResourceNode * relayRS    = new httpsserver::ResourceNode("/relayTest", "GET", &handleRelays);
  httpsserver::ResourceNode * cronRS    = new httpsserver::ResourceNode("/cron", "GET", &handleCron);
  httpsserver::ResourceNode * post_calRS = new httpsserver::ResourceNode("/postCalForm", "POST", &postCalForm);
  httpsserver::ResourceNode * post_cronRS = new httpsserver::ResourceNode("/postCronForm", "POST", &postCronForm);
  secureServer.registerNode(sensorRS);
  secureServer.registerNode(caldataRS);
  secureServer.registerNode(relayRS);
  secureServer.registerNode(cronRS);
  secureServer.registerNode(post_calRS);		   
  secureServer.registerNode(post_cronRS);		   
  iotfw.setRoot(handleGenRoot);
  iotfw.setUsrTokHandler(genSmartTokHandler);
  ndx=0;
  pndx=0;
  File f = LittleFS.open("/calibration.dat","r");
  if(!f) { // file does not exist set some ok defaults
    setDefRef();
  } else {
    int r=f.read((byte*)&cal, sizeof(cal));
    f.close();
    if(r!=sizeof(cal)) {
      setDefRef();
    }
  }
  /*            SS MM  HH DM Mo DW,  Func      ,true = once
  */
  if(timeStatus()==timeSet) { // only setup cron if we have good time
    //setCronID=Cron.create("*  5   10 1-6 1,4,7,10 4", setSet,false); // test at 10:05 am first thurday of every quarter, January, April, July, November"
    //defCronID=Cron.create("*  0   10 *  *  4" , setDefeat, false); //defeat the automated weekly test from 10:00 to 10:10 every thursday
    loadCront();
    setCronFromCront();
  }
}

void setDefRef() {
    cal.refSlope = 0.0002010;
    cal.refOffset = 0.11;
    cal.batSlope = 0.00119; // (1000/(1000+4990))/4 *1.06
    cal.batOffset = 0.86;
    cal.lineSlope = 0.024;//(220.0/16.0) * 0.002371541501976; //(16/220) * (1500/(1500+10000))/4
    cal.lineOffset = 10.0;
    cal.currSlope = 0.104;//(30/(3.1*4095)); //1.0v = 30A
    cal.currOffset = 0;
}
void loop() {
  sensorFilter();
  measureCurrent();
  iotfw.wDelay(1);
}

void sensorFilter() {
  static const unsigned long SAMPLERATE = 1000; // miliseconds
  static unsigned long lastSampleTime = 0;
  float sumA;
  float sumB;
  float sumVac;
  int vrefTmp;
  int p;
  if(millis() - lastSampleTime >= SAMPLERATE) {
    lastSampleTime += SAMPLERATE;
    vbatFilt[ndx] = analogRead(BATPIN);
    vrefFilt[ndx] = analogRead(VREF12PIN);
    sensors.requestTemperatures();
    tempFilt[ndx] = sensors.getTempFByIndex(0);
    ndx = (ndx+1)%4;
    tempF = (tempFilt[0] + tempFilt[1] + tempFilt[2] + tempFilt[3])/4.0;
    vrefTmp  = (vrefFilt[0] + vrefFilt[1] + vrefFilt[2] + vrefFilt[3]);
    vrefRaw = vrefTmp/4;
    vref = float(vrefTmp) * cal.refSlope + cal.refOffset; //(3.3/16380.0) * 1.07
    vbat = float(vbatFilt[0] + vbatFilt[1] + vbatFilt[2] + vbatFilt[3]) * cal.batSlope + cal.batOffset; 
    //    vac  = float(vacFilt[0] + vacFilt[1] + vacFilt[2] + vacFilt[3]) * cal.lineSlope + cal.lineOffset;
    // rms calculation
    sumA=0.0;
    sumB=0.0;
    sumVac=0.0;
    for(p=0; p<20; p++) {
      sumA += (currAS[p]/16);
      sumB += (currBS[p]/16);
      sumVac += vacS[p];
    }
    currArms = sqrt(sumA/20);
    currBrms = sqrt(sumB/20);
    vacrms = sqrt(sumVac/20);
    if(vacrms < 100) { // power fail
      if(defeatRelay == HIGH) {
	defeatRelay = LOW;
	digitalWrite(DEFEATRELAYPIN, defeatRelay);
      }
      if(!powerFail) {
	powerFail = true;
	// Send SMS for power fail
	sendSMS(powerFailSubject, StringTok((char *) powerFailBody));
      }
    } else {
      if(powerFail) {
	powerFail = false;
	// Send SMS for power restore
	sendSMS(powerRestoreSubject, StringTok((char *) powerRestoreBody));
      }
    }
  }
}
void measureCurrent() {
  static const unsigned long SAMPLERATE = 5; // miliseconds
  static unsigned long lastSampleTime = 0;
  int currA;
  int currB;
  int vacRaw;
  float currAcal;
  float currBcal;
  float vacCal;
  if(millis() - lastSampleTime >= SAMPLERATE) {
    lastSampleTime += SAMPLERATE;
    currA = analogRead(CURRAPIN) - vrefRaw;
    currB = analogRead(CURRBPIN) - vrefRaw;
    vacRaw = analogRead(LINEPIN);
    pointsA[pndx] = currA;
    pointsB[pndx] = currB;
    pointsvac[pndx] = vacRaw;
    currAcal = float(currA) * cal.currSlope + cal.currOffset;
    currAS[pndx] = currAcal * currAcal;
    currBcal = float(currB) * cal.currSlope + cal.currOffset;
    currBS[pndx] = currBcal * currBcal;
    vacCal = float(vacRaw) * cal.lineSlope + cal.lineOffset;
    vacS[pndx] = vacCal * vacCal;
    pndx = (pndx+1)%20;
  }
}

String genPlot() {
  int p;
  int px, py;
  String ptr = "<script>\nfunction draw() {\nconst canvas = document.getElementById(\"plot\");\nif (canvas.getContext) {\nconst ctx = canvas.getContext(\"2d\");\n";
  // background
  ptr+= "ctx.fillStyle = \"light grey\"; ctx.fillRect(0, 0, canvas.width, canvas.height);\n";
  // zero line
  ptr+="ctx.strokeStyle = \"green\";\nctx.beginPath();\nctx.moveTo(0,128);\nctx.lineTo(400,128);\nctx.stroke();\n";
  // plot A red
  ptr+="ctx.strokeStyle = \"red\";\nctx.beginPath();\n";
  for(p=0; p<20; p++) {
    px = p*20;
    py = 128 - (pointsA[((p+pndx)%20)]/16);
    if(p==0) {
      ptr+="ctx.move";
    } else {
      ptr+="ctx.line";
    }
    ptr+="To(" + String(px) + "," + String(py) + ");\n";
  }
  ptr+="ctx.stroke();\n";
  // plot B Blue
  ptr+="ctx.strokeStyle = \"blue\";\nctx.beginPath();\n";
  for(p=0; p<20; p++) {
    px = p*20;
    py = 128 - (pointsB[((p+pndx)%20)]/16);
    if(p==0) {
      ptr+="ctx.move";
    } else {
      ptr+="ctx.line";
    }
    ptr+="To(" + String(px) + "," + String(py) + ");\n";
  }
  ptr+="ctx.stroke();\n";
  // plot C orange
  ptr+="ctx.strokeStyle = \"orange\";\nctx.beginPath();\n";
  for(p=0; p<20; p++) {
    px = p*20;
    py = 256 - (pointsvac[((p+pndx)%20)]/16);
    if(p==0) {
      ptr+="ctx.move";
    } else {
      ptr+="ctx.line";
    }
    ptr+="To(" + String(px) + "," + String(py) + ");\n";
  }
  ptr+="ctx.stroke();\n";

  ptr+="}\n}\n</script>\n<style>\ncanvas {\nborder: 1px solid black;\n}\n</style>\n";
  return(ptr);
}
/* setSet() is caled from a timer when it is needing to set the excercise timer 
   (start the auto excercise time)
   it will call clearSet() after several seconds
*/
void setSet() {
  clearDefeat();
  //tmpCronID=Cron.create("*/6 * * * * *", setSet1,true); //several seconds
  tmpCronID=iotfw.runIn(6,iotfw::sec,setSet1); //several seconds
  //if(DEBUG) {Serial.print(__LINE__);Serial.print(" Next cron event: ");Serial.print(timeText(Cron.getNextTrigger(tmpCronID)));}
}
void setSet1() {
  setRelay = HIGH;
  digitalWrite(SETRELAYPIN, setRelay);
  //tmpCronID=Cron.create("*/10 * * * * *", clearSet,true); //several seconds
  tmpCronID=iotfw.runIn(10,iotfw::sec,clearSet); //several seconds
  //if(DEBUG) {String ptr = String(__LINE__) + " Set Set: %TIME"; Serial.println(StringTok((char *) ptr.c_str()));}
  //if(DEBUG) {Serial.print(__LINE__);Serial.print(" Next cron event: ");Serial.print(timeText(Cron.getNextTrigger(tmpCronID)));}
}
void clearSet() {
  setRelay = LOW;
  digitalWrite(AUXRELAYPIN, setRelay);
  //if(DEBUG) {String ptr = String(__LINE__) + " Clear Set: %TIME"; Serial.println(StringTok((char *) ptr.c_str()));}
}
/* setDefeat() will be called about 5 minutes before the next expected excercise time (1 week from last)
   This will disable the auto excercise from occuring every week to allow a montly or quarterly run
   it will call clearDefeat() 10 minutes later allowing a window of -5 to +5 minutes from the weekly excercise

   clearDefeat will also be called if a power failure is detected
*/
void setDefeat() {
  if(!powerFail) {
    defeatRelay = HIGH;
    digitalWrite(DEFEATRELAYPIN, defeatRelay);
  //           SS MM  HH DM Mo DW  Func        true = once
  //tmpCronID=Cron.create("* */10 *  *  *  *", clearDefeat,true); // 10 minutes
    tmpCronID=iotfw.runIn(10,iotfw::min,clearDefeat); // 10 minutes
  //if(DEBUG) {Serial.print(__LINE__);Serial.print(" Next cron event: ");Serial.print(timeText(Cron.getNextTrigger(tmpCronID)));}
  }
}
void clearDefeat() {
  defeatRelay = LOW;
  digitalWrite(DEFEATRELAYPIN, defeatRelay);
  //if(DEBUG) {String ptr = String(__LINE__) + " Clearing defeat: %TIME"; Serial.println(StringTok((char *) ptr.c_str()));}
}

String genSmartTokHandler(const String& var) {
  String ret = "__UNSET__";   
  if (var=="DEFEAT") {
    if(defeatRelay == HIGH)
      {ret="<p>Defeat: ON</p><a class=\"button button-off\" href=\"/relayTest?def=off\">OFF</a>\n";}
    else
      {ret="<p>Defeat: OFF</p><a class=\"button button-on\" href=\"/relayTest?def=on\">ON</a>\n";}
  }
  if (var=="SET") {
    if(setRelay == HIGH)
      {ret="<p>Set: ON</p><a class=\"button button-off\" href=\"/relayTest?set=off\">OFF</a>\n";}
    else
      {ret="<p>Set: OFF</p><a class=\"button button-on\" href=\"/relayTest?set=on\">ON</a>\n";}
  }
  if (var=="AUX") {
  if(auxRelay == HIGH)
      {ret="<p>Aux: ON</p><a class=\"button button-off\" href=\"/relayTest?aux=off\">OFF</a>\n";}
    else
      {ret="<p>Aux: OFF</p><a class=\"button button-on\" href=\"/relayTest?aux=on\">ON</a>\n";}
  }
  if (var=="GEN_FOOTER") {
    ret="<a href =\"/sensors\">sensors</a><br><a href =\"/caldata\">Calibraion data</a><br><a href =\"/relayTest\">Relay Test</a><br><a href =\"/cron\">Cron status and test</a><br>";
  }
  if (var=="TEMPF") {
    ret=String(tempF);
  }
  if (var=="VREF") {
    ret=String(vref);
  }
  if (var=="VBAT") {
    ret=String(vbat);
  }
  if (var=="VACRMS") {
    ret=String(vacrms);
  }
  if (var=="CURRA") {
    ret=String(currArms);
  }
  if (var=="CURRB") {
    ret=String(currBrms);
  }
  if (var=="CRONCOUNT") {
    ret=String(Cron.count());
  }
  if (var=="SETTIME") {
    ret=timeText(Cron.getNextTrigger(setCronID));
  }
  if (var=="DEFTIME") {
    ret=timeText(Cron.getNextTrigger(defCronID));
  }
  if (var=="CRONNEXTTRIGGER") {
    ret=timeText(Cron.getNextTrigger());
  }
  if (var=="TMPTIME") {
    if(Cron.isAllocated(tmpCronID)) {
      ret=timeText(Cron.getNextTrigger(tmpCronID));
    } else {
      ret = "NO tmp cron allocation";
    }
  }
  if (var=="ALLCRON") {
    ret="";
    for(uint8_t i=0; i<dtNBR_ALARMS; i++) {
      ret += "Cron[" + String(i) + "] : "; 
      if(Cron.isAllocated(i)) {
	ret += timeText(Cron.getNextTrigger(i)) + "<br>";
      } else {
	ret += "not allocated<br>";
      }
    }
  }
  if (var=="CRONTIMEFORM") {
    ret=gen_cronTimeForm();
  }
  return(ret);
}
String timeText(time_t time) {
  struct tm * tmp_timeinfo;
  tmp_timeinfo = localtime (& time);
  String ret = asctime(tmp_timeinfo);
  return(ret);
}
String gen_cronTimeForm() {
  char timestr [6];
  sprintf(timestr,"%02d:%02d",cront.hr,cront.min);
   String ptr="<label for=\"time\">Time</label><input type=\"time\" id=\"time\" name=\"time\" value=\"";
   ptr += String(timestr) +"\" min=\"9:00\" max=\"20:00\">";
   ptr+="<label for=\"dow\">Day</label><select name=\"dow\" id=\"dow\">";
   for (int i =0; i<7; i++) {
     ptr+= "<option ";
     if(cront.dow == i) {ptr += "selected ";}
     ptr+= "value=" + String(i) + ">" + String(days[i]) +"</option>\n";
   }
   ptr += "</select>";
   ptr+= "<label for=\"frequency\">Frequency</label><select name=\"frequency\" id=\"frequency\">";
   for (int i =1; i<6; i++) {
     ptr+= "<option ";
     if(cront.freq == i) {ptr += "selected ";}
     ptr+= "value=" + String(i) + ">" + String(freq[i]) +"</option>\n";
   }
   ptr += "</select>";
   return(ptr);
}
/*----------------------------------------------------------------------
  unset the generac controller run time by disconnecting it from power
  by setting defeat (manual) and holding the set button (open) to
  essentualy diconnecting all power form the board.

  After holding power disconnected the lights will flash until the
  cron run sets a new excercise time
----------------------------------------------------------------------*/
void unsetAuto() {
  defeatRelay = HIGH;
  digitalWrite(DEFEATRELAYPIN, defeatRelay);
  setRelay = HIGH;
  digitalWrite(SETRELAYPIN, setRelay);
  tmpCronID=iotfw.runIn(10,iotfw::sec,releaseAuto); //a minute
}
void releaseAuto() {
  setRelay = LOW;
  digitalWrite(SETRELAYPIN, setRelay);
  defeatRelay = LOW;
  digitalWrite(DEFEATRELAYPIN, defeatRelay);
}
/*----------------------------------------------------------------------
  Simulate power failure by disconnecting sense transformer winding on line 225.
  simulates a 15 minute power failure
  ----------------------------------------------------------------------*/
void simulatePowerFail() {
  auxRelay = HIGH;
  digitalWrite(AUXRELAYPIN, auxRelay);
  tmpCronID=iotfw.runIn(15,iotfw::min,restorePowerFail); //15 minutes
}
void restorePowerFail() {
  auxRelay = LOW;
  digitalWrite(AUXRELAYPIN, auxRelay);
}
