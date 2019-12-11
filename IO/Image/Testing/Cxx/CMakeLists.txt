vtk_add_test_cxx(vtkIOImageCxxTests data_tests
  TestNrrdReader.cxx
  TestNIFTIReaderWriter.cxx
  TestNIFTIReaderAnalyze.cxx
  TestNIFTI2.cxx
  TestDICOMImageReaderFileCollection.cxx,NO_OUTPUT
  )
set(TestMetaIO_ARGS "DATA{${_vtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/HeadMRVolume.mhd,HeadMRVolume.raw}")
vtk_add_test_cxx(vtkIOImageCxxTests tests
  NO_DATA NO_VALID NO_OUTPUT
  TestDataObjectIO.cxx
  TestMetaIO.cxx
  TestImportExport.cxx
  )

# Each of these must be added in a separate vtk_add_test_cxx
vtk_add_test_cxx(vtkIOImageCxxTests tests
  TestJPEGReader.cxx,NO_OUTPUT
    "DATA{${_vtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/beach.jpg}")

vtk_add_test_cxx(vtkIOImageCxxTests tests
  TestDICOMImageReader.cxx,NO_OUTPUT
    "DATA{${_vtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/dicom/prostate.IMG}")

vtk_add_test_cxx(vtkIOImageCxxTests tests
  TestBMPReader.cxx,NO_OUTPUT
    "DATA{${_vtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/masonry.bmp}")

vtk_add_test_cxx(vtkIOImageCxxTests tests
  TestBMPReaderDoNotAllow8BitBMP.cxx,NO_OUTPUT
    "DATA{${_vtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/masonry.bmp}")

vtk_add_test_cxx(vtkIOImageCxxTests tests
  TestSEPReader.cxx,NO_OUTPUT)

vtk_add_test_cxx(vtkIOImageCxxTests tests
  TestTIFFReaderMultipleMulti,TestTIFFReaderMultiple.cxx,NO_VALID,NO_OUTPUT
    "DATA{${_vtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/libtiff/multipage_tiff_example.tif}")

vtk_add_test_cxx(vtkIOImageCxxTests tests
  TestTIFFReaderMultipleNormal,TestTIFFReaderMultiple.cxx,NO_VALID,NO_OUTPUT
    "DATA{${_vtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/libtiff/test.tif}")

vtk_add_test_cxx(vtkIOImageCxxTests tests
  TestTIFFReaderMultipleTiled,TestTIFFReaderMultiple.cxx,NO_VALID,NO_OUTPUT
    "DATA{${_vtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/libtiff/tiled_10x30_tiff_example.tif}")

vtk_add_test_cxx(vtkIOImageCxxTests tests
  TestTIFFReaderMulti,TestTIFFReader.cxx,NO_OUTPUT
    "DATA{${_vtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/libtiff/multipage_tiff_example.tif}")

vtk_add_test_cxx(vtkIOImageCxxTests tests
  TestTIFFReaderTiled,TestTIFFReader.cxx,NO_OUTPUT
    "DATA{${_vtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/libtiff/tiled_64x64_tiff_example.tif}")

vtk_add_test_cxx(vtkIOImageCxxTests tests
  TestTIFFReaderTiledRGB,TestTIFFReader.cxx,NO_OUTPUT
    "DATA{${_vtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/libtiff/gourds_tiled_200x300.tif}")

vtk_add_test_cxx(vtkIOImageCxxTests tests
  TestCompressedTIFFReader,TestCompressedTIFFReader.cxx,NO_OUTPUT
    "DATA{${_vtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/al_foam_smallest.0.tif}")

vtk_add_test_cxx(vtkIOImageCxxTests tests
  TestWriteToMemoryPNG,TestWriteToMemory.cxx,NO_DATA NO_VALID NO_OUTPUT
    "test.png")

vtk_add_test_cxx(vtkIOImageCxxTests tests
  TestWriteToMemoryJPEG,TestWriteToMemory.cxx,NO_DATA NO_VALID NO_OUTPUT
    "test.jpeg")

vtk_add_test_cxx(vtkIOImageCxxTests tests
  TestWriteToMemoryBMP,TestWriteToMemory.cxx,NO_DATA NO_VALID NO_OUTPUT
    "test.bmp")

vtk_add_test_cxx(vtkIOImageCxxTests tests
  TestPNGReader.cxx,NO_OUTPUT
    "DATA{${_vtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/vtk.png}")

vtk_add_test_cxx(vtkIOImageCxxTests tests
  TestPNGReaderReadFromMemory.cxx,NO_OUTPUT
    "DATA{${_vtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/vtk.png}")

vtk_add_test_cxx(vtkIOImageCxxTests tests
  TestWriteToUnicodeFileBMP,TestWriteToUnicodeFile.cxx,NO_VALID
    "image.bmp")

vtk_add_test_cxx(vtkIOImageCxxTests tests
  TestWriteToUnicodeFilePNG,TestWriteToUnicodeFile.cxx,NO_VALID
    "image.png")

vtk_add_test_cxx(vtkIOImageCxxTests tests
  TestWriteToUnicodeFileJPEG,TestWriteToUnicodeFile.cxx,NO_VALID
    "image.jpg")

if (VTK_USE_LARGE_DATA)
  vtk_add_test_cxx(vtkIOImageCxxTests large_data_tests
    TestMRCReader,TestMRCReader.cxx,NO_OUTPUT
      "DATA{${_vtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/mrc/emd_1056.mrc}")

  vtk_add_test_cxx(vtkIOImageCxxTests large_data_tests
    TestOMETIFFReaderTwoChannel,TestOMETIFFReader.cxx,NO_OUTPUT,NO_VALID
    "--data" "DATA{${_vtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/ometiff/test_small_TwoChannel_Singleframe.ome.tif}"
    "--size" "256x256x75"
    "--physical-size" "0.208x0.208x0.604"
    "--size_c" "2"
    "--size_t" "1")

  vtk_add_test_cxx(vtkIOImageCxxTests large_data_tests
    TestOMETIFFReaderSingleChannel,TestOMETIFFReader.cxx,NO_OUTPUT,NO_VALID
    "--data" "DATA{${_vtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/ometiff/test_small_singleChannel_timeSeries.ome.tif}"
    "--size" "256x256x75"
    "--physical-size" "0.208x0.208x0.604"
    "--size_c" "1"
    "--size_t" "6")

  list(APPEND data_tests ${large_data_tests})
endif()

set(all_tests
  ${data_tests}
  ${tests}
  )
vtk_test_cxx_executable(vtkIOImageCxxTests all_tests
  RENDERING_FACTORY
)
