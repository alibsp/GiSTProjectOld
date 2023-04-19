QT -= gui

CONFIG += c++17 console
CONFIG -= app_bundle

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
        BTree/gist_btree.cpp \
        GiST/gist.cpp \
        GiST/gist_cursorext.cpp \
        GiST/gist_file.cpp \
        GiST/gist_p.cpp \
        GiST/gist_query.cpp \
        #GiST/gist_support.cpp \
        GiST/gist_ustk.cpp \
        GiST/vec_t.cpp \
        GiST/gist_cursor.cpp \
        GiST/gist_htab.cpp \
        main.cpp

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

HEADERS += \
    BTree/gist_btree.h \
    GiST/gist.h \
    GiST/gist_compat.h \
    GiST/gist_cursor.h \
    GiST/gist_cursorext.h \
    GiST/gist_defs.h \
    GiST/gist_ext.h \
    GiST/gist_file.h \
    GiST/gist_htab.h \
    GiST/gist_p.h \
    GiST/gist_query.h \
    GiST/gist_support.h \
    GiST/gist_ustk.h \
    GiST/vec_t.h
