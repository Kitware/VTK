vtk_add_test_python(
  TestCellArray.py,NO_DATA,NO_VALID,NO_RT
  TestEmptyXML.py,NO_DATA,NO_VALID,NO_RT
  TestReadXMLUnstructuredGridWithTime.py,NO_VALID,NO_RT
  TestWriteRandomHyperTreeGridAppendBinaryMode1.py,NO_DATA,NO_VALID,NO_RT
  TestWriteReadRandomHyperTreeGridAppendBinaryMode1.py,NO_DATA,NO_VALID,NO_RT
  TestXMLMultiBlock.py,NO_DATA,NO_VALID,NO_RT
  TestXMLTimeInField.py,NO_DATA,NO_VALID,NO_RT
  # TestXMLHyperTreeGrid.py # test code does not run (fix me!)
  TestXMLImageDataIO.py
  TestXMLPolyDataIO.py
  TestXMLRectilinearGridIO.py
  TestXMLStructuredGridIO.py
  TestXMLTableIO.py,NO_VALID
  TestXMLUnstructuredGridIO.py
  )
