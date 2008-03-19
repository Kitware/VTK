# Test whether the compiler supports explicit template instantiation.
# This actually creates a class template instantiation in one source
# file and tries to use it from another.  This approach checks that
# both the instantiation syntax and symbol linkage is handled
# properly.
IF("VTK_EXPLICIT_TEMPLATES" MATCHES "^VTK_EXPLICIT_TEMPLATES")
  MESSAGE(STATUS "Checking support for C++ explicit template instantiation")

  MAKE_DIRECTORY(${VTK_BINARY_DIR}/CMakeTmp/TestExplicitInstantiation)
  STRING(ASCII 35 POUND)
  WRITE_FILE(
    ${VTK_BINARY_DIR}/CMakeTmp/TestExplicitInstantiation/CMakeLists.txt
    "CMAKE_MINIMUM_REQUIRED(VERSION 2.4)\n"
    "PROJECT(EXPLICIT)\n"
    "ADD_LIBRARY(A A.cxx)\n"
    "ADD_EXECUTABLE(B B.cxx)\n"
    "TARGET_LINK_LIBRARIES(B A)\n"
    )
  WRITE_FILE(
    ${VTK_BINARY_DIR}/CMakeTmp/TestExplicitInstantiation/A.h
    "${POUND}ifndef A_h\n"
    "${POUND}define A_h\n"
    "template <class T> class A { public: static T Method(); };\n"
    "${POUND}endif\n"
    )
  WRITE_FILE(
    ${VTK_BINARY_DIR}/CMakeTmp/TestExplicitInstantiation/A.cxx
    "${POUND}include \"A.h\"\n"
    "template <class T> T A<T>::Method() { return 0; }\n"
    "template class A<int>;"
    )
  WRITE_FILE(
    ${VTK_BINARY_DIR}/CMakeTmp/TestExplicitInstantiation/B.cxx
    "${POUND}include \"A.h\"\n"
    "int main() { return A<int>::Method(); }\n"
    )
  TRY_COMPILE(VTK_EXPLICIT_TEMPLATES
    ${VTK_BINARY_DIR}/CMakeTmp/TestExplicitInstantiation/Build
    ${VTK_BINARY_DIR}/CMakeTmp/TestExplicitInstantiation
    EXPLICIT OUTPUT_VARIABLE OUTPUT
    )
  IF(VTK_EXPLICIT_TEMPLATES)
    MESSAGE(STATUS "Checking support for C++ explicit template instantiation -- yes")
    SET(VTK_EXPLICIT_TEMPLATES 1 CACHE INTERNAL "Support for C++ explict templates")
    WRITE_FILE(${CMAKE_BINARY_DIR}/CMakeFiles/CMakeOutput.log
      "Determining if the C++ compiler supports explict template instantiation "
      "passed with the following output:\n"
      "${OUTPUT}\n" APPEND)
  ELSE(VTK_EXPLICIT_TEMPLATES)
    MESSAGE(STATUS "Checking support for C++ explicit template instantiation -- no")
    SET(VTK_EXPLICIT_TEMPLATES 0 CACHE INTERNAL "Support for C++ explict templates")
    WRITE_FILE(${CMAKE_BINARY_DIR}/CMakeFiles/CMakeError.log
      "Determining if the C++ compiler supports explict template instantiation "
      "failed with the following output:\n"
      "${OUTPUT}\n" APPEND)
  ENDIF(VTK_EXPLICIT_TEMPLATES)
ENDIF("VTK_EXPLICIT_TEMPLATES" MATCHES "^VTK_EXPLICIT_TEMPLATES")
