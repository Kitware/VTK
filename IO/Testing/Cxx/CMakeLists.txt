LINK_LIBRARIES (vtkIO)

LINK_LIBRARIES( vtkpng vtkzlib vtkjpeg vtkexpat )
ADD_EXECUTABLE(TestXML TestXML.cxx)
ADD_TEST(TestXML ${CXX_TEST_PATH}/TestXML ${VTK_DATA_ROOT}/Data/sample.xml)

