SOURCE_FILES( Common_SRCS
vtkAbstractMapper 
vtkAbstractTransform 
vtkAttributeData 
vtkCell 
vtkCell3D
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
vtkPlanes
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
vtkHeap
vtkHexahedron 
vtkIdentityTransform 
vtkIdList 
vtkIdTypeArray
vtkImageData 
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
vtkMultiThreader 
vtkMutexLock 
vtkNormals 
vtkObject 
vtkOrderedTriangulator
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
vtkCell3D
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

ADD_LIBRARY(vtkCommon Common_SRCS)

IF (WIN32)
  SOURCE_FILES(Common_SRCS vtkWin32OutputWindow)
ENDIF (WIN32)
WRAP_EXCLUDE_FILES(vtkLargeInteger)

# Allow the user to customize their build with some local options
#
INCLUDE (${VTK_BINARY_DIR}/Common/LocalUserOptions.cmake OPTIONAL)
INCLUDE (${VTK_SOURCE_DIR}/Common/LocalUserOptions.cmake OPTIONAL)

# if we are wrapping into Tcl then add the library and extra 
# source files
#
IF (VTK_WRAP_TCL)
  VTK_WRAP_TCL(vtkCommonTCL CommonTCL_SRCS Common_SRCS)
  ADD_LIBRARY(vtkCommonTCL CommonTCL_SRCS vtkTclUtil)
  TARGET_LINK_LIBRARIES (vtkCommonTCL ${TCL_LIBRARY})
  INSTALL_TARGETS(/lib/vtk vtkCommonTCL)
ENDIF (VTK_WRAP_TCL)

# if we are wrapping into Python then add the library and extra 
# source files
#
IF (VTK_WRAP_PYTHON)
  VTK_WRAP_PYTHON(vtkCommonPython CommonPython_SRCS Common_SRCS)
  IF (APPLE)
    ADD_LIBRARY(vtkCommonPython MODULE CommonPython_SRCS vtkPythonUtil)
  ELSE (APPLE)
    ADD_LIBRARY(vtkCommonPython SHARED CommonPython_SRCS vtkPythonUtil)
  ENDIF(APPLE)
  TARGET_LINK_LIBRARIES (vtkCommonPython 
                         debug ${PYTHON_DEBUG_LIBRARY} 
                         optimized ${PYTHON_LIBRARY})
  INSTALL_TARGETS(/lib/vtk vtkCommonPython)
ENDIF (VTK_WRAP_PYTHON)

# if we are wrapping into Java then add the library and extra 
# source files
#
IF (VTK_WRAP_JAVA)
  VTK_WRAP_JAVA(vtkCommonJava CommonJava_SRCS Common_SRCS)
  ADD_LIBRARY(vtkCommonJava SHARED CommonJava_SRCS vtkJavaUtil)
  INSTALL_TARGETS(/lib/vtk vtkCommonJava)
ENDIF (VTK_WRAP_JAVA)

LINK_LIBRARIES (vtkCommon )

INSTALL_TARGETS(/lib/vtk vtkCommon)
INSTALL_FILES(/include/vtk .h Common_SRCS 
  vtkCellType
  vtkSetGet 
  vtkSystemIncludes
  vtkWin32Header 
  )

