SET(KIT GenericFiltering)
SET(UKIT GENERIC_FILTERING)
SET(KIT_TCL_LIBS vtkFilteringTCL vtkGraphicsTCL)
SET(KIT_PYTHON_LIBS vtkFilteringPythonD vtkGraphicsPythonD)
SET(KIT_JAVA_LIBS vtkFilteringJava vtkGraphicsJava)
SET(KIT_LIBS vtkFiltering vtkGraphics)

SET( Kit_SRCS
vtkGenericContourFilter.cxx
vtkGenericGeometryFilter.cxx
vtkGenericClip.cxx
vtkGenericProbeFilter.cxx
vtkGenericDataSetTessellator.cxx
vtkGenericCutter.cxx
vtkGenericGlyph3DFilter.cxx
vtkGenericStreamTracer.cxx
vtkGenericOutlineFilter.cxx
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
