#ifndef SCANNINGMODESCOLLECTION_H
#define SCANNINGMODESCOLLECTION_H

#include "DeviceGlobal.h"

#include <QVector>
#include <QMap>

#include "DeviceConfiguration.h"

class DEVICELIB_EXPORT ScanningModesCollection final
{
    Q_DISABLE_COPY(ScanningModesCollection)
public:
    struct DEVICELIB_EXPORT Item final
    {
    public:
        Item();
        QString uuid() const { return m_uuid; }
        QString title;
        bool isEnabled;
        quint16 sortOrder;
        qreal calibrationAmperageMa;
        qreal calibrationVoltageKv;
        qreal minAmperageMa;
        qreal maxAmperageMa;
        qreal minVoltageKv;
        qreal maxVoltageKv;
        bool binningSum;
        quint16 binningHorizontal;
        quint16 binningVertical;
        quint16 snapshotWidth;
        quint16 rollbackAlpha;
        qreal rollbackBeta;
        DeviceConfigurationMap devicesConfigurations;
        bool isNull() const;
        /**
         * Copy all fields except uuid
         */
        void copy(const Item &other);
        bool operator==(const Item &other) const;
    private:
        QString m_uuid;

        friend class ScanningModesCollection;
    };

    typedef QVector<Item> ItemsList;
    typedef QMap<QString, Item> ItemsMap;

    static ScanningModesCollection &instance();

    /**
     * Returned UUID may exists in previous Item class instances 
     * returned by this function. Check them manual for absolute 
     * unique UUID
     **/
    Item create();
    ItemsList list(bool onlyEnabled = true, bool sorted = true) const;
    const ItemsMap &items() const;
    bool store(const ItemsList &list);
private:
    explicit ScanningModesCollection(const QString &path);
    void load();

    QString m_path;
    ItemsMap m_items;
};

Q_DECLARE_METATYPE(ScanningModesCollection::Item)

#endif // SCANNINGMODESCOLLECTION_H
