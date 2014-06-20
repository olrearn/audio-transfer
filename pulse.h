#ifdef PULSE

#ifndef PULSE_H
#define PULSE_H

#include <pulse/simple.h>
#include <pulse/channelmap.h>

#include <QObject>
#include <QIODevice>
#include <QtMultimedia/QAudioFormat>
#include <QString>
#include <QTimer>

class PulseDevice : public QIODevice {
    Q_OBJECT
public:
    PulseDevice(const QString name, const QString target, QAudioFormat *format, QObject *parent);
    ~PulseDevice();
    bool open(OpenMode mode);
    void close();
private:
    qint64 writeData(const char *data, qint64 len);
    qint64 readData(char *data, qint64 maxlen);
    QObject *parent;
    pa_simple *s;
    pa_simple *rec;
    pa_sample_spec ss;
    pa_sample_format getSampleSize();
    bool makeChannelMap(pa_channel_map *map);
    QAudioFormat *format;
    void say(const QString message);
    QString target;
    QString name;
    bool debugMode;
    QTimer* timer;
    int latencyRec;
public slots:
    void deleteLater();
private slots:
    void testSlot();
};

#endif // PULSE_H

#endif