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

# if we are wrapping into Tcl then add the library and extra 
# source files
#
IF (VTK_WRAP_TCL)
  VTK_WRAP_TCL(vtkCommonTCL CommonTCL_SRCS Common_SRCS)
  ADD_LIBRARY(vtkCommonTCL CommonTCL_SRCS vtkTclUtil)
  INSTALL_TARGETS(/lib vtkCommonTCL)
ENDIF (VTK_WRAP_TCL)

# if we are wrapping into Python then add the library and extra 
# source files
#
IF (VTK_WRAP_PYTHON)
  VTK_WRAP_PYTHON(vtkCommonPython CommonPython_SRCS Common_SRCS)
  ADD_LIBRARY(vtkCommonPython CommonPython_SRCS vtkPythonUtil)
  TARGET_LINK_LIBRARIES (vtkCommonPython 
                         debug ${PYTHON_DEBUG_LIBRARY} 
                         optimized ${PYTHON_LIBRARY})
  INSTALL_TARGETS(/lib vtkCommonPython)
ENDIF (VTK_WRAP_PYTHON)

# if we are wrapping into Java then add the library and extra 
# source files
#
IF (VTK_WRAP_JAVA)
  VTK_WRAP_JAVA(vtkCommonJava CommonJava_SRCS Common_SRCS)
  ADD_LIBRARY(vtkCommonJava CommonJava_SRCS vtkJavaUtil)
  INSTALL_TARGETS(/lib vtkCommonJava)
ENDIF (VTK_WRAP_JAVA)

LINK_LIBRARIES (vtkCommon )

INSTALL_TARGETS(/lib vtkCommon)
INSTALL_FILES(/include/vtk .h Common_SRCS)