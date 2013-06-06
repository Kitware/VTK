set(_known_little_endian FALSE)
if (DEFINED CMAKE_WORDS_BIGENDIAN)
  if (NOT CMAKE_WORDS_BIGENDIAN)
    set(_known_little_endian TRUE)
  endif()
endif()

if (VTK_USE_LARGE_DATA AND _known_little_endian AND NOT WIN32)
  find_package(MPI REQUIRED)
  include_directories(${MPI_INCLUDE_PATH})
  vtk_module_test_executable(${vtk-module}CxxTests TestPWindBladeReader.cxx)

  # Tell ExternalData to fetch test input at build time.
  ExternalData_Expand_Arguments(VTKData _
    "DATA{${VTK_TEST_INPUT_DIR}/WindBladeReader/,REGEX:.*}"
    "DATA{${VTK_TEST_INPUT_DIR}/WindBladeReader/field/,REGEX:.*}"
    "DATA{${VTK_TEST_INPUT_DIR}/WindBladeReader/turbine/,REGEX:.*}"
    )

  ExternalData_add_test(VTKData
    NAME ${vtk-module}Cxx-TestPWindBladeReader
    COMMAND ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} 1 ${MPIEXEC_PREFLAGS}
    $<TARGET_FILE:${vtk-module}CxxTests> TestPWindBladeReader
    -D ${VTK_TEST_DATA_DIR}
    -T ${VTK_TEST_OUTPUT_DIR}
    -V DATA{../Data/Baseline/TestWindBladeReader.png,:}
    ${VTK_MPI_POSTFLAGS})
endif()
