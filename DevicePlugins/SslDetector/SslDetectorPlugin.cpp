#include "SslDetectorPlugin.h"

#include "SslDetector.h"

SslDetectorPlugin::SslDetectorPlugin(QObject *parent) : QObject(parent)
{

}

SslDetectorPlugin::~SslDetectorPlugin()
{

}

Device *SslDetectorPlugin::create()
{
    return new SslDetector;
}

QString SslDetectorPlugin::description() const
{
    return tr("ТЛД с подключением к Ethernet");
}

QString SslDetectorPlugin::notes() const
{
    return tr("У сетевой карты, подключенной к детектору, необходимо установить IP адрес "
              "10.10.5.16 маска 255.255.255.0. (IP адрес детектора 10.10.5.15). "
              "Если снимок содержит диагональные полосы возможно нужно использовать "
              "драйвер без проверки контрольных сумм");
}
