PROJECT(Array)

FIND_PACKAGE(VTK REQUIRED)

IF(NOT VTK_USE_N_WAY_ARRAYS)
  MESSAGE(FATAL_ERROR "Example ${PROJECT_NAME} requires VTK_USE_N_WAY_ARRAYS.")
ENDIF(NOT VTK_USE_N_WAY_ARRAYS)

ADD_EXECUTABLE(ArrayBasics ArrayBasics.cxx)
TARGET_LINK_LIBRARIES(ArrayBasics vtkCommon)

ADD_EXECUTABLE(IdentityMatrix IdentityMatrix.cxx)
TARGET_LINK_LIBRARIES(IdentityMatrix vtkCommon)

ADD_EXECUTABLE(ArrayIteration ArrayIteration.cxx)
TARGET_LINK_LIBRARIES(ArrayIteration vtkCommon)

