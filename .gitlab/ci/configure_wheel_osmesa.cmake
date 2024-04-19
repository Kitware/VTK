set(VTK_DIST_NAME_SUFFIX "osmesa" CACHE STRING "")
set(VTK_USE_X OFF CACHE BOOL "")
set(VTK_USE_WIN32_OPENGL OFF CACHE BOOL "")

# vtkMFCWindow requires the Windows API.
set(VTK_MODULE_ENABLE_VTK_GUISupportMFC NO CACHE STRING "")

if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "linux")
  set(CMAKE_PREFIX_PATH "/opt/osmesa" CACHE PATH "")
elseif ("$ENV{CMAKE_CONFIGURATION}" MATCHES "windows")
  set(VTK_DLL_PATHS "$ENV{CI_PROJECT_DIR}/.gitlab/osmesa/bin" CACHE PATH "")
endif ()
