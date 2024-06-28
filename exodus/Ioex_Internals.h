/*
 * Copyright(C) 1999-2024 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */

#pragma once

#include <cstdint>
#include <vtk_exodusII.h>
#include <string>
#include <vector>

#include "Ioss_ParallelUtils.h"
#include "Ioss_Utils.h"
#include "ioex_export.h"
#include "vtk_ioss_mangle.h"

namespace Ioss {
  class Assembly;
  class Blob;
  class EdgeBlock;
  class EdgeSet;
  class ElementBlock;
  class ElementSet;
  class FaceBlock;
  class FaceSet;
  class NodeBlock;
  class NodeSet;
  class SideBlock;
  class SideSet;
  class Region;
} // namespace Ioss

using entity_id = int64_t;

namespace Ioss {
} // namespace Ioss
/*!
 * This set of classes provides a thin wrapper around the exodusII
 * internals.  It supplants several of the exodusII API calls in
 * order to avoid ncredef calls which totally rewrite the existing
 * database and can be very expensive.  These routines provide all
 * required variable, dimension, and attribute definitions to the
 * underlying netcdf file with only a single ncredef call.
 *
 * To use the application must create an Internals instance
 * and call the Internals::write_meta_data() function.  This
 * function requires several classes as arguments including:
 * <ul>
 * <li> Mesh -- defines mesh global metadata
 * <li> Block -- defines metadata for each block
 * <li> NodeSet -- defines metadata for each nodeset
 * <li> SideSet -- defines metadata for each sideset
 * <li> CommunicationMetaData -- global metadata relating to
 * parallel info.
 * </ul>
 *
 * Calling Internals::write_meta_data(), replaces the
 * following exodusII and nemesis API calls:
 * <ul>
 * <li> ex_put_init(),
 * <li> ex_put_elem_block(),
 * <li> ex_put_node_set_param(),
 * <li> ex_put_side_set_param(),
 * <li> ne_put_init_info(),
 * <li> ne_put_loadbal_param(),
 * <li> ne_put_cmap_params(),
 * </ul>
 */
namespace Ioex {
  struct IOEX_EXPORT NodeBlock
  {
    explicit NodeBlock(const Ioss::NodeBlock &other);

    std::string name{};
    entity_id   id{0};
    int64_t     entityCount{0};
    int64_t     localOwnedCount{0};
    int64_t     attributeCount{0};
    int64_t     procOffset{0};
  };

  struct IOEX_EXPORT Assembly
  {
    explicit Assembly(const Ioss::Assembly &other);

    std::string          name{};
    entity_id            id{0};
    int64_t              entityCount{0};
    int64_t              attributeCount{0};
    ex_entity_type       type{};
    std::vector<int64_t> memberIdList;
  };

  struct IOEX_EXPORT Blob
  {
    explicit Blob(const Ioss::Blob &other);

    std::string name{};
    entity_id   id{0};
    int64_t     entityCount{0};
    int64_t     localOwnedCount{0};
    int64_t     attributeCount{0};
    int64_t     procOffset{0};
  };

  struct IOEX_EXPORT EdgeBlock
  {
    EdgeBlock() { Ioss::Utils::copy_string(elType, ""); }

    EdgeBlock(const EdgeBlock &other)
        : name(other.name), id(other.id), entityCount(other.entityCount),
          nodesPerEntity(other.nodesPerEntity), attributeCount(other.attributeCount),
          procOffset(other.procOffset)
    {
      Ioss::Utils::copy_string(elType, other.elType);
    }

    explicit EdgeBlock(const Ioss::EdgeBlock &other);

    char        elType[MAX_STR_LENGTH + 1]{};
    std::string name{};
    entity_id   id{0};
    int64_t     entityCount{0};
    int64_t     nodesPerEntity{0};
    int64_t     attributeCount{0};
    int64_t     procOffset{0};

  private:
  };

  struct IOEX_EXPORT FaceBlock
  {
    FaceBlock() { Ioss::Utils::copy_string(elType, ""); }

    FaceBlock(const FaceBlock &other)
        : name(other.name), id(other.id), entityCount(other.entityCount),
          nodesPerEntity(other.nodesPerEntity), edgesPerEntity(other.edgesPerEntity),
          attributeCount(other.attributeCount), procOffset(other.procOffset)
    {
      Ioss::Utils::copy_string(elType, other.elType);
    }

    explicit FaceBlock(const Ioss::FaceBlock &other);

    char        elType[MAX_STR_LENGTH + 1]{};
    std::string name{};
    entity_id   id{0};
    int64_t     entityCount{0};
    int64_t     nodesPerEntity{0};
    int64_t     edgesPerEntity{0};
    int64_t     attributeCount{0};
    int64_t     procOffset{0};

  private:
  };

  struct IOEX_EXPORT ElemBlock
  {
    ElemBlock() { Ioss::Utils::copy_string(elType, ""); }

    ElemBlock(const ElemBlock &other)
        : name(other.name), id(other.id), entityCount(other.entityCount),
          globalEntityCount(other.globalEntityCount), nodesPerEntity(other.nodesPerEntity),
          edgesPerEntity(other.edgesPerEntity), facesPerEntity(other.facesPerEntity),
          attributeCount(other.attributeCount), offset_(other.offset_), procOffset(other.procOffset)
    {
      Ioss::Utils::copy_string(elType, other.elType);
    }

    explicit ElemBlock(const Ioss::ElementBlock &other);

    char        elType[MAX_STR_LENGTH + 1]{};
    std::string name{};
    entity_id   id{0};
    int64_t     entityCount{0};
    int64_t     globalEntityCount{0};
    int64_t     nodesPerEntity{0};
    int64_t     edgesPerEntity{0};
    int64_t     facesPerEntity{0};
    int64_t     attributeCount{0};
    int64_t     offset_{-1};
    int64_t     procOffset{0};
  };

  struct IOEX_EXPORT NodeSet
  {
    explicit NodeSet(const Ioss::NodeSet &other);

    std::string name{};
    entity_id   id{0};
    int64_t     entityCount{0};
    int64_t     globalEntityCount{0};
    int64_t     localOwnedCount{0};
    int64_t     attributeCount{0};
    int64_t     dfCount{0};
    int64_t     procOffset{0};
  };

  struct IOEX_EXPORT EdgeSet
  {
    explicit EdgeSet(const Ioss::EdgeSet &other);

    std::string name{};
    entity_id   id{0};
    int64_t     entityCount{0};
    int64_t     attributeCount{0};
    int64_t     dfCount{0};
    int64_t     procOffset{0};
  };

  struct IOEX_EXPORT FaceSet
  {
    explicit FaceSet(const Ioss::FaceSet &other);

    std::string name{};
    entity_id   id{0};
    int64_t     entityCount{0};
    int64_t     attributeCount{0};
    int64_t     dfCount{0};
    int64_t     procOffset{0};
  };

  struct IOEX_EXPORT ElemSet
  {
    explicit ElemSet(const Ioss::ElementSet &other);

    std::string name{};
    entity_id   id{0};
    int64_t     entityCount{0};
    int64_t     attributeCount{0};
    int64_t     dfCount{0};
    int64_t     procOffset{0};
  };

  struct IOEX_EXPORT SideSet
  {
    explicit SideSet(const Ioss::SideBlock &other);
    explicit SideSet(const Ioss::SideSet &other);

    std::string name{};
    entity_id   id{0};
    int64_t     entityCount{0};
    int64_t     globalEntityCount{0};
    int64_t     dfCount{0};
    int64_t     procOffset{0};
    int64_t     dfProcOffset{0};
  };

  struct IOEX_EXPORT CommunicationMap
  {
    CommunicationMap(entity_id the_id, int64_t count, char the_type)
        : id(the_id), entityCount(count), type(the_type)
    {
    }
    entity_id id{0};
    int64_t   entityCount{0};
    char      type{'U'}; // 'n' for node, 'e' for element
  };

  struct IOEX_EXPORT CommunicationMetaData
  {
    std::vector<CommunicationMap> nodeMap{};
    std::vector<CommunicationMap> elementMap{};
    int                           processorId{0};
    int                           processorCount{0};
    int64_t                       globalNodes{0};
    int64_t                       globalElements{0};
    int64_t                       globalElementBlocks{0};
    int64_t                       globalNodeSets{0};
    int64_t                       globalSideSets{0};
    int64_t                       nodesInternal{0};
    int64_t                       nodesBorder{0};
    int64_t                       nodesExternal{0};
    int64_t                       elementsInternal{0};
    int64_t                       elementsBorder{0};
    bool                          outputNemesis{false};
  };

  class IOEX_EXPORT Redefine
  {
  public:
    explicit Redefine(int exoid);
    Redefine(const Redefine &from)            = delete;
    Redefine &operator=(const Redefine &from) = delete;
    ~Redefine();

  private:
    int exodusFilePtr;
  };

  class IOEX_EXPORT Mesh
  {
  public:
    Mesh(int dim, const char *the_title, const Ioss::ParallelUtils &util, bool file_pp)
        : dimensionality(dim), file_per_processor(file_pp), parallelUtil(util)
    {
      Ioss::Utils::copy_string(title, the_title);
    }

    void populate(Ioss::Region *region);
    void get_global_counts();

    char title[MAX_LINE_LENGTH + 1]{};
    int  dimensionality{};
    bool file_per_processor{true};
    bool use_node_map{true};
    bool use_elem_map{true};
    bool use_face_map{true};
    bool use_edge_map{true};
    bool full_nemesis_data{true};

    std::vector<Assembly> assemblies{};
    std::vector<Blob>     blobs{};

    std::vector<NodeBlock> nodeblocks{};
    std::vector<EdgeBlock> edgeblocks{};
    std::vector<FaceBlock> faceblocks{};
    std::vector<ElemBlock> elemblocks{};
    std::vector<NodeSet>   nodesets{};
    std::vector<EdgeSet>   edgesets{};
    std::vector<FaceSet>   facesets{};
    std::vector<ElemSet>   elemsets{};
    std::vector<SideSet>   sidesets{};
    CommunicationMetaData  comm{};
    Ioss::ParallelUtils    parallelUtil;
  };

  class IOEX_EXPORT Internals
  {
  public:
    Internals(int exoid, int maximum_name_length, const Ioss::ParallelUtils &util);
    Internals(const Internals &from)            = delete;
    Internals &operator=(const Internals &from) = delete;

    int initialize_state_file(Mesh &mesh, const ex_var_params &var_params,
                              const std::string &base_filename);

    int write_meta_data(Mesh &mesh);

    /* Special use for updating assembly data in-place in existing db file */
    /* See src/main/io_assembly.C for current use */
    static void update_assembly_data(int exoid, std::vector<Assembly> &assemblies, int stage = 0);

    // Simple wrapper around `ex_copy`, but keeps users from including `exodusII.h`
    static void copy_database(int in_file, int out_file, bool transient_also = true);

  private:
    int put_metadata(const Mesh &mesh, const CommunicationMetaData &comm);
    int put_metadata(const std::vector<Assembly> &assemblies);
    int put_metadata(const std::vector<Blob> &blobs);
    int put_metadata(const std::vector<NodeBlock> &nodeblocks, bool count_only = false);
    int put_metadata(const std::vector<EdgeBlock> &blocks, bool count_only = false);
    int put_metadata(const std::vector<FaceBlock> &blocks, bool count_only = false);
    int put_metadata(const std::vector<ElemBlock> &blocks, bool count_only = false);

    int put_metadata(const std::vector<NodeSet> &nodesets, bool count_only = false);
    int put_metadata(const std::vector<EdgeSet> &edgesets, bool count_only = false);
    int put_metadata(const std::vector<FaceSet> &facesets, bool count_only = false);
    int put_metadata(const std::vector<ElemSet> &elemsets, bool count_only = false);

    int put_metadata(const std::vector<SideSet> &sidesets, bool count_only = false);

    int put_non_define_data(const CommunicationMetaData &comm, bool minimal_nemesis_data);
    int put_non_define_data(const std::vector<Assembly> &assemblies);
    int put_non_define_data(const std::vector<Blob> &blobs);
    int put_non_define_data(const std::vector<NodeBlock> &nodeblocks);
    int put_non_define_data(const std::vector<EdgeBlock> &blocks);
    int put_non_define_data(const std::vector<FaceBlock> &blocks);
    int put_non_define_data(const std::vector<ElemBlock> &blocks, bool output_global_data);

    int put_non_define_data(const std::vector<NodeSet> &nodesets, bool output_global_data);
    int put_non_define_data(const std::vector<EdgeSet> &edgesets);
    int put_non_define_data(const std::vector<FaceSet> &facesets);
    int put_non_define_data(const std::vector<ElemSet> &elemsets);

    int put_non_define_data(const std::vector<SideSet> &sidesets, bool output_global_data);

    int max_name_length() const { return maximumNameLength; }

    int                 exodusFilePtr{0};
    int                 nodeMapVarID[3]{};
    int                 elementMapVarID[2]{};
    int                 commIndexVar{0};
    int                 elemCommIndexVar{0};
    int                 maximumNameLength{32};
    Ioss::ParallelUtils parallelUtil;
  };
} // namespace Ioex
