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
  set(METAIO_TARGET "metaio")
  set(METAIO_INSTALL_NO_LIBRARIES TRUE)
  set(METAIO_INSTALL_NO_DEVELOPMENT TRUE)
  if(BUILD_SHARED_LIBS)
    add_definitions(-Dvtkmetaio_BUILD_SHARED_LIBS)
  endif()
elseif("${METAIO_NAMESPACE}" STREQUAL "ITKMetaIO")
  set(METAIO_FOR_ITK 1)
elseif(NOT METAIO_NAMESPACE)
  set(METAIO_NAMESPACE MetaIO)
endif()

if(NOT DEFINED METAIO_TARGET)
  set(METAIO_TARGET "${METAIO_NAMESPACE}")
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

set(sources
  metaUtils.cxx
  metaArray.cxx
  metaArrow.cxx
  metaBlob.cxx
  metaCommand.cxx
  metaContour.cxx
  metaDTITube.cxx
  metaEllipse.cxx
  metaFEMObject.cxx
  metaForm.cxx
  metaGroup.cxx
  metaGaussian.cxx
  metaImage.cxx
  metaImageUtils.cxx
  metaLandmark.cxx
  metaLine.cxx
  metaMesh.cxx
  metaObject.cxx
  metaScene.cxx
  metaSurface.cxx
  metaTube.cxx
  metaTransform.cxx
  metaTubeGraph.cxx
  metaVesselTube.cxx
  )

set(headers
  localMetaConfiguration.h
  metaArray.h
  metaArrow.h
  metaBlob.h
  metaCommand.h
  metaContour.h
  metaDTITube.h
  metaEllipse.h
  metaEvent.h
  metaFEMObject.h
  metaForm.h
  metaGaussian.h
  metaGroup.h
  metaImage.h
  metaImageTypes.h
  metaImageUtils.h
  metaLandmark.h
  metaLine.h
  metaMesh.h
  metaObject.h
  metaScene.h
  metaSurface.h
  metaTube.h
  metaTransform.h
  metaTubeGraph.h
  metaTypes.h
  metaUtils.h
  metaVesselTube.h
  "${CMAKE_CURRENT_BINARY_DIR}/metaIOConfig.h"
)

if (METAIO_FOR_VTK)
  vtk_module_add_module(VTK::metaio
    SOURCES ${sources}
    HEADERS ${headers}
    HEADERS_SUBDIR "vtkmetaio")
else ()
  add_library(${METAIO_TARGET}
    ${sources}
    ${headers}
  )
endif ()

include_regular_expression("^.*$")

# Need nsl to resolve gethostbyname on SunOS-5.8
# and socket also
if(CMAKE_SYSTEM MATCHES "SunOS.*")
  target_link_libraries(${METAIO_TARGET} PRIVATE socket nsl)
endif()

target_link_libraries(${METAIO_TARGET} PUBLIC
  ${METAIO_LIBXML2_LIBRARIES}
  ${METAIO_ZLIB_LIBRARIES}
  )
if(METAIO_LIBRARY_PROPERTIES)
  set_target_properties(${METAIO_TARGET}
                        PROPERTIES ${METAIO_LIBRARY_PROPERTIES})
endif()

if(WIN32)
  if(BORLAND)
    target_link_libraries(${METAIO_TARGET} PRIVATE import32)
  else()
    target_link_libraries(${METAIO_TARGET} PRIVATE comctl32 wsock32)
  endif()
endif()

if(BUILD_TESTING)
  add_subdirectory(tests)
endif()

if(NOT METAIO_INSTALL_NO_LIBRARIES)
  install(TARGETS ${METAIO_TARGET}
    EXPORT ${METAIO_INSTALL_EXPORT_NAME}
    RUNTIME DESTINATION ${METAIO_INSTALL_RUNTIME_DIR} COMPONENT RuntimeLibraries
    LIBRARY DESTINATION ${METAIO_INSTALL_LIBRARY_DIR} COMPONENT RuntimeLibraries
    ARCHIVE DESTINATION ${METAIO_INSTALL_ARCHIVE_DIR} COMPONENT Development
    )
endif()
# TODO: Install Copyright.txt
if(NOT METAIO_INSTALL_NO_DEVELOPMENT)
  install(FILES ${headers}
    DESTINATION ${METAIO_INSTALL_INCLUDE_DIR} COMPONENT Development
    )
endif()
