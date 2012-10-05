 
project(Cosmo)

set(vtkCosmo_THIRD_PARTY 1)
set(vtkCosmo_LIBRARIES Cosmo)
vtk_module_export_info()

# Build with or without MPI, for now do this rather compiling a
# parallel and non parallel version
option(VTK_COSMO_USE_MPI "Build Cosmo with MPI" OFF)
mark_as_advanced(VTK_COSMO_USE_MPI)

#SET(Cosmo_BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS})
SET(Cosmo_LIBS)
if(VTK_COSMO_USE_MPI)
  find_package(MPI REQUIRED)
  include_directories(${MPI_C_INCLUDE_PATH})
  set(Cosmo_LIBS ${MPI_C_LIBRARIES})
  if (MPI_CXX_LIBRARIES)
    set (Cosmo_LIBS ${Cosmo_LIBS} ${MPI_CXX_LIBRARIES})
  endif()
  add_definitions(-DMPICH_IGNORE_CXX_SEEK -DUSE_VTK_COSMO)
else()
  add_definitions(-DUSE_SERIAL_COSMO -DUSE_VTK_COSMO)
endif()
# Needed for mpich 2

CONFIGURE_FILE(${Cosmo_SOURCE_DIR}/CosmoDefinition.h.in
               ${Cosmo_BINARY_DIR}/CosmoDefinition.h)
 
SET(Cosmo_SOURCES
  Partition.cxx
  ParticleDistribute.cxx
  ParticleExchange.cxx
  Message.cxx
  CosmoHaloFinder.cxx
  CosmoHaloFinderP.cxx
  FOFHaloProperties.cxx
  ChainingMesh.cxx
  HaloCenterFinder.cxx
  SODHalo.cxx
)

INCLUDE_DIRECTORIES(${Cosmo_SOURCE_DIR} ${Cosmo_BINARY_DIR})

VTK_ADD_LIBRARY(Cosmo ${Cosmo_SOURCES})
TARGET_LINK_LIBRARIES(Cosmo ${vtkCommonCore_LIBRARIES} vtksys ${Cosmo_LIBS})

IF(NOT VTK_INSTALL_NO_DEVELOPMENT)
  INSTALL(FILES
    ${Cosmo_SOURCE_DIR}/BasicDefinition.h
    ${Cosmo_SOURCE_DIR}/ChainingMesh.h
    ${Cosmo_SOURCE_DIR}/CosmoHalo.h
    ${Cosmo_SOURCE_DIR}/CosmoHaloFinder.h
    ${Cosmo_SOURCE_DIR}/CosmoHaloFinderP.h
    ${Cosmo_SOURCE_DIR}/FOFHaloProperties.h
    ${Cosmo_SOURCE_DIR}/HaloCenterFinder.h
    ${Cosmo_SOURCE_DIR}/Message.h
    ${Cosmo_SOURCE_DIR}/ParticleDistribute.h
    ${Cosmo_SOURCE_DIR}/ParticleExchange.h
    ${Cosmo_SOURCE_DIR}/Partition.h
    ${Cosmo_SOURCE_DIR}/winDirent.h
    ${Cosmo_BINARY_DIR}/CosmoDefinition.h
    DESTINATION ${VTK_INSTALL_INCLUDE_DIR_CM24}/Cosmo
    COMPONENT Development)
ENDIF(NOT VTK_INSTALL_NO_DEVELOPMENT)
