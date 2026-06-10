
# Base Fides sources
set(fides_srcs
  internal/Array.cxx
  internal/CellGridAttribute.cxx
  internal/CellGridModel.cxx
  internal/CellSet.cxx
  internal/CoordinateSystem.cxx
  internal/DataModel.cxx
  internal/DataSetModel.cxx
  internal/DataWrapHelper.cxx
  internal/Field.cxx
  internal/OutputBuilder.cxx
  internal/ReadPlan.cxx
  internal/Value.cxx
  internal/predefined/DataModelHelperFunctions.cxx
  internal/predefined/DataModelFactory.cxx
  internal/predefined/InternalMetadataSource.cxx
  internal/predefined/PredefinedDataModel.cxx
  internal/predefined/SupportedDataModels.cxx
  ADIOSDataSource.cxx
  DataSetReader.cxx
  DataSourceFactory.cxx
  ExternalDataRegistry.cxx
  FidesDataSetWriter.cxx
  FidesTypes.cxx
  FidesWriter.cxx
  Keys.cxx
  MetaData.cxx
)

# These headers are private and will not be installed
set(private_headers
  internal/Array.h
  internal/CellGridAttribute.h
  internal/CellGridModel.h
  internal/CellSet.h
  internal/CoordinateSystem.h
  internal/DataModel.h
  internal/DataObjectModel.h
  internal/DataSetModel.h
  internal/DataWrapHelper.h
  internal/Field.h
  internal/MpiHelper.h
  internal/OutputBuilder.h
  internal/ReadPlan.h
  internal/Value.h
  internal/predefined/DataModelHelperFunctions.h
  internal/predefined/DataModelFactory.h
  internal/predefined/InternalMetadataSource.h
  internal/predefined/PredefinedDataModel.h
  internal/predefined/SupportedDataModels.h
)

if(FIDES_USE_VISKORES)
  list(APPEND fides_srcs
    viskores/FidesDataSetWriterViskores.cxx
    viskores/ViskoresBuilder.cxx
    xgc/XGCCommon.cxx
    DataSetWriter.cxx)
  list(APPEND private_headers
    viskores/FidesDataSetWriterViskores.h
    viskores/ViskoresBuilder.h
    xgc/XGCCommon.h )
endif()

if(FIDES_USE_VTK)
  list(APPEND fides_srcs vtk/VTKBuilder.cxx vtk/FidesDataSetWriterVTK.cxx)
  list(APPEND private_headers vtk/VTKBuilder.h vtk/FidesDataSetWriterVTK.h)
endif()

set(headers
  ADIOSDataSource.h
  Configure.h
  DataContainer.h
  DataSetReader.h
  DataSetWriter.h
  DataSource.h
  DataSourceFactory.h
  Deprecated.h
  ExternalDataRegistry.h
  FidesDataSetWriter.h
  FidesTypes.h
  FidesWriter.h
  Keys.h
  MetaData.h
  PartitionInfo.h
  fides_mpi.h
)

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/FidesOptions.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/FidesOptions.h
  @ONLY
)

if (FIDES_USE_CONDUIT)
  list(APPEND headers
    ConduitDataSource.h
  )
  list(APPEND fides_srcs
    ConduitDataSource.cxx
  )
endif()

if (FALSE) # XXX(kitware): use vtk_module system
# Tell CMake to keep external library paths (eg Conduit) in the RPATH upon install
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# The sole target
add_library(fides ${fides_srcs} ${private_headers})

# Uses GenerateExportHeader internally, and then makes sure the header can
# be included by ourselves and consumers
include(${FIDES_SOURCE_DIR}/cmake/fides_generate_export_header.cmake)
fides_generate_export_header(fides)
else()
  # Ensure VTK installs the configured options header
  list(APPEND headers "${CMAKE_CURRENT_BINARY_DIR}/FidesOptions.h")

  vtk_module_add_module(VTK::fides
    SOURCES ${fides_srcs} ${private_headers}
    HEADERS ${headers}
    HEADERS_SUBDIR "vtkfides/fides"
  )

  # Generate the export header using the mangled target
  include(GenerateExportHeader)
  generate_export_header(vtkfides
    BASE_NAME fides
    EXPORT_MACRO_NAME FIDES_EXPORT
    DEPRECATED_MACRO_NAME FIDES_UNUSED_DEPRECATED
    EXPORT_FILE_NAME "${CMAKE_CURRENT_BINARY_DIR}/fides_export.h"
  )

  # Manually install export header
  install(FILES "${CMAKE_CURRENT_BINARY_DIR}/fides_export.h"
    DESTINATION "${_vtk_build_HEADERS_DESTINATION}/vtkfides/fides"
    COMPONENT Development
  )
endif()

if (FALSE) # XXX(kitware): VTK handles linking/compiling settings natively
target_compile_definitions(fides PRIVATE ${ADIOS_DEFS})
target_link_libraries(fides PUBLIC adios2::adios2 PRIVATE fides_rapidjson)

if(FIDES_USE_MPI)
  # Fides directly includes <mpi.h>, so we need to explicitly link MPI to make sure
  # include directories are always available
  target_link_libraries(fides PUBLIC ${MPI_LIBS})
endif()

if (FIDES_USE_CONDUIT)
  target_link_libraries(fides PRIVATE conduit::conduit)
endif()

# Set up include dirs with appropriate visibility
target_include_directories(fides
  PUBLIC
    $<BUILD_INTERFACE:${FIDES_SOURCE_DIR}>
    $<BUILD_INTERFACE:${FIDES_BINARY_DIR}>
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/internal
    ${CMAKE_CURRENT_SOURCE_DIR}/internal/predefined
    $<$<BOOL:${FIDES_USE_VISKORES}>:${FIDES_SOURCE_DIR}/viskores>
    $<$<BOOL:${FIDES_USE_VISKORES}>:${FIDES_SOURCE_DIR}/xgc>
)

# Warn about undefined preprocessor macros, and optionally allow
# those warnings to become errors
target_compile_options(fides PRIVATE -Wundef)
option(FIDES_ENABLE_STRICT_WARNINGS "Treat warnings as errors" OFF)

if(FIDES_ENABLE_STRICT_WARNINGS)
  if(MSVC)
    # C4668 warns if an undefined macro is evaluated in an #if directive
    target_compile_options(fides PRIVATE /we4668)
  else()
    target_compile_options(fides PRIVATE -Werror=undef)
  endif()
endif()

# Backend specific settings
if(FIDES_USE_VISKORES)
  target_link_libraries(fides PRIVATE viskores::cont viskores::filter_clean_grid)

  viskores_add_target_information(fides
    DROP_UNUSED_SYMBOLS
    MODIFY_CUDA_FLAGS
    DEVICE_SOURCES ${fides_srcs}
  )
  if(Viskores_ENABLE_CUDA)
    set_target_properties(fides PROPERTIES
      CUDA_SEPARABLE_COMPILATION ON
      POSITION_INDEPENDENT_CODE ON
    )
  endif()
endif()

if(FIDES_USE_VTK)
  target_link_libraries(fides PRIVATE VTK::CommonCore VTK::CommonDataModel
    VTK::FiltersCellGrid VTK::IOCellGrid)
endif()
else()
  if (ADIOS_DEFS)
    vtk_module_definitions(VTK::fides PRIVATE ${ADIOS_DEFS})
  endif()

  # Flattened link list: Viskores and VTK are unconditionally required in this branch
  set(fides_private_links
    adios2::adios2
    fides_rapidjson
    VTK::vtkviskores_worklet
    VTK::CommonCore
    VTK::CommonDataModel
    VTK::FiltersCellGrid
    VTK::IOCellGrid
  )

  if (FIDES_USE_CONDUIT)
    list(APPEND fides_private_links VTK::conduit)
  endif()

  if (FIDES_USE_MPI)
    list(APPEND fides_private_links VTK::mpi)
  endif()

  vtk_module_link(VTK::fides PRIVATE ${fides_private_links})

  # Flattened include dirs: Viskores dirs are unconditionally included
  vtk_module_include(VTK::fides
    PUBLIC
      $<BUILD_INTERFACE:${FIDES_SOURCE_DIR}>
      $<BUILD_INTERFACE:${FIDES_BINARY_DIR}>
      $<INSTALL_INTERFACE:${_vtk_build_HEADERS_DESTINATION}/vtkfides>
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/internal
      ${CMAKE_CURRENT_SOURCE_DIR}/internal/predefined
      ${FIDES_SOURCE_DIR}/viskores
      ${FIDES_SOURCE_DIR}/xgc
  )

  # Viskores target info is unconditionally required
  viskores_add_target_information(vtkfides
    DROP_UNUSED_SYMBOLS
    MODIFY_CUDA_FLAGS
    DEVICE_SOURCES ${fides_srcs}
  )
endif()
if (FALSE) # XXX(kitware): VTK handles installation
# Packaging and installation logic
include(CMakePackageConfigHelpers)
configure_package_config_file(${FIDES_SOURCE_DIR}/cmake/Config.cmake.in
  "${FIDES_BINARY_DIR}/FidesConfig.cmake"
  INSTALL_DESTINATION lib/cmake/fides
  NO_SET_AND_CHECK_MACRO
)

install(FILES "${FIDES_BINARY_DIR}/FidesConfig.cmake" DESTINATION lib/cmake/fides)

install(TARGETS fides
  EXPORT ${FIDES_EXPORT_NAME}
  DESTINATION lib
)

install(EXPORT ${FIDES_EXPORT_NAME}
  NAMESPACE fides::
  FILE FidesTargets.cmake
  DESTINATION lib/cmake/fides
)

export(EXPORT ${FIDES_EXPORT_NAME}
  FILE FidesTargets.cmake
)

install(FILES
    ${headers}
    ${CMAKE_CURRENT_BINARY_DIR}/FidesOptions.h
  DESTINATION ${FIDES_INSTALL_INCLUDE_DIR}
)

# The following list of headers was made private by !199. Generate and install
# empty/error headers to prevent developers from wasting time trying to figure
# out why their build is broken. We only need to keep these around until our
# versioning rules allow us to remove them.
set(deprecated_headers
  Array.h
  CellSet.h
  CoordinateSystem.h
  DataModel.h
  Field.h
  Value.h
  predefined/DataModelFactory.h
  predefined/DataModelHelperFunctions.h
  predefined/InternalMetadataSource.h
  predefined/PredefinedDataModel.h
  predefined/SupportedDataModels.h
)

# Generate empty/error headers for each now-private header
foreach(dep_hdr IN LISTS deprecated_headers)
  set(gen_file_path "${CMAKE_CURRENT_BINARY_DIR}/deprecated_api/${dep_hdr}")
  file(WRITE "${gen_file_path}"
    "#error \"fides/${dep_hdr} is no longer part of the public API. It has been moved to fides/internal/ and is for internal use only. Please see changelog for details.\"\n"
  )
endforeach()

# Install the directory containing the generate empty/error headers,
# preserving the directory structure of the original headers
install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/deprecated_api/"
  DESTINATION ${FIDES_INSTALL_INCLUDE_DIR}
)
endif()
