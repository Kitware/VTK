IF("VTK_COMPILER_HAS_FULL_SPECIALIZATION" MATCHES "^VTK_COMPILER_HAS_FULL_SPECIALIZATION$")
  MESSAGE(STATUS "Checking support for full template specialization syntax")
  TRY_COMPILE(VTK_COMPILER_HAS_FULL_SPECIALIZATION
              ${VTK_BINARY_DIR}/CMakeTmp
              ${VTK_CMAKE_DIR}/vtkTestFullSpecialization.cxx
              OUTPUT_VARIABLE OUTPUT)
  IF(VTK_COMPILER_HAS_FULL_SPECIALIZATION)
    MESSAGE(STATUS "Checking support for full template specialization syntax -- yes")
    SET(VTK_COMPILER_HAS_FULL_SPECIALIZATION 1 CACHE INTERNAL "Support for full template specialization syntax")
    WRITE_FILE(${CMAKE_BINARY_DIR}/CMakeFiles/CMakeOutput.log
      "Determining if the C++ compiler supports full template specialization syntax "
      "passed with the following output:\n"
      "${OUTPUT}\n" APPEND)
  ELSE(VTK_COMPILER_HAS_FULL_SPECIALIZATION)
    MESSAGE(STATUS "Checking support for full template specialization syntax -- no")
    SET(VTK_COMPILER_HAS_FULL_SPECIALIZATION 0 CACHE INTERNAL "Support for full template specialization syntax")
    WRITE_FILE(${CMAKE_BINARY_DIR}/CMakeFiles/CMakeError.log
      "Determining if the C++ compiler supports full template specialization syntax "
      "failed with the following output:\n"
      "${OUTPUT}\n" APPEND)
  ENDIF(VTK_COMPILER_HAS_FULL_SPECIALIZATION)
ENDIF("VTK_COMPILER_HAS_FULL_SPECIALIZATION" MATCHES "^VTK_COMPILER_HAS_FULL_SPECIALIZATION$")

