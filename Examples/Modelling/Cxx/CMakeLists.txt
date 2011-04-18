PROJECT (Modelling)

IF(NOT VTK_BINARY_DIR)
FIND_PACKAGE(VTK REQUIRED)
IF(NOT VTK_USE_RENDERING)
  MESSAGE(FATAL_ERROR "Example ${PROJECT_NAME} requires VTK_USE_RENDERING.")
ENDIF(NOT VTK_USE_RENDERING)
INCLUDE(${VTK_USE_FILE})
ENDIF(NOT VTK_BINARY_DIR)

ADD_EXECUTABLE(finance finance.cxx)
TARGET_LINK_LIBRARIES(finance vtkRendering)

ADD_EXECUTABLE(Delaunay3D Delaunay3D.cxx)
TARGET_LINK_LIBRARIES(Delaunay3D vtkHybrid)

ADD_EXECUTABLE(Delaunay3DAlpha Delaunay3DAlpha.cxx)
TARGET_LINK_LIBRARIES(Delaunay3DAlpha vtkHybrid)

######## Regression Testing ########
SET(KIT ModellingExamples)
SET(MyTests TestFinance.cxx TestDelaunay3D.cxx TestDelaunay3DAlpha.cxx)

INCLUDE(vtkTestingObjectFactory)

ADD_EXECUTABLE(${KIT}CxxTests ${KIT}CxxTests.cxx
               ${MyTests})
TARGET_LINK_LIBRARIES(${KIT}CxxTests vtkRendering vtkWidgets)

IF (VTK_DATA_ROOT)
ADD_TEST(${KIT}-Finance ${EXECUTABLE_OUTPUT_PATH}/${KIT}CxxTests
         TestFinance ${VTK_DATA_ROOT}/Data/financial.txt)
ADD_TEST(${KIT}-Delaunay3D ${EXECUTABLE_OUTPUT_PATH}/${KIT}CxxTests
         TestDelaunay3D
           ${VTK_DATA_ROOT}/Data/cow.vtp
           ${VTK_BINARY_DIR}/Testing/Temporary/Delaunay3D.vtu)
ADD_TEST(${KIT}-Delaunay3DAlpa ${EXECUTABLE_OUTPUT_PATH}/${KIT}CxxTests
         TestDelaunay3DAlpha
           .5 ${VTK_DATA_ROOT}/Data/cow.vtp
           ${VTK_BINARY_DIR}/Testing/Temporary/Delaunay3DAlpha.vtu)
ENDIF (VTK_DATA_ROOT)
