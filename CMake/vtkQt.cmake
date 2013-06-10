set(VTK_QT_VERSION "4" CACHE STRING "Expected Qt version")
mark_as_advanced(VTK_QT_VERSION)

set_property(CACHE VTK_QT_VERSION PROPERTY STRINGS 4 5)

if(NOT (VTK_QT_VERSION VERSION_EQUAL "4" OR VTK_QT_VERSION VERSION_EQUAL "5"))
  message(FATAL_ERROR "Expected value for VTK_QT_VERSION is either '4' or '5'")
endif()
