#include "readini.h"
#include <QStringList>
#include <QFile>
#include <QTextStream>
#include <QMap>

//i prefer to use this class than QSettings: less code, memory usage and here: i know whats appens
//todo: dont remove user commentary from the ini when load->flush
//      allow adding of commentary with the class (maybe using the item name for this)

Readini::Readini(const QString filePath, QObject *parent) :
    QObject(parent)
{
    this->filePath = filePath;
    file.setFileName(filePath);
    parseIni();
}
bool Readini::parseIni() {
    if (!file.exists()) return false;
    else if (!file.open(QIODevice::ReadOnly)) return false;
    else {
        QString section;
        QStringList lines = QString(file.readAll()).split("\n");
        QStringList::iterator i;
        for (i = lines.begin();i != lines.end();i++) {
            QString &line = *i;
            int lineLen = line.length();
            if (!lineLen);
            //ignoring lines starting with ";" or "#"
            else if ((line.startsWith(";")) || (line.startsWith("#")));
            else if ((line.startsWith("[")) && (line.endsWith("]"))) {
                //qDebug() << "new section";
                section = line.mid(1,lineLen -2);
            }
            else if (section.isEmpty());
            else {
                QStringList fields;
                int pos = line.indexOf("=");
                fields << line.mid(0,pos);
                fields << line.mid(pos +1);
                //qDebug() << "section: " << section << "key:" << fields[0] << "value: " << fields[1];
                this->iniContent[section][fields.at(0)]= fields.at(1);
            }
        }
        file.close();
        return true;
    }
    return false;
}
QStringList Readini::getSections() {
    if (iniContent.isEmpty()) return QStringList();
    return iniContent.keys();
}
QStringList Readini::getKeys(const QString section) {
    if (!iniContent.contains(section)) return QStringList();
    return iniContent[section].keys();
}
QString Readini::getValue(const QString section, const QString key) {
    if (!isKey(section,key)) return QString();
    else return iniContent[section][key];
}

bool Readini::isSection(const QString section) {
    return iniContent.contains(section);
}
bool Readini::isSections(QStringList keys) {
    QStringList::iterator i;
    for (i = keys.begin();i != keys.end();i++) {
        if (!this->isSection(*i)) return false;
    }
    return true;
}

bool Readini::isKey(const QString section, const QString key) {
    if (!isSection(section)) return false;
    return iniContent[section].contains(key);
}
void Readini::setValue(const QString section, const QString item, const QString value) {
    this->iniContent[section][item] = value;
}
void Readini::setValue(const QString section, const QString item, const int value) {
    setValue(section,item,QString::number(value));
}
void Readini::setValue(const QString section, const QString item, const bool value) {
    if (value) setValue(section,item,QString::number(1));
    else setValue(section,item,QString::number(0));
}

bool Readini::removeItem(const QString section, const QString item) {
    if (isKey(section,item)) {
        iniContent[section].remove(item);
        return true;
    }
    return false;
}
bool Readini::removeSection(const QString section) {
    if (isSection(section)) {
        iniContent.remove(section);
        return true;
    }
    return false;
}
bool Readini::flush() {
    //this method overwrite the current ini file with the actual content in memory
    //todo: prevent comments to be deleted in the file
    QFile file(filePath);
    if (file.exists()) file.remove();
    if (!file.open(QIODevice::WriteOnly)) return false;
    QTextStream out(&file);
    QMap<QString,QMap<QString,QString> >::iterator i;
    for (i = iniContent.begin();i != iniContent.end();i++) {
         QString section = i.key();
         out << "[" << section << "]" << endl;
         QMap<QString,QString>::iterator x;
         for (x = iniContent[section].begin();x != iniContent[section].end();x++) {
             out << x.key() << "=" << x.value() << endl;
         }
    }

    file.close();
    return true;
}
bool Readini::exists() {
    return QFile::exists(filePath);
}
bool Readini::isWritable() {
    return file.isWritable();
}
bool Readini::open(QIODevice::OpenModeFlag mode) {
    return file.open(mode);
}
bool Readini::isValuesFor(QMap<QString, QString> targets) {
    QMap<QString,QString>::iterator i;
    for (i = targets.begin();i != targets.end();i++) {
        if (!this->isKey(i.key(),i.value())) return false;
    }
    return true;
}
QMap<QString,QMap<QString,QString> > Readini::getRawData() {
    return iniContent;
}
void Readini::clear() {
    iniContent.clear();
}
QString Readini::getFilePath() {
    return filePath;
}
void Readini::reload() {
    clear();
    parseIni();
}
