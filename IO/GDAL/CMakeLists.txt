find_package(GDAL REQUIRED)
mark_as_advanced(GDAL_INCLUDE_DIR GDAL_LIBRARY)

set(GDAL_SRCS vtkGDALVectorReader.cxx)

include_directories(${GDAL_INCLUDE_DIRS})

vtk_module_library(vtkIOGDAL ${GDAL_SRCS})

target_link_libraries(vtkIOGDAL LINK_PRIVATE ${GDAL_LIBRARY})
