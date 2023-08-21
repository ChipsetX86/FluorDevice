#ifndef MICDETECTOR_COMMON_H
#define MICDETECTOR_COMMON_H

#include <QString>
#include <QVector>

typedef QVector<ushort> MicFrame;

const QString NAME_FIELD_IS_SUCCESS = QStringLiteral("flagIsSuccess");
const QString NAME_FIELD_TEXT_ERROR = QStringLiteral("textError");
const QString NAME_FIELD_WIDTH_DETECTOR = QStringLiteral("widthDetectorPx");
const QString NAME_FIELD_FRAME = QStringLiteral("frame");

#endif // MICDETECTOR_COMMON_H
