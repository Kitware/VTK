SET( Filtering_SRCS
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

# Setup vtkInstantiator registration for this library's classes.
VTK_MAKE_INSTANTIATOR2(vtkFilteringInstantiator FilteringInstantiator_SRCS
                       ${Filtering_SRCS}
                       EXPORT_MACRO VTK_FILTERING_EXPORT
                       HEADER_LOCATION ${VTK_BINARY_DIR})

ADD_LIBRARY(vtkFiltering ${Filtering_SRCS} ${FilteringInstantiator_SRCS})

IF(VTK_APPLE_SHARED_FLAGS_NEEDED)
  SET_TARGET_PROPERTIES(vtkFiltering PROPERTIES LINK_FLAGS "-init _oft_initFil")
ENDIF(VTK_APPLE_SHARED_FLAGS_NEEDED)

TARGET_LINK_LIBRARIES (vtkFiltering vtkCommon)

# Allow the user to customize their build with some local options
#
INCLUDE (${VTK_BINARY_DIR}/Filtering/LocalUserOptions.cmake OPTIONAL)
INCLUDE (${VTK_SOURCE_DIR}/Filtering/LocalUserOptions.cmake OPTIONAL)

# if we are wrapping into Tcl then add the library and extra
# source files
#
IF (VTK_WRAP_TCL)
  VTK_WRAP_TCL2(vtkFilteringTCL FilteringTCL_SRCS ${Filtering_SRCS})
  ADD_LIBRARY(vtkFilteringTCL ${FilteringTCL_SRCS})
  TARGET_LINK_LIBRARIES (vtkFilteringTCL vtkCommonTCL vtkFiltering ${TCL_LIBRARY})
  INSTALL_TARGETS(/lib/vtk vtkFilteringTCL)
ENDIF (VTK_WRAP_TCL)

# if we are wrapping into Python then add the library and extra
# source files
#
IF (VTK_WRAP_PYTHON)
  VTK_WRAP_PYTHON2(vtkFilteringPython FilteringPython_SRCS ${Filtering_SRCS})
  ADD_LIBRARY(vtkFilteringPython MODULE ${FilteringPython_SRCS})
  IF(NOT APPLE)
    TARGET_LINK_LIBRARIES (vtkFilteringPython vtkCommonPython)
  ENDIF(NOT APPLE)
  IF(WIN32)
    TARGET_LINK_LIBRARIES (vtkFilteringPython
                           debug ${PYTHON_DEBUG_LIBRARY}
                           optimized ${PYTHON_LIBRARY})
  ENDIF(WIN32)
  TARGET_LINK_LIBRARIES(vtkFilteringPython vtkFiltering)

  INSTALL_TARGETS(/lib/vtk vtkFilteringPython)
ENDIF (VTK_WRAP_PYTHON)

# if we are wrapping into Java then add the library and extra
# source files
#
IF (VTK_WRAP_JAVA)
  VTK_WRAP_JAVA2(vtkFilteringJava FilteringJava_SRCS ${Filtering_SRCS})
  ADD_LIBRARY(vtkFilteringJava SHARED ${FilteringJava_SRCS})
  TARGET_LINK_LIBRARIES (vtkFilteringJava vtkCommonJava vtkFiltering)
  INSTALL_TARGETS(/lib/vtk vtkFilteringJava)
ENDIF (VTK_WRAP_JAVA)


INSTALL_TARGETS(/lib/vtk vtkFiltering)
INSTALL_FILES(/include/vtk .h ${Filtering_SRCS})
