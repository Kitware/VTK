SET(KIT Filtering)
SET(UKIT FILTERING)
SET(KIT_TCL_LIBS vtkCommonTCL)
SET(KIT_PYTHON_LIBS vtkCommonPython)
SET(KIT_JAVA_LIBS vtkCommonJava)
SET(KIT_LIBS vtkCommon)

SET( Kit_SRCS
vtkCardinalSpline.cxx
vtkCastToConcrete.cxx
vtkCellLocator.cxx
vtkColorTransferFunction.cxx
vtkCone.cxx
vtkCylinder.cxx
vtkDataObjectSource.cxx
vtkDataSetSource.cxx
vtkDataSetToDataSetFilter.cxx
vtkDataSetToImageFilter.cxx
vtkDataSetToPolyDataFilter.cxx
vtkDataSetToStructuredGridFilter.cxx
vtkDataSetToStructuredPointsFilter.cxx
vtkDataSetToUnstructuredGridFilter.cxx
vtkImageInPlaceFilter.cxx
vtkImageMultipleInputFilter.cxx
vtkImageMultipleInputOutputFilter.cxx
vtkImageSource.cxx
vtkImageToImageFilter.cxx
vtkImageToStructuredPoints.cxx
vtkImageTwoInputFilter.cxx
vtkImplicitBoolean.cxx
vtkImplicitDataSet.cxx
vtkImplicitSelectionLoop.cxx
vtkImplicitSum.cxx
vtkImplicitVolume.cxx
vtkImplicitWindowFunction.cxx
vtkKochanekSpline.cxx
vtkMergePoints.cxx
vtkPerlinNoise.cxx
vtkPiecewiseFunction.cxx
vtkPiecewiseFunctionShiftScale.cxx
vtkPiecewiseFunctionSource.cxx
vtkPiecewiseFunctionToPiecewiseFunctionFilter.cxx
vtkPointSetSource.cxx
vtkPointSetToPointSetFilter.cxx
vtkPolyDataCollection.cxx
vtkPolyDataSource.cxx
vtkPolyDataToPolyDataFilter.cxx
vtkRectilinearGridSource.cxx
vtkRectilinearGridToPolyDataFilter.cxx
vtkScalarTree.cxx
vtkSimpleImageToImageFilter.cxx
vtkSimpleScalarTree.cxx
vtkSphere.cxx
vtkSpline.cxx
vtkStructuredGridSource.cxx
vtkStructuredGridToPolyDataFilter.cxx
vtkStructuredGridToStructuredGridFilter.cxx
vtkStructuredPointsCollection.cxx
vtkStructuredPointsSource.cxx
vtkStructuredPointsToPolyDataFilter.cxx
vtkStructuredPointsToStructuredPointsFilter.cxx
vtkStructuredPointsToUnstructuredGridFilter.cxx
vtkSuperquadric.cxx
vtkUnstructuredGridSource.cxx
vtkUnstructuredGridToPolyDataFilter.cxx
vtkUnstructuredGridToUnstructuredGridFilter.cxx
)

SET_SOURCE_FILES_PROPERTIES(
vtkDataObjectSource
vtkDataSetSource
vtkDataSetToDataSetFilter
vtkDataSetToImageFilter
vtkDataSetToPolyDataFilter
vtkDataSetToStructuredGridFilter
vtkDataSetToStructuredPointsFilter
vtkDataSetToUnstructuredGridFilter
vtkImageInPlaceFilter
vtkImageMultipleInputFilter
vtkImageMultipleInputOutputFilter
vtkImageSource
vtkImageToImageFilter
vtkImageTwoInputFilter
vtkPiecewiseFunctionSource
vtkPiecewiseFunctionToPiecewiseFunctionFilter
vtkPointSetSource
vtkPointSetToPointSetFilter
vtkPolyDataSource
vtkPolyDataToPolyDataFilter
vtkRectilinearGridSource
vtkRectilinearGridToPolyDataFilter
vtkScalarTree
vtkSimpleImageToImageFilter
vtkSpline
vtkStructuredGridSource
vtkStructuredGridToPolyDataFilter
vtkStructuredGridToStructuredGridFilter
vtkStructuredPointsSource
vtkStructuredPointsToPolyDataFilter
vtkStructuredPointsToStructuredPointsFilter
vtkStructuredPointsToUnstructuredGridFilter
vtkUnstructuredGridSource
vtkUnstructuredGridToPolyDataFilter
vtkUnstructuredGridToUnstructuredGridFilter
  ABSTRACT
)

SET(Kit_EXTRA_SRCS)
SET(Kit_EXTRA_CMDS)
SET(Kit_TCL_EXTRA_SRCS)
SET(Kit_PYTHON_EXTRA_SRCS)
SET(Kit_JAVA_EXTRA_SRCS)

# -----------------------------------------------------------------
# Start of common section

# Setup vtkInstantiator registration for this library's classes.
VTK_MAKE_INSTANTIATOR2(vtk${KIT}Instantiator KitInstantiator_SRCS
                       ${Kit_SRCS}
                       EXPORT_MACRO VTK_${UKIT}_EXPORT
                       HEADER_LOCATION ${VTK_BINARY_DIR})

ADD_LIBRARY(vtk${KIT} ${Kit_SRCS} ${Kit_EXTRA_SRCS} ${KitInstantiator_SRCS})

IF(VTK_APPLE_SHARED_FLAGS_NEEDED)
  SET_TARGET_PROPERTIES(vtk${KIT} PROPERTIES LINK_FLAGS "-init _oft_init${KIT}")
ENDIF(VTK_APPLE_SHARED_FLAGS_NEEDED)

# Allow the user to customize their build with some local options
#
INCLUDE (${VTK_BINARY_DIR}/${KIT}/LocalUserOptions.cmake OPTIONAL)
INCLUDE (${VTK_SOURCE_DIR}/${KIT}/LocalUserOptions.cmake OPTIONAL)

# if we are wrapping into Tcl then add the library and extra
# source files
#
IF (VTK_WRAP_TCL)
  VTK_WRAP_TCL2(vtk${KIT}TCL SOURCES KitTCL_SRCS 
                ${Kit_SRCS}
                COMMANDS ${Kit_TCL_EXTRA_CMDS})
  ADD_LIBRARY(vtk${KIT}TCL ${KitTCL_SRCS} ${Kit_TCL_EXTRA_SRCS})
  TARGET_LINK_LIBRARIES (vtk${KIT}TCL vtk${KIT} ${KIT_TCL_LIBS})
  INSTALL_TARGETS(/lib/vtk vtk${KIT}TCL)
ENDIF (VTK_WRAP_TCL)

# if we are wrapping into Python then add the library and extra
# source files
#
IF (VTK_WRAP_PYTHON)
  VTK_WRAP_PYTHON2(vtk${KIT}Python KitPython_SRCS ${Kit_SRCS})
  ADD_LIBRARY(vtk${KIT}Python MODULE ${KitPython_SRCS} ${Kit_PYTHON_EXTRA_SRCS})
  IF(NOT APPLE)
    TARGET_LINK_LIBRARIES (vtk${KIT}Python ${KIT_PYTHON_LIBS})
  ENDIF(NOT APPLE)
  IF(WIN32)
    TARGET_LINK_LIBRARIES (vtk${KIT}Python
                           debug ${PYTHON_DEBUG_LIBRARY}
                           optimized ${PYTHON_LIBRARY})
  ELSE(WIN32)
    TARGET_LINK_LIBRARIES (vtk${KIT}Python ${PYTHON_LIBRARY})
  ENDIF(WIN32)
  TARGET_LINK_LIBRARIES(vtk${KIT}Python vtk${KIT})

  INSTALL_TARGETS(/lib/vtk vtk${KIT}Python)
ENDIF (VTK_WRAP_PYTHON)

# if we are wrapping into Java then add the library and extra
# source files
#
IF (VTK_WRAP_JAVA)
  VTK_WRAP_JAVA2(vtk${KIT}Java KitJava_SRCS ${Kit_SRCS})
  ADD_LIBRARY(vtk${KIT}Java SHARED ${KitJava_SRCS} ${Kit_JAVA_EXTRA_SRCS})
  TARGET_LINK_LIBRARIES(vtk${KIT}Java vtk${KIT} ${KIT_JAVA_LIBS})
  INSTALL_TARGETS(/lib/vtk vtk${KIT}Java)
ENDIF (VTK_WRAP_JAVA)

TARGET_LINK_LIBRARIES(vtk${KIT} ${KIT_LIBS})

INSTALL_TARGETS(/lib/vtk vtk${KIT})
INSTALL_FILES(/include/vtk .h ${Kit_SRCS})

# End of common section
# -----------------------------------------------------------------
