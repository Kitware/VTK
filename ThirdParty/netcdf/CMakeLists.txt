if(NOT VTK_USE_SYSTEM_NETCDF)
  vtk_module_third_party(NetCDF
    LIBRARIES vtkNetCDF
    INCLUDE_DIRS
      ${CMAKE_CURRENT_SOURCE_DIR}/vtknetcdf/include
      ${CMAKE_CURRENT_BINARY_DIR}/vtknetcdf
    COMPONENTS C
    )
else()
  vtk_module_third_party(NetCDF
    LIBRARIES vtkNetCDF netcdf
    COMPONENTS C
    )
endif()

# Remove old netcdfc++ header files if they exist.
if (EXISTS "${CMAKE_CURRENT_BINARY_DIR}/vtk_netcdfcpp.h")
  file(REMOVE "${CMAKE_CURRENT_BINARY_DIR}/vtk_netcdfcpp.h")
endif ()
