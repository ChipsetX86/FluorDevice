#include "DeviceConfiguration.h"

#include <QFile>
#include <QSettings>
#include <QDataStream>

#include "DeviceLogging.h"

static const int qmtDeviceConfiguration = qRegisterMetaType<DeviceConfiguration>();
static const int qmtDeviceConfigurationItem = qRegisterMetaType<DeviceConfiguration::Item>();

struct DeviceConfiguration::PImpl
{
    QStringList prefix;
    Items items;

    QString keyWithPrefix(const QString &key) const
    {
        QStringList prefixParts = prefix;
        prefixParts.append(key);
        return prefixParts.join(QStringLiteral("/"));
    }

    QByteArray serializeItems()
    {
        QByteArray result;
        QDataStream stream(&result, QIODevice::WriteOnly);

        for (auto it = items.cbegin(); it != items.cend(); ++it) {
            QVariant value = it.value().value;

            switch (value.type()) {
            case QVariant::LongLong:
            case QVariant::ULongLong:
            case QVariant::Int:
            case QVariant::UInt:
            case QVariant::Bool:
            case QVariant::Double:
            case QVariant::KeySequence:
                value = value.toString();
                break;
            default:
                break;
            }

            stream << qMakePair<QString, QVariant>(it.key(), value);
        }

        return result;
    }
};

DeviceConfiguration::DeviceConfiguration() :
    m_pimpl(new PImpl)
{

}

DeviceConfiguration::DeviceConfiguration(const QString &filename) :
    m_pimpl(new PImpl)
{
    if (QFile::exists(filename)) {
        infoDevice << "Loading configuration from file:" << filename;
        QSettings s(filename, QSettings::IniFormat);

        const auto keys = s.allKeys();
        infoDevice << "Keys found:" << keys.size();

        for (auto &key : keys) {
            Item newItem;
            newItem.value = s.value(key);
            m_pimpl->items.insert(key, newItem);
        }
    } else {
        warnDevice << "Configuration file not found:" << filename;
    }
}

DeviceConfiguration::DeviceConfiguration(const DeviceConfiguration &other) :
    m_pimpl(new PImpl)
{
    m_pimpl->items = other.m_pimpl->items;
}

DeviceConfiguration::~DeviceConfiguration()
{

}

bool DeviceConfiguration::save(const QString &filename) const
{
    QSettings s(filename, QSettings::IniFormat);

    s.clear();

    for (auto it = m_pimpl->items.cbegin(); it != m_pimpl->items.cend(); ++it) {
        s.setValue(it.key(), it.value().value);
    }

    s.sync();
    return s.status() == QSettings::Status::NoError;
}

DeviceConfiguration &DeviceConfiguration::operator=(const DeviceConfiguration &other)
{
    m_pimpl->items = other.m_pimpl->items;
    return *this;
}

bool DeviceConfiguration::isEmpty() const
{
    return m_pimpl->items.isEmpty();
}

QStringList DeviceConfiguration::allKeys() const
{
    return m_pimpl->items.keys();
}

void DeviceConfiguration::beginGroup(const QString &prefix)
{
    m_pimpl->prefix.append(prefix);
}

void DeviceConfiguration::endGroup()
{
    m_pimpl->prefix.removeLast();
}

void DeviceConfiguration::insert(const QString &key, const DeviceConfiguration::Item &item)
{
    m_pimpl->items.insert(m_pimpl->keyWithPrefix(key), item);
}

void DeviceConfiguration::insert(const QString &key, const QVariant &value, const QString &description)
{
    insert(key, Item(value, description));
}

DeviceConfiguration::Item DeviceConfiguration::item(const QString &key) const
{
    return m_pimpl->items.value(m_pimpl->keyWithPrefix(key));
}

QVariant DeviceConfiguration::value(const QString &key) const
{
    if (auto i = item(key)) {
        return i.value;
    }

    warnDevice << "Key not found:" << key;
    return QVariant();
}

void DeviceConfiguration::setValue(const QString &key, const QVariant &value)
{
    auto fullKey = m_pimpl->keyWithPrefix(key);

    Item item;
    if (m_pimpl->items.contains(fullKey)) {
        item = m_pimpl->items.value(fullKey);
    }

    item.value = value;

    m_pimpl->items.insert(fullKey, item);
}

QString DeviceConfiguration::description(const QString &key) const
{
    return item(key).description;
}

void DeviceConfiguration::replaceValues(const DeviceConfiguration &other, bool preserveSrcTypes)
{
    for (auto it = m_pimpl->items.begin(); it != m_pimpl->items.end(); ++it) {
        Item &item = *it;

        if (other.m_pimpl->items.contains(it.key())) {
            QVariant newValue = other.m_pimpl->items.value(it.key()).value;

            if (preserveSrcTypes) {
                newValue.convert(item.value.type());
            }

            item.value = newValue;
        }
    }
}

DeviceConfiguration::Item::Item()
{

}

DeviceConfiguration::Item::Item(const QVariant &value, const QString &description) :
    value(value),
    description(description)
{

}

bool DeviceConfiguration::Item::isNull() const
{
    return value.isNull() && description.isNull();
}

DeviceConfiguration::Item::operator bool() const
{
    return !isNull();
}

bool operator==(const DeviceConfiguration &lhs, const DeviceConfiguration &rhs)
{
    return lhs.m_pimpl->serializeItems() == rhs.m_pimpl->serializeItems();
}

DeviceConfiguration &DeviceConfiguration::operator+=(const DeviceConfiguration &other)
{
    for (auto it = other.m_pimpl->items.cbegin(); it != other.m_pimpl->items.cend(); ++it) {
        m_pimpl->items.insert(it.key(), it.value());
    }

    return *this;
}
