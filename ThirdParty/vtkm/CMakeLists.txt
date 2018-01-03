if (NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vtk-m/CMakeLists.txt")
  message(FATAL_ERROR "VTKm requested, but the VTKm submodule is not initialized.\n"
    "Please run 'git submodule update --init --recursive' in the source directory.")
endif()

set(vtk_using_tbb OFF)
if (VTK_SMP_IMPLEMENTATION_TYPE STREQUAL "TBB")
  set(vtk_using_tbb ON)
endif()

if(NOT VTKm_ENABLE_TBB)
  set(VTKm_ENABLE_TBB ${vtk_using_tbb} CACHE BOOL "Set to match VTK" FORCE)
endif()

# VTK only hides symbols for shared libraries (see
# vtkModuleMacros::vtk_module_library), and uses default visiblity for static
# builds. VTK-m always hides symbols by default. This option tells VTK-m to use
# default visibility for static builds so we get consistent linkage:
if(NOT BUILD_SHARED_LIBS)
  set(VTKm_USE_DEFAULT_SYMBOL_VISIBILITY ON)
endif()

set(VTKm_ENABLE_TESTING OFF CACHE BOOL "Disabled when building as part of VTK" FORCE)
set(VTKm_ENABLE_RENDERING OFF CACHE BOOL "Disabled when building as part of VTK" FORCE)
set(VTKm_USE_64BIT_IDS ${VTK_USE_64BIT_IDS} CACHE BOOL "Set to match VTK" FORCE)

set(lib_suffix "-vtk${VTK_MAJOR_VERSION}.${VTK_MINOR_VERSION}")
if(VTK_CUSTOM_LIBRARY_SUFFIX)
  set(lib_suffix "${VTK_CUSTOM_LIBRARY_SUFFIX}")
endif()

if (WIN32)
  # Force the dlls to be in the bin folder with VTK's so that running tests
  # don't need to set path on Windows
  set(VTKm_EXECUTABLE_OUTPUT_PATH "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}" CACHE PATH "Set to match VTK" FORCE)
endif()

set(VTKm_INSTALL_LIB_DIR "${VTK_INSTALL_LIBRARY_DIR}")
set(VTKm_INSTALL_INCLUDE_DIR "${VTK_INSTALL_INCLUDE_DIR}")
set(VTKm_INSTALL_CMAKE_MODULE_DIR "${VTK_INSTALL_PACKAGE_DIR}/vtkm")
set(VTKm_INSTALL_CONFIG_DIR "${VTK_INSTALL_PACKAGE_DIR}/vtkm")
set(VTKm_CUSTOM_LIBRARY_SUFFIX "${lib_suffix}")
set(VTKm_NO_ASSERT "ON") #helps improve performance in debug mode

# These need to be explicitly set as the wrapping code doesn't properly handle
# grabbing include directories from dependent interface libraries
set(vtkVTKm
    ${CMAKE_CURRENT_BINARY_DIR}/vtk-m/include
    ${CMAKE_CURRENT_SOURCE_DIR}/vtk-m
    )

add_library(vtkVTKm INTERFACE)
vtk_module_export_info()
add_subdirectory(vtk-m)

set_target_properties(vtkVTKm PROPERTIES INTERFACE_LINK_LIBRARIES vtkm_cont)
vtk_target_export(vtkVTKm)
