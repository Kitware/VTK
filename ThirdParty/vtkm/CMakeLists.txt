set(vtk_using_tbb OFF)
set(vtkm_components_to_find Serial)
if (VTK_SMP_IMPLEMENTATION_TYPE STREQUAL "TBB")
  set(vtk_using_tbb ON)
  list(APPEND vtkm_components_to_find TBB)
endif()

set(VTKm_ENABLE_TBB ${vtk_using_tbb} CACHE BOOL "Set to match VTK" FORCE)
set(VTKm_ENABLE_TESTING OFF CACHE BOOL "Disabled when building as part of VTK" FORCE)
set(VTKm_ENABLE_RENDERING OFF CACHE BOOL "Disabled when building as part of VTK" FORCE)
set(VTKm_USE_64BIT_IDS ${VTK_USE_64BIT_IDS} CACHE BOOL "Set to match VTK" FORCE)

add_subdirectory(vtk-m)

set(vtkm_INCLUDE_DIRS ${CMAKE_CURRENT_BINARY_DIR}/vtk-m/include)
set(vtkm_LIBRARIES vtkm vtkm_cont)
vtk_module_export_info()
