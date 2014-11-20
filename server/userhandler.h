#ifndef USERHANDLER_H
#define USERHANDLER_H

#include <QObject>
#include <QList>
#include "server/user.h"
#include "manager.h"

class UserHandler : public QObject
{
    Q_OBJECT
public:
    explicit UserHandler(QObject *parent = 0);
    bool append(User *user);
    void showUsersOnline();
    void killAll(const QString reason);
    bool contains(const QObject* socket);
    int indexOf(const QObject* socket);
    User* at(const int pos);
    quint64 getBytesRead();
    Readini* getIni();
private:
    QList<User*> users;
    Manager::userConfig defaultConfig;
    quint64 bytesRead;
signals:
    void debug(const QString message);
public slots:
private slots:
    void say(const QString message);
    void sockClose(User* user);
    void bytesNewRead(int size);
    void kicked();
};

#endif // USERHANDLER_H