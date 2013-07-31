project(VPIC)

set(vtkVPIC_THIRD_PARTY 1)
set(vtkVPIC_LIBRARIES VPIC)
vtk_module_export_info()

SET(VPIC_BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS})

CONFIGURE_FILE(${VPIC_SOURCE_DIR}/VPICDefinition.h.in
               ${VPIC_BINARY_DIR}/VPICDefinition.h)
 
SET(VPIC_SOURCES
  VPICDataSet.cxx
  VPICDefinition.cxx
  VPICGlobal.cxx
  VPICHeader.cxx
  VPICPart.cxx
  VPICView.cxx
  GridExchange.cxx
)

# Build with or without MPI, for now do this rather compiling a
# parallel and non parallel version
option(VTK_VPIC_USE_MPI "Build VPIC with MPI" OFF)
mark_as_advanced(VTK_VPIC_USE_MPI)

if(VTK_VPIC_USE_MPI)
  find_package(MPI REQUIRED)
  include_directories(${MPI_C_INCLUDE_PATH})
  add_definitions("-DUSE_PARALLEL_VPIC -DMPICH_IGNORE_CXX_SEEK")
endif()

INCLUDE_DIRECTORIES (${VPIC_SOURCE_DIR} ${VPIC_BINARY_DIR})

VTK_ADD_LIBRARY(VPIC ${VPIC_SOURCES})
target_link_libraries(VPIC vtksys)
if(VTK_VPIC_USE_MPI)
  target_link_libraries(VPIC ${MPI_C_LIBRARIES})
  if (MPI_CXX_LIBRARIES)
    target_link_libraries(VPIC ${MPI_CXX_LIBRARIES})
  endif()
endif()

IF(NOT VTK_INSTALL_NO_DEVELOPMENT)
  INSTALL(FILES
    ${VPIC_SOURCE_DIR}/VPICDataSet.h
    ${VPIC_SOURCE_DIR}/VPICGlobal.h
    ${VPIC_SOURCE_DIR}/VPICHeader.h
    ${VPIC_SOURCE_DIR}/VPICPart.h
    ${VPIC_SOURCE_DIR}/VPICView.h
    ${VPIC_SOURCE_DIR}/GridExchange.h
    ${VPIC_BINARY_DIR}/VPICDefinition.h
    DESTINATION ${VTK_INSTALL_INCLUDE_DIR}/VPIC
    COMPONENT Development)
ENDIF(NOT VTK_INSTALL_NO_DEVELOPMENT)
