SET(KIT IO)
#
# Create a test lists
CREATE_TEST_SOURCELIST(Tests ${KIT}CxxTests.cxx
  TestXML.cxx
  TestCompress.cxx
)

ADD_EXECUTABLE(${KIT}CxxTests ${Tests})
TARGET_LINK_LIBRARIES(${KIT}CxxTests vtkIO)

IF (VTK_DATA_ROOT)
  ADD_TEST(TestXML ${CXX_TEST_PATH}/${KIT}CxxTests TestXML ${VTK_DATA_ROOT}/Data/sample.xml)
  ADD_TEST(TestXML ${CXX_TEST_PATH}/${KIT}CxxTests TestCompress ${VTK_DATA_ROOT}/Data/sample.xml)
ENDIF (VTK_DATA_ROOT)
