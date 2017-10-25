find_package(PDAL REQUIRED)
mark_as_advanced(PDAL_INCLUDE_DIR PDAL_LIBRARY)

set(PdalReader_SRCS vtkPdalReader.cxx)

include_directories(${PDAL_INCLUDE_DIRS})

vtk_module_library(vtkIOPDAL ${PdalReader_SRCS})
vtk_module_link_libraries(vtkIOPDAL LINK_PRIVATE ${PDAL_LIBRARIES})
