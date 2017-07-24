# this was more useful when we supported Qt 4 and Qt 5. Not this is
# simply a place holder for when we add support for next major version of Qt.
# We're also making it INTERNAL since users can have no effect on it.
set(VTK_QT_VERSION "5" CACHE INTERNAL "Expected Qt version")
set_property(CACHE VTK_QT_VERSION PROPERTY STRINGS 5)
if(NOT VTK_QT_VERSION VERSION_EQUAL "5")
  message(FATAL_ERROR "Expected value for VTK_QT_VERSION is '5'")
endif()
