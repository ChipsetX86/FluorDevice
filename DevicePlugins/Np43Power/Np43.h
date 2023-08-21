#ifndef NP43_H
#define NP43_H

#include <QtCore>

const int WIDTH_TABLE_ERROR = 6;
const int HEIGHT_TABLE_ERROR = 8;

const quint8 TABLE_ERROR_P1_DT[HEIGHT_TABLE_ERROR][WIDTH_TABLE_ERROR] = {
    {1, 1, 1, 1, 1, 0},
    {1, 1, 1, 1, 1, 0},
    {0, 0, 0, 0, 0, 0},
    {1, 1, 1, 1, 1, 0},
    {0, 0, 0, 0, 0, 0},
    {1, 1, 1, 1, 1, 0},
    {0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0}
};

const quint8 TABLE_ERROR_P2_DT1[HEIGHT_TABLE_ERROR][WIDTH_TABLE_ERROR] = {
    {0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0},
    {1, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0},
    {1, 1, 1, 0, 0, 0},
};

const quint8 TABLE_ERROR_P2_DT2[HEIGHT_TABLE_ERROR][WIDTH_TABLE_ERROR] = {
    {1, 1, 0, 0, 0, 0},
    {1, 1, 1, 1, 0, 0},
    {0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0},
    {1, 0, 0, 0, 0, 0},
    {1, 1, 1, 0, 0, 0},
    {1, 1, 1, 1, 0, 0},
    {1, 1, 1, 0, 0, 0},
};

const QString ERROR_P1_DT[HEIGHT_TABLE_ERROR] = {
    "SN_EX",
    "X_RAY",
    "0",
    "Z_KV",
    "T_WD",
    "T_SEC",
    "P_AW",
    "T_MAS"
};

const QString ERROR_P2_DT1[HEIGHT_TABLE_ERROR] = {
    "+KV_MAX",
    "-KV_MIN",
    "+KV_MIN",
    "+15V_EX",
    "KZ_BRG",
    "-15V_EX",
    "-KV_MAX",
    "RVZ_EX"
};

const QString ERROR_P2_DT2[HEIGHT_TABLE_ERROR] = {
    "IFL_EX",
    "KN_RYM",
    "T+AW",
    "IA_MAX",
    "KNT_LS",
    "KNT_PS",
    "SNM_ST",
    "PRE_ST",
};

#endif // NP43_H
