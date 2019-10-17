#pragma once

const char state_P[] PROGMEM = "$state";
const char disconnected_P[] PROGMEM = "disconnected";
const char ready_P[] PROGMEM = "ready";

const char homie_P[] PROGMEM = "$homie";
const char homiever_P[] PROGMEM = "2.1.0";

const char name_P[] PROGMEM = "$name";
const char nameval_P[] PROGMEM = "LightHub ";

const char nodes_P[] PROGMEM = "$nodes";
const char localip_P[] PROGMEM = "$localip";
const char mac_P[] PROGMEM = "$mac";
const char fwname_P[] PROGMEM = "$fw/name";
const char fwversion_P[] PROGMEM = "$fw/version";
const char implementation_P[] PROGMEM =  "$implementation";
const char interval_P[] PROGMEM =  "$stats/interval";
const char color_P[] PROGMEM = "color";

const char datatype_P[] PROGMEM = "$datatype";

const char float_P[] PROGMEM = "float";
const char hsv_P[]   PROGMEM = "hsv";
const char int_P[]   PROGMEM = "integer";
const char enum_P[]   PROGMEM = "enum";
const char format_P[] PROGMEM = "$format";
const char true_P[]  PROGMEM = "true";
const char false_P[] PROGMEM = "false";

const char enumformat_P[]  PROGMEM = "ON,OFF,HALT,REST,XON,XOFF,TOGGLE";
const char intformat_P[]   PROGMEM = "0-100";

const char stats_P[]     PROGMEM = "$stats";
const char statsval_P[]  PROGMEM = "uptime,freeheap";
const char uptime_P[]    PROGMEM = "uptime";
const char freeheap_P[]  PROGMEM = "freeheap";


//Commands
const char ON_P[]   PROGMEM = "ON";
const char OFF_P[]  PROGMEM = "OFF";
const char REST_P[] PROGMEM = "REST";
const char TOGGLE_P[] PROGMEM = "TOGGLE";
const char HALT_P[] PROGMEM = "HALT";
const char XON_P[]  PROGMEM = "XON";
const char XOFF_P[] PROGMEM = "XOFF";
const char INCREASE_P[] PROGMEM = "INCREASE";
const char DECREASE_P[] PROGMEM = "DECREASE";
const char TRUE_P[]  PROGMEM = "TRUE";
const char FALSE_P[] PROGMEM = "FALSE";

const char ENABLED_P[]  PROGMEM = "ENABLED";
const char DISABLED_P[] PROGMEM = "DISABLED";

const char HEAT_P[] PROGMEM = "HEAT";
const char COOL_P[] PROGMEM = "COOL";
const static char AUTO_P[] PROGMEM = "AUTO";
const char FAN_ONLY_P[]  PROGMEM = "FAN_ONLY";
const char DRY_P[]  PROGMEM = "DRY";
const char HIGH_P[] PROGMEM = "HIGH";
const char MED_P[]  PROGMEM = "MEDIUM";
const char LOW_P[]  PROGMEM = "LOW";
// SubTopics
const char SET_P[]  PROGMEM = "set";
const char CMD_P[]  PROGMEM = "cmd";
const char MODE_P[] PROGMEM = "mode";
const char FAN_P[]  PROGMEM = "fan";
/*
const char TEMP_P[] PROGMEM = "temp";
const char SETPOINT_P[] PROGMEM = "setpoint";
const char POWER_P[] PROGMEM = "power";
const char VOL_P[] PROGMEM = "vol";
const char HEAT_P[] PROGMEM = "heat";
*/
const char HSV_P[]   PROGMEM = "HSV";
const char RGB_P[]   PROGMEM = "RGB";
/*
const char RPM_P[]   PROGMEM = "rpm";
const char STATE_P[] PROGMEM = "state";
*/
