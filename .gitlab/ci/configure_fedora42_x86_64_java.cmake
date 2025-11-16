set(VTK_JAVA_RELEASE_VERSION $ENV{VTK_JAVA_VERSION} CACHE STRING "" FORCE)

include("${CMAKE_CURRENT_LIST_DIR}/configure_fedora42.cmake")

string(TOLOWER "${CMAKE_BUILD_TYPE}" cmake_build_type)
set(MAVEN_LOCAL_NATIVE_NAME "linux-amd-${cmake_build_type}" CACHE STRING "" FORCE)
set(VTK_GENERATE_SPDX OFF CACHE BOOL "" FORCE)

set(VTK_MODULE_ENABLE_VTK_fides NO CACHE STRING "")
set(VTK_MODULE_ENABLE_VTK_FiltersONNX NO CACHE BOOL "")
set(VTK_MODULE_ENABLE_VTK_IOADIOS2 NO CACHE STRING "")
set(VTK_MODULE_ENABLE_VTK_IOFides NO CACHE STRING "")

set(JOGL_GLUE "$ENV{HOME}/.m2/repository/org/jogamp/gluegen/gluegen-rt/2.6.0/gluegen-rt-2.6.0.jar" CACHE FILEPATH "")
set(JOGL_LIB  "$ENV{HOME}/.m2/repository/org/jogamp/jogl/jogl-all/2.6.0/jogl-all-2.6.0.jar" CACHE FILEPATH "")

# We need to set MAVEN_NATIVE_ARTIFACTS since this configuration is also used
# by the java upload job.

# Naming is <arch-platform-build_type> we avoid adding numbers in the arch
# such as 64/86 since some maven versions fail to properly parse it.
set(native_artifacts
  darwin-amd-${cmake_build_type}
  darwin-arm-${cmake_build_type}
  linux-amd-${cmake_build_type}
  windows-amd-${cmake_build_type}
)
set(MAVEN_NATIVE_ARTIFACTS "${native_artifacts}" CACHE STRING "" FORCE)
unset(native_artifacts)
unset(cmake_build_type)
