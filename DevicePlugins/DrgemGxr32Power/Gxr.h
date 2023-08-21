#ifndef GXR_H
#define GXR_H

#include <QtCore>

typedef quint8 GxrRegister;

const QMap<GxrRegister, float> TABLE_MA = {{0, 10},   {1, 12.5},
                                     {2, 16},   {3, 20},
                                     {4, 25},   {5, 32},
                                     {6, 40},   {7, 50},
                                     {8, 64},   {9, 80},
                                     {10, 100}, {11, 110}};
const QMap<GxrRegister, float> TABLE_TIME = {{24, 500}, {25, 640}, {26, 800},
                                     {27, 1000}, {28, 1250}, {29, 1600},
                                     {30, 2000}, {31, 2500}, {32, 3200},
                                     {33, 4000},  {34, 5000}, {35, 6400},
                                     {36, 8000}, {37, 10000}};

const QMap<GxrRegister, QString> TABLE_ERROR = {
   {0x1, "TXRX_NOANSWER_ERROR"},
   {0x2, "NVSRAM_FAIL_ERROR"},
   {0x3, "CHARGE_ERROR"},
   {0x4, "IGBT1_FAULT_ERROR"},
   {0x5, "IGBT2_FAULT_ERROR"},
   {0x6, "TUBE_TEMPERATURE_ERROR"},
   {0x7, "FAULT_SENSING_ERROR"},
   {0x8, "NVSRAM_PARA_ERROR"},
   {0x9, "TXRX0_NOANSWER_ERROR"},
   {0x10, "HW_OVER_MA_ERROR"},
   {0x11, "HW_OVER_KV_ERROR"},
   {0x12, "KV_INEQUALITY1_ERROR"},
   {0x13, "KV_INEQUALITY2_ERROR"},
   {0x14, "CAL_DATA_EMPTY_ERROR"},
   {0x15, "FIL1_SELECT_ERROR"},
   {0x16, "FIL2_SELECT_ERROR"},
   {0x17, "SW_OVER_MA_ERROR"},
   {0x18, "HEAT_UNIT_ERROR"},
   {0x19, "ROTOR_ACCEL_C_LOW_ERROR"},
   {0x20, "ROTOR_RUNNING_C_LOW_ERROR"},
   {0x21, "DSS_IPM_FAULT"},
   {0x22, "ROTOR_ACCEL_C_HIGH_ERROR"},
   {0x23, "ROTOR_RUNNING_C_HIGH_ERROR"},
   {0x24, "DSS_UNDER_VOLTAGE_ERROR"},
   {0x25, "DSS_HARDWARE_ERROR"},
   {0x26, "DSS_SPEED_SELECT_ERROR"},
   {0x27, "AEC_RAMP_ERROR"},
   {0x28, "AEC_INT_ERROR"},
   {0x29, "AEC_MAS_ERROR"},
   {0x30, "AEC_BUT_ERROR"},
   {0x31, "DR_READY_SET_ERROR"},
   {0x32, "TOMO_SET_ERROR"},
   {0x33, "BUCKY1_FEEDBACK_ERROR"},
   {0x34, "BUCKY2_FEEDBACK_ERROR"},
   {0x35, "CHARGE_MC_FAIL_ERROR"},
   {0x36, "EXT_MC_FAIL_ERROR"},
   {0x37, "MAIN_MC_FAIL_ERROR"},
   {0x38, "NO_KV_ERROR"},
   {0x39, "NO_MA_ERROR"},
   {0x40, "FIL_PREHEAT_LOW_ERROR"},
   {0x41, "FIL_PREHEAT_HIGH_ERROR"},
   {0x42, "FIL_READY_LOW_ERROR"},
   {0x43, "FIL_READY_HIGH_ERROR"},
   {0x44, "FIL_SMALL_ERROR"},
   {0x45, "FIL_LARGE_ERROR"},
   {0x46, "INTERLOCK_DOOR_ERROR"},
   {0x47, "INTERLOCK_EXT_ERROR"},
   {0x48, "HT_TXRX_NOANSWER_ERROR"},
   {0x49, "AEC_DATA_EMPTY_ERROR"},
   {0x50, "ROTOR_BRAKE_FAIL_ERROR"},
   {0x51, "CHARGER_UNDER_VOLTAGE_ERROR"},
   {0x52, "CHARGER_OVER_VOLTAGE_ERROR"},
   {0x53, "CHARGER_UNDER_CURRENT_ERROR"},
   {0x54, "CHARGER_OVER_CURRENT_ERROR"},
   {0x55, "CMB_OVER_VOLTAGE_ERROR"},
   {0x56, "BATTERY_EMPTY_ERROR"},
   {0x57, "CHARGER_FAIL_ERROR"},
   {0x58, "INV_MC_FAIL_ERROR"},
   {0x59, "INV_IPM_FAULT_ERROR"},
   {0x61, "TOMO_MOTOR_MOVING_ERROR"},
   {0x62, "TOMO_MOTOR_ERROR"},
   {0x63, "TOMO_EEPROM_ERROR"},
   {0x64, "TOMO_OVER_LIMIT_ERROR"},
   {0x65, "TOMO_SEQUENCY_ERROR"},
   {0x66, "TOMO_CASSETTE_ERROR"},
   {0x67, "TOMO_COMM_ERROR"},
   {0x70, "MA_SHORTBAR_ERROR"},
   {0x71, "ROTOR_SSR_FAIL_ERROR"}
};

const QMap<GxrRegister, QString> TABLE_WARNING = {
   {0x50, "SW_KV_LOW_WARNING"},
   {0x51, "SW_KV_HIGH_WARNING"},
   {0x52, "SW_MA_LOW_WARNING"},
   {0x54, "HU_LEVEL_WARNING"},
   {0x55, "TUBE_OVERLOAD_WARNING"},
   {0x56, "NVSRAM_RTC_FAIL_WARNING"},
   {0x57, "INTERLOCK_SOFTWARE_WARNING"},
   {0x58, "GEN_DUTY_LIMIT_WARNING"},
   {0x59, "ROTOR_DUTY_LIMIT_WARNING"},
   {0x60, "CRC_FAIL_WARNING"},
   {0x61, "VDC_LOW_LEVEL_WARNING"},
   {0x62, "SMPS_LOW_VOLTAGE_WARNING"},
   {0x63, "EXP_SWITCH_RELEASE_WARNING"},
   {0x64, "KV_FB_ABNORMAL_WARNING"},
   {0x65, "MA_FB_ABNORMAL_WARNING"},
   {0x66, "NVSRAM_INIT_WARNING"},
   {0x67, "TX_FIFO_OVERFLOW_WARNING"},
   {0x68, "HT_CRC_FAIL_WARNING"},
   {0x69, "TUBE_FAIL_SAFE_WARNING"},
   {0x70, "MINIMUM_KV_WARNING"},
   {0x71, "MAXIMUM_KV_WARNING"},
   {0x72, "MINIMUM_MA_WARNING"},
   {0x73, "MAXIMUM_MA_WARNING"},
   {0x74, "MINIMUM_EXP_TIME_WARNING"},
   {0x75, "MAXIMUM_EXP_TIME_WARNING"},
   {0x76, "MINIMUM_DENSITY_WARNING"},
   {0x77, "MAXIMUM_DENSITY_WARNING"},
   {0x78, "CONSOLE_RX_OVERFLOW_WARNING"},
   {0x79, "CONSOLE_TX_OVERFLOW_WARNING"},
   {0x80, "MINIMUM_MAS_WARNING"},
   {0x81, "MAXIMUM_MAS_WARNING"},
   {0x82, "MAXIMUM_OUTPUT_RATING_WARNING"},
   {0x83, "AEC_UNAVAILABLE_WARNING"},
   {0x84, "USB_RX_FIFO_OVERFLOW_WARNING"},
   {0x85, "USB_CONNECTION_FAIL_WARNING"},
   {0x86, "USB_CRC_FAIL_WARNING"},
   {0x87, "USB_TX_FAIL_WARNING"},
   {0x88, "AEC_FIELD_WARNING"},
   {0x89, "APR_DATA_WARNING"},
   {0x90, "CMB_OVER_VOLTAGE_WARNING"},
   {0x91, "INVERTER_UNDER_C_WARNING"},
   {0x92, "INVERTER_OVER_C_WARNING"},
   {0x93, "INVERTER_OVER_V_WARNING"},
   {0x94, "INVERTER_UNDER_V_WARNING"},
   {0x95, "BATTERY_LOW_WARNING"},
   {0x96, "EXP_TIME_OVER_WARNING"},
   {0x97, "FOCUS_CHANGE_WARNING"},
};

const GxrRegister UDP_PACKET_HEADER1 = 0xF0;	// UDP Header #1 : Fixed
const GxrRegister UDP_PACKET_HEADER2 = 0x0A;	// UDP Header #2 : Fixed
const GxrRegister UDP_PACKET_TAIL = 0x0F;	// UDP Tail : Fixed
const GxrRegister UDP_PACKET_FROM_SERVER	= 0x02;	// 0x02 : from Server to Client
const GxrRegister UDP_PACKET_FROM_CLIENT	= 0x01;	// 0x01 : from Clinet to Server

const GxrRegister UDP_CMD_SET_CONNECT = 1;
const GxrRegister UDP_CMD_SET_RKVMATIM = 10;
const GxrRegister UDP_CMD_SET_RKV = 11;
const GxrRegister UDP_CMD_SET_RMA = 12;
const GxrRegister UDP_CMD_SET_RTIME = 13;
const GxrRegister UDP_CMD_SET_RKV_DOWN = 14;
const GxrRegister UDP_CMD_SET_RKV_UP = 15;
const GxrRegister UDP_CMD_SET_RMA_DOWN = 16;
const GxrRegister UDP_CMD_SET_RMA_UP = 17;
const GxrRegister UDP_CMD_SET_RTIME_DOWN = 18;
const GxrRegister UDP_CMD_SET_RTIME_UP = 19;
const GxrRegister UDP_CMD_SET_NO_BUCKY = 21;
const GxrRegister UDP_CMD_SET_BUCKY1 = 22;
const GxrRegister UDP_CMD_SET_BUCKY2 = 23;
const GxrRegister UDP_CMD_SET_AEC = 30;
const GxrRegister UDP_CMD_SET_AEC_ON = 31;
const GxrRegister UDP_CMD_SET_AEC_OFF = 32;
const GxrRegister UDP_CMD_SET_AEC_SCREEN_SLOW = 33;
const GxrRegister UDP_CMD_SET_AEC_SCREEN_NORMAL = 34;
const GxrRegister UDP_CMD_SET_AEC_SCREEN_FAST = 35;
const GxrRegister UDP_CMD_SET_AEC_FIELD_LEFT = 36;
const GxrRegister UDP_CMD_SET_AEC_FIELD_CENTER = 37;
const GxrRegister UDP_CMD_SET_AEC_FIELD_RIGHT = 38;
const GxrRegister UDP_CMD_SET_AEC_FIELD_ALL = 39;
const GxrRegister UDP_CMD_SET_AEC_DENSITY = 40;
const GxrRegister UDP_CMD_SET_AEC_DENSITY_DEFAULT = 41;
const GxrRegister UDP_CMD_SET_AEC_DENSITY_MIN = 42;
const GxrRegister UDP_CMD_SET_AEC_DENSITY_MAX = 43;
const GxrRegister UDP_CMD_SET_AEC_DENSITY_DOWN = 44;
const GxrRegister UDP_CMD_SET_AEC_DENSITY_UP = 45;
const GxrRegister UDP_CMD_SET_APR = 50;
const GxrRegister UDP_CMD_SET_APR_ON = 51;
const GxrRegister UDP_CMD_SET_APR_OFF = 52;
const GxrRegister UDP_CMD_SET_APR_BODY_CHILD = 53;
const GxrRegister UDP_CMD_SET_APR_BODY_SMALL = 54;
const GxrRegister UDP_CMD_SET_APR_BODY_MEDIUM = 55;
const GxrRegister UDP_CMD_SET_APR_BODY_LARGE = 56;
const GxrRegister UDP_CMD_SET_APR_CM_SIZE_UP = 57;
const GxrRegister UDP_CMD_SET_APR_CM_SIZE_DOWN = 58;
const GxrRegister UDP_CMD_SET_XRAY_ENABLE = 60;
const GxrRegister UDP_CMD_SET_SAVE = 90;
const GxrRegister UDP_CMD_SET_RESET = 91;
const GxrRegister UDP_CMD_SET_REBOOT = 92;
const GxrRegister UDP_CMD_SET_SOFTWARE_INTERLOCK = 93;
const GxrRegister UDP_CMD_SET_SOFTWARE_EXP_OFF = 94;
const GxrRegister UDP_CMD_SET_SOFTWARE_EXP = 95;
const GxrRegister UDP_CMD_SET_FOCAL_SPOT = 96;
const GxrRegister UDP_CMD_SET_SYSTEM_GUI = 100;
const GxrRegister UDP_CMD_SET_TIME_MAS_SELECT = 110;
const GxrRegister UDP_CMD_SET_2POINT_KV_DOWN = 111;
const GxrRegister UDP_CMD_SET_2POINT_KV_UP = 112;
const GxrRegister UDP_CMD_SET_2POINT_MAS_DOWN = 113;
const GxrRegister UDP_CMD_SET_2POINT_MAS_UP = 114;
const GxrRegister UDP_CMD_SET_2POINT_RKVMAS = 115;
const GxrRegister UDP_CMD_SET_RECEPTOR_NORMAL = 121;
const GxrRegister UDP_CMD_SET_RECEPTOR_DR = 122;
const GxrRegister UDP_CMD_TC_PARA = 140;
const GxrRegister UDP_CMD_TC_DATA = 141;
const GxrRegister UDP_CMD_TC_REQ = 142;
const GxrRegister UDP_CMD_GET_RKVMATIME = 151;
const GxrRegister UDP_CMD_GET_BUCKY = 152;
const GxrRegister UDP_CMD_GET_AEC = 153;
const GxrRegister UDP_CMD_GET_APR = 154;
const GxrRegister UDP_CMD_GET_HU_PERCENT = 155;
const GxrRegister UDP_CMD_GET_EXP_CNT = 156;
const GxrRegister UDP_CMD_GET_FOCAL_SPOT = 157;
const GxrRegister UDP_CMD_GET_XRAY_EXP_CTRL = 158;
const GxrRegister UDP_CMD_GET_SYSTEM_GUI = 160;
const GxrRegister UDP_CMD_GET_TIME_MAS_SELECT = 170;
const GxrRegister UDP_CMD_GET_ERROR = 194;
const GxrRegister UDP_CMD_UPDATE_RKVMATIME = 201;
const GxrRegister UDP_CMD_UPDATE_BUCKY = 202;
const GxrRegister UDP_CMD_UPDATE_AEC = 203;
const GxrRegister UDP_CMD_UPDATE_APR = 204;
const GxrRegister UDP_CMD_UPDATE_STATE = 205;
const GxrRegister UDP_CMD_UPDATE_CHARGING_STATE = 210;
const GxrRegister UDP_CMD_UPDATE_GXR_MODEL = 211;
const GxrRegister UDP_CMD_UPDATE_SLIDING_DOWN = 212;
const GxrRegister UDP_CMD_UPDATE_INV_SLEEP = 220;
const GxrRegister UDP_CMD_UPDATE_TIME_MAS_SELECT = 230;
const GxrRegister UDP_CMD_UPDATE_RECEPTOR_TYPE = 231;
const GxrRegister UDP_CMD_UPDATE_PROGRAM_MODE = 236;
const GxrRegister UDP_CMD_UPDATE_EXP_STATE = 240;
const GxrRegister UDP_CMD_UPDATE_HU_PERCENT = 241;
const GxrRegister UDP_CMD_UPDATE_EXP_CNT = 242;
const GxrRegister UDP_CMD_UPDATE_FOCAL_SPOT = 243;
const GxrRegister UDP_CMD_UPDATE_XRAY_EXP_CTRL = 244;
const GxrRegister UDP_CMD_UPDATE_FB_RKVMATIME = 245;
const GxrRegister UDP_CMD_UPDATE_XRAY_ENABLE = 246;
const GxrRegister UDP_CMD_UPDATE_SYSTEM_GUI = 250;
const GxrRegister UDP_CMD_UPDATE_WARNING = 253;
const GxrRegister UDP_CMD_UPDATE_ERROR = 254;
const GxrRegister UDP_CMD_ECHO = 255;

#endif // GXR_H
