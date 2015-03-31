find_package(GDAL REQUIRED)
mark_as_advanced(GDAL_INCLUDE_DIR GDAL_LIBRARY)

set(GDAL_SRCS vtkGDAL.cxx vtkGDALVectorReader.cxx vtkGDALRasterReader.cxx)

include_directories(${GDAL_INCLUDE_DIRS})
set_source_files_properties(vtkGDAL WRAP_EXCLUDE)

vtk_module_library(vtkIOGDAL ${GDAL_SRCS})

vtk_module_link_libraries(vtkIOGDAL LINK_PRIVATE ${GDAL_LIBRARY})
