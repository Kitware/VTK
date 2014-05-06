set(_known_little_endian FALSE)
if (DEFINED CMAKE_WORDS_BIGENDIAN)
  if (NOT CMAKE_WORDS_BIGENDIAN)
    set(_known_little_endian TRUE)
  endif()
endif()

if (VTK_USE_LARGE_DATA AND _known_little_endian AND NOT WIN32)
  include(vtkMPI)

  # Tell ExternalData to fetch test input at build time.
  ExternalData_Expand_Arguments(VTKData _
    "DATA{${VTK_TEST_INPUT_DIR}/WindBladeReader/,REGEX:.*}"
    "DATA{${VTK_TEST_INPUT_DIR}/WindBladeReader/field/,REGEX:.*}"
    "DATA{${VTK_TEST_INPUT_DIR}/WindBladeReader/turbine/,REGEX:.*}"
    )

  set(TestPWindBladeReader_NUMPROCS 1)
  vtk_add_test_mpi(${vtk-module}CxxTests-MPI tests
    TESTING_DATA
    TestPWindBladeReader.cxx
    )
  vtk_test_mpi_executable(${vtk-module}CxxTests-MPI tests)
endif()
