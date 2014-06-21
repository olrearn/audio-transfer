#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "manager.h"
#include "readini.h"

#include <QtGui>
#include <QString>
#include <QtMultimedia/QAudioOutput>
#include <QFileDialog>
#include <QTimer>
#include <QDesktopWidget>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    moveToCenter();
    modeSource = Manager::Device;
    modeDest = Manager::Tcp;

    manager = new Manager(this);

    timer = new QTimer(this);
    timer->setInterval(1000);
    connect(manager,SIGNAL(stoped()),this,SLOT(recStoped()));
    on_refreshSources_clicked();
    ui->destinationDeviceCombo->addItems(Manager::getDevicesNames(QAudio::AudioOutput));

    connect(ui->sourceRadioDevice,SIGNAL(clicked()),this,SLOT(refreshEnabledSources()));
    connect(ui->sourceRadioFile,SIGNAL(clicked()),this,SLOT(refreshEnabledSources()));
    connect(ui->sourceRadioZeroDevice,SIGNAL(clicked()),this,SLOT(refreshEnabledSources()));
    connect(ui->sourceRadioPulseAudio,SIGNAL(clicked()),this,SLOT(refreshEnabledSources()));
    connect(ui->destinationDeviceRadio,SIGNAL(clicked()),this,SLOT(refreshEnabledDestinations()));
    connect(ui->destinationRadioFile,SIGNAL(clicked()),this,SLOT(refreshEnabledDestinations()));
    connect(ui->destinationRadioTcp,SIGNAL(clicked()),this,SLOT(refreshEnabledDestinations()));
    connect(ui->destinationRadioPulseAudio,SIGNAL(clicked()),SLOT(refreshEnabledDestinations()));
    connect(ui->destinationRadioZeroDevice,SIGNAL(clicked()),this,SLOT(refreshEnabledDestinations()));
    connect(ui->checkboxSourceOutput,SIGNAL(clicked()),this,SLOT(on_refreshSources_clicked()));

    connect(manager,SIGNAL(errors(QString)),this,SLOT(errors(QString)));
    connect(manager,SIGNAL(started()),this,SLOT(started()));
    connect(manager,SIGNAL(debug(QString)),this,SLOT(debug(QString)));
    connect(ui->samplesRates,SIGNAL(currentIndexChanged(int)),this,SLOT(refreshEstimatedBitrate()));
    connect(ui->samplesSize,SIGNAL(currentIndexChanged(int)),this,SLOT(refreshEstimatedBitrate()));
    connect(ui->channelsCount,SIGNAL(valueChanged(int)),this,SLOT(refreshEstimatedBitrate()));
    connect(timer,SIGNAL(timeout()),this,SLOT(refreshReadedData()));
#ifndef PULSE
    ui->destinationRadioPulseAudio->setEnabled(false);
    ui->destinationRadioPulseAudio->deleteLater();
    ui->destinationPulseAudioLineEdit->deleteLater();
    ui->sourceRadioPulseAudio->setEnabled(false);
    ui->sourceRadioPulseAudio->deleteLater();
#endif
    debug("configuration path: " + getConfigFilePath());

    this->ini = new Readini(getConfigFilePath(),this);
    if (ini->exists()) {
        configLoad(ini);
        if (ini->getValue("general","auto-start") == "1") on_pushButton_clicked();
    }
}

MainWindow::~MainWindow()
{
    delete manager;
    delete ini;
    delete ui;
}
void MainWindow::errors(const QString error) {
    ui->statusBar->showMessage(error,3000);
    this->debug(error);
}
void MainWindow::debug(const QString message) {
    ui->debug->addItem(message);
}

void MainWindow::on_refreshSources_clicked()
{
    ui->sourcesList->clear();
    ui->sourcesList->addItems(Manager::getDevicesNames(QAudio::AudioInput));
    if (ui->checkboxSourceOutput->isChecked()) {
        ui->sourcesList->addItems(Manager::getDevicesNames(QAudio::AudioOutput));
    }
}


void MainWindow::on_pushButton_clicked()
{
    //record button.
    if (!manager->isRecording()) {
        ui->debug->clear();
        const int bitrate = (ui->samplesRates->currentText().toInt() * ui->samplesSize->currentText().toInt() / 8) * ui->channelsCount->value();
        debug("estimated bitrate: " + wsize(bitrate));
        Manager::userConfig mc;
        mc.modeInput = Manager::None;
        mc.modeOutput = Manager::None;

        mc.format = new AudioFormat();
        mc.format->setCodec(ui->codecList->currentText());
        mc.format->setSampleRate(ui->samplesRates->currentText().toInt());
        mc.format->setSampleSize(ui->samplesSize->currentText().toInt());
        mc.format->setChannelCount(ui->channelsCount->value());

        mc.filePathOutput = ui->destinationFilePath->text();
        mc.filePathInput = ui->sourceFilePath->text();
        mc.devices.input = ui->sourcesList->currentIndex();
        mc.devices.output = ui->destinationDeviceCombo->currentIndex();
        mc.bufferSize = bitrate * ui->destinationTcpBufferDuration->value() / 1000 / 100;
        mc.bufferMaxSize = 2097152; //2Mb
        debug("buffer size: " + wsize(mc.bufferSize));

        //SOURCES
        if (ui->sourceRadioFile->isChecked()) mc.modeInput = Manager::File;
        else if (ui->sourceRadioDevice->isChecked()) mc.modeInput = Manager::Device;
        else if (ui->sourceRadioZeroDevice->isChecked()) mc.modeInput = Manager::Zero;
        else if (ui->sourceRadioPulseAudio->isChecked()) mc.modeInput = Manager::PulseAudio;

        //DESTINATIONS
        if (ui->destinationRadioFile->isChecked()) {
            mc.modeOutput = Manager::File;
        }

        else if (ui->destinationDeviceRadio->isChecked()) {
            mc.modeOutput = Manager::Device;
            ui->statusBar->showMessage("redirecting from " + ui->sourcesList->currentText() + " to " + ui->destinationDeviceCombo->currentText());
        }
        else if (ui->destinationRadioTcp->isChecked()) {
            const QString host = ui->destinationTcpSocket->text().split(":").first();
            const int port = ui->destinationTcpSocket->text().split(":").last().toInt();
            if (!isValidIp(host)) {
                ui->statusBar->showMessage("Error: invalid target ip: refusing to connect");
                return;
            }
            if ((port >= 65535) || (port <= 0)) {
                ui->statusBar->showMessage("Error: invalid port: refusing to connect");
                return;
            }
            mc.tcpTarget.host = host;
            mc.tcpTarget.port = port;
            mc.tcpTarget.sendConfig = true;
            mc.modeOutput = Manager::Tcp;

            ui->statusBar->showMessage("Connecting to " + host + " on port " + QString().number(port));
        }
        else if (ui->destinationRadioPulseAudio->isChecked()) {
            debug("using pulseaudio target: this is an experimental feature!");
            if (!ui->destinationPulseAudioLineEdit->text().isEmpty()) {
                mc.pulseTarget = ui->destinationPulseAudioLineEdit->text();
            }
            mc.modeOutput = Manager::PulseAudio;

        }
        else if (ui->destinationRadioZeroDevice->isChecked()) mc.modeOutput = Manager::Zero;

        manager->setUserConfig(mc);
        manager->start();
    }
    else {
        manager->stop();
        ui->pushButton->setText("Record");
    }
}

void MainWindow::on_sourcesList_currentTextChanged()
{
    ui->codecList->clear();
    ui->samplesRates->clear();
    ui->samplesSize->clear();
    if (ui->sourcesList->currentIndex() >= 0) {
        QList<QAudioDeviceInfo> infoList = QAudioDeviceInfo::availableDevices(QAudio::AudioInput);
        if (ui->checkboxSourceOutput->isChecked()) {
            infoList << QAudioDeviceInfo::availableDevices(QAudio::AudioOutput);
        }

        QAudioDeviceInfo info = infoList.at(ui->sourcesList->currentIndex());
        ui->pushButton->setEnabled(true);
        ui->codecList->addItems(info.supportedCodecs());
        ui->codecList->setCurrentIndex(0);
        ui->samplesSize->addItems(Manager::intListToQStringList(info.supportedSampleSizes()));
        ui->samplesRates->addItems(Manager::intListToQStringList(info.supportedSampleRates()));
        ui->channelsCount->setMaximum(info.supportedChannelCounts().last());

        //setting current config as the best possible
        ui->samplesSize->setCurrentIndex(ui->samplesSize->count() -1);
        ui->samplesRates->setCurrentIndex(ui->samplesRates->count() -1);
        ui->channelsCount->setValue(info.supportedChannelCounts().last());
    }
    else ui->pushButton->setEnabled(false);
}

void MainWindow::on_browseSourceFilePath_clicked()
{
    QFileDialog sel(this);
    sel.setFilter(QDir::Files);
    sel.setModal(true);
    sel.show();
    sel.exec();
    if (sel.result() == QFileDialog::Rejected) return;
    else if (!sel.selectedFiles().isEmpty()) ui->sourceFilePath->setText(sel.selectedFiles().first());
}


void MainWindow::recStoped() {
    timer->stop();
    ui->statusBar->showMessage("Record stoped",3000);
    ui->pushButton->setText("Record");
    setUserControlState(true);
    refreshEnabledSources();
    refreshEnabledDestinations();
}

bool MainWindow::isValidIp(const QString host) {
    //return true if the host has a valid ip, else false, valid ip example: 192.168.1.1
    const QStringList x = host.split(".");
    if (x.count() != 4) return false;
    int v;
    foreach (QString f,x) {
        v = f.toInt();
        if (v > 255) return false;
        else if (v < 0) return false;
    }
    return true;
}

void MainWindow::on_sourcesList_currentIndexChanged(int index)
{
    ui->samplesRates->clear();
    ui->codecList->clear();
    if (index >= 0) {
        QList<QAudioDeviceInfo> infoList = QAudioDeviceInfo::availableDevices(QAudio::AudioInput);
        infoList.append(QAudioDeviceInfo::availableDevices(QAudio::AudioOutput));

        QAudioDeviceInfo info = infoList.at(ui->sourcesList->currentIndex());

        ui->samplesRates->addItems(Manager::intListToQStringList(info.supportedSampleRates()));
        ui->codecList->addItems(info.supportedCodecs());
        //ui->channelsCount->setMaximum(rec->getMaxChannelsCount());
    }
}
void MainWindow::refreshEnabledSources() {
    ui->refreshSources->setEnabled(false);
    ui->sourceFilePath->setEnabled(false);
    ui->sourcesList->setEnabled(false);
    ui->browseSourceFilePath->setEnabled(false);
    if (ui->sourceRadioDevice->isChecked()) {
        ui->sourcesList->setEnabled(true);
        ui->refreshSources->setEnabled(true);
        modeSource = Manager::Device;
    }
    else if (ui->sourceRadioFile->isChecked()) {
        ui->sourceFilePath->setEnabled(true);
        ui->browseSourceFilePath->setEnabled(true);
        modeSource = Manager::File;
    }
    else if (ui->sourceRadioZeroDevice->isChecked()) modeSource = Manager::Zero;
    else if (ui->sourceRadioPulseAudio->isChecked()) modeSource = Manager::PulseAudio;
}
void MainWindow::refreshEnabledDestinations() {
    ui->destinationFilePath->setEnabled(false);
    ui->destinationPathBrowse->setEnabled(false);
    ui->destinationTcpBufferDuration->setEnabled(false);
    ui->destinationTcpSocket->setEnabled(false);
    ui->destinationDeviceCombo->setEnabled(false);
    ui->refreshOutputDevices->setEnabled(false);
#ifdef PULSE
    ui->destinationPulseAudioLineEdit->setEnabled(false);
#endif
    if (ui->destinationDeviceRadio->isChecked()) {
        ui->destinationDeviceCombo->setEnabled(true);
        ui->refreshOutputDevices->setEnabled(true);
        modeDest = Manager::Device;
    }
    else if (ui->destinationRadioFile->isChecked()) {
        ui->destinationFilePath->setEnabled(true);
        ui->destinationPathBrowse->setEnabled(true);
        modeDest = Manager::File;
    }
    else if (ui->destinationRadioTcp->isChecked()) {
        ui->destinationTcpSocket->setEnabled(true);
        ui->destinationTcpBufferDuration->setEnabled(true);
        modeDest = Manager::Tcp;
    }
#ifdef PULSE
    else if (ui->destinationRadioPulseAudio->isChecked()) {
        ui->destinationPulseAudioLineEdit->setEnabled(true);
        modeDest = Manager::PulseAudio;
    }
#endif
    else if (ui->destinationRadioZeroDevice->isChecked()) modeDest = Manager::Zero;

}
void MainWindow::refreshReadedData() {
    quint64 size = manager->getTransferedSize();
    int speed = size - lastReadedValue;
    ui->statusBar->showMessage("Readed data: " + wsize(size) + " - speed: " + wsize(speed) + "/s");

    lastReadedValue = size;
}
QString MainWindow::wsize(const quint64 size) {
    double isize = size;
    QStringList keys;
    keys << "o" << "Kb" << "Mb" << "Gb" << "Tb" << "Pb" << "Eb" << "Zb" << "Yb";
    int n;
    for (n = 0;isize >= 1024;n++) isize = isize / 1024;
    if (n >= keys.count()) n = keys.count() -1;
    return QString::number(isize,10,2) + keys.at(n);
}

void MainWindow::on_refreshOutputDevices_clicked()
{
    ui->destinationDeviceCombo->clear();
    ui->destinationDeviceCombo->addItems(Manager::getDevicesNames(QAudio::AudioOutput));
}

void MainWindow::started() {
    setUserControlState(false);
    ui->pushButton->setText("Stop");
    lastReadedValue = 0;
    timer->start();
}
void MainWindow::refreshEstimatedBitrate() {
    ui->statusBar->showMessage("estimated bitrate: " + wsize(this->getBitrate()) + "/s",2000);
}
int MainWindow::getBitrate() {
    return (ui->samplesRates->currentText().toInt() * ui->samplesSize->currentText().toInt() / 8) * ui->channelsCount->value();
}

void MainWindow::setUserControlState(const bool state) {
    ui->samplesRates->setEnabled(state);
    ui->samplesSize->setEnabled(state);
    ui->channelsCount->setEnabled(state);
    ui->codecList->setEnabled(state);

    ui->sourceRadioDevice->setEnabled(state);
    ui->sourcesList->setEnabled(state);
    ui->refreshSources->setEnabled(state);

    ui->sourceRadioFile->setEnabled(state);
    ui->sourceFilePath->setEnabled(state);
    ui->browseSourceFilePath->setEnabled(state);

    ui->sourceRadioZeroDevice->setEnabled(state);

    ui->destinationDeviceRadio->setEnabled(state);
    ui->destinationDeviceCombo->setEnabled(state);

    ui->destinationRadioFile->setEnabled(state);
    ui->destinationFilePath->setEnabled(state);
    ui->destinationTcpBufferDuration->setEnabled(state);
    ui->destinationRadioTcp->setEnabled(state);
    ui->destinationTcpSocket->setEnabled(state);
#ifdef PULSE
    ui->destinationRadioPulseAudio->setEnabled(state);
    ui->destinationPulseAudioLineEdit->setEnabled(state);
    ui->sourceRadioPulseAudio->setEnabled(state);
#endif
    ui->destinationRadioZeroDevice->setEnabled(state);

    ui->checkboxSourceOutput->setEnabled(state);
}

QString MainWindow::getConfigFilePath() {
    return QDir::homePath() + "/.audio-transfer-client.ini";
}

void MainWindow::on_configSave_clicked()
{
    configSave(this->ini);
}
void MainWindow::configSave(Readini *ini) {
    ini->open(QIODevice::WriteOnly);
    if (!ini->isWritable()) {
        errors("cannot write in the configuration file: check perms");
        return;
    }
    ui->configSave->setEnabled(false);
    ini->setValue("format","codec",ui->codecList->currentText());
    ini->setValue("format","sampleSize",ui->samplesSize->currentText());
    ini->setValue("format","sampleRate",ui->samplesRates->currentText());
    ini->setValue("format","channels",ui->channelsCount->value());

    ini->setValue("source","device",ui->sourcesList->currentText());
    ini->setValue("source","mode",modeSource);
    ini->setValue("source","file",ui->sourceFilePath->text());

    ini->setValue("target","mode",modeDest);
    ini->setValue("target","file",ui->destinationFilePath->text());
    ini->setValue("target","tcp",ui->destinationTcpSocket->text());
    ini->setValue("target","tcpBuffer",ui->destinationTcpBufferDuration->value());
#ifdef PULSE
    ini->setValue("target","pulse",ui->destinationPulseAudioLineEdit->text());
#endif
    ini->setValue("target","device",ui->destinationDeviceCombo->currentText());
    ini->setValue("options","sourceOutput",ui->checkboxSourceOutput->isChecked());

    //write all of this inside the .ini file
    ini->flush();
    debug("configuration saved");
    ui->statusBar->showMessage("configuration saved",3000);
    ui->configSave->setEnabled(true);
}
void MainWindow::configLoad(Readini *ini) {
    ui->configSave->setEnabled(false);
    if (!ini->exists()) {
        ui->configSave->setEnabled(true);
        return;
    }
    ui->checkboxSourceOutput->setChecked(intToBool(ini->getValue("options","sourceOutput").toInt()));

    const int deviceIdSource = ui->sourcesList->findText(ini->getValue("source","device"));
    if (deviceIdSource) ui->sourcesList->setCurrentIndex(deviceIdSource);

    const int codecId = ui->codecList->findText(ini->getValue("format","codec"));
    if (codecId) ui->codecList->setCurrentIndex(codecId);

    const int sampleSizePos = ui->samplesSize->findText(ini->getValue("format","sampleSize"));
    if (sampleSizePos) ui->samplesSize->setCurrentIndex(sampleSizePos);

    const int sampleRatePos = ui->samplesRates->findText(ini->getValue("format","sampleRate"));
    if (sampleRatePos) ui->samplesRates->setCurrentIndex(sampleRatePos);

    const int channels = ini->getValue("format","channels").toInt();
    if ((channels) && (channels <= ui->channelsCount->maximum())) ui->channelsCount->setValue(channels);

    const int deviceIdTarget = ui->destinationDeviceCombo->findText(ini->getValue("target","device"));
    if (deviceIdTarget) ui->destinationDeviceCombo->setCurrentIndex(deviceIdTarget);


    ui->sourceFilePath->setText(ini->getValue("source","file"));


#ifdef PULSE
    ui->destinationPulseAudioLineEdit->setText(ini->getValue("target","pulse"));
#endif

    ui->destinationFilePath->setText(ini->getValue("target","file"));
    ui->destinationTcpSocket->setText(ini->getValue("target","tcp"));
    ui->destinationTcpBufferDuration->setValue(ini->getValue("target","tcpBuffer").toInt());
    switch (ini->getValue("source","mode").toInt()) {
        case Manager::Device:
            ui->sourceRadioDevice->setChecked(true);
            break;
        case Manager::File:
            ui->sourceRadioFile->setChecked(true);
            break;
        case Manager::Zero:
            ui->sourceRadioZeroDevice->setChecked(true);
            break;
#ifdef PULSE
        case Manager::PulseAudio:
            ui->sourceRadioPulseAudio->setChecked(true);
            break;
#endif
        default:
            debug("config: ignored source mode");
            break;
    }
    refreshEnabledSources();

    switch (ini->getValue("target","mode").toInt()) {
        case Manager::Device:
            ui->destinationDeviceRadio->setChecked(true);
            break;
        case Manager::Tcp:
            ui->destinationRadioTcp->setChecked(true);
            break;
        case Manager::File:
            ui->destinationRadioFile->setChecked(true);
            break;
#ifdef PULSE
        case Manager::PulseAudio:
            ui->destinationRadioPulseAudio->setChecked(true);
            break;
#endif
        case Manager::Zero:
            ui->destinationRadioZeroDevice->setChecked(true);
            break;
        default:
            debug("config: ignored target mode");
            break;
    }
    refreshEnabledDestinations();
    ui->configSave->setEnabled(true);
}
void MainWindow::moveToCenter() {
    const QRect screen = QApplication::desktop()->screenGeometry();
    this->move(screen.center() - this->rect().center());
}
bool MainWindow::intToBool(const int value) {
    if (value) return true;
    else return false;
}
