SOURCE_FILES( Filtering_SRCS
vtkCardinalSpline
vtkCastToConcrete
vtkCellLocator
vtkColorTransferFunction
vtkCone
vtkCylinder
vtkDataObjectSource
vtkDataObjectToDataSetFilter
vtkDataSetSource
vtkDataSetToDataObjectFilter
vtkDataSetToDataSetFilter
vtkDataSetToPolyDataFilter
vtkDataSetToStructuredGridFilter
vtkDataSetToStructuredPointsFilter
vtkDataSetToUnstructuredGridFilter
vtkFieldDataToAttributeDataFilter
vtkImageInPlaceFilter
vtkImageMultipleInputFilter
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
vtkScalarTree
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

ADD_LIBRARY(vtkFiltering Filtering_SRCS)

# Allow the user to customize their build with some local options
#
INCLUDE (LocalUserOptions.cmake OPTIONAL)

# if we are wrapping into Tcl then add the library and extra 
# source files
#
IF (VTK_WRAP_TCL)
  VTK_WRAP_TCL(vtkFilteringTCL FilteringTCL_SRCS Filtering_SRCS)
  ADD_LIBRARY(vtkFilteringTCL FilteringTCL_SRCS)
  TARGET_LINK_LIBRARIES (vtkFilteringTCL vtkCommonTCL ${TCL_LIBRARY})
  INSTALL_TARGETS(/lib vtkFilteringTCL)
ENDIF (VTK_WRAP_TCL)

# if we are wrapping into Python then add the library and extra 
# source files
#
IF (VTK_WRAP_PYTHON)
  VTK_WRAP_PYTHON(vtkFilteringPython FilteringPython_SRCS Filtering_SRCS)
  ADD_LIBRARY(vtkFilteringPython SHARED FilteringPython_SRCS)
  TARGET_LINK_LIBRARIES (vtkFilteringPython vtkCommonPython )
  TARGET_LINK_LIBRARIES (vtkFilteringPython 
                         debug ${PYTHON_DEBUG_LIBRARY} 
                         optimized ${PYTHON_LIBRARY})
  INSTALL_TARGETS(/lib vtkFilteringPython)
ENDIF (VTK_WRAP_PYTHON)

# if we are wrapping into Java then add the library and extra 
# source files
#
IF (VTK_WRAP_JAVA)
  VTK_WRAP_JAVA(vtkFilteringJava FilteringJava_SRCS Filtering_SRCS)
  ADD_LIBRARY(vtkFilteringJava FilteringJava_SRCS)
  TARGET_LINK_LIBRARIES (vtkFilteringJava vtkCommonJava )
  INSTALL_TARGETS(/lib vtkFilteringJava)
ENDIF (VTK_WRAP_JAVA)

LINK_LIBRARIES (
vtkCommon 
vtkFiltering 
)

INSTALL_TARGETS(/lib vtkFiltering)
INSTALL_FILES(/include/vtk .h Filtering_SRCS)
