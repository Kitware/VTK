SET(KIT Views)
SET(UKIT VIEWS)
SET(KIT_TCL_LIBS vtkInfovisTCL)
SET(KIT_PYTHON_LIBS vtkInfovisPythonD)
SET(KIT_JAVA_LIBS vtkInfovisJava)
SET(KIT_LIBS vtkInfovis)

SET( Kit_SRCS
vtkDataRepresentation.cxx
vtkGraphLayoutView.cxx
vtkRenderView.cxx
vtkSurfaceRepresentation.cxx
vtkTreeLayoutView.cxx
vtkTreeMapView.cxx
vtkView.cxx
)

#SET_SOURCE_FILES_PROPERTIES(
#vtkView.cxx
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
