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
set(SEACAS_HAVE_CGNS ON)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/SEACASIoss_config.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/SEACASIoss_config.h")

set(classes
  Ioss_Assembly
  Ioss_Beam2
  Ioss_Beam3
  Ioss_Beam4
  Ioss_Blob
  Ioss_ChainGenerator
  Ioss_CommSet
  Ioss_Compare
  Ioss_CompositeVariableType
  Ioss_ConcreteVariableType
  Ioss_ConstructedVariableType
  Ioss_CoordinateFrame
  Ioss_CopyDatabase
  Ioss_DatabaseIO
  Ioss_Edge2
  Ioss_Edge2D2
  Ioss_Edge2D3
  Ioss_Edge3
  Ioss_Edge4
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
  Ioss_GroupingEntity
  Ioss_Hex16
  Ioss_Hex20
  Ioss_Hex27
  Ioss_Hex32
  Ioss_Hex64
  Ioss_Hex8
  Ioss_Hex9
  Ioss_Initializer
  Ioss_IOFactory
  Ioss_Map
  Ioss_NodeBlock
  Ioss_Node
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
  Ioss_Tet40
  Ioss_Tet4
  Ioss_Tet7
  Ioss_Tet8
  Ioss_Tracer
  Ioss_Transform
  Ioss_Tri13
  Ioss_Tri3
  Ioss_Tri4a
  Ioss_Tri4
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

  cgns/Iocgns_DatabaseIO
  cgns/Iocgns_Utils
  cgns/Iocgns_Initializer
  cgns/Iocgns_StructuredZoneData
  cgns/Iocgns_IOFactory

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
  transform/Iotr_Offset3D
  transform/Iotr_Offset
  transform/Iotr_Scale3D
  transform/Iotr_Scale
  transform/Iotr_Tensor
  transform/Iotr_VectorMagnitude)

if (SEACAS_HAVE_MPI)
  list(APPEND classes
    Ioss_Decomposition
    cgns/Iocgns_DecompositionData
    cgns/Iocgns_ParallelDatabaseIO
    exodus/Ioex_DecompositionData
    exodus/Ioex_ParallelDatabaseIO)
endif ()

set(sources)
set(headers)
foreach (class IN LISTS classes)
  list(APPEND sources "${class}.C")
  list(APPEND headers "${class}.h")
endforeach ()

list(APPEND sources
  transform/Iotr_Factory.C)

list(APPEND headers
  bhopscotch_map.h
  bhopscotch_set.h
  hopscotch_growth_policy.h
  hopscotch_hash.h
  hopscotch_map.h
  hopscotch_set.h
  Ioss_Assembly.h
  Ioss_Beam2.h
  Ioss_Beam3.h
  Ioss_Beam4.h
  Ioss_Blob.h
  Ioss_BoundingBox.h
  Ioss_CodeTypes.h
  Ioss_CommSet.h
  Ioss_Compare.h
  Ioss_CompositeVariableType.h
  Ioss_ConcreteVariableType.h
  Ioss_ConstructedVariableType.h
  Ioss_CoordinateFrame.h
  Ioss_CopyDatabase.h
  Ioss_DatabaseIO.h
  Ioss_DataPool.h
  Ioss_DataSize.h
  Ioss_DBUsage.h
  Ioss_Decomposition.h
  Ioss_Doxygen.h
  Ioss_Edge2D2.h
  Ioss_Edge2D3.h
  Ioss_Edge2.h
  Ioss_Edge3.h
  Ioss_Edge4.h
  Ioss_EdgeBlock.h
  Ioss_EdgeSet.h
  Ioss_ElementBlock.h
  Ioss_ElementSet.h
  Ioss_ElementTopology.h
  Ioss_ElementVariableType.h
  Ioss_EntityBlock.h
  Ioss_EntitySet.h
  Ioss_EntityType.h
  Ioss_FaceBlock.h
  Ioss_FaceGenerator.h
  Ioss_FaceSet.h
  Ioss_Field.h
  Ioss_FieldManager.h
  Ioss_FileInfo.h
  Ioss_Getline.h
  Ioss_GetLongOpt.h
  Ioss_Glob.h
  Ioss_GroupingEntity.h
  Ioss_Hex16.h
  Ioss_Hex20.h
  Ioss_Hex27.h
  Ioss_Hex32.h
  Ioss_Hex64.h
  Ioss_Hex8.h
  Ioss_Initializer.h
  Ioss_IOFactory.h
  Ioss_Map.h
  Ioss_MeshCopyOptions.h
  Ioss_MeshType.h
  Ioss_NamedSuffixVariableType.h
  Ioss_NodeBlock.h
  Ioss_Node.h
  Ioss_NodeSet.h
  Ioss_NullEntity.h
  Ioss_ParallelUtils.h
  Ioss_Property.h
  Ioss_PropertyManager.h
  Ioss_Pyramid13.h
  Ioss_Pyramid14.h
  Ioss_Pyramid18.h
  Ioss_Pyramid19.h
  Ioss_Pyramid5.h
  Ioss_Quad12.h
  Ioss_Quad16.h
  Ioss_Quad4.h
  Ioss_Quad6.h
  Ioss_Quad8.h
  Ioss_Quad9.h
  Ioss_Region.h
  Ioss_ScopeGuard.h
  Ioss_SerializeIO.h
  Ioss_Shell4.h
  Ioss_Shell8.h
  Ioss_Shell9.h
  Ioss_ShellLine2D2.h
  Ioss_ShellLine2D3.h
  Ioss_SideBlock.h
  Ioss_SideSet.h
  Ioss_SmartAssert.h
  Ioss_Sort.h
  Ioss_Sphere.h
  Ioss_Spring2.h
  Ioss_Spring3.h
  Ioss_StandardElementTypes.h
  Ioss_State.h
  Ioss_StructuredBlock.h
  Ioss_SubSystem.h
  Ioss_Super.h
  Ioss_SurfaceSplit.h
  Ioss_Tet10.h
  Ioss_Tet11.h
  Ioss_Tet14.h
  Ioss_Tet15.h
  Ioss_Tet16.h
  Ioss_Tet40.h
  Ioss_Tet4.h
  Ioss_Tet7.h
  Ioss_Tet8.h
  Ioss_Tracer.h
  Ioss_Transform.h
  Ioss_Tri13.h
  Ioss_Tri3.h
  Ioss_Tri4a.h
  Ioss_Tri4.h
  Ioss_Tri6.h
  Ioss_Tri7.h
  Ioss_Tri9.h
  Ioss_TriShell3.h
  Ioss_TriShell4.h
  Ioss_TriShell6.h
  Ioss_TriShell7.h
  Ioss_Unknown.h
  Ioss_Utils.h
  Ioss_VariableType.h
  Ioss_Version.h
  Ioss_Wedge12.h
  Ioss_Wedge15.h
  Ioss_Wedge16.h
  Ioss_Wedge18.h
  Ioss_Wedge20.h
  Ioss_Wedge21.h
  Ioss_Wedge24.h
  Ioss_Wedge52.h
  Ioss_Wedge6.h
  Ioss_ZoneConnectivity.h
  pdqsort.h
  robin_growth_policy.h
  robin_hash.h
  robin_map.h
  robin_set.h
  tokenize.h

  "${CMAKE_CURRENT_BINARY_DIR}/ioss_export.h"
  iocgns_export.h
  ioex_export.h
  iogn_export.h
  iogs_export.h
  iohb_export.h
  ionit_export.h
  iotr_export.h

  vtk_ioss_mangle.h)

vtk_module_find_package(PACKAGE Threads)

# This header gets flattened in the install. In order to support the same path
# as the install tree, copy it down to the same directory level, but in the
# build tree.
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/init/Ionit_Initializer.h"
  "${CMAKE_CURRENT_BINARY_DIR}/Ionit_Initializer.h"
  COPYONLY)

vtk_module_add_module(VTK::ioss
  SOURCES ${sources}
  PRIVATE_HEADERS ${headers})
vtk_module_definitions(VTK::ioss
  PRIVATE
    NO_ZOLTAN_SUPPORT
    NO_PARMETIS_SUPPORT)
vtk_module_link(VTK::ioss
  PRIVATE
    Threads::Threads)

include(GenerateExportHeader)
generate_export_header(ioss
  EXPORT_MACRO_NAME IOSS_EXPORT
  EXPORT_FILE_NAME ioss_export.h)

# for GetProcessMemoryInfo
if (MINGW)
  vtk_module_link(VTK::ioss PRIVATE psapi)
endif ()
