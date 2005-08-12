SET(KIT Widgets)
SET(UKIT WIDGETS)

SET(KIT_TCL_LIBS vtkRenderingTCL vtkHybridTCL)
SET(KIT_PYTHON_LIBS vtkRenderingPythonD vtkHybridPythonD)
SET(KIT_JAVA_LIBS vtkRenderingJava vtkHybridJava)
SET(KIT_LIBS vtkRendering vtkHybrid)

SET ( Kit_SRCS
vtk3DWidget.cxx
vtkBoxWidget.cxx
vtkImagePlaneWidget.cxx
vtkImageTracerWidget.cxx
vtkImplicitPlaneWidget.cxx
vtkLineWidget.cxx
vtkOrientationMarkerWidget.cxx
vtkPlaneWidget.cxx
vtkPointWidget.cxx
vtkPolyDataSourceWidget.cxx
vtkScalarBarWidget.cxx
vtkSphereWidget.cxx
vtkSplineWidget.cxx
vtkXYPlotWidget.cxx
)

SET_SOURCE_FILES_PROPERTIES(
vtk3DWidget
vtkPolyDataSourceWidget
ABSTRACT
)

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
INCLUDE(${VTK_SOURCE_DIR}/CMake/KitCommonBlock.cmake)
#-----------------------------------------------------------------------------
