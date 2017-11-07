find_package(PDAL CONFIG REQUIRED)
mark_as_advanced(PDAL_INCLUDE_DIR PDAL_LIBRARY)

set(PDALReader_SRCS vtkPDALReader.cxx)

include_directories(${PDAL_INCLUDE_DIRS})

# PDAL_LIBRARIES don't provide full paths which does not work if PDAL is not
# installed as a system library
foreach(pdal_lib IN LISTS PDAL_LIBRARIES)
  if (NOT IS_ABSOLUTE ${pddal_lib})
    link_directories("${PDAL_DIR}/../../")
    break()
  endif()
endforeach()
vtk_module_library(vtkIOPDAL ${PDALReader_SRCS})
vtk_module_link_libraries(vtkIOPDAL LINK_PRIVATE ${PDAL_LIBRARIES})
