ADD_EXECUTABLE(TestXML TestXML.cxx)
ADD_TEST(TestXML ${CXX_TEST_PATH}/TestXML ${VTK_DATA_ROOT}/Data/sample.xml)

LINK_LIBRARIES (
vtkIO
vtkCommon 
)

LINK_LIBRARIES( vtkpng vtkzlib vtkjpeg vtkexpat )
