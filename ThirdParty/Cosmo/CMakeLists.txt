 
project(Cosmo)

#SET(Cosmo_BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS})

SET(Cosmo_LIBS)
IF(VTK_USE_MPI)
  INCLUDE_DIRECTORIES(${MPI_INCLUDE_PATH})
  IF(MPI_EXTRA_LIBRARY)
    SET(Cosmo_LIBS ${MPI_LIBRARY} ${MPI_EXTRA_LIBRARY})
  ELSE(MPI_EXTRA_LIBRARY)
    SET(Cosmo_LIBS ${MPI_LIBRARY})
  ENDIF(MPI_EXTRA_LIBRARY)
  ADD_DEFINITIONS(-DMPICH_IGNORE_CXX_SEEK -DUSE_VTK_COSMO)
ELSE(VTK_USE_MPI)
  ADD_DEFINITIONS(-DUSE_SERIAL_COSMO -DUSE_VTK_COSMO)
ENDIF(VTK_USE_MPI)
# Needed for mpich 2

CONFIGURE_FILE (${Cosmo_SOURCE_DIR}/CosmoDefinition.h.in
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

INCLUDE_DIRECTORIES (${Cosmo_SOURCE_DIR} ${Cosmo_BINARY_DIR})

VTK_ADD_LIBRARY(Cosmo ${Cosmo_SOURCES})
TARGET_LINK_LIBRARIES(Cosmo vtksys vtkCommon ${Cosmo_LIBS})

# Apply user-defined properties to the library target. Such as SO Version
IF(VTK_LIBRARY_PROPERTIES)
  SET_TARGET_PROPERTIES(Cosmo PROPERTIES ${VTK_LIBRARY_PROPERTIES})
ENDIF(VTK_LIBRARY_PROPERTIES)

IF(NOT VTK_INSTALL_NO_LIBRARIES)
  INSTALL(TARGETS Cosmo
    EXPORT ${VTK_INSTALL_EXPORT_NAME}
    RUNTIME DESTINATION ${VTK_INSTALL_BIN_DIR_CM24} COMPONENT RuntimeLibraries
    LIBRARY DESTINATION ${VTK_INSTALL_LIB_DIR_CM24} COMPONENT RuntimeLibraries
    ARCHIVE DESTINATION ${VTK_INSTALL_LIB_DIR_CM24} COMPONENT Development)
ENDIF(NOT VTK_INSTALL_NO_LIBRARIES)

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
