SUBDIRS(examplesCxx)

SOURCE_FILES( LIBRARY_SRCS
vtkAbstractMapper 
vtkAbstractTransform 
vtkAttributeData 
vtkCell 
vtkCommand 
vtkDataArray 
vtkDataSet 
vtkFunctionParser
vtkHomogeneousTransform 
vtkLinearTransform 
vtkImplicitFunction 
vtkLocator 
vtkObjectFactory 
vtkObjectFactoryCollection
vtkOverrideInformation 
vtkOverrideInformationCollection 
vtkPointSet 
vtkScalarsToColors 
vtkViewport 
vtkWarpTransform 
vtkWindow 
vtkFunctionSet 
vtkInitialValueProblemSolver
vtkActor2D 
vtkActor2DCollection 
vtkAssemblyNode 
vtkAssemblyPath 
vtkAssemblyPaths 
vtkBitArray 
vtkByteSwap 
vtkCellArray 
vtkCellData 
vtkCellLinks 
vtkCellTypes 
vtkCharArray 
vtkCollection 
vtkContourValues 
vtkCoordinate 
vtkCriticalSection 
vtkDataObject 
vtkDataObjectCollection 
vtkDataSetAttributes 
vtkDataSetCollection 
vtkDebugLeaks 
vtkDirectory 
vtkDynamicLoader 
vtkDoubleArray 
vtkEdgeTable 
vtkEmptyCell 
vtkExtentTranslator 
vtkFieldData 
vtkFileOutputWindow 
vtkFloatArray 
vtkGeneralTransform 
vtkGenericCell 
vtkHexahedron 
vtkIdentityTransform 
vtkIdList 
vtkImageData 
vtkImageSource 
vtkImageToStructuredPoints 
vtkImplicitFunctionCollection 
vtkIndent 
vtkIntArray 
vtkLine 
vtkLargeInteger
vtkLogLookupTable 
vtkLongArray 
vtkLookupTable 
vtkMapper2D 
vtkMath 
vtkMatrix4x4 
vtkMatrixToHomogeneousTransform 
vtkMatrixToLinearTransform 
vtkMergePoints2D 
vtkMultiThreader 
vtkMutexLock 
vtkNormals 
vtkObject 
vtkOutputWindow 
vtkPerspectiveTransform 
vtkPixel 
vtkPlane 
vtkPlaneCollection 
vtkPointData 
vtkPointLocator 
vtkPointLocator2D 
vtkPoints 
vtkPolyData 
vtkPolyDataSource 
vtkPolyLine 
vtkPolyVertex 
vtkPolygon 
vtkPriorityQueue 
vtkProcessObject 
vtkProp 
vtkPropAssembly 
vtkPropCollection 
vtkProperty2D 
vtkPyramid 
vtkQuad 
vtkQuadric 
vtkRectilinearGrid 
vtkReferenceCount 
vtkScalars 
vtkShortArray 
vtkSource 
vtkStack 
vtkStructuredData 
vtkStructuredGrid 
vtkStructuredPoints 
vtkTCoords 
vtkTensor 
vtkTensors 
vtkTetra 
vtkTimeStamp 
vtkTimerLog 
vtkTransform 
vtkTransformCollection 
vtkTriangle 
vtkTriangleStrip 
vtkUnsignedCharArray 
vtkUnsignedIntArray 
vtkUnsignedLongArray 
vtkUnsignedShortArray 
vtkUnstructuredGrid 
vtkVectors 
vtkVersion 
vtkVertex 
vtkVoidArray 
vtkVoxel 
vtkWedge 
vtkWindowLevelLookupTable 
vtkWindowToImageFilter 
vtkInterpolatedVelocityField 
vtkRungeKutta2 
vtkRungeKutta4 
vtkXMLFileOutputWindow 
)

ABSTRACT_FILES(
vtkAbstractMapper 
vtkAbstractTransform 
vtkAttributeData 
vtkCell 
vtkCommand 
vtkDataArray 
vtkDataSet 
vtkHomogeneousTransform 
vtkLinearTransform 
vtkImplicitFunction 
vtkLocator 
vtkObjectFactory 
vtkOverrideInformation 
vtkOverrideInformationCollection 
vtkPointSet 
vtkScalarsToColors 
vtkViewport 
vtkWarpTransform 
vtkWindow 
vtkFunctionSet 
vtkInitialValueProblemSolver
)

ADD_LIBRARY(vtkCommon LIBRARY_SRCS)

IF (WIN32)
  SOURCE_FILES(LIBRARY_SRCS vtkWin32OutputWindow)
ENDIF (WIN32)
WRAP_EXCLUDE_FILES(vtkLargeInteger)

# if we are wrapping into Tcl then add the library and extra 
# source files
#
IF (VTK_WRAP_TCL)
  SOURCE_FILES (CommonTCL_SRCS vtkTclUtil)
  WRAP_EXCLUDE_FILES(vtkTclUtil)
  VTK_WRAP_TCL(vtkCommonTCL CommonTCL_SRCS LIBRARY_SRCS)
  ADD_LIBRARY(vtkCommonTCL CommonTCL_SRCS)
ENDIF (VTK_WRAP_TCL)

# if we are wrapping into Tcl then add the library and extra 
# source files
#
IF (VTK_WRAP_PYTHON)
  SOURCE_FILES (CommonPython_SRCS vtkPythonUtil)
  WRAP_EXCLUDE_FILES(vtkPythonUtil)
  VTK_WRAP_PYTHON(vtkCommonPython CommonPython_SRCS LIBRARY_SRCS)
  ADD_LIBRARY(vtkCommonPython CommonPython_SRCS)
ENDIF (VTK_WRAP_PYTHON)

LINK_LIBRARIES (vtkCommon )

CONFIGURE_FILE_NOAUTOCONF(
${VTK_SOURCE_DIR}/common/vtkConfigure.h.in 
${VTK_BINARY_DIR}/vtkConfigure.h)

CONFIGURE_FILE_NOAUTOCONF(
${VTK_SOURCE_DIR}/common/vtkToolkits.h.in 
${VTK_BINARY_DIR}/vtkToolkits.h)


