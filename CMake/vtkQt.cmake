set(VTK_QT_VERSION "5" CACHE STRING "Expected Qt version")

set_property(CACHE VTK_QT_VERSION PROPERTY STRINGS 4 5)

if(NOT (VTK_QT_VERSION VERSION_EQUAL "4" OR VTK_QT_VERSION VERSION_EQUAL "5"))
  message(FATAL_ERROR "Expected value for VTK_QT_VERSION is either '4' or '5'")
endif()

if(VTK_QT_VERSION VERSION_EQUAL "4")
  message(WARNING "VTK_QT_VERSION is set to 4. "
    "VTK use of Qt 4 is deprecated for version 8.1 and will not be be available "
    "in future versions. Please switch to using Qt 5 instead.")
endif()
