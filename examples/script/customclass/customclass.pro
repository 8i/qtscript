QT = core script
win32: CONFIG += console
mac:CONFIG -= app_bundle

SOURCES += main.cpp

include(bytearrayclass.pri)

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtscript/script/customclass
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS *.pro *.pri
sources.path = $$[QT_INSTALL_EXAMPLES]/qtscript/script/customclass
INSTALLS += target sources

symbian: CONFIG += qt_example
maemo5: CONFIG += qt_example

symbian: warning(This example does not work on Symbian platform)
