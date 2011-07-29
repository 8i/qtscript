TEMPLATE = app
QT += widgets script
# Input
HEADERS += qcontext2dcanvas.h \
	   context2d.h \
	   domimage.h \
	   environment.h \
	   window.h
SOURCES += qcontext2dcanvas.cpp \
	   context2d.cpp \
	   domimage.cpp \
	   environment.cpp \
	   window.cpp \
	   main.cpp
RESOURCES += context2d.qrc

contains(QT_CONFIG, scripttools): QT += scripttools

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtscript/script/context2d
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS context2d.pro scripts
sources.path = $$[QT_INSTALL_EXAMPLES]/qtscript/script/context2d
INSTALLS += target sources

symbian:{
    TARGET.UID3 = 0xA000C608
    CONFIG += qt_example
    TARGET.EPOCHEAPSIZE = 0x200000 0xA00000
    contextScripts.path = .
    contextScripts.files = scripts
    DEPLOYMENT += contextScripts
}
maemo5: CONFIG += qt_example

