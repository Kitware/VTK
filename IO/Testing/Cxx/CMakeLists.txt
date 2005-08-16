SET(KIT IO)
#
# Create a test lists
CREATE_TEST_SOURCELIST(Tests ${KIT}CxxTests.cxx
  TestXML.cxx
  TestCompress.cxx
  EXTRA_INCLUDE vtkTestDriver.h
)

# Currently, only the _WIN32 branch of TestMovieWriter.cxx is
# working. Once a valid MovieWriter derived class is compiled
# and linked in on other platforms, get rid of this IF:
IF(WIN32)
#  SET(Tests ${Tests} TestMovieWriter.cxx)
ENDIF(WIN32)

ADD_EXECUTABLE(${KIT}CxxTests ${Tests})
TARGET_LINK_LIBRARIES(${KIT}CxxTests vtkIO)

IF (VTK_DATA_ROOT)
  ADD_TEST(TestXML ${CXX_TEST_PATH}/${KIT}CxxTests TestXML ${VTK_DATA_ROOT}/Data/sample.xml)
  ADD_TEST(TestCompress ${CXX_TEST_PATH}/${KIT}CxxTests TestCompress ${VTK_DATA_ROOT}/Data/sample.xml)
ENDIF (VTK_DATA_ROOT)

