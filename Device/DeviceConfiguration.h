#ifndef DEVICECONFIGURATION_H
#define DEVICECONFIGURATION_H

#include "DeviceGlobal.h"

#include <QVariant>
#include <QStringList>
#include <QMap>
#include <QScopedPointer>

class DEVICELIB_EXPORT DeviceConfiguration final
{
public:
    struct DEVICELIB_EXPORT Item
    {
        Item();
        Item(const QVariant &value, const QString &description);
        QVariant value;
        QString description;
        bool isNull() const;
        operator bool() const;
    };

    typedef QMap<QString, Item> Items;

    DeviceConfiguration();
    explicit DeviceConfiguration(const QString &filename);
    DeviceConfiguration(const DeviceConfiguration &other);
    ~DeviceConfiguration();

    bool save(const QString &filename) const;

    DeviceConfiguration &operator=(const DeviceConfiguration &other);
    DeviceConfiguration &operator+=(const DeviceConfiguration &other);
    friend bool operator==(const DeviceConfiguration &lhs, const DeviceConfiguration &rhs);

    bool isEmpty() const;

    QStringList allKeys() const;
    void beginGroup(const QString &prefix);
    void endGroup();

    Item item(const QString &key) const;
    void insert(const QString &key, const Item &item);
    void insert(const QString &key, const QVariant &value, const QString &description = QString());
    QVariant value(const QString &key) const;
    void setValue(const QString &key, const QVariant &value);
    QString description(const QString &key) const;

    void replaceValues(const DeviceConfiguration &other, bool preserveSrcTypes = true);
private:
    struct PImpl;
    QScopedPointer<PImpl> m_pimpl;
};

typedef QMap<QString, DeviceConfiguration> DeviceConfigurationMap;

Q_DECLARE_METATYPE(DeviceConfiguration)
Q_DECLARE_METATYPE(DeviceConfiguration::Item)

#endif // DEVICECONFIGURATION_H
