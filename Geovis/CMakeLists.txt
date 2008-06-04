SET(KIT Geovis)
SET(UKIT GEOVIS)
SET(KIT_TCL_LIBS vtkWidgetsTCL)
SET(KIT_PYTHON_LIBS vtkWidgetsPythonD)
SET(KIT_JAVA_LIBS vtkWidgetsJava)
SET(KIT_LIBS vtkWidgets ${VTK_LIBXML2_LIBRARIES})

SET( Kit_SRCS
  vtkGeoMath.cxx
)

#SET_SOURCE_FILES_PROPERTIES(
#vtkUnivariateStatisticsAlgorithm.cxx
#ABSTRACT
#)

SET(Kit_EXTRA_SRCS)
SET(Kit_EXTRA_CMDS)
SET(Kit_TCL_EXTRA_SRCS)
SET(Kit_PYTHON_EXTRA_SRCS)
SET(Kit_JAVA_EXTRA_SRCS)
SET(KIT_TCL_DEPS)
SET(KIT_PYTHON_DEPS)
SET(KIT_JAVA_DEPS)

#-----------------------------------------------------------------------------
# Include CMake code common to all kits.
INCLUDE(${VTK_CMAKE_DIR}/KitCommonBlock.cmake)
#-----------------------------------------------------------------------------
