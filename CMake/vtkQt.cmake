set(VTK_QT_VERSION "5" CACHE STRING "Expected Qt version")

set_property(CACHE VTK_QT_VERSION PROPERTY STRINGS 5)

if(NOT (VTK_QT_VERSION VERSION_EQUAL "5"))
  message(FATAL_ERROR "Expected value for VTK_QT_VERSION is '5'")
endif()
