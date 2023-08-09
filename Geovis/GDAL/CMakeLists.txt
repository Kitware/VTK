vtk_module_find_package(PRIVATE_IF_SHARED
  PACKAGE GDAL)

set(classes
  vtkGDALRasterConverter
  vtkGDALRasterReprojection
  vtkRasterReprojectionFilter)

vtk_module_add_module(VTK::GeovisGDAL
  CLASSES ${classes})
vtk_module_link(VTK::GeovisGDAL
  PRIVATE
    GDAL::GDAL)
vtk_add_test_mangling(VTK::GeovisGDAL)
