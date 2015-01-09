# Tests with data
ExternalData_Expand_Arguments(VTKData _
  "DATA{${VTK_TEST_INPUT_DIR}/OpenFOAM/cavity/,REGEX:.*}"
  "DATA{${VTK_TEST_INPUT_DIR}/OpenFOAM/cavity/0/,REGEX:.*}"
  "DATA{${VTK_TEST_INPUT_DIR}/OpenFOAM/cavity/0.5/,REGEX:.*}"
  "DATA{${VTK_TEST_INPUT_DIR}/OpenFOAM/cavity/0.5/uniform/,REGEX:.*}"
  "DATA{${VTK_TEST_INPUT_DIR}/OpenFOAM/cavity/1/,REGEX:.*}"
  "DATA{${VTK_TEST_INPUT_DIR}/OpenFOAM/cavity/1/uniform/,REGEX:.*}"
  "DATA{${VTK_TEST_INPUT_DIR}/OpenFOAM/cavity/1.5/,REGEX:.*}"
  "DATA{${VTK_TEST_INPUT_DIR}/OpenFOAM/cavity/1.5/uniform/,REGEX:.*}"
  "DATA{${VTK_TEST_INPUT_DIR}/OpenFOAM/cavity/2/,REGEX:.*}"
  "DATA{${VTK_TEST_INPUT_DIR}/OpenFOAM/cavity/2/uniform/,REGEX:.*}"
  "DATA{${VTK_TEST_INPUT_DIR}/OpenFOAM/cavity/2.5/,REGEX:.*}"
  "DATA{${VTK_TEST_INPUT_DIR}/OpenFOAM/cavity/2.5/uniform/,REGEX:.*}"
  "DATA{${VTK_TEST_INPUT_DIR}/OpenFOAM/cavity/constant/,REGEX:.*}"
  "DATA{${VTK_TEST_INPUT_DIR}/OpenFOAM/cavity/constant/polyMesh/,REGEX:.*}"
  "DATA{${VTK_TEST_INPUT_DIR}/OpenFOAM/cavity/system/,REGEX:.*}"
  )
vtk_add_test_cxx(${vtk-module}CxxTests tests
  TestDataObjectIO.cxx,NO_VALID
  TestIncrementalOctreePointLocator.cxx,NO_VALID
  UnstructuredGridCellGradients.cxx
  UnstructuredGridFastGradients.cxx
  UnstructuredGridGradients.cxx
  TestOBJReaderRelative.cxx,NO_VALID
  TestOBJReaderNormalsTCoords.cxx,NO_VALID
  TestOpenFOAMReader.cxx
  TestProStarReader.cxx
  TestTecplotReader.cxx
  TestAMRReadWrite.cxx,NO_VALID
  TestSimplePointsReaderWriter.cxx,NO_VALID
  )

set(_known_little_endian FALSE)
if (DEFINED CMAKE_WORDS_BIGENDIAN)
  if (NOT CMAKE_WORDS_BIGENDIAN)
    set(_known_little_endian TRUE)
  endif()
endif()
if(VTK_USE_LARGE_DATA AND _known_little_endian)
  # Tell ExternalData to fetch test input at build time.
  ExternalData_Expand_Arguments(VTKData _
    "DATA{${VTK_TEST_INPUT_DIR}/WindBladeReader/,REGEX:.*}"
    "DATA{${VTK_TEST_INPUT_DIR}/WindBladeReader/field/,REGEX:.*}"
    "DATA{${VTK_TEST_INPUT_DIR}/WindBladeReader/turbine/,REGEX:.*}"
    "DATA{${VTK_TEST_INPUT_DIR}/MFIXReader/,REGEX:.*}"
    )

  vtk_add_test_cxx(${vtk-module}CxxTests large_data_tests
    TestWindBladeReader.cxx
    TestMFIXReader.cxx,NO_VALID
    )
  list(APPEND tests
    ${large_data_tests}
    )
endif()

vtk_test_cxx_executable(${vtk-module}CxxTests tests)
