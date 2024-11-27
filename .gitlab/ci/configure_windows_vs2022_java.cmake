set(VTK_JAVA_SOURCE_VERSION $ENV{VTK_JAVA_VERSION} CACHE STRING "" FORCE)
set(VTK_JAVA_TARGET_VERSION $ENV{VTK_JAVA_VERSION} CACHE STRING "" FORCE)

set(VTK_MODULE_ENABLE_VTK_vtkvtkm NO CACHE STRING "") # Java Wrap errors in windows

include("${CMAKE_CURRENT_LIST_DIR}/configure_windows.cmake")

set(MAVEN_LOCAL_NATIVE_NAME "amd64-windows" CACHE STRING "" FORCE)
set(JOGL_GLUE "$ENV{GIT_CLONE_PATH}/.gitlab/m2/org/jogamp/gluegen/gluegen-rt/2.3.2/gluegen-rt-2.3.2.jar" CACHE FILEPATH "")
set(JOGL_LIB  "$ENV{GIT_CLONE_PATH}/.gitlab/m2/org/jogamp/jogl/jogl-all/2.3.2/jogl-all-2.3.2.jar" CACHE FILEPATH "")
