
QT += script
win32:CONFIG += console no_batch
mac:CONFIG -= app_bundle

SOURCES += main.cpp

include(../customclass/bytearrayclass.pri)

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtscript/script/qscript
sources.files = $$RESOURCES $$FORMS main.cpp qscript.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtscript/script/qscript
INSTALLS += target sources

maemo5: CONFIG += qt_example
