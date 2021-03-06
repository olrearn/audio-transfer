#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <QtGui>
#include <QMainWindow>
#include <QTimer>
#include <QString>
#include <QComboBox>
#include "manager.h"
#include "readini.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    bool isValidIp(const QString host);
    void setUserControlState(const bool state);
    void configSave(Readini *ini);
    void configLoad(Readini *ini);
    void moveToCenter();
    bool intToBool(const int value);
    int getBitrate();

private slots:
#ifdef MULTIMEDIA
    void on_refreshSources_clicked();
    void on_sourcesList_currentTextChanged();
    void on_sourcesList_currentIndexChanged(int index);
#endif
    void on_pushButton_clicked();
    void on_browseSourceFilePath_clicked();
    void recStoped();
    void refreshEnabledSources();
    void refreshEnabledDestinations();
    void refreshReadedData();
    void on_refreshOutputDevices_clicked();
    void on_configSave_clicked();
    void on_portAudioRefreshButton_clicked();
    void on_refreshPortAudioDestinationButton_clicked();
    void on_buttonResetGraphic_clicked();
    void on_buttonSaveGraphic_clicked();
    void on_actionExit_triggered();
    void on_actionFreqgen_triggered();

public slots:
    void errors(const QString error);
    void debug(const QString message);
    void started();
    void refreshEstimatedBitrate();
    static QString getConfigFilePath();
    void refreshGraphics();
private:
    Ui::MainWindow *ui;
    QTimer *timer;
    quint64 lastReadedValue;
    Manager *manager;
    Manager::Mode modeSource;
    Manager::Mode modeDest;
    Readini *ini;
    void refreshPortAudioDevices(QComboBox* target);
    void setDefaultFormats();
    QList<int> speeds;
    Manager::Mode getNetModeForLine(QString *line);
};


#endif // MAINWINDOW_H
