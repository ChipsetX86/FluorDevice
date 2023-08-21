#ifndef POWER_SUPPLY_H
#define POWER_SUPPLY_H

#include "Device.h"

class Hardware;

class DEVICELIB_EXPORT PowerSupply : public Device
{
    Q_OBJECT
    Q_DISABLE_COPY(PowerSupply)
public:
    struct DEVICELIB_EXPORT Params
    {
        Params();
        quint16 exposureMs;
        float amperageMA;
        float voltageKV;
    };

    struct DEVICELIB_EXPORT Results
    {
        Results();
        quint16 exposureMs;
        float amperageMA;
        float voltageKV;
    };

    explicit PowerSupply(QObject *parent = nullptr);
    ~PowerSupply();

    Results results() const;
public slots:
    bool prepare(const PowerSupply::Params &params);
    /**
     * Returns true if on-off is success
     * otherwise returns false
     **/
    bool launch();
    bool getResults();
signals:
    void prepared(QPrivateSignal);
    void toggled(bool state, QPrivateSignal);
protected:
    const Params &currentParams() const;
    bool waitForButtonsReleased();
    
    virtual bool doPrepare() = 0;
    virtual bool doOn() = 0;
    /**
      * Returns true if error occurred while xray is on
      * otherwise returns false. Default implementation wait
      * for exposure time and returns false
     **/
    virtual bool doWaitForError();
    virtual bool doOff() = 0;
    virtual bool doGetResults(Results &results) = 0;
private:
    void onClosed() override final;
    void buttonsReleased();

    friend class Hardware;

    struct PImpl;
    QScopedPointer<PImpl> m_pimpl;
};

#endif // POWER_SUPPLY_H
