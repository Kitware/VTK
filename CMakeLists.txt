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

set(SEACAS_HAVE_EXODUS ON)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/SEACASIoss_config.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/SEACASIoss_config.h")

set(classes
  Ioss_Assembly
  Ioss_Beam2
  Ioss_Beam3
  Ioss_Beam4
  Ioss_Blob
  Ioss_CommSet
  Ioss_CompositeVariableType
  Ioss_ConcreteVariableType
  Ioss_ConstructedVariableType
  Ioss_CoordinateFrame
  Ioss_DatabaseIO
  Ioss_Edge2
  Ioss_Edge2D2
  Ioss_Edge2D3
  Ioss_Edge3
  Ioss_Edge4
  Ioss_EdgeBlock
  Ioss_EdgeSet
  Ioss_ElementBlock
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
  Ioss_GetLongOpt
  Ioss_GroupingEntity
  Ioss_Hex16
  Ioss_Hex20
  Ioss_Hex27
  Ioss_Hex32
  Ioss_Hex64
  Ioss_Hex8
  Ioss_Initializer
  Ioss_IOFactory
  Ioss_Map
  Ioss_Node
  Ioss_NodeBlock
  Ioss_NodeSet
  Ioss_ParallelUtils
  Ioss_Property
  Ioss_PropertyManager
  Ioss_Pyramid13
  Ioss_Pyramid14
  Ioss_Pyramid18
  Ioss_Pyramid19
  Ioss_Pyramid5
  Ioss_Quad12
  Ioss_Quad16
  Ioss_Quad4
  Ioss_Quad6
  Ioss_Quad8
  Ioss_Quad9
  Ioss_Region
  Ioss_SerializeIO
  Ioss_Shell4
  Ioss_Shell8
  Ioss_Shell9
  Ioss_ShellLine2D2
  Ioss_ShellLine2D3
  Ioss_SideBlock
  Ioss_SideSet
  Ioss_SmartAssert
  Ioss_Sphere
  Ioss_Spring2
  Ioss_Spring3
  Ioss_StructuredBlock
  Ioss_Super
  Ioss_Tet10
  Ioss_Tet11
  Ioss_Tet14
  Ioss_Tet15
  Ioss_Tet16
  Ioss_Tet4
  Ioss_Tet40
  Ioss_Tet7
  Ioss_Tet8
  Ioss_Tracer
  Ioss_Transform
  Ioss_Tri13
  Ioss_Tri3
  Ioss_Tri4
  Ioss_Tri4a
  Ioss_Tri6
  Ioss_Tri7
  Ioss_Tri9
  Ioss_TriShell3
  Ioss_TriShell4
  Ioss_TriShell6
  Ioss_TriShell7
  Ioss_Unknown
  Ioss_Utils
  Ioss_VariableType
  Ioss_Wedge12
  Ioss_Wedge15
  Ioss_Wedge16
  Ioss_Wedge18
  Ioss_Wedge20
  Ioss_Wedge21
  Ioss_Wedge24
  Ioss_Wedge52
  Ioss_Wedge6
  Ioss_ZoneConnectivity
  tokenize

  exodus/Ioex_BaseDatabaseIO
  exodus/Ioex_DatabaseIO
  exodus/Ioex_Internals
  exodus/Ioex_IOFactory
  exodus/Ioex_SuperElement
  exodus/Ioex_Utils

  gen_struc/Iogs_DatabaseIO
  gen_struc/Iogs_GeneratedMesh

  generated/Iogn_DashSurfaceMesh
  generated/Iogn_DatabaseIO
  generated/Iogn_GeneratedMesh

  heartbeat/Iohb_DatabaseIO
  heartbeat/Iohb_Layout

  init/Ionit_Initializer

  transform/Iotr_Initializer
  transform/Iotr_MinMax
  transform/Iotr_Offset
  transform/Iotr_Offset3D
  transform/Iotr_Scale
  transform/Iotr_Scale3D
  transform/Iotr_Tensor
  transform/Iotr_VectorMagnitude)

if (SEACAS_HAVE_MPI)
  list(APPEND classes
    Ioss_Decomposition

    exodus/Ioex_DecompositionData
    exodus/Ioex_ParallelDatabaseIO)
endif ()

set(sources)
set(headers)
foreach (class IN LISTS classes)
  list(APPEND sources "${class}.C")
  list(APPEND headers "${class}.h")
endforeach ()

set(c_sources
  Ioss_Getline)

foreach (c_source IN LISTS c_sources)
  list(APPEND sources "${c_source}.c")
  list(APPEND headers "${c_source}.h")
endforeach ()

list(APPEND sources
  transform/Iotr_Factory.C)

list(APPEND headers
  catch.hpp
  Ioss_BoundingBox.h
  Ioss_CodeTypes.h
  Ioss_DataSize.h
  Ioss_DBUsage.h
  Ioss_Doxygen.h
  Ioss_ElementVariableType.h
  Ioss_EntityType.h
  Ioss_MeshCopyOptions.h
  Ioss_MeshType.h
  Ioss_NamedSuffixVariableType.h
  Ioss_NullEntity.h
  Ioss_ScopeGuard.h
  Ioss_Sort.h
  Ioss_StandardElementTypes.h
  Ioss_State.h
  Ioss_SubSystem.h
  Ioss_SurfaceSplit.h
  Ioss_Version.h

  fmt/chrono.h
  fmt/color.h
  fmt/compile.h
  fmt/core.h
  fmt/format-inl.h
  fmt/format.h
  fmt/locale.h
  fmt/os.h
  fmt/ostream.h
  fmt/posix.h
  fmt/printf.h
  fmt/ranges.h

  hash/bhopscotch_map.h
  hash/bhopscotch_set.h
  hash/hopscotch_growth_policy.h
  hash/hopscotch_hash.h
  hash/hopscotch_map.h
  hash/hopscotch_set.h
  hash/robin_growth_policy.h
  hash/robin_hash.h
  hash/robin_map.h
  hash/robin_set.h

  vtk_ioss_mangle.h)

vtk_module_add_module(VTK::ioss
  SOURCES ${sources}
  PRIVATE_HEADERS ${headers})
vtk_module_definitions(VTK::ioss
  PRIVATE
    NO_ZOLTAN_SUPPORT
    NO_PARMETIS_SUPPORT)
