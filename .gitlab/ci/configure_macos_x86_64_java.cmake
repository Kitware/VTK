set(VTK_JAVA_RELEASE_VERSION $ENV{VTK_JAVA_VERSION} CACHE STRING "" FORCE)

set(VTK_WRAP_SERIALIZATION OFF CACHE BOOL "")
include("${CMAKE_CURRENT_LIST_DIR}/configure_macos.cmake")

string(TOLOWER "${CMAKE_BUILD_TYPE}" cmake_build_type)
set(MAVEN_LOCAL_NATIVE_NAME "darwin-amd-${cmake_build_type}" CACHE STRING "" FORCE)
unset(cmake_build_type)

# Ensure that we're targeting 11.0.
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "")

set(JOGL_GLUE "$ENV{GIT_CLONE_PATH}/.gitlab/m2/org/jogamp/gluegen/gluegen-rt/2.3.2/gluegen-rt-2.3.2.jar" CACHE FILEPATH "")
set(JOGL_LIB  "$ENV{GIT_CLONE_PATH}/.gitlab/m2/org/jogamp/jogl/jogl-all/2.3.2/jogl-all-2.3.2.jar" CACHE FILEPATH "")
