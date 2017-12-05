QT += widgets

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
