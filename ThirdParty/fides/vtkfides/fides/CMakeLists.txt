set(srcs
  Array.cxx
  CellSet.cxx
  CoordinateSystem.cxx
  DataModel.cxx
  DataSetReader.cxx
  DataSetWriter.cxx
  DataSource.cxx
  FidesTypes.cxx
  Field.cxx
  FieldData.cxx
  FieldDataManager.cxx
  Keys.cxx
  MetaData.cxx
  Value.cxx
)

set(headers
  Array.h
  CellSet.h
  Configure.h
  CoordinateSystem.h
  DataModel.h
  DataSetReader.h
  DataSetWriter.h
  DataSource.h
  Deprecated.h
  FidesTypes.h
  Field.h
  FieldData.h
  FieldDataManager.h
  Keys.h
  MetaData.h
  Value.h
  "${CMAKE_CURRENT_BINARY_DIR}/fides_export.h"
)

include(${FIDES_SOURCE_DIR}/cmake/fides_generate_export_header.cmake)

if (FALSE) # XXX(kitware): use vtk_module system
add_library(fides ${srcs})
vtkm_add_target_information(fides
    DROP_UNUSED_SYMBOLS
    MODIFY_CUDA_FLAGS
    DEVICE_SOURCES ${srcs})

if (VTKm_ENABLE_CUDA)
  set_target_properties(fides PROPERTIES CUDA_SEPARABLE_COMPILATION ON)
  set_target_properties(fides PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()
else ()
vtk_module_add_module(VTK::fides
  SOURCES ${srcs}
  HEADERS ${headers}
  HEADERS_SUBDIR "vtkfides/fides")
vtkm_add_target_information(fides
    DROP_UNUSED_SYMBOLS
    MODIFY_CUDA_FLAGS
    DEVICE_SOURCES ${srcs})
endif ()

fides_generate_export_header(fides)

if (FALSE) # XXX(kitware): use vtk_module system
target_compile_definitions(fides PRIVATE ${MPI_DEFS} ${ADIOS_DEFS})
if (VTKm_VERSION VERSION_EQUAL "1.9")
  set(VTKM_LIBS vtkm_cont)
elseif(VTKm_VERSION VERSION_GREATER_EQUAL "2.0")
  set(VTKM_LIBS vtkm::cont)
endif()
target_link_libraries(fides PUBLIC ${VTKM_LIBS} adios2::adios2 PRIVATE
  fides_rapidjson)

target_include_directories(fides PUBLIC
  $<BUILD_INTERFACE:${FIDES_SOURCE_DIR}>
  $<BUILD_INTERFACE:${FIDES_BINARY_DIR}/fides>
  $<INSTALL_INTERFACE:include>
  )
else()
if (${MPI_DEFS})
vtk_module_definitions(VTK::fides PRIVATE ${MPI_DEFS})
endif()
if (ADIOS_DEFS)
vtk_module_definitions(VTK::fides PRIVATE ${ADIOS_DEFS})
endif()
vtk_module_link(VTK::fides
  PRIVATE
    adios2::adios2
    fides_rapidjson)
vtk_module_include(VTK::fides
  PUBLIC
    $<BUILD_INTERFACE:${FIDES_SOURCE_DIR}>
    $<BUILD_INTERFACE:${FIDES_BINARY_DIR}/fides>
    $<INSTALL_INTERFACE:${_vtk_build_HEADERS_DESTINATION}/vtkfides/fides>
  )

include(GenerateExportHeader)
generate_export_header(fides
  EXPORT_MACRO_NAME VTKFIDES_EXPORT
  EXPORT_FILE_NAME vtkfides_export.h)
endif()

add_subdirectory(xgc)
add_subdirectory(predefined)

#-------------------------------------------------------------------------------
# generate the config file that includes the exports
if (FALSE) # XXX(kitware): VTK handles installation
include(CMakePackageConfigHelpers)
configure_package_config_file(${FIDES_SOURCE_DIR}/cmake/Config.cmake.in
  "${FIDES_BINARY_DIR}/FidesConfig.cmake"
  INSTALL_DESTINATION lib/cmake/fides
  NO_SET_AND_CHECK_MACRO
  NO_CHECK_REQUIRED_COMPONENTS_MACRO
  )

install(FILES "${FIDES_BINARY_DIR}/FidesConfig.cmake" DESTINATION lib/cmake/fides)

install(TARGETS fides DESTINATION lib
  EXPORT ${FIDES_EXPORT_NAME})

install(EXPORT ${FIDES_EXPORT_NAME} FILE FidesTargets.cmake DESTINATION lib/cmake/fides)

export(EXPORT ${FIDES_EXPORT_NAME}
  FILE FidesTargets.cmake
  )

install(FILES ${headers} ${CMAKE_CURRENT_BINARY_DIR}/fides_export.h
  DESTINATION ${FIDES_INSTALL_INCLUDE_DIR})
endif ()
