vtk_add_test_cxx(vtkIOOCCTCxxTests tests
    TestOCCTReader.cxx,NO_OUTPUT
    TestOCCTPDCReader.cxx,NO_OUTPUT
    TestOCCTPDCStreamReader.cxx,NO_OUTPUT
    TestOCCTPDCAssembly.cxx,NO_OUTPUT,NO_VALID
    TestOCCTPDCNodeMapping.cxx,NO_OUTPUT,NO_VALID
)

vtk_test_cxx_executable(vtkIOOCCTCxxTests tests)
