#=============================================================================
# MetaIO
# Copyright 2000-2011 Insight Software Consortium
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================

if(METAIO_FOR_VTK)
  set(METAIO_NAMESPACE "vtkmetaio")
  set(METAIO_ZLIB_LIBRARIES ${VTK_ZLIB_LIBRARIES})
  set(METAIO_INSTALL_EXPORT_NAME ${VTK_INSTALL_EXPORT_NAME})
  set(METAIO_INSTALL_RUNTIME_DIR ${VTK_INSTALL_BIN_DIR_CM24})
  set(METAIO_INSTALL_LIBRARY_DIR ${VTK_INSTALL_LIB_DIR_CM24})
  set(METAIO_INSTALL_ARCHIVE_DIR ${VTK_INSTALL_LIB_DIR_CM24})
  set(METAIO_INSTALL_NO_LIBRARIES ${VTK_INSTALL_NO_LIBRARIES})
  set(METAIO_INSTALL_NO_DEVELOPMENT ${VTK_INSTALL_NO_DEVELOPMENT})
  set(METAIO_INSTALL_INCLUDE_DIR ${VTK_INSTALL_INCLUDE_DIR_CM24}/vtkmetaio)
  set(METAIO_LIBRARY_PROPERTIES ${VTK_LIBRARY_PROPERTIES})
  if(BUILD_SHARED_LIBS)
    add_definitions(-Dvtkmetaio_BUILD_SHARED_LIBS)
  endif()
elseif("${METAIO_NAMESPACE}" STREQUAL "ITKMetaIO")
  set(METAIO_FOR_ITK 1)
elseif(NOT METAIO_NAMESPACE)
  set(METAIO_NAMESPACE MetaIO)
endif()

if(NOT DEFINED METAIO_PROJECT_NAME)
  set(METAIO_PROJECT_NAME ${METAIO_NAMESPACE})
endif()
project(${METAIO_PROJECT_NAME})

if(NOT METAIO_INSTALL_EXPORT_NAME)
  set(METAIO_INSTALL_EXPORT_NAME MetaIO)
endif()
if(NOT METAIO_INSTALL_RUNTIME_DIR)
  set(METAIO_INSTALL_RUNTIME_DIR bin)
endif()
if(NOT METAIO_INSTALL_LIBRARY_DIR)
  set(METAIO_INSTALL_LIBRARY_DIR lib)
endif()
if(NOT METAIO_INSTALL_ARCHIVE_DIR)
  set(METAIO_INSTALL_ARCHIVE_DIR lib)
endif()
if(NOT METAIO_INSTALL_INCLUDE_DIR)
  set(METAIO_INSTALL_INCLUDE_DIR include)
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/metaIOConfig.h.in
               ${CMAKE_CURRENT_BINARY_DIR}/metaIOConfig.h @ONLY)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_library(${METAIO_NAMESPACE}
  metaTypes.h
  metaUtils.cxx
  metaArrow.cxx
  metaBlob.cxx
  metaCommand.h
  metaCommand.cxx
  metaContour.h
  metaContour.cxx
  metaDTITube.cxx
  metaEllipse.cxx
  metaGroup.cxx
  metaGaussian.cxx
  metaImage.cxx
  metaImageUtils.cxx
  metaLandmark.cxx
  metaLine.cxx
  metaMesh.cxx
  metaMesh.h
  metaObject.cxx
  metaOutput.h
  metaOutput.cxx
  metaScene.cxx
  metaSurface.cxx
  metaTube.cxx
  metaVesselTube.cxx
  metaTransform.cxx
  metaTubeGraph.cxx
  metaForm.h
  metaForm.cxx
  metaArray.h
  metaArray.cxx
  metaFEMObject.h
  metaFEMObject.cxx
)

INCLUDE_REGULAR_EXPRESSION("^.*$")

# Need nsl to resolve gethostbyname on SunOS-5.8
# and socket also
IF(CMAKE_SYSTEM MATCHES "SunOS.*")
  TARGET_LINK_LIBRARIES(${METAIO_NAMESPACE} socket nsl)
ENDIF(CMAKE_SYSTEM MATCHES "SunOS.*")

target_link_libraries(${METAIO_NAMESPACE}
  ${METAIO_LIBXML2_LIBRARIES}
  ${METAIO_ZLIB_LIBRARIES}
  )
if(METAIO_LIBRARY_PROPERTIES)
  set_target_properties(${METAIO_NAMESPACE}
                        PROPERTIES ${METAIO_LIBRARY_PROPERTIES})
endif()

IF(WIN32)
  IF(BORLAND)
    TARGET_LINK_LIBRARIES(${METAIO_NAMESPACE} import32)
  ELSE(BORLAND)
    TARGET_LINK_LIBRARIES(${METAIO_NAMESPACE} comctl32 wsock32)
  ENDIF(BORLAND)
ENDIF(WIN32)


#SUBDIRS(tests)

if(NOT METAIO_INSTALL_NO_LIBRARIES)
  install(TARGETS ${METAIO_NAMESPACE}
    EXPORT ${METAIO_INSTALL_EXPORT_NAME}
    RUNTIME DESTINATION ${METAIO_INSTALL_RUNTIME_DIR} COMPONENT RuntimeLibraries
    LIBRARY DESTINATION ${METAIO_INSTALL_LIBRARY_DIR} COMPONENT RuntimeLibraries
    ARCHIVE DESTINATION ${METAIO_INSTALL_ARCHIVE_DIR} COMPONENT Development
    )
endif(NOT METAIO_INSTALL_NO_LIBRARIES)
# TODO: Install Copyright.txt
if(NOT METAIO_INSTALL_NO_DEVELOPMENT)
  file(GLOB __files "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
  install(FILES ${__files}
                ${CMAKE_CURRENT_BINARY_DIR}/metaIOConfig.h
    DESTINATION ${METAIO_INSTALL_INCLUDE_DIR} COMPONENT Development
    )
endif()
