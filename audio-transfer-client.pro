#-------------------------------------------------
#
# Project created by QtCreator 2014-02-17T01:16:41
#
#-------------------------------------------------
# Availables defines for this project:
# - MULTIMEDIA : enable native audio output (mandatory)
# - PULSE : enable pulseaudio (unix systems only)
# - PORTAUDIO : enable portaudio support
# - DEBUG : enable the debug mode
# - SERVER : enable the built in server (require COMLINE)
# - COMLINE : enable the command line mode
# - GUI : enable the user interface

QT       += core gui network
CONFIG   += c++11
DEFINES += MULTIMEDIA PULSE PORTAUDIO COMLINE SERVER GUI DEBUG

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport

TARGET = audio-transfer
TEMPLATE = app

contains(DEFINES,GUI) {
    SOURCES += ui/mainwindow.cpp \
	ui/graphicgenerator.cpp
	#ui/soundanalyser.cpp

    HEADERS += ui/mainwindow.h \
	ui/graphicgenerator.h
	#ui/soundanalyser.h

    FORMS    += ui/mainwindow.ui \
	ui/soundanalyser.ui
}

SOURCES += main.cpp\
    manager.cpp \
    modules/tcpdevice.cpp \
    modules/udpdevice.cpp \
    modules/pulse.cpp \
    modules/zerodevice.cpp \
    modules/nativeaudio.cpp \
    modules/pipedevice.cpp \
    readini.cpp \
    audioformat.cpp \
    size.cpp \
    modules/portaudiodevice.cpp \
    modules/pulsedeviceasync.cpp \
    circularbuffer.cpp \
    modules/circulardevice.cpp \
    modules/freqgen.cpp

HEADERS  += manager.h \
    main.h \
    readini.h \
    modules/pulse.h \
    modules/zerodevice.h \
    modules/tcpdevice.h \
    modules/udpdevice.h \
    modules/nativeaudio.h \
    modules/pipedevice.h \
    modules/circulardevice.h \
    audioformat.h \
    size.h \
    modules/portaudiodevice.h \
    modules/pulsedeviceasync.h \
    circularbuffer.h \
    modules/freqgen.h

contains(DEFINES,COMLINE) {
    CONFIG += console
    SOURCES += comline.cpp
    HEADERS += comline.h

    contains(DEFINES,SERVER) {
	SOURCES += server/serversocket.cpp \
	server/servermain.cpp \
	server/userhandler.cpp \
	server/user.cpp \
	server/security/serversecurity.cpp \
	server/security/flowchecker.cpp


	HEADERS +=    server/serversocket.h \
	server/servermain.h \
	server/userhandler.h \
	server/user.h \
	server/security/serversecurity.h \
	server/security/flowchecker.h


	config.files = server/server.ini
	config.path = ${DESTDIR}/etc/audio-transfer/
	INSTALLS += config
    }
}


#comment thoses lines to disable pulseaudio , for windows it disabled because: pulseaudio is a **** bullshit on windows:
#the windows port is version 1.1 and make audio-transfer crash: go thanks the PA developers who absolutly dont care about win32...
win32 {
    contains(DEFINES,PULSE) {
	DEFINES -= PULSE
    }
    DEFINES += WIN32
}
contains(DEFINES,PORTAUDIO) {
    LIBS += -lportaudio
}
contains(DEFINES,PULSE) {
    LIBS += -lpulse-simple -lpulse
 }
contains(DEFINES,MULTIMEDIA) {
    QT += multimedia
}


contains(DEFINES,DEBUG) {
    QMAKE_CFLAGS_DEBUG     += -fsanitize=address -fno-omit-frame-pointer -qqdb
    QMAKE_CXXFLAGS_DEBUG   += -fsanitize=address -fno-omit-frame-pointer -ggdb
    QMAKE_LFLAGS_DEBUG     += -fsanitize=address -fno-omit-frame-pointer -ggdb
}
