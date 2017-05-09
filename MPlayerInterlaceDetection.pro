TEMPLATE = app
TARGET = MPlayerInterlaceDetection
QT += core \
    gui

# Qt 5+ adjustments
greaterThan(QT_MAJOR_VERSION, 4) { # QT5+
    QT += widgets # for all widgets
}
HEADERS += MyListWidget.h \
    MPlayerInterlaceDetection.h
SOURCES += MyListWidget.cpp \
    main.cpp \
    MPlayerInterlaceDetection.cpp
FORMS += MPlayerInterlaceDetection.ui
RESOURCES +=
