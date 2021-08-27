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
)

include(${FIDES_SOURCE_DIR}/cmake/fides_generate_export_header.cmake)

if (FALSE) # XXX(kitware): use vtk_module system
add_library(fides ${srcs})
else()
  vtk_module_add_module(VTK::fides
    SOURCES ${srcs}
    HEADERS ${headers}
    HEADERS_SUBDIR "vtkfides/fides")
endif()

fides_generate_export_header(fides)

target_compile_definitions(fides PRIVATE ${MPI_DEFS})

if (FALSE) # XXX(kitware): use vtk_module system
target_link_libraries(fides PUBLIC vtkm_cont adios2::adios2 PRIVATE
  fides_rapidjson)

target_include_directories(fides PUBLIC
  $<BUILD_INTERFACE:${FIDES_SOURCE_DIR}>
  $<BUILD_INTERFACE:${FIDES_BINARY_DIR}/fides>
  $<INSTALL_INTERFACE:include>
  )
else()
target_link_libraries(fides PUBLIC VTK::vtkm PRIVATE
  adios2::adios2 fides_rapidjson)
target_include_directories(fides PUBLIC
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
endif()
