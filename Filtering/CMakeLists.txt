SOURCE_FILES( Filtering_SRCS
vtkCardinalSpline
vtkCastToConcrete
vtkCellLocator
vtkColorTransferFunction
vtkCone
vtkCylinder
vtkDataObjectSource
vtkDataSetSource
vtkDataSetToDataSetFilter
vtkDataSetToPolyDataFilter
vtkDataSetToStructuredGridFilter
vtkDataSetToStructuredPointsFilter
vtkDataSetToUnstructuredGridFilter
vtkImageInPlaceFilter
vtkImageMultipleInputFilter
vtkImageMultipleInputOutputFilter 
vtkImageSource 
vtkImageToImageFilter
vtkImageToStructuredPoints 
vtkImageTwoInputFilter
vtkImplicitBoolean
vtkImplicitDataSet
vtkImplicitSelectionLoop
vtkImplicitVolume
vtkImplicitWindowFunction
vtkKochanekSpline
vtkMergePoints
vtkMergePoints2D 
vtkPiecewiseFunction
vtkPointSetSource
vtkPointSetToPointSetFilter
vtkPolyDataCollection
vtkPolyDataSource 
vtkPolyDataToPolyDataFilter
vtkRectilinearGridSource
vtkRectilinearGridToPolyDataFilter
vtkSimpleScalarTree
vtkSimpleImageToImageFilter 
vtkSphere
vtkSpline
vtkStructuredGridSource
vtkStructuredGridToPolyDataFilter
vtkStructuredGridToStructuredGridFilter
vtkStructuredPointsCollection
vtkStructuredPointsSource
vtkStructuredPointsToPolyDataFilter
vtkStructuredPointsToStructuredPointsFilter
vtkStructuredPointsToUnstructuredGridFilter
vtkSuperquadric 
vtkUnstructuredGridSource
vtkUnstructuredGridToPolyDataFilter
vtkUnstructuredGridToUnstructuredGridFilter
)

ABSTRACT_FILES(
vtkDataSetToDataSetFilter
vtkDataSetToPolyDataFilter
vtkDataSetToStructuredGridFilter
vtkDataSetToStructuredPointsFilter
vtkDataSetToUnstructuredGridFilter
vtkInterpolatingSubdivisionFilter
vtkPointSetToPointSetFilter
vtkPolyDataToPolyDataFilter
vtkProp3D
vtkRectilinearGridToPolyDataFilter
vtkScalarTree
vtkSimpleImageToImageFilter 
vtkSpline
vtkStructuredGridToPolyDataFilter
vtkStructuredGridToStructuredGridFilter
vtkStructuredPointsToPolyDataFilter
vtkStructuredPointsToStructuredPointsFilter
vtkStructuredPointsToUnstructuredGridFilter
vtkUnstructuredGridToPolyDataFilter
vtkUnstructuredGridToUnstructuredGridFilter
)

IF(VTK_MAKE_INSTANTIATOR)
  # Setup vtkInstantiator registration for this library's classes.
  VTK_MAKE_INSTANTIATOR(vtkFilteringInstantiator FilteringInstantiator_SRCS
                        Filtering_SRCS
                        EXPORT_MACRO VTK_FILTERING_EXPORT
                        HEADER_LOCATION ${VTK_BINARY_DIR})
  ADD_LIBRARY(vtkFiltering Filtering_SRCS FilteringInstantiator_SRCS)
  INSTALL_FILES(/include/vtk .h ${VTK_BINARY_DIR}/vtkFilteringInstantiator)
ELSE(VTK_MAKE_INSTANTIATOR)
  ADD_LIBRARY(vtkFiltering Filtering_SRCS)
ENDIF(VTK_MAKE_INSTANTIATOR)

# Allow the user to customize their build with some local options
#
INCLUDE (${VTK_BINARY_DIR}/Filtering/LocalUserOptions.cmake OPTIONAL)
INCLUDE (${VTK_SOURCE_DIR}/Filtering/LocalUserOptions.cmake OPTIONAL)

# if we are wrapping into Tcl then add the library and extra 
# source files
#
IF (VTK_WRAP_TCL)
  VTK_WRAP_TCL(vtkFilteringTCL FilteringTCL_SRCS Filtering_SRCS)
  IF (APPLE)
    ADD_LIBRARY(vtkFilteringTCL SHARED FilteringTCL_SRCS)
  ELSE (APPLE)
    ADD_LIBRARY(vtkFilteringTCL FilteringTCL_SRCS)
  ENDIF (APPLE)
  TARGET_LINK_LIBRARIES (vtkFilteringTCL vtkCommonTCL ${TCL_LIBRARY})
  INSTALL_TARGETS(/lib/vtk vtkFilteringTCL)
ENDIF (VTK_WRAP_TCL)

# if we are wrapping into Python then add the library and extra 
# source files
#
IF (VTK_WRAP_PYTHON)
  VTK_WRAP_PYTHON(vtkFilteringPython FilteringPython_SRCS Filtering_SRCS)
  IF (APPLE)
    ADD_LIBRARY(vtkFilteringPython MODULE FilteringPython_SRCS)
  ELSE (APPLE)
    ADD_LIBRARY(vtkFilteringPython SHARED FilteringPython_SRCS)
    TARGET_LINK_LIBRARIES (vtkFilteringPython vtkCommonPython )
  ENDIF (APPLE)
  IF(WIN32)
    TARGET_LINK_LIBRARIES (vtkFilteringPython 
                           debug ${PYTHON_DEBUG_LIBRARY} 
                           optimized ${PYTHON_LIBRARY})
  ELSE(WIN32)
    TARGET_LINK_LIBRARIES (vtkFilteringPython ${PYTHON_LIBRARY})
  ENDIF(WIN32)
  INSTALL_TARGETS(/lib/vtk vtkFilteringPython)
ENDIF (VTK_WRAP_PYTHON)

# if we are wrapping into Java then add the library and extra 
# source files
#
IF (VTK_WRAP_JAVA)
  VTK_WRAP_JAVA(vtkFilteringJava FilteringJava_SRCS Filtering_SRCS)
  ADD_LIBRARY(vtkFilteringJava SHARED FilteringJava_SRCS)
  TARGET_LINK_LIBRARIES (vtkFilteringJava vtkCommonJava )
  INSTALL_TARGETS(/lib/vtk vtkFilteringJava)
ENDIF (VTK_WRAP_JAVA)

LINK_LIBRARIES (
vtkCommon 
vtkFiltering 
)

INSTALL_TARGETS(/lib/vtk vtkFiltering)
INSTALL_FILES(/include/vtk .h Filtering_SRCS)
