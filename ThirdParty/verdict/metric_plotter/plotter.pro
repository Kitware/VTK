CONFIG += app opengl debug warn_on

SOURCES = \
  main.cpp \
  metrics.cpp \
  tetmetrics.cpp \
  quadmetrics.cpp \
  trimetrics.cpp

FORMS = metrics_plotter.ui

HEADERS = \
  metrics.hpp \
  trimetrics.hpp \
  quadmetrics.hpp \
  tetmetrics.hpp \
  plotwindow.hpp

TARGET = metrics_plotter

INCLUDEPATH += ../

LIBS += -L../ -lverdict110

