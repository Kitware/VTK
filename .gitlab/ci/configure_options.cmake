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

# cuda
configuration_flag(VTK_USE_CUDA "cuda")

# osmesa
configuration_flag(VTK_OPENGL_HAS_OSMESA "osmesa")

# python
configuration_flag(VTK_WRAP_PYTHON "python")

# java
configuration_flag(VTK_WRAP_JAVA "java")

# qt
configuration_flag_module(VTK_GROUP_ENABLE_Qt "qt")

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

# vtkmoverride
configuration_flag(VTK_ENABLE_VTKM_OVERRIDES "vtkmoverride")
