vtk_add_test_tcl(
  TestXMLImageDataIO.tcl
  TestXMLRectilinearGridIO.tcl
  TestEmptyXML.tcl,NO_DATA,NO_VALID
  TestXMLStructuredGridIO.tcl
  # TestXMLUnstructuredGridIO.tcl # depends on Parallel
  # TestXMLPolyDataIO.tcl # depends on Parallel
  )
