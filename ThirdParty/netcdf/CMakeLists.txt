if(NOT VTK_USE_SYSTEM_NETCDF)
  vtk_module_third_party(NetCDF
    LIBRARIES vtkNetCDF vtkNetCDF_cxx
    INCLUDE_DIRS
      ${CMAKE_CURRENT_SOURCE_DIR}/vtknetcdf/include
      ${CMAKE_CURRENT_BINARY_DIR}/vtknetcdf
    COMPONENTS C CXX
    )
else()
  vtk_module_third_party(NetCDF
    LIBRARIES vtkNetCDF netcdf
    COMPONENTS C CXX
    )
endif()


#Configure the top cpp header to switch between system and internal
#netcdf just like vtk_module_third_party does for the c header.
configure_file(vtk_netcdfcpp.h.in vtk_netcdfcpp.h)

#Install the top cpp header too.
if (NOT VTK_INSTALL_NO_DEVELOPMENT)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/vtk_netcdfcpp.h
          DESTINATION ${VTK_INSTALL_INCLUDE_DIR}
          COMPONENT Development)
endif()
