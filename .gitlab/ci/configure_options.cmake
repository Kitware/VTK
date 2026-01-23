# Options that can be overridden based on the
# configuration name.
function (configuration_flag variable configuration)
  if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "${configuration}")
    set("${variable}" ON CACHE BOOL "")
  else ()
    set("${variable}" OFF CACHE BOOL "")
  endif ()
endfunction ()

function (configuration_flag_module variable configuration)
  if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "${configuration}")
    set("${variable}" YES CACHE STRING "")
  else ()
    set("${variable}" NO CACHE STRING "")
  endif ()
endfunction ()

# doxygen
configuration_flag(VTK_BUILD_DOCUMENTATION "doxygen")

# kits
configuration_flag(VTK_ENABLE_KITS "kits")

# mpi
configuration_flag(VTK_USE_MPI "mpi")
configuration_flag_module(VTK_GROUP_ENABLE_MPI "mpi")

# offscreen
configuration_flag(VTK_DEFAULT_RENDER_WINDOW_OFFSCREEN "offscreen")
if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "offscreen")
  set(VTK_USE_COCOA OFF CACHE BOOL "")
  set(VTK_USE_X OFF CACHE BOOL "")
endif ()

# webgpu
configuration_flag(VTK_ENABLE_WEBGPU "webgpu")

# cuda
configuration_flag(VTK_USE_CUDA "cuda")

# python
configuration_flag(VTK_WRAP_PYTHON "python")

# java
configuration_flag(VTK_WRAP_JAVA "java")
configuration_flag(VTK_BUILD_MAVEN_PKG "java")

if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "java")
  set(JOGL_VERSION "2.6.0" CACHE STRING "")

  # Naming is <arch-platform-build_type> since some maven versions fail to
  # properly parse the artifact name when numbers are trailing in the classifer.
  string(TOLOWER "${CMAKE_BUILD_TYPE}" cmake_build_type)
  set(native_artifacts
    darwin-amd-${cmake_build_type}
    darwin-arm-${cmake_build_type}
    linux-amd-${cmake_build_type}
    windows-amd-${cmake_build_type}
  )
  set(MAVEN_NATIVE_ARTIFACTS "${native_artifacts}" CACHE STRING "" FORCE)
  unset(cmake_build_type)
  unset(native_artifacts)

  set(MAVEN_VTK_ARTIFACT_SUFFIX "-java${VTK_JAVA_RELEASE_VERSION}" CACHE STRING "")
  # Disable snapshots for tag releases and also when the env variable
  # VTK_JAVA_FORCE_RELEASE is defined through the Gitlab schedule pipeline UI.
  # Note that VTK_JAVA_FORCE_RELEASE is used to create/override VTK java
  # releases.
  if (NOT DEFINED ENV{CI_COMMIT_TAG} AND NOT DEFINED ENV{VTK_JAVA_FORCE_RELEASE})
    set(MAVEN_VTK_SNAPSHOT "-SNAPSHOT" CACHE STRING "")
  endif()
  set(VTK_BUILD_TESTING OFF CACHE BOOL "" FORCE)
  set(VTK_CUSTOM_LIBRARY_SOVERSION "" CACHE STRING "")
  set(VTK_CUSTOM_LIBRARY_VERSION "" CACHE STRING "")
  set(VTK_DEBUG_LEAKS OFF CACHE BOOL "" FORCE)
  set(VTK_GROUP_ENABLE_Rendering "YES" CACHE STRING "")
  set(VTK_JAVA_JOGL_COMPONENT "YES" CACHE STRING "")
  set(VTK_MODULE_ENABLE_VTK_RenderingOpenXR NO CACHE STRING "" FORCE)
  set(VTK_MODULE_ENABLE_VTK_TestingCore NO CACHE STRING "")
  set(VTK_MODULE_ENABLE_VTK_TestingDataModel NO CACHE STRING "")
  set(VTK_MODULE_ENABLE_VTK_TestingGenericBridge NO STRING STRING "")
  set(VTK_MODULE_ENABLE_VTK_TestingIOSQL NO CACHE STRING "")
  set(VTK_MODULE_ENABLE_VTK_TestingRendering NO CACHE STRING "")
  set(VTK_VERSIONED_INSTALL "OFF" CACHE BOOL "" FORCE)
endif()

# qt
configuration_flag_module(VTK_GROUP_ENABLE_Qt "qt")
if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "qt5")
  set(VTK_QT_VERSION 5 CACHE STRING "")
elseif ("$ENV{CMAKE_CONFIGURATION}" MATCHES "qt")
  set(VTK_QT_VERSION 6 CACHE STRING "")
endif ()

# "nogl" builds
if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "renderless")
  set(VTK_MODULE_ENABLE_VTK_RenderingCore NO CACHE STRING "")

  # "mpi" forces MPI modules to be ON, but these require rendering, so force
  # them off too.
  set(VTK_MODULE_ENABLE_VTK_DomainsParallelChemistry NO CACHE STRING "")
  set(VTK_MODULE_ENABLE_VTK_FiltersParallelGeometry NO CACHE STRING "")
  set(VTK_MODULE_ENABLE_VTK_FiltersParallelMPI NO CACHE STRING "")
  set(VTK_MODULE_ENABLE_VTK_IOMPIParallel NO CACHE STRING "")
endif ()

# "tbb"/"openmp"/"stdthread" builds
if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "tbb")
  set(VTK_SMP_IMPLEMENTATION_TYPE TBB CACHE STRING "")
elseif ("$ENV{CMAKE_CONFIGURATION}" MATCHES "openmp")
  set(VTK_SMP_IMPLEMENTATION_TYPE OpenMP CACHE STRING "")
elseif ("$ENV{CMAKE_CONFIGURATION}" MATCHES "stdthread")
  set(VTK_SMP_IMPLEMENTATION_TYPE STDThread CACHE STRING "")
else ()
  set(VTK_SMP_IMPLEMENTATION_TYPE Sequential CACHE STRING "")
endif ()

# Shared/static
if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "shared")
  set(VTK_BUILD_SHARED_LIBS ON CACHE BOOL "")
elseif ("$ENV{CMAKE_CONFIGURATION}" MATCHES "static")
  set(VTK_BUILD_SHARED_LIBS OFF CACHE BOOL "")
endif ()

# viskoresoverride
configuration_flag(VTK_ENABLE_VISKORES_OVERRIDES "viskoresoverride")

# ospray
if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "ospray")
  set(VTK_MODULE_ENABLE_VTK_RenderingRayTracing YES CACHE STRING "")
else ()
  set(VTK_MODULE_ENABLE_VTK_RenderingRayTracing NO CACHE STRING "")
endif ()

# anari/helide
if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "helide")
  set(VTK_MODULE_ENABLE_VTK_RenderingAnari YES CACHE STRING "")
else ()
  set(VTK_MODULE_ENABLE_VTK_RenderingAnari NO CACHE STRING "")
endif ()

# Mangling
if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "mangling")
  set(VTK_ABI_NAMESPACE_NAME "vtk_mangle_test" CACHE STRING "")
endif()
