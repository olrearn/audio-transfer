#ifdef PORTAUDIO

#include "portaudiodevice.h"
#include <portaudio.h>

#include <QDebug>

/*note:
 *  j'ignore moi même pourquoi j'ai codé ce module puisqu'il ne semble pas possible de faire du loopback avec portaudio, sans doute un de mes délires
 * de plus le module n'est pour l'heure pas fonctionel: le flux se créé bien aussi bien en lecture qu'en enregistrement, les périferiques se lisent tousa tousa c'est super
 * mais pas moyen de faire transiter des données par les flux, j'imagine qu'il faudra utiliser les callback mais... je n'ai aucune idée de ce que je fais...
 * futur moi: pardone moi pour mon ignorance.
*/

/* utilisation:
 * QIODevice *output = new PortAudioDevice(format,this);
 * output->open(QIODevice::WriteOnly);
 * output->write(YourSoundBuffer);
 * ...
 * output->close();
 * output->deleteLater();
 * */

PortAudioDevice::PortAudioDevice(AudioFormat *format, QObject *parent) :
    QIODevice(parent)
{
    say("init");
    say("api version: " + QString(Pa_GetVersionText()));
    binit = false;
    stream = NULL;
    this->format = format;
    this->currentDeviceIdInput = 0;
    this->currentDeviceIdOutput = 0;
    this->timer = new QTimer(this);
    timer->setInterval(100);
    timer->setObjectName("PortAudio module timer");
    connect(timer,SIGNAL(timeout()),this,SIGNAL(readyRead()));
    say("devices count: " + QString::number(getDevicesCount()));
    say("init done");
}
PortAudioDevice::~PortAudioDevice() {
    Pa_Terminate();
    say("deleted");
}
void PortAudioDevice::close() {
    say("stoping");
    timer->stop();
    if (stream) {
        Pa_StopStream(stream);
        Pa_CloseStream(stream);
    }
    stream = NULL;
    QIODevice::close();
    say("stoped");
}

bool PortAudioDevice::open(OpenMode mode) {
    say("opening device");
    if (!initPa()) return false;
    say("creating stream");
    PaError err;

    PaStreamParameters  *outputParameters = NULL,
                        *inputParameters = NULL;
    if ((mode == QIODevice::ReadOnly) || (mode == QIODevice::ReadWrite)) {
        say("configuring input");
        inputParameters = new PaStreamParameters();
        bzero(inputParameters, sizeof(inputParameters)); //not necessary if you are filling in all the fields
        inputParameters->channelCount = format->getChannelsCount();
        inputParameters->sampleFormat = getSampleFormat(format->getSampleSize());
        inputParameters->device = currentDeviceIdInput;
        inputParameters->suggestedLatency = getDeviceInfo(currentDeviceIdInput)->defaultLowInputLatency;
        inputParameters->hostApiSpecificStreamInfo = NULL;
        say("request " + QString::number(format->getChannelsCount()) + " channel(s) for input");
        say("done");
    }
    if ((mode == QIODevice::WriteOnly) || (mode == QIODevice::ReadWrite)) {
        say("configuring output");
        outputParameters = new PaStreamParameters();
        bzero(outputParameters, sizeof(outputParameters)); //not necessary if you are filling in all the fields
        outputParameters->channelCount = format->getChannelsCount();
        outputParameters->sampleFormat = getSampleFormat(format->getSampleSize());
        outputParameters->device = currentDeviceIdOutput;
        outputParameters->suggestedLatency = getDeviceInfo(currentDeviceIdOutput)->defaultLowOutputLatency;
        outputParameters->hostApiSpecificStreamInfo = NULL;
        say("request " + QString::number(format->getChannelsCount()) + " channel(s) for output");
    }

    //stream is a PaStream* (stored in the object)
    err =  Pa_OpenStream(
                   &stream,
                   inputParameters,
                   outputParameters,
                   format->getSampleRate(),
                   paFramesPerBufferUnspecified,
                   paNoFlag, //flags that can be used to define dither, clip settings and more
                   PortAudioDevice::PaStreamCallback, //your callback function
                   (void *)this ); //data to be passed to callback. In C++, it is frequently (void *)this
    if (err != paNoError) {
        say("open error: " + QString::number(err) + " -> " + Pa_GetErrorText(err));
        say("input max channels: " + QString::number(getDeviceInfo(currentDeviceIdInput)->maxInputChannels));
        say("output max channels: " + QString::number(getDeviceInfo(currentDeviceIdOutput)->maxOutputChannels));
        return false;
    }
    say("starting stream");
    err = Pa_StartStream(stream);
    if (err != paNoError) {
        say("cannot start stream: " + QString::number(err) + " -> " + Pa_GetErrorText(err));
        if (!stream) say("empty stream pointer");
        qDebug() << "stream pointers:" << stream << &stream;

        return false;
    }
    say("stream started");

    QIODevice::open(mode);
    say("open ok");
    timer->start();
    return true;
}
void PortAudioDevice::say(const QString message) {
#ifdef DEBUG
    qDebug() << "PortAudio Device: " + message;
#endif
    emit(debug("PortAudio: " + message));
}
qint64 PortAudioDevice::readData(char *data, qint64 maxlen) {
    //ici la méthode pour enregistrer du son (record)
    if (!stream) return -1;

    qint64 maxread = Pa_GetStreamReadAvailable(stream);
    //say("read data : " + QString::number((maxread)));

    if (maxread > maxlen) maxread = maxlen;
    else if (!maxread) return 0;
    PaError err = Pa_ReadStream(stream,data,(unsigned long) maxread);
    if (err != paNoError) return -1;

    return maxlen;
}
qint64 PortAudioDevice::writeData(const char *data, qint64 len) {
    //ici la méthode pour "jouer" du son (output)
    //say("write data");
    if (!stream) return -1;
    qint64 max = Pa_GetStreamWriteAvailable(stream);
    if (max > len) max = len;
    PaError err = Pa_WriteStream(stream,data,max);
    if (err != paNoError) return -1;

    return max;
}
int PortAudioDevice::PaStreamCallback(const void *input, void *output, unsigned long frameCount, const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags, void *userData) {
    PortAudioDevice *obj = (PortAudioDevice*)userData;
    qDebug() << "callback call: " << input << output << frameCount << statusFlags << userData << timeInfo << obj;

    //super la fonction de callback de lecture fonctione mais... je ne sais absolument quoi en faire :)
    //futur moi: arrete d'écrire des commentaires inutiles dans le code: personne ne les lis !
    return 0;
}
int PortAudioDevice::getDevicesCount() {
    if (!initPa()) return -1;
    return (int) Pa_GetDeviceCount();
}
QList<PaDeviceInfo *> PortAudioDevice::getDevicesInfo() {
    QList<PaDeviceInfo*> devices;
    if (!initPa()) return devices;
    const int m = getDevicesCount();
    for(int i = 0 ; i < m; i++ ) {
       devices.append((PaDeviceInfo*) Pa_GetDeviceInfo(i));
    }
    return devices;
}
QStringList PortAudioDevice::getDevicesNames() {
    QStringList devicesNames;
    QList<PaDeviceInfo*> devices = getDevicesInfo();
    QList<PaDeviceInfo*>::iterator i;
    for (i = devices.begin() ; i != devices.end() ; i++) {
        PaDeviceInfo* device = *i;
        devicesNames << device->name;
    }
    return devicesNames;
}
void PortAudioDevice::say(QStringList *list) {
    QStringList::iterator i;
    for (i = list->begin();i != list->end();i++) say("- " + *i);
}
bool PortAudioDevice::initPa() {
    //this method init the portaudio api, it MUST be called before any other command
    if (binit) return true;
    PaError err = Pa_Initialize();
    if (err != paNoError) {
        say("error: " + QString(Pa_GetErrorText(err)));
        return false;
    }
    binit = true;
    return true;
}
const PaDeviceInfo *PortAudioDevice::getDeviceInfo(const int deviceId) {
    if (deviceId > getDevicesCount()) return (PaDeviceInfo *) NULL;
    if (!initPa()) return (PaDeviceInfo*)NULL;
    return Pa_GetDeviceInfo(deviceId);
}
bool PortAudioDevice::setDeviceId(const int deviceId,QIODevice::OpenModeFlag mode) {
    if (deviceId > getDevicesCount()) return false;
    else if (deviceId < 0) return false;
    if (mode == QIODevice::ReadOnly) currentDeviceIdInput = deviceId;
    else if (mode == QIODevice::WriteOnly) currentDeviceIdOutput = deviceId;
    else {
        currentDeviceIdInput = deviceId;
        currentDeviceIdOutput = deviceId;
    }
    say("switching to device: " + QString(getDeviceInfo(deviceId)->name));
    return true;
}
PaSampleFormat PortAudioDevice::getSampleFormat(const int sampleSize) {
    switch (sampleSize) {
        case 8:
            return paInt8;
        case 16:
            return paInt16;
        case 24:
            return paInt24;
        case 32:
            return paInt32;
    }
    return paInt16;
}
int PortAudioDevice::getDeviceIdByName(const QString name) {
    QStringList devices = getDevicesNames();
    return devices.indexOf(name);
}

#endif
