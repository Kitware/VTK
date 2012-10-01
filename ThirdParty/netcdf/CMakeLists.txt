set(vtknetcdf_THIRD_PARTY 1)
set(vtknetcdf_LIBRARIES vtkNetCDF vtkNetCDF_cxx)
set(vtknetcdf_INCLUDE_DIRS
  ${vtknetcdf_SOURCE_DIR}/vtknetcdf/include
  ${vtknetcdf_BINARY_DIR}/vtknetcdf
  )
vtk_module_export_info()
add_subdirectory(vtknetcdf)
