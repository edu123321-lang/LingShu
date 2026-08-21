#-------------------------------------------------
#
# Project created by QtCreator 2026-07-20T16:23:16
# 项目名称：凌枢云台 (LingShu Cloud Platform)
#
#-------------------------------------------------

QT       += core gui sql serialport multimedia multimediawidgets concurrent

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = LingShu
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

CONFIG += c++11

SOURCES += \
        main.cpp \
        widget.cpp \
    mysql.cpp \
    gmwidget.cpp \
    userwidget.cpp \
    serialwidget.cpp \
    noticeuser.cpp \
    noticegm.cpp \
    attendancewidget.cpp \
    userattendancewidget.cpp \
    gmcardwidget.cpp \
    usercardwidget.cpp \
    terminalmanagement.cpp \
    consumedialog.cpp \
    dailysummarydialog.cpp \
    usermanagement.cpp \
    mycamera.cpp \
    facerecognizer.cpp \
    faceattendancedialog.cpp

HEADERS += \
        widget.h \
    mysql.h \
    gmwidget.h \
    userwidget.h \
    serialwidget.h \
    noticeuser.h \
    noticegm.h \
    attendancewidget.h \
    userattendancewidget.h \
    gmcardwidget.h \
    usercardwidget.h \
    terminalmanagement.h \
    consumedialog.h \
    dailysummarydialog.h \
    usermanagement.h \
    mycamera.h \
    facerecognizer.h \
    faceattendancedialog.h

FORMS += \
        widget.ui \
    gmwidget.ui \
    userwidget.ui \
    serialwidget.ui \
    noticeuser.ui \
    noticegm.ui \
    attendancewidget.ui \
    userattendancewidget.ui \
    gmcardwidget.ui \
    usercardwidget.ui \
    usermanagement.ui \
    faceattendancedialog.ui

# SeetaFace6 库路径（自动检测 Windows/Linux）
win32 {
    SEETA_ROOT = $$(SEETA_ROOT)
    isEmpty(SEETA_ROOT) {
        SEETA_ROOT = D:/tools/SeetaFace6
    }
    INCLUDEPATH += $$SEETA_ROOT/include
    LIBS += -L$$SEETA_ROOT/lib \
            -lSeetaFaceDetector600 \
            -lSeetaFaceLandmarker600 \
            -lSeetaFaceRecognizer610 \
            -lSeetaAuthorize \
            -ltennis \
            -lORZ
}

unix {
    INCLUDEPATH += /home/edu/opt/SeetaFace6/include
    LIBS += -L/home/edu/opt/SeetaFace6/lib64 \
            -lSeetaFaceDetector600 \
            -lSeetaFaceLandmarker600 \
            -lSeetaFaceRecognizer610 \
            -lSeetaAuthorize \
            -ltennis \
            -lORZ_static
}

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
