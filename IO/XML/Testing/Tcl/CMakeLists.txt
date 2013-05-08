if(VTK_DATA_ROOT)
  vtk_add_test_tcl(TestXMLImageDataIO IO)
  vtk_add_test_tcl(TestXMLRectilinearGridIO IO)
  vtk_add_test_tcl(TestEmptyXML NO_DATA NO_VALID)
  vtk_add_test_tcl(TestXMLStructuredGridIO IO)
endif()

#    TestXMLUnstructuredGridIO depends on Parallel
#    TestXMLPolyDataIO depends on Parallel
