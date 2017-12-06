QT += widgets

# With C++11 support
greaterThan(QT_MAJOR_VERSION, 4){
CONFIG += c++11
} else {
QMAKE_CXXFLAGS += -std=c++0x
}

HEADERS       = mainwindow.h \
    client.h \
    file_handler.h
SOURCES       = main.cpp \
                mainwindow.cpp \
    client.cpp \
    file_handler.cpp
#! [0]
RESOURCES     = application.qrc
#! [0]

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/mainwindows/application
INSTALLS += target
