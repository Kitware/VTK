# Fedora 34 ships with Java 11. Setting this as our version avoids warnings
# about not specifying a "bootstrap classpath".
set(VTK_JAVA_SOURCE_VERSION 11 CACHE STRING "")
set(VTK_JAVA_TARGET_VERSION 11 CACHE STRING "")

include("${CMAKE_CURRENT_LIST_DIR}/configure_fedora_common.cmake")
