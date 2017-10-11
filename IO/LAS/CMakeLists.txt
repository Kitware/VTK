find_package(libLAS REQUIRED)
mark_as_advanced(libLAS_INCLUDE_DIR libLAS_LIBRARY)

set(LASReader_SRCS vtkLASReader.cxx)

include_directories(${libLAS_INCLUDE_DIRS})

vtk_module_library(vtkIOLAS ${LASReader_SRCS})
vtk_module_link_libraries(vtkIOLAS LINK_PRIVATE ${libLAS_LIBRARIES})
