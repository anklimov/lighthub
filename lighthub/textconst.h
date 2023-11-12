#pragma once

const char state_P[] PROGMEM = "$state";
const char disconnected_P[] PROGMEM = "disconnected";
const char ready_P[] PROGMEM = "ready";
const char salt_P[] PROGMEM = "$salt";

const char homie_P[] PROGMEM = "$homie";
const char homiever_P[] PROGMEM = "3.0.1";

const char name_P[] PROGMEM = "$name";
const char nameval_P[] PROGMEM = "LightHub ";
const char lighthub[]  = "lighthub";

const char nodes_P[] PROGMEM = "$nodes";
const char properties_P[] PROGMEM = "$properties";
const char propertiesVal_P[] PROGMEM = "set,cmd";
const char settable_P[] PROGMEM = "settable";
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
const char intformat_P[]   PROGMEM = "0-255";

const char stats_P[]     PROGMEM = "$stats";
const char statsval_P[]  PROGMEM = "uptime,freeheap";
const char uptime_P[]    PROGMEM = "uptime";
const char freeheap_P[]  PROGMEM = "freeheap";
const char CMDTOPIC_P[]  PROGMEM = CMDTOPIC;

const char action_P[] PROGMEM = "action";
const char cooling_P[] PROGMEM = "cooling";
const char heating_P[] PROGMEM = "heating";
const char drying_P[] PROGMEM = "drying";
const char idle_P[] PROGMEM = "idle";
const char fan_P[] PROGMEM = "fan";
const char off_P[] PROGMEM = "off";
const char on_P[] PROGMEM = "on";

/*

{"name": "Спальня LED",
"command_topic": "myhome/in/bedr/cmd",
"state_topic": "myhome/s_out/bedr/cmd",
"hs_command_topic": "myhome/in/bedr/set",
"hs_state_topic": "myhome/s_out/bedr/set",
"hs_value_template": "{%set a=value.split(',')%}       {% if a[1] -%}          {{a[0]+','+a[1]}}      {%- endif %}",
"brightness_scale": 100,
"brightness_command_topic": "myhome/in/bedr/set",
"brightness_state_topic": "myhome/s_out/bedr/set",
"brightness_value_template": "{{ value.split(',')[2] }}"
}

{
"~":"myhome"
"name": "Спальня LED",
"cmd_t": "~/in/bedr/cmd",
"tat_t": "~/s_out/bedr/cmd",
"hs_cmd_t": "~/in/bedr/set",
"hs_stat_t": "~/s_out/bedr/set",
"hs_val_tpl": "{%set a=value.split(',')%}{% if a[1] -%}{{a[0]+','+a[1]}}{%- endif %}",
"bri_scl": 100,
"bri_cmd_t": "~/in/bedr/set",
"bri_stat_t": "~/s_out/bedr/set",
"bri_val_tpl": "{{ value.split(',')[2] }}"
}

*/

#define ON_P commands_P[CMD_ON]
#define OFF_P commands_P[CMD_OFF]
#define REST_P commands_P[CMD_RESTORE]
#define TOGGLE_P commands_P[CMD_TOGGLE]
#define HALT_P commands_P[CMD_HALT]
#define XON_P commands_P[CMD_XON]
#define XOFF_P commands_P[CMD_XOFF]
#define HEAT_P commands_P[CMD_HEAT]
#define COOL_P commands_P[CMD_COOL]
#define AUTO_P commands_P[CMD_AUTO]
#define FAN_ONLY_P commands_P[CMD_FAN]
#define DRY_P commands_P[CMD_DRY]
#define HIGH_P commands_P[CMD_HIGH]
#define MED_P commands_P[CMD_MED]
#define LOW_P commands_P[CMD_LOW]
#define ENABLE_P commands_P[CMD_ENABLE]
#define DISABLE_P commands_P[CMD_DISABLE]
#define FREEZE_P commands_P[CMD_FREEZE]
#define UNFREEZE_P commands_P[CMD_UNFREEZE]

const char ERROR_P[]  PROGMEM = "ERR";

//Commands
/*
const char INCREASE_P[] PROGMEM = "INCREASE";
const char DECREASE_P[] PROGMEM = "DECREASE";
const char TRUE_P[]  PROGMEM = "TRUE";
const char FALSE_P[] PROGMEM = "FALSE";

const char ENABLED_P[]  PROGMEM = "ENABLED";
const char DISABLED_P[] PROGMEM = "DISABLED";
*/

/*
const char ON_P[]   PROGMEM = "ON";
const char OFF_P[]  PROGMEM = "OFF";
const char REST_P[] PROGMEM = "REST";
const char TOGGLE_P[] PROGMEM = "TOGGLE";
const char HALT_P[] PROGMEM = "HALT";
const char XON_P[]  PROGMEM = "XON";
const char XOFF_P[] PROGMEM = "XOFF";
const char HEAT_P[] PROGMEM = "HEAT";
const char COOL_P[] PROGMEM = "COOL";
const static char AUTO_P[] PROGMEM = "AUTO";


const char FAN_ONLY_P[]  PROGMEM = "FAN_ONLY";
const char DRY_P[]  PROGMEM = "DRY";
const char HIGH_P[] PROGMEM = "HIGH";
const char MED_P[]  PROGMEM = "MEDIUM";
const char LOW_P[]  PROGMEM = "LOW";

const char ENABLE_P[]  PROGMEM = "ENABLE";
const char DISABLE_P[] PROGMEM = "DISABLE";
const char FREEZE_P[]  PROGMEM = "FREEZE";
const char UNFREEZE_P[] PROGMEM = "UNFREEZE";
*/

// SubTopics
/*
const char SET_P[]  PROGMEM = "set";
const char CMD_P[]  PROGMEM = "cmd";
const char MODE_P[] PROGMEM = "mode";
const char FAN_P[]  PROGMEM = "fan";
const char HUE_P[]   PROGMEM = "hue";
const char SAT_P[]   PROGMEM = "sat";
const char TEMP_P[]   PROGMEM = "temp";
const char HSV_P[]   PROGMEM = "HSV";
const char RGB_P[]   PROGMEM = "RGB";
const char VAL_P[]   PROGMEM = "val";
const char DEL_P[]   PROGMEM = "del";
const char _RAW_P[]   PROGMEM = "raw";
const char CTRL_P[]  PROGMEM = "ctrl";
*/
/*
const char RPM_P[]   PROGMEM = "rpm";
const char STATE_P[] PROGMEM = "state";
*/

