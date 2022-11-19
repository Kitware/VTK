vtk_add_test_python(
  TestNek5000Reader.py
  TestPDataSetReaderGrid.py
  TestPImageWriter.py
  TestPDataSetReaderWriterWithFieldData.py,NO_RT,NO_VALID)

if (TARGET VTK::IOEnSight)
  vtk_add_test_python(
    TestEnSightWriterElementTypes.py,NO_VALID,NO_RT
    TestEnSightWriterNodeIdsElementIds.py,NO_VALID,NO_RT
    TestEnSightWriterConvexPointSet.py,NO_VALID,NO_RT
    TestEnSightWriterVariables.py,NO_VALID,NO_RT
  )
endif()
