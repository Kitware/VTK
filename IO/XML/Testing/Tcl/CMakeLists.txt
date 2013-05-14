if(VTK_DATA_ROOT)
  vtk_add_test_tcl(TestXMLImageDataIO)
  vtk_add_test_tcl(TestXMLRectilinearGridIO)
  vtk_add_test_tcl(TestEmptyXML NO_DATA NO_VALID)
  vtk_add_test_tcl(TestXMLStructuredGridIO)
endif()

#    TestXMLUnstructuredGridIO depends on Parallel
#    TestXMLPolyDataIO depends on Parallel
