set(_known_little_endian FALSE)
if (DEFINED CMAKE_WORDS_BIGENDIAN)
  if (NOT CMAKE_WORDS_BIGENDIAN)
    set(_known_little_endian TRUE)
  endif()
endif()

if (VTK_LARGE_DATA_ROOT AND _known_little_endian AND NOT WIN32)
  find_package(MPI REQUIRED)
  include_directories(${MPI_INCLUDE_PATH})
  vtk_module_test_executable(${vtk-module}CxxTests TestPWindBladeReader.cxx)
  add_test(NAME ${vtk-module}Cxx-TestPWindBladeReader
    COMMAND ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} 1 ${MPIEXEC_PREFLAGS}
    $<TARGET_FILE:${vtk-module}CxxTests> TestPWindBladeReader
    -D ${VTK_LARGE_DATA_ROOT}
    -T ${VTK_TEST_OUTPUT_DIR}
    -V Baseline/Parallel/TestWindBladeReader.png
    ${VTK_MPI_POSTFLAGS})
endif()
