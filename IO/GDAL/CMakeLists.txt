vtk_module_find_package(PACKAGE GDAL)

set(classes
  vtkGDAL
  vtkGDALVectorReader
  vtkGDALRasterReader)

vtk_module_add_module(VTK::IOGDAL
  CLASSES ${classes})
vtk_module_link(VTK::IOGDAL
  PRIVATE
    GDAL::GDAL)
