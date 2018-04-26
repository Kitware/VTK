find_package(GDAL REQUIRED)
mark_as_advanced(GDAL_INCLUDE_DIR GDAL_LIBRARY)

set(Module_SRCS
  vtkGDALRasterConverter.cxx
  vtkGDALRasterReprojection.cxx
  vtkRasterReprojectionFilter.cxx
  )

include_directories(${GDAL_INCLUDE_DIRS})
vtk_module_library(vtkGeovisGDAL ${Module_SRCS})
vtk_module_link_libraries(vtkGeovisGDAL LINK_PRIVATE ${GDAL_LIBRARY})
