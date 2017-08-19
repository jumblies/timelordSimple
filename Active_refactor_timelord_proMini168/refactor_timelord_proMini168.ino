/*  Refactor 20170819
 *   Extended delay to 5 minutes to cut down on bouncing.  
 *   fits on proMini 168.
 */


#include <Wire.h>
//#include <Time.h>  //Testing whether needed
#include <DS1307RTC.h>
#include <Wire.h>
#include <TimeLord.h>
//#include <TimeLib.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// DHT11 Temperature and Humidity Sensors
#define REDPIN 5
#define GREENPIN 6
#define BLUEPIN 3

//Dallas Onewire
#define ONE_WIRE_BUS 10
// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);
// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);
// arrays to hold device address
DeviceAddress insideThermometer;


/*DEBUGGING SETTINGS    *******************************************************************/
#define DEBUG 0  // Set to 1 to enable debug messages through serial port monitor
#define TIMEDRIFT 0 //Set to 1 enable systemtime and RTC sync in the loop

/*GLOBAL VARIABLES*/
byte sunTime[]  = {0, 0, 0, 1, 1, 13}; //Byte Array for storing date to feed into Timelord
int dayMinNow, dayMinLast = -1, hourNow, hourLast = -1, minOfDay; //time parts to trigger various actions.
// -1 init so hour/min last inequality is triggered the first time around
int mSunrise, mSunset; //sunrise and sunset expressed as minute of day (0-1439)

//  TIMELORD VAR
TimeLord tardis;  //INSTANTIATE TIMELORD OBJECT
// Greensboro, NC  Latitude: 36.145833 | Longitude: -79.801728
float const LONGITUDE = -79.80;
float const LATITUDE = 36.14;
byte const TIMEZONE = -4;


const char *monthName[12] = {
  "Jan", "Feb", "Mar", "Apr", "May", "Jun",
  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

tmElements_t tm;

void setup() {
  bool parse = false;
  bool config = false;

  // get the date and time the compiler was run
  if (getDate(__DATE__) && getTime(__TIME__)) {
    parse = true;
    // and configure the RTC with this info
    if (RTC.write(tm)) {
      config = true;
    }
  }
# if DEBUG == 1
  Serial.begin(115200);
  while (!Serial) ; // wait for Arduino Serial Monitor
  delay(200);
  if (parse && config) {
    Serial.print("DS1307 configured Time=");
    Serial.print(__TIME__);
    Serial.print(", Date=");
    Serial.println(__DATE__);
  } else if (parse) {
    Serial.println("DS1307 Communication Error :-{");
    Serial.println("Please check your circuitry");
  } else {
    Serial.print("Could not parse info from the compiler, Time=\"");
    Serial.print(__TIME__);
    Serial.print("\", Date=\"");
    Serial.print(__DATE__);
    Serial.println("\"");
  }
#endif


  //Dallas Sensor Setup
  if (!sensors.getAddress(insideThermometer, 0)) Serial.println("Unable to find address for Device 0");
  // set the resolution to 9 bit (Each Dallas/Maxim device is capable of several different resolutions)
  sensors.setResolution(insideThermometer, 9);

  /* TimeLord Object Initialization */
  tardis.TimeZone(TIMEZONE * 60);
  tardis.Position(LATITUDE, LONGITUDE);

  setSyncProvider(RTC.get);   // the function to get the time from the RTC
  if (timeStatus() != timeSet) {
    Serial.println("Unable to sync with the RTC");
  }
  else {
    Serial.println("RTC has set the system time");
  }
  blinkWhite();
}

void loop() {

  tmElements_t tm; //Instantiate time object

  // Dallas onewire
  sensors.requestTemperatures(); // Send the command to get temperatures
  float tempC = sensors.getTempC(insideThermometer);
  float t = (DallasTemperature::toFahrenheit(tempC));

  /* CHECK TIME */
  if (timeStatus() == timeNeedsSync) {
    Serial.println("Hey, Time is out of Sync");
  };

  if (RTC.read(tm)) {
# if DEBUG == 1
    Serial.print("RTC TIME = ");
    print2digits(tm.Hour);
    Serial.print(':');
    print2digits(tm.Minute);
    Serial.print(':');
    print2digits(tm.Second);
    Serial.print(" Date = ");
    Serial.print(tmYearToCalendar(tm.Year));
    Serial.print('/');
    print2digits(tm.Month);
    Serial.print('/');
    print2digits(tm.Day);
    Serial.println();
#endif
    // Lets figure out where we are in the day in terms of minutes
    dayMinNow = (minute() + (hour() * 60));
#if DEBUG ==1
    Serial.print("Minutes into the current Day: ");
    Serial.println(dayMinNow);
#endif
    // Set sunTime Array to today's DateUses the Time library to give Timelord the current date
    sunTime[3] = day();
    sunTime[4] = month();
    sunTime[5] = year();

    /* Sunrise: */
    tardis.SunRise(sunTime); // Computes Sun Rise.
    mSunrise = sunTime[2] * 60 + sunTime[1];  //Minutes after midnight that sunrise occurs

    /* Sunset: */
    tardis.SunSet(sunTime); // Computes Sun Set.
    mSunset = sunTime[2] * 60 + sunTime[1];
#if DEBUG ==1
    Serial.print("SunriseMin: ");
    Serial.println(mSunrise);           // Print minutes after midnight that sunrise occurs
    Serial.print("SunsetMin: ");
    Serial.println(mSunset);            // Print minutes after midnight that sunrise occurs
    Serial.print("Min of daylight today: ");
    int mDaylight = (mSunset - mSunrise);
    Serial.println(mDaylight);          // Print minutes of daylight today
    Serial.print("Min until Sunrise: ");
    int mLeftTilDawn = (mSunrise - dayMinNow);
    Serial.println(mLeftTilDawn);              // Print Minutes Until
    Serial.print("Min until Sunset: ");
    int mLeftInDay = (mSunset - dayMinNow);
    Serial.println(mLeftInDay);              // Print Minutes Until Sunset
    int pctDayLeft = ((float(mLeftInDay) / float(mDaylight)) * 100);
    if (pctDayLeft > 100) {
      pctDayLeft = 100;
    };
    Serial.print("% of daylight left: ");
    Serial.println (pctDayLeft);
    Serial.print("Temp: ");    //print current environmental conditions
    Serial.println(t, 2);    //print the temperature
    Serial.println("**************************************");
#endif

    /* SET THE COLOR SCHEME DEPENDING ON TEMPERATURE
        Currently set for summer and checking every 3 seconds which is excessive.
        this will lead to color bouncing at border temperatures and probably need debouncing.
        I can probably change this to switch or "functionalize" the color change
        and further I can have it automatically change colors based on summer or winter
        to save hassle and reuploading of code.
        Can base summer and winter on min of daylight to get solstice.
    */

    if
    (((dayMinNow >= (mSunset)) && (dayMinNow <= (mSunset + 60)))
        ||
        ((dayMinNow >= (mSunrise)) && (dayMinNow <= (mSunrise + 60)))) {
      if (t > 85) {
        analogWrite(REDPIN, 255);
        analogWrite(BLUEPIN, 0);
        analogWrite(GREENPIN, 0);
      }
      else if (t <= 85 && t >= 80) {
        analogWrite(GREENPIN, 255);
        analogWrite(BLUEPIN, 0);
        analogWrite(REDPIN, 255);
      }
      else if (t <= 80 && t >= 70) {
        analogWrite(GREENPIN, 150);
        analogWrite(BLUEPIN, 150);
        analogWrite(REDPIN, 0);
      }
      else if (t <= 70 && t >= 60) {
        analogWrite(GREENPIN, 255);
        analogWrite(BLUEPIN, 0);
        analogWrite(REDPIN, 0);
      }
      else if (t <= 60 && t >= 50) {
        analogWrite(GREENPIN, 0);
        analogWrite(BLUEPIN, 150);
        analogWrite(REDPIN, 150);
      }
      else {
        analogWrite(BLUEPIN, 255);
        analogWrite(REDPIN, 255);
        analogWrite(GREENPIN, 255);
      }
    }
    else {
      analogWrite(REDPIN, 0);
      analogWrite(BLUEPIN, 0);
      analogWrite(GREENPIN, 0);
    }
        delay(5 * 60000);
//    delay(1000);
  }




}

bool getTime(const char *str)
{
  int Hour, Min, Sec;

  if (sscanf(str, "%d:%d:%d", &Hour, &Min, &Sec) != 3) return false;
  tm.Hour = Hour;
  tm.Minute = Min;
  tm.Second = Sec;
  return true;
}

bool getDate(const char *str)
{
  char Month[12];
  int Day, Year;
  uint8_t monthIndex;

  if (sscanf(str, "%s %d %d", Month, &Day, &Year) != 3) return false;
  for (monthIndex = 0; monthIndex < 12; monthIndex++) {
    if (strcmp(Month, monthName[monthIndex]) == 0) break;
  }
  if (monthIndex >= 12) return false;
  tm.Day = Day;
  tm.Month = monthIndex + 1;
  tm.Year = CalendarYrToTm(Year);
  return true;
}
void print2digits(int number) {
  if (number >= 0 && number < 10) {
    Serial.print('0');
  }
  Serial.print(number);
}
void blinkWhite() {
  //BLINK WHITE LIGHTS STARTUP TO CONFIRM ACTIVITY
  analogWrite(REDPIN, 255);
  analogWrite(BLUEPIN, 255);
  analogWrite(GREENPIN, 255);
  delay(5000);
  analogWrite(REDPIN, 0);
  analogWrite(BLUEPIN, 0);
  analogWrite(GREENPIN, 0);
}


