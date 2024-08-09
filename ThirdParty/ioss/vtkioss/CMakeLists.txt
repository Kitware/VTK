set(SEACAS_HAVE_MPI OFF)

# XXX(kitware): MPI support needs to come from hdf5 and netcdf. When NetCDF has
# parallel support, exodus auto-detects it and enables Exodus parallel API that
# ioss must use. Checking this global-property helps use determine if NetCDF has
# parallel support.
get_property(vtk_netcdf_has_parallel GLOBAL PROPERTY "_vtk_netcdf_has_parallel")
if (vtk_netcdf_has_parallel)
  if (NOT TARGET VTK::mpi)
    message(FATAL_ERROR
      "An external MPI-aware NetCDF is requires that VTK be built with MPI "
      "support as well.")
  endif()
  set(SEACAS_HAVE_MPI ON)
endif ()

set(SEACAS_HAVE_CATALYST2 OFF)
if (TARGET VTK::catalyst)
  set(SEACAS_HAVE_CATALYST2 ON)
endif ()

set(SEACAS_HAVE_EXODUS ON)
set(SEACAS_HAVE_CGNS ON)
set(SEACAS_HAVE_EXONULL OFF)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/SEACASIoss_config.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/SEACASIoss_config.h")

set(classes
  Ioss_Assembly
  Ioss_Blob
  Ioss_ChainGenerator
  Ioss_CommSet
  Ioss_Compare
  Ioss_ComposedVariableType
  Ioss_CompositeVariableType
  Ioss_ConcreteVariableType
  Ioss_ConstructedVariableType
  Ioss_CoordinateFrame
  Ioss_CopyDatabase
  Ioss_DatabaseIO
  Ioss_EdgeBlock
  Ioss_EdgeSet
  Ioss_ElementBlock
  Ioss_ElementPermutation
  Ioss_ElementSet
  Ioss_ElementTopology
  Ioss_EntityBlock
  Ioss_EntitySet
  Ioss_FaceBlock
  Ioss_FaceGenerator
  Ioss_FaceSet
  Ioss_Field
  Ioss_FieldManager
  Ioss_FileInfo
  Ioss_Getline
  Ioss_GetLongOpt
  Ioss_Glob
  Ioss_GroupingEntity
  Ioss_Initializer
  Ioss_IOFactory
  Ioss_Map
  Ioss_MemoryUtils
  Ioss_NodeBlock
  Ioss_NodeSet
  Ioss_NullEntity
  Ioss_ParallelUtils
  Ioss_Property
  Ioss_PropertyManager
  Ioss_Region
  Ioss_SerializeIO
  Ioss_SideBlock
  Ioss_SideSet
  Ioss_SmartAssert
  Ioss_StructuredBlock
  Ioss_Tracer
  Ioss_Transform
  Ioss_TransformFactory
  Ioss_Utils
  Ioss_VariableType
  Ioss_ZoneConnectivity

  tokenize

  elements/Ioss_Beam2
  elements/Ioss_Beam3
  elements/Ioss_Beam4
  elements/Ioss_Edge2
  elements/Ioss_Edge2D2
  elements/Ioss_Edge2D3
  elements/Ioss_Edge3
  elements/Ioss_Edge4
  elements/Ioss_Hex8
  elements/Ioss_Hex9
  elements/Ioss_Hex16
  elements/Ioss_Hex20
  elements/Ioss_Hex27
  elements/Ioss_Hex32
  elements/Ioss_Hex64
  elements/Ioss_Node
  elements/Ioss_Pyramid5
  elements/Ioss_Pyramid13
  elements/Ioss_Pyramid14
  elements/Ioss_Pyramid18
  elements/Ioss_Pyramid19
  elements/Ioss_Quad4
  elements/Ioss_Quad6
  elements/Ioss_Quad8
  elements/Ioss_Quad9
  elements/Ioss_Quad12
  elements/Ioss_Quad16
  elements/Ioss_Shell4
  elements/Ioss_Shell8
  elements/Ioss_Shell9
  elements/Ioss_ShellLine2D2
  elements/Ioss_ShellLine2D3
  elements/Ioss_Sphere
  elements/Ioss_Spring2
  elements/Ioss_Spring3
  elements/Ioss_Super
  elements/Ioss_Tet4
  elements/Ioss_Tet7
  elements/Ioss_Tet8
  elements/Ioss_Tet10
  elements/Ioss_Tet11
  elements/Ioss_Tet14
  elements/Ioss_Tet15
  elements/Ioss_Tet16
  elements/Ioss_Tet40
  elements/Ioss_Tri3
  elements/Ioss_Tri4
  elements/Ioss_Tri4a
  elements/Ioss_Tri6
  elements/Ioss_Tri7
  elements/Ioss_Tri9
  elements/Ioss_Tri13
  elements/Ioss_TriShell3
  elements/Ioss_TriShell4
  elements/Ioss_TriShell6
  elements/Ioss_TriShell7
  elements/Ioss_Unknown
  elements/Ioss_Wedge6
  elements/Ioss_Wedge12
  elements/Ioss_Wedge15
  elements/Ioss_Wedge16
  elements/Ioss_Wedge18
  elements/Ioss_Wedge20
  elements/Ioss_Wedge21
  elements/Ioss_Wedge24
  elements/Ioss_Wedge52

  gen_struc/Iogs_DatabaseIO
  gen_struc/Iogs_GeneratedMesh

  generated/Iogn_DashSurfaceMesh
  generated/Iogn_DatabaseIO
  generated/Iogn_GeneratedMesh

  heartbeat/Iohb_DatabaseIO
  heartbeat/Iohb_Layout

  init/Ionit_Initializer

  null/Ionull_DatabaseIO
  null/Ionull_IOFactory

  transform/Iotr_Initializer
  transform/Iotr_MinMax
  transform/Iotr_Offset
  transform/Iotr_Offset3D
  transform/Iotr_Scale
  transform/Iotr_Scale3D
  transform/Iotr_Tensor
  transform/Iotr_VectorMagnitude)

if (SEACAS_HAVE_CATALYST2)
  list(APPEND classes
    catalyst/Iocatalyst_CatalystLogging
    catalyst/Iocatalyst_CatalystManager
    catalyst/Iocatalyst_DatabaseIO
    catalyst/Iocatalyst_Initializer
    catalyst/Iocatalyst_IOFactory)
endif ()

if (SEACAS_HAVE_CGNS)
  list(APPEND classes
    cgns/Iocgns_DatabaseIO
    cgns/Iocgns_Utils
    cgns/Iocgns_Initializer
    cgns/Iocgns_StructuredZoneData
    cgns/Iocgns_IOFactory)
endif ()

if (SEACAS_HAVE_EXODUS)
  list(APPEND classes
    exodus/Ioex_BaseDatabaseIO
    exodus/Ioex_DatabaseIO
    exodus/Ioex_Internals
    exodus/Ioex_IOFactory
    exodus/Ioex_SuperElement
    exodus/Ioex_Utils)
endif ()

if (SEACAS_HAVE_EXONULL)
  list(APPEND classes
    exonull/Ioexnl_BaseDatabaseIO
    exonull/Ioexnl_DatabaseIO
    exonull/Ioexnl_Internals
    exonull/Ioexnl_IOFactory
    exonull/Ioexnl_Utils)
endif ()

if (SEACAS_HAVE_MPI)
  list(APPEND classes
    Ioss_Decomposition
    Ioss_DecompositionUtils)
  if (SEACAS_HAVE_CGNS)
    list(APPEND classes
      cgns/Iocgns_DecompositionData
      cgns/Iocgns_ParallelDatabaseIO)
  endif ()
  if (SEACAS_HAVE_EXODUS)
    list(APPEND classes
      exodus/Ioex_DecompositionData
      exodus/Ioex_ParallelDatabaseIO)
  endif ()
  if (SEACAS_HAVE_EXONULL)
    list(APPEND classes
      exonull/Ioexnl_DecompositionData
      exonull/Ioexnl_ParallelDatabaseIO)
  endif ()
endif ()

set(sources)
set(headers)
foreach (class IN LISTS classes)
  list(APPEND sources "${class}.C")
  list(APPEND headers "${class}.h")
endforeach ()

list(APPEND headers
  bhopscotch_map.h
  bhopscotch_set.h

  hopscotch_growth_policy.h
  hopscotch_hash.h
  hopscotch_map.h
  hopscotch_set.h

  Ioss_BasisVariableType.h
  Ioss_BoundingBox.h
  Ioss_CodeTypes.h
  Ioss_DataPool.h
  Ioss_DataSize.h
  Ioss_DBUsage.h
  Ioss_Doxygen.h
  Ioss_ElementVariableType.h
  Ioss_EntityType.h
  Ioss_Enumerate.h
  Ioss_MeshCopyOptions.h
  Ioss_MeshType.h
  Ioss_NamedSuffixVariableType.h
  Ioss_QuadratureVariableType.h
  Ioss_ScopeGuard.h
  Ioss_Sort.h
  Ioss_StandardElementTypes.h
  Ioss_State.h
  Ioss_SubSystem.h
  Ioss_SurfaceSplit.h
  Ioss_Version.h

  pdqsort.h

  robin_growth_policy.h
  robin_hash.h
  robin_map.h
  robin_set.h

  "${CMAKE_CURRENT_BINARY_DIR}/ioss_export.h"
  iocatalyst_export.h
  iocgns_export.h
  ioex_export.h
  ioexnl_export.h
  iogn_export.h
  iogs_export.h
  iohb_export.h
  ionit_export.h
  ionull_export.h
  iotr_export.h

  "${CMAKE_CURRENT_BINARY_DIR}/SEACASIoss_config.h"

  vtk_ioss_mangle.h)

# This header gets flattened in the install. In order to support the same path
# as the install tree, copy it down to the same directory level, but in the
# build tree.
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/init/Ionit_Initializer.h"
  "${CMAKE_CURRENT_BINARY_DIR}/Ionit_Initializer.h"
  COPYONLY)

vtk_module_find_package(PACKAGE Threads)

vtk_module_add_module(VTK::ioss
  HEADER_DIRECTORIES
  SOURCES ${sources}
  HEADERS ${headers}
  HEADERS_SUBDIR "vtkioss")
vtk_module_include(VTK::ioss
  INTERFACE
    # IOSS includes its own headers with `<>`, so we need an explicit `-I` for
    # them to find each other.
    "$<INSTALL_INTERFACE:${_vtk_build_HEADERS_DESTINATION}/vtkioss>")
vtk_module_definitions(VTK::ioss
  PRIVATE
    NO_ZOLTAN_SUPPORT
    NO_PARMETIS_SUPPORT)
vtk_module_link(VTK::ioss
  PRIVATE
    Threads::Threads)
# for GetProcessMemoryInfo
if (MINGW)
  vtk_module_link(VTK::ioss PRIVATE psapi)
endif ()
vtk_module_include(VTK::ioss
  PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/elements>"
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/exodus>"
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/exonull>"
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/gen_struc>"
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/generated>"
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/heartbeat>"
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/init>"
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/null>"
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/transform>")

include(GenerateExportHeader)
generate_export_header(ioss
  EXPORT_MACRO_NAME IOSS_EXPORT
  EXPORT_FILE_NAME ioss_export.h)
