 
project(Cosmo)

#SET(Cosmo_BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS})

#CONFIGURE_FILE (${Cosmo_SOURCE_DIR}/VPICDefinition.h.in
#                ${Cosmo_BINARY_DIR}/VPICDefinition.h)
 
SET(Cosmo_SOURCES
  Partition.cxx
  ParticleDistribute.cxx
  ParticleExchange.cxx
  Message.cxx
  CosmoHaloFinder.cxx
  CosmoHaloFinderP.cxx
  FOFHaloProperties.cxx
  ChainingMesh.cxx
)

INCLUDE_DIRECTORIES (${Cosmo_SOURCE_DIR} ${Cosmo_BINARY_DIR})

ADD_LIBRARY(Cosmo ${Cosmo_SOURCES})
TARGET_LINK_LIBRARIES(Cosmo vtksys)

IF(NOT VTK_INSTALL_NO_LIBRARIES)
  INSTALL(TARGETS Cosmo
    RUNTIME DESTINATION ${VTK_INSTALL_BIN_DIR_CM24} COMPONENT RuntimeLibraries
    LIBRARY DESTINATION ${VTK_INSTALL_LIB_DIR_CM24} COMPONENT RuntimeLibraries
    ARCHIVE DESTINATION ${VTK_INSTALL_LIB_DIR_CM24} COMPONENT Development)
ENDIF(NOT VTK_INSTALL_NO_LIBRARIES)

IF(NOT VTK_INSTALL_NO_DEVELOPMENT)
  INSTALL(FILES
    ${Cosmo_SOURCE_DIR}/VPICDataSet.h
    ${Cosmo_SOURCE_DIR}/VPICGlobal.h
    ${Cosmo_SOURCE_DIR}/VPICHeader.h
    ${Cosmo_SOURCE_DIR}/VPICPart.h
    ${Cosmo_SOURCE_DIR}/VPICView.h
    ${Cosmo_BINARY_DIR}/VPICDefinition.h
    DESTINATION ${VTK_INSTALL_INCLUDE_DIR_CM24}/Cosmo
    COMPONENT Development)
ENDIF(NOT VTK_INSTALL_NO_DEVELOPMENT)
