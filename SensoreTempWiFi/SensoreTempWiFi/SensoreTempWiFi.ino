/*
  Auth: andreoidb64 @github
  Date: 06.02.2025

*/

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

String logIndx = "00";  // Set temperature ID number
String logFile = "TempProbe" + logIndx + "-"; // Set FTP log files prefix here.
int sleepTime = 50 ; // Set sleep time [s]
#include <LocalConf.h>  // Put your local configuration (such as SSID, password, etc. here)


/* Configuration of NTP */
const char* MY_NTP_SERVER = "it.pool.ntp.org";
const char* MY_TZ = "CET-1CEST,M3.5.0/02,M10.5.0/03";
//#define MY_NTP_SERVER "it.pool.ntp.org"           
//#define MY_TZ "CET-1CEST,M3.5.0/02,M10.5.0/03"   


/* Necessary Includes */
//#include <WiFi.h>                     // we need wifi to get internet access
#include <time.h>                     // for time() ctime()
#include <FTPClient_Generic.h>

/* Globals */
time_t now;                         // this are the seconds since Epoch (1970) - UTC
tm tm;                              // the structure tm holds time information in a more convenient way

char ftp_server[] = FTP_SERVER ;
char ftp_user[]   = FTP_USER ;
char ftp_pass[]   = FTP_PASS ;

// you can pass a FTP timeout and debbug mode on the last 2 arguments
FTPClient_Generic ftp (ftp_server,21,ftp_user,ftp_pass, 5000);

// DS18B20 Temperature sensors initializing
#include <DallasTemperature.h>
#include <OneWire.h>

// Data wire is plugged into port 2 on the Arduino [ == "D4" on esp8266 (sigh!)]
#define ONE_WIRE_BUS 2
#define TEMPERATURE_PRECISION 12

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

// arrays to hold device addresses
DeviceAddress inletThermometer, outletThermometer;

// Assign address manually. The addresses below will need to be changed
// to valid device addresses on your bus. Device address can be retrieved
// by using either oneWire.search(deviceAddress) or individually via
// sensors.getAddress(deviceAddress, index)
// DeviceAddress inletThermometer = { 0x28, 0x1D, 0x39, 0x31, 0x2, 0x0, 0x0, 0xF0 };
// DeviceAddress outletThermometer   = { 0x28, 0x3F, 0x1C, 0x31, 0x2, 0x0, 0x0, 0x2 };


String showTime() {
  time(&now);                       // read the current time
  localtime_r(&now, &tm);           // update the structure tm with the current time
  String MyTime = "";
  //Serial.print("year:");
  MyTime += (tm.tm_year + 1900);  // years since 1900
  MyTime += ":";
  MyTime += tm.tm_mon + 1;      // January = 0 (!)
  MyTime += ":";
  MyTime += tm.tm_mday;         // day of month
  MyTime += ":";
  MyTime += tm.tm_hour;         // hours since midnight  0-23
  MyTime += ":";
  MyTime += tm.tm_min;          // minutes after the hour  0-59
  MyTime += ":";
  MyTime += tm.tm_sec;          // seconds after the minute  0-61*
  MyTime += ";";
  MyTime += tm.tm_wday;         // days since Sunday 0-6
  //if (tm.tm_isdst == 1)             // Daylight Saving Time flag
  //  Serial.print("\tDST");
  //else
  //  Serial.print("\tstandard");
  return(MyTime);
}

String showTemp() {
  // call sensors.requestTemperatures() to issue a global temperature
  // request to all devices on the bus
  //Serial.print("Requesting temperatures...");
  sensors.requestTemperatures();

  String MyTemp = "";
  //getSensorTemp(inletThermometer);
  MyTemp += sensors.getTempC(inletThermometer);

  //debug
  return(MyTemp);
  //debug

  MyTemp += ";";
  //getSensorTemp(outletThermometer);
  MyTemp += sensors.getTempC(outletThermometer);
  return(MyTemp);
}

void NetworkConnect() { // start network
  int timeout = 15;
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.begin(STASSID, STAPSK);
  while (WiFi.status() != WL_CONNECTED && timeout-- > 0 ) {
    delay(1000);
    Serial.print ( "." );
  }
  Serial.print("\nWiFi connected. IP: ");
  Serial.println(WiFi.localIP());

  configTime(0, 0, MY_NTP_SERVER); // Config NTP
}

// the setup function runs once when you press reset or power the board
void setup() {
  Serial.begin(74880);
  
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);
  setupTempSensors();
}

void setupTempSensors() {
  Serial.println("Dallas Temperature IC Control Library Demo");

  // Start up the library
  sensors.begin();

  // locate devices on the bus
  Serial.print("Locating devices...");
  Serial.print("Found ");
  Serial.print(sensors.getDeviceCount(), DEC);
  Serial.println(" devices.");

  // report parasite power requirements
  Serial.print("Parasite power is: ");
  if (sensors.isParasitePowerMode()) Serial.println("ON");
  else Serial.println("OFF");

  // Search for devices on the bus and assign based on an index. Ideally,
  // you would do this to initially discover addresses on the bus and then
  // use those addresses and manually assign them (see above) once you know
  // the devices on your bus (and assuming they don't change).
  //
  // method 1: by index
  if (!sensors.getAddress(inletThermometer, 0)) Serial.println("Unable to find address for Device 0");
  if (!sensors.getAddress(outletThermometer, 1)) Serial.println("Unable to find address for Device 1");

  // method 2: search()
  // search() looks for the next device. Returns 1 if a new address has been
  // returned. A zero might mean that the bus is shorted, there are no devices,
  // or you have already retrieved all of them. It might be a good idea to
  // check the CRC to make sure you didn't get garbage. The order is
  // deterministic. You will always get the same devices in the same order
  //
  // Must be called before search()
  //oneWire.reset_search();
  // assigns the first address found to inletThermometer
  //if (!oneWire.search(inletThermometer)) Serial.println("Unable to find address for inletThermometer");
  // assigns the seconds address found to outletThermometer
  //if (!oneWire.search(outletThermometer)) Serial.println("Unable to find address for outletThermometer");

  // show the addresses we found on the bus
  Serial.print("Device 0 Address: ");
  printAddress(inletThermometer);
  Serial.println();

  Serial.print("Device 1 Address: ");
  printAddress(outletThermometer);
  Serial.println();

  // set the resolution to 9 bit per device
  sensors.setResolution(inletThermometer, TEMPERATURE_PRECISION);
  sensors.setResolution(outletThermometer, TEMPERATURE_PRECISION);

  Serial.print("Device 0 Resolution: ");
  Serial.print(sensors.getResolution(inletThermometer), DEC);
  Serial.println();

  Serial.print("Device 1 Resolution: ");
  Serial.print(sensors.getResolution(outletThermometer), DEC);
  Serial.println();
  delay(100);
}

// function to print a device address
void printAddress(DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    // zero pad the address if necessary
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}

// function to print the temperature for a device
void printTemperature(DeviceAddress deviceAddress)
{
  float tempC = sensors.getTempC(deviceAddress);
  if(tempC == DEVICE_DISCONNECTED_C) 
  {
    Serial.println("Error: Could not read temperature data");
    return;
  }
  Serial.print("Temp C:");
  Serial.print(tempC);
  //Serial.print(" Temp F: ");
  //Serial.print(DallasTemperature::toFahrenheit(tempC));
}

// function to print a device's resolution
void printResolution(DeviceAddress deviceAddress)
{
  Serial.print("Resolution: ");
  Serial.print(sensors.getResolution(deviceAddress));
  Serial.println();
}

// main function to print information about a device
void getSensorTemp(DeviceAddress deviceAddress)
{
  Serial.print("Device Address: ");
  printAddress(deviceAddress);
  Serial.print(" ");
  printTemperature(deviceAddress);
  Serial.println();
}

void (* Reboot)(void) = 0; // Restart OS

// the loop function runs over and over again forever
void loop() {
/*
  digitalWrite(LED_BUILTIN, HIGH);  // turn the LED on (HIGH is the voltage level)
  delay(1000);                      // wait for a second
  digitalWrite(LED_BUILTIN, LOW);   // turn the LED off by making the voltage LOW
  delay(1000);                      // wait for a second
*/

  String logTemp = showTemp();
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("Not connected...");
    delay(200);
    NetworkConnect();
  }

  time(&now);                       // read the current time
  localtime_r(&now, &tm);           // update the structure tm with the current time

  String dummyString1 = "";
  String dummyString2 = "";
  String logDate = "";

  logFile += (tm.tm_year + 1900);
  logFile += "-";
  dummyString2 = tm.tm_mon + 1;
  if (dummyString2.length() < 2) dummyString2 = "0" + dummyString2;
  logFile += dummyString2 + "-";
  dummyString2 = tm.tm_mday;
  if (dummyString2.length() < 2) dummyString2 = "0" + dummyString2;
  logFile += dummyString2 + ".log";

  logDate += (tm.tm_year + 1900);  // years since 1900
  logDate += ":";
  logDate += (tm.tm_mon + 1);    // January = 0 (!)
  logDate += ":";
  logDate += tm.tm_mday;         // day of month
  logDate += ":";
  logDate += tm.tm_hour;         // hours since midnight  0-23
  logDate += ":";
  logDate += tm.tm_min;          // minutes after the hour  0-59
  logDate += ":";
  logDate += tm.tm_sec;          // seconds after the minute  0-61*
  logDate += ";";
  logDate += tm.tm_wday;         // days since Sunday 0-6

  dummyString1 = logDate + ";" + logIndx + ";" + logTemp + "\n";

  // Print collected data to serial console
  Serial.print(logFile);
  Serial.print(" ");
  Serial.print(dummyString1);

  // Log data to FTP
  Serial.println("FTP login...");
  ftp.OpenConnection();
  delay(100);
  if (ftp.isConnected()) {
    const char * ftpLine = dummyString1.c_str();
    const char * ftpFile = logFile.c_str();
    ftp.ChangeWorkDir("/IOT");
    ftp.InitFile(COMMAND_XFER_TYPE_ASCII);
    ftp.AppendFile(ftpFile);
    ftp.Write(ftpLine);
    ftp.CloseFile();
  }

  Serial.print("Start deepSleep... Zzzzzz Zzzzzz!");
  ESP.deepSleep(sleepTime * 1e6);
}
