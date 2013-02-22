if(VTK_DATA_ROOT)
  add_test_tcl(TestXMLImageDataIO IO)
  add_test_tcl(TestXMLRectilinearGridIO IO)
  add_test_tcl(TestEmptyXML IO)
  add_test_tcl(TestXMLStructuredGridIO IO)
endif()

#    TestXMLUnstructuredGridIO depends on Parallel
#    TestXMLPolyDataIO depends on Parallel
