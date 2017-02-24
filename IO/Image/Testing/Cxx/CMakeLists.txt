# Tell ExternalData to fetch test input at build time.
  ExternalData_Expand_Arguments(VTKData _
    "DATA{${VTK_TEST_INPUT_DIR}/dicom/collection/,REGEX:.*}"
    )

vtk_add_test_cxx(${vtk-module}CxxTests data_tests
  # TestImageReader2Factory.cxx   # fixme (deps not satisfied)
  TestNrrdReader.cxx
  TestNIFTIReaderWriter.cxx
  TestNIFTIReaderAnalyze.cxx
  TestNIFTI2.cxx
  )
set(TestMetaIO_ARGS "DATA{${VTK_TEST_INPUT_DIR}/HeadMRVolume.mhd,HeadMRVolume.raw}")
vtk_add_test_cxx(${vtk-module}CxxTests tests
  NO_DATA NO_VALID NO_OUTPUT
  TestDataObjectIO.cxx
  TestMetaIO.cxx
  TestImportExport.cxx
  )

# Each of these must be added in a separate vtk_add_test_cxx
vtk_add_test_cxx(${vtk-module}CxxTests tests
  TestJPEGReader.cxx,NO_OUTPUT
    "DATA{${VTK_TEST_INPUT_DIR}/beach.jpg}")

vtk_add_test_cxx(${vtk-module}CxxTests tests
  TestDICOMImageReader.cxx,NO_OUTPUT
    "DATA{${VTK_TEST_INPUT_DIR}/dicom/prostate.IMG}")

vtk_add_test_cxx(${vtk-module}CxxTests tests
  TestDICOMImageReaderFileCollection.cxx,NO_OUTPUT
    "collection")

vtk_add_test_cxx(${vtk-module}CxxTests tests
  TestBMPReader.cxx,NO_OUTPUT
    "DATA{${VTK_TEST_INPUT_DIR}/masonry.bmp}")

vtk_add_test_cxx(${vtk-module}CxxTests tests
  TestBMPReaderDoNotAllow8BitBMP.cxx,NO_OUTPUT
    "DATA{${VTK_TEST_INPUT_DIR}/masonry.bmp}")

vtk_add_test_cxx(${vtk-module}CxxTests tests
  TestTIFFReaderMultipleMulti,TestTIFFReaderMultiple.cxx,NO_VALID,NO_OUTPUT
    "DATA{${VTK_TEST_INPUT_DIR}/libtiff/multipage_tiff_example.tif}")

vtk_add_test_cxx(${vtk-module}CxxTests tests
  TestTIFFReaderMultipleNormal,TestTIFFReaderMultiple.cxx,NO_VALID,NO_OUTPUT
    "DATA{${VTK_TEST_INPUT_DIR}/libtiff/test.tif}")

vtk_add_test_cxx(${vtk-module}CxxTests tests
  TestTIFFReaderMultipleTiled,TestTIFFReaderMultiple.cxx,NO_VALID,NO_OUTPUT
    "DATA{${VTK_TEST_INPUT_DIR}/libtiff/tiled_10x30_tiff_example.tif}")

vtk_add_test_cxx(${vtk-module}CxxTests tests
  TestTIFFReaderMulti,TestTIFFReader.cxx,NO_OUTPUT
    "DATA{${VTK_TEST_INPUT_DIR}/libtiff/multipage_tiff_example.tif}")

vtk_add_test_cxx(${vtk-module}CxxTests tests
  TestTIFFReaderTiled,TestTIFFReader.cxx,NO_OUTPUT
    "DATA{${VTK_TEST_INPUT_DIR}/libtiff/tiled_64x64_tiff_example.tif}")

vtk_add_test_cxx(${vtk-module}CxxTests tests
  TestTIFFReaderTiledRGB,TestTIFFReader.cxx,NO_OUTPUT
    "DATA{${VTK_TEST_INPUT_DIR}/libtiff/gourds_tiled_200x300.tif}")

vtk_add_test_cxx(${vtk-module}CxxTests tests
  TestCompressedTIFFReader,TestCompressedTIFFReader.cxx,NO_OUTPUT
    "DATA{${VTK_TEST_INPUT_DIR}/al_foam_smallest.0.tif}")

vtk_add_test_cxx(${vtk-module}CxxTests tests
  TestWriteToMemoryPNG,TestWriteToMemory.cxx,NO_DATA NO_VALID NO_OUTPUT
    "test.png")

vtk_add_test_cxx(${vtk-module}CxxTests tests
  TestWriteToMemoryJPEG,TestWriteToMemory.cxx,NO_DATA NO_VALID NO_OUTPUT
    "test.jpeg")

if (VTK_USE_LARGE_DATA)
  vtk_add_test_cxx(${vtk-module}CxxTests large_data_tests
    TestMRCReader,TestMRCReader.cxx,NO_OUTPUT
      "DATA{${VTK_TEST_INPUT_DIR}/mrc/emd_1056.mrc}")
  list(APPEND data_tests ${large_data_tests})
endif()

set(all_tests
  ${data_tests}
  ${tests}
  )
vtk_test_cxx_executable(${vtk-module}CxxTests all_tests
  RENDERING_FACTORY
)
