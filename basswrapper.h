#ifndef BASSWRAPPER_H
#define BASSWRAPPER_H

#include "bass.h"
#include <QtWidgets>
#include <QProcess>

class BassErrorMessages
{
    BassErrorMessages();
    QMap<int,QString> errors;
    static BassErrorMessages* instance;
public:
    static QString string(const int& number);
};

struct DeviceInfo
{
    QString name;
    QString device;
    uint flags;
    bool active;

    DeviceInfo(const BASS_DEVICEINFO& info);
    DeviceInfo(const QString& _name, const QString& _device, const bool& _active);
};

class BassWrapper : public QObject
{
    Q_OBJECT
public:
    static const int InitState;
    static const int ErrorState;
    static const int PausedState;
    static const int PlayingState;
    static const int StoppedState;
    bool isNetMedia;
    uint request;

    explicit BassWrapper(QWidget* parent);
    virtual ~BassWrapper();
    bool init(int device = -1);
    bool prepareMedia(const QString& url);
    bool prepareFileStream(const QString& path);
    void doMeta();

    bool play();
    bool pause();
    bool stop();
    int state() const;
    int volume();
    int activeDevice() const;
    QString activeDevice(const QFile& /*file*/) const;
    bool setActiveDevice(int device);
    QList<DeviceInfo> devices();
    QList<DeviceInfo> devices(const QString &cmd);
    bool setVolume(const unsigned int& vol);
    QString deviceFlagsToString(uint flags) const;
    QList<DeviceInfo> getDevicesInfo() const;
signals:
    void metaData(QString data);
    void metaStatus(int status, QString reason);
    void mediaPos(QString pos, int pospercent);
    void mediaLen(QString len);
    void mediaEnd();
private slots:
    void elapsedTime();
private:
    QTimer timer;
    uint currentState;
    HSTREAM currentStream;
    quint64 mediaLength;
    bool filePositionOk;
    QWidget* parentWindow;
    QList<DeviceInfo> devicesInfo;

    void doStatus(const int& status, const QString& reason=QString());
    QList<QByteArray> doCommand(const QString& cmd,
        const QStringList& params, QList<QByteArray>& rows);
    QString formatNumber(quint64 number);

};

#endif // BASSWRAPPER_H
