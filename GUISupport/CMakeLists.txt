


# Determine Qt GUI.
IF(APPLE)
  # must use Carbon on the Mac to interface with Qt
  VTK_DEPENDENT_OPTION(VTK_USE_QVTK "Build QVTK widget and plugin for Qt" OFF
                       "VTK_USE_GUISUPPORT;VTK_USE_RENDERING;VTK_USE_CARBON" OFF)
ELSE(APPLE)
  VTK_DEPENDENT_OPTION(VTK_USE_QVTK "Build QVTK widget and plugin for Qt" OFF
                       "VTK_USE_GUISUPPORT;VTK_USE_RENDERING" OFF)
ENDIF(APPLE)


IF(VTK_USE_QVTK)
  SUBDIRS(Qt)
ENDIF(VTK_USE_QVTK)


