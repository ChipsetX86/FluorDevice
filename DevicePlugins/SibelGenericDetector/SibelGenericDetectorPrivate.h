#ifndef SIBELGENERICDETECTORPRIVATE_H
#define SIBELGENERICDETECTORPRIVATE_H

#include <QVector>
#include <QSizeF>
#include <QScopedArrayPointer>
#include <QObject>

#include <ftdi/ftd2xx.h>

class SibelGenericDetectorPrivate: public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(SibelGenericDetectorPrivate)
public:
    struct Configuration
    {
        quint32 rdw;
        quint32 vdw;
        quint32 od;
        quint32 ttl;
        quint32 c0;
        quint32 c1;
        bool gain;
        bool binning;
    };

    static const int defaultLatency;
    static const int defaultBufferSize;

    virtual ~SibelGenericDetectorPrivate();

    bool open();
    bool isOpen() const;
    void close();
    bool testConnection();

    bool prepare(int linesCount);
    bool takeSnapshot();
    virtual QVector<float> decodeSnapshot() const;

    virtual int width() const { return m_width; }
    virtual int batches() const { return m_batches; }
    virtual int depth() const { return m_depth; }
    virtual QSizeF pixelSize() const { return m_pixelSize; }
    virtual qreal chargeTime() const = 0;

    QString lastError() const { return m_lastError; }

    bool setConfiguration(const Configuration &conf);
    bool setLatency(int value);
    bool setBufferSize(int value);
protected:
    explicit SibelGenericDetectorPrivate(int width, int batches, int depth, int bitsForLinesCount,
                                         const QSizeF &pixelSize, QObject *parent = nullptr);

    const Configuration &configuration() const;
    qreal frequency() const;
    int linesCount() const { return m_linesCount; }
    const uchar *snapshot() const { return m_snapshot.data(); }
private:
    void setLastError(const QString &error) { m_lastError = error; }
    bool writeBytes(uchar *buffer, uint size);
    bool setLinesCount(int lines);
    bool startFrame();
    bool serialNumberDetector(QString&);

    int m_width;
    int m_batches;
    int m_depth;
    int m_bitsForLinesCount;
    QSizeF m_pixelSize;

    FT_HANDLE m_handle;
    Configuration m_conf;
    int m_latency;
    int m_bufferSize;
    float m_frequency;
    int m_linesCount;
    QScopedArrayPointer<uchar> m_snapshot;
    uint m_snapshotSize;
    QString m_lastError;
};

class SibelDetectorPR2048 : public SibelGenericDetectorPrivate
{
    Q_DISABLE_COPY(SibelDetectorPR2048)
public:
    explicit SibelDetectorPR2048(QObject *parent = nullptr);
    qreal chargeTime() const override;
};

class SibelDetectorPR4096 : public SibelGenericDetectorPrivate
{
    Q_DISABLE_COPY(SibelDetectorPR4096)
public:
    explicit SibelDetectorPR4096(QObject *parent = nullptr);
    int width() const override;
    QSizeF pixelSize() const override;
    qreal chargeTime() const override;
private:
    bool isBinningEnabled() const;
};

class SibelDetectorPR4608 : public SibelGenericDetectorPrivate
{
    Q_DISABLE_COPY(SibelDetectorPR4608)
public:
    explicit SibelDetectorPR4608(QObject *parent = nullptr);
    qreal chargeTime() const override;
};

#endif // SIBELGENERICDETECTORPRIVATE_H
