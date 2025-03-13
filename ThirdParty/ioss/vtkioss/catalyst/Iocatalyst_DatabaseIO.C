// Copyright(C) 1999-2021, 2024 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include <cstddef>
#include <tokenize.h>

#include "Ioss_CodeTypes.h"       // for IOSS_SCALAR()
#include "Ioss_CommSet.h"         // for CommSet
#include "Ioss_DBUsage.h"         // for DatabaseUsage, etc
#include "Ioss_DatabaseIO.h"      // for DatabaseIO
#include "Ioss_EdgeBlock.h"       // for EdgeBlock
#include "Ioss_EdgeSet.h"         // for EdgeSet
#include "Ioss_ElementBlock.h"    // for ElementBlock
#include "Ioss_ElementSet.h"      // for ElementSet
#include "Ioss_EntityType.h"      // for EntityType::ELEMENTBLOCK
#include "Ioss_FaceBlock.h"       // for FaceBlock
#include "Ioss_FaceSet.h"         // for FaceSet
#include "Ioss_FileInfo.h"        // for FileInfo
#include "Ioss_Map.h"             // for Map, MapContainer
#include "Ioss_NodeBlock.h"       // for NodeBlock
#include "Ioss_NodeSet.h"         // for NodeSet
#include "Ioss_Property.h"        // for Property
#include "Ioss_SideBlock.h"       // for SideBlock
#include <Ioss_Assembly.h>        // for Assembly
#include <Ioss_Blob.h>            // for Blob
#include <Ioss_CodeTypes.h>       // for HAVE_MPI
#include <Ioss_ElementTopology.h> // for NameList
#include <Ioss_ParallelUtils.h>   // for ParallelUtils, etc
#include <Ioss_Region.h>          // for Region
#include <Ioss_SerializeIO.h>     // for SerializeIO
#include <Ioss_StructuredBlock.h> // for StructuredBlock

#include <Ioss_Utils.h> // for Utils, IOSS_ERROR, etc
#include <algorithm>
#include <climits>
#include <cstdlib>
#include "vtk_fmt.h"
#include VTK_FMT(fmt/ostream.h)
#include <map>

#include <catalyst.hpp>
#include <catalyst/Iocatalyst_DatabaseIO.h>

namespace Ioss {
  class PropertyManager;
}

namespace Iocatalyst {

  namespace detail {

    inline static const std::string ASSEMBLIES       = "assemblies";
    inline static const std::string BLOBS            = "blobs";
    inline static const std::string COMMSETS         = "commsets";
    inline static const std::string EDGEBLOCKS       = "edgeblocks";
    inline static const std::string EDGESETS         = "edgesets";
    inline static const std::string ELEMENTBLOCKS    = "elementblocks";
    inline static const std::string ELEMENTSETS      = "elementsets";
    inline static const std::string FACEBLOCKS       = "faceblocks";
    inline static const std::string FACESETS         = "facesets";
    inline static const std::string NODEBLOCKS       = "nodeblocks";
    inline static const std::string NODESETS         = "nodesets";
    inline static const std::string REGION           = "region";
    inline static const std::string SIDESETS         = "sidesets";
    inline static const std::string STRUCTUREDBLOCKS = "structuredblocks";

    inline static const std::string BLOCKLOCNODEIND    = "blocklocalnodeindex";
    inline static const std::string BOUNDARYCONDS      = "boundaryconditions";
    inline static const std::string CATCONDNODE        = "CATALYST_CONDUIT_NODE";
    inline static const std::string CATDUMPDIR         = "CATALYST_DATA_DUMP_DIRECTORY";
    inline static const std::string CATREADTIMESTEP    = "CATALYST_READER_TIME_STEP";
    inline static const std::string CELLIDS            = "cell_ids";
    inline static const std::string CELLNODEIDS        = "cell_node_ids";
    inline static const std::string COMPONENTCOUNT     = "component_count";
    inline static const std::string COMPONENTDEGREE    = "component_degree";
    inline static const std::string COUNT              = "count";
    inline static const std::string CONNECTIVITY       = "connectivity";
    inline static const std::string CONNECTIVITYRAW    = "connectivity_raw";
    inline static const char        DASH_CHAR          = '-';
    inline static const std::string DATABASE           = "database";
    inline static const std::string DOT                = ".";
    inline static const std::string ENTITYCOUNT        = "entity_count";
    inline static const std::string ENTITYTYPE         = "entity_type";
    inline static const std::string EXECUTE_INVC       = "execute_invc";
    inline static const std::string FIELDS             = "fields";
    inline static const std::string FS                 = "/";
    inline static const char        FS_CHAR            = '/';
    inline static const std::string GLOBALIDMAP        = "globalidmap";
    inline static const std::string KEY                = "key";
    inline static const std::string INTBYTESIZEAPI     = "int_byte_size_api";
    inline static const std::string IOSSCONTAINEDIN    = "IOSS_INTERNAL_CONTAINED_IN";
    inline static const std::string MEMBERS            = "members";
    inline static const std::string MEMBER_TYPE        = "member_type";
    inline static const std::string MESHMODCO          = "mesh_model_coordinates";
    inline static const std::string MESHMODCOX         = "mesh_model_coordinates_x";
    inline static const std::string MESHMODCOY         = "mesh_model_coordinates_y";
    inline static const std::string MESHMODCOZ         = "mesh_model_coordinates_z";
    inline static const std::string NAME               = "name";
    inline static const std::string NI                 = "ni";
    inline static const std::string NJ                 = "nj";
    inline static const std::string NK                 = "nk";
    inline static const std::string NIGLOBAL           = "ni_global";
    inline static const std::string NJGLOBAL           = "nj_global";
    inline static const std::string NKGLOBAL           = "nk_global";
    inline static const std::string NODEBLOCKONE       = "nodeblock_1";
    inline static const std::string IDS                = "ids";
    inline static const std::string INDEX              = "index";
    inline static const std::string OFFSET_I           = "offset_i";
    inline static const std::string OFFSET_J           = "offset_j";
    inline static const std::string OFFSET_K           = "offset_k";
    inline static const std::string ORIGIN             = "origin";
    inline static const std::string PARAMS_CONDUIT_BIN = "_params.conduit_bin.";
    inline static const std::string PARENTTOPOLOGYTYPE = "parent_topology_type";
    inline static const std::string PROPERTIES         = "properties";
    inline static const std::string REGION_ZERO        = "region_0";
    inline static const std::string ROLE               = "role";
    inline static const std::string SHALLOWCOPYFIELDS  = "SHALLOW_COPY_FIELDS";
    inline static const std::string SIDEBLOCKS         = "sideblocks";
    inline static const std::string STORAGE            = "storage";
    inline static const std::string TIME               = "time";
    inline static const std::string TOPOLOGYTYPE       = "topology_type";
    inline static const std::string TYPE               = "type";
    inline static const std::string VALUE              = "value";
    inline static const std::string ZONECONNECTIVITY   = "zoneconnectivity";

    inline static const std::string CONNECTIONNAME = "m_connectionName";
    inline static const std::string DONORNAME      = "m_donorName";
    inline static const std::string TRANSFORM      = "m_transform";
    inline static const std::string OWNERRANGEBEG  = "m_ownerRangeBeg";
    inline static const std::string OWNERRANGEEND  = "m_ownerRangeEnd";
    inline static const std::string OWNEROFFSET    = "m_ownerOffset";
    inline static const std::string DONORRANGEBEG  = "m_donorRangeBeg";
    inline static const std::string DONORRANGEEND  = "m_donorRangeEnd";
    inline static const std::string DONOROFFSET    = "m_donorOffset";
    inline static const std::string OWNERZONE      = "m_ownerZone";
    inline static const std::string DONORZONE      = "m_donorZone";
    inline static const std::string FROMDECOMP     = "m_fromDecomp";

    inline static const std::string BCNAME   = "m_bcName";
    inline static const std::string FAMNAME  = "m_famName";
    inline static const std::string RANGEBEG = "m_rangeBeg";
    inline static const std::string RANGEEND = "m_rangeEnd";
    inline static const std::string FACE     = "m_face";

    inline static const std::string SURFACESPLITTYPE = "surface_split_type";

    std::string getValuePath(const std::string &prop)
    {
      return detail::PROPERTIES + detail::FS + prop + detail::FS + detail::VALUE;
    }

    std::string getAPISizePath() { return detail::DATABASE + detail::FS + detail::INTBYTESIZEAPI; }

    template <typename GroupingEntityT>
    GroupingEntityT *createEntityGroup(const conduit_cpp::Node &node, Ioss::DatabaseIO *dbase);

    template <>
    Ioss::NodeBlock *createEntityGroup<Ioss::NodeBlock>(const conduit_cpp::Node &node,
                                                        Ioss::DatabaseIO        *dbase)
    {
      return new Ioss::NodeBlock(dbase, node[getValuePath(detail::NAME)].as_string(),
                                 node[getValuePath(detail::ENTITYCOUNT)].as_int64(),
                                 node[getValuePath(detail::COMPONENTDEGREE)].as_int64());
    }

    template <>
    Ioss::ElementBlock *createEntityGroup<Ioss::ElementBlock>(const conduit_cpp::Node &node,
                                                              Ioss::DatabaseIO        *dbase)
    {
      return new Ioss::ElementBlock(dbase, node[getValuePath(detail::NAME)].as_string(),
                                    node[getValuePath(detail::TOPOLOGYTYPE)].as_string(),
                                    node[getValuePath(detail::ENTITYCOUNT)].as_int64());
    }

    template <>
    Ioss::NodeSet *createEntityGroup<Ioss::NodeSet>(const conduit_cpp::Node &node,
                                                    Ioss::DatabaseIO        *dbase)
    {
      return new Ioss::NodeSet(dbase, node[getValuePath(detail::NAME)].as_string(),
                               node[getValuePath(detail::ENTITYCOUNT)].as_int64());
    }

    template <>
    Ioss::SideBlock *createEntityGroup<Ioss::SideBlock>(const conduit_cpp::Node &node,
                                                        Ioss::DatabaseIO        *dbase)
    {
      return new Ioss::SideBlock(dbase, node[getValuePath(detail::NAME)].as_string(),
                                 node[getValuePath(detail::TOPOLOGYTYPE)].as_string(),
                                 node[getValuePath(detail::PARENTTOPOLOGYTYPE)].as_string(),
                                 node[getValuePath(detail::ENTITYCOUNT)].as_int64());
    }

    template <>
    Ioss::SideSet *createEntityGroup<Ioss::SideSet>(const conduit_cpp::Node &node,
                                                    Ioss::DatabaseIO        *dbase)
    {
      return new Ioss::SideSet(dbase, node[getValuePath(detail::NAME)].as_string());
    }

    template <>
    Ioss::StructuredBlock *createEntityGroup<Ioss::StructuredBlock>(const conduit_cpp::Node &node,
                                                                    Ioss::DatabaseIO        *dbase)
    {
      Ioss::IJK_t localSizes    = {{(int)node[getValuePath(detail::NI)].as_int64(),
                                    (int)node[getValuePath(detail::NJ)].as_int64(),
                                    (int)node[getValuePath(detail::NK)].as_int64()}};
      Ioss::IJK_t globalSizes   = {{(int)node[getValuePath(detail::NIGLOBAL)].as_int64(),
                                    (int)node[getValuePath(detail::NJGLOBAL)].as_int64(),
                                    (int)node[getValuePath(detail::NKGLOBAL)].as_int64()}};
      Ioss::IJK_t parentOffsets = {{(int)node[getValuePath(detail::OFFSET_I)].as_int64(),
                                    (int)node[getValuePath(detail::OFFSET_J)].as_int64(),
                                    (int)node[getValuePath(detail::OFFSET_K)].as_int64()}};
      return new Ioss::StructuredBlock(dbase, node[getValuePath(detail::NAME)].as_string(),
                                       node[getValuePath(detail::COMPONENTDEGREE)].as_int64(),
                                       localSizes, parentOffsets, globalSizes);
    }

    template <>
    Ioss::Assembly *createEntityGroup<Ioss::Assembly>(const conduit_cpp::Node &node,
                                                      Ioss::DatabaseIO        *dbase)
    {
      return new Ioss::Assembly(dbase, node[getValuePath(detail::NAME)].as_string());
    }

    template <>
    Ioss::Blob *createEntityGroup<Ioss::Blob>(const conduit_cpp::Node &node,
                                              Ioss::DatabaseIO        *dbase)
    {
      return new Ioss::Blob(dbase, node[getValuePath(detail::NAME)].as_string(),
                            node[getValuePath(detail::ENTITYCOUNT)].as_int64());
    }

    template <>
    Ioss::EdgeBlock *createEntityGroup<Ioss::EdgeBlock>(const conduit_cpp::Node &node,
                                                        Ioss::DatabaseIO        *dbase)
    {
      return new Ioss::EdgeBlock(dbase, node[getValuePath(detail::NAME)].as_string(),
                                 node[getValuePath(detail::TOPOLOGYTYPE)].as_string(),
                                 node[getValuePath(detail::ENTITYCOUNT)].as_int64());
    }

    template <>
    Ioss::FaceBlock *createEntityGroup<Ioss::FaceBlock>(const conduit_cpp::Node &node,
                                                        Ioss::DatabaseIO        *dbase)
    {
      return new Ioss::FaceBlock(dbase, node[getValuePath(detail::NAME)].as_string(),
                                 node[getValuePath(detail::TOPOLOGYTYPE)].as_string(),
                                 node[getValuePath(detail::ENTITYCOUNT)].as_int64());
    }

    template <>
    Ioss::ElementSet *createEntityGroup<Ioss::ElementSet>(const conduit_cpp::Node &node,
                                                          Ioss::DatabaseIO        *dbase)
    {
      return new Ioss::ElementSet(dbase, node[getValuePath(detail::NAME)].as_string(),
                                  node[getValuePath(detail::ENTITYCOUNT)].as_int64());
    }

    template <>
    Ioss::EdgeSet *createEntityGroup<Ioss::EdgeSet>(const conduit_cpp::Node &node,
                                                    Ioss::DatabaseIO        *dbase)
    {
      return new Ioss::EdgeSet(dbase, node[getValuePath(detail::NAME)].as_string(),
                               node[getValuePath(detail::ENTITYCOUNT)].as_int64());
    }

    template <>
    Ioss::FaceSet *createEntityGroup<Ioss::FaceSet>(const conduit_cpp::Node &node,
                                                    Ioss::DatabaseIO        *dbase)
    {
      return new Ioss::FaceSet(dbase, node[getValuePath(detail::NAME)].as_string(),
                               node[getValuePath(detail::ENTITYCOUNT)].as_int64());
    }

    template <>
    Ioss::CommSet *createEntityGroup<Ioss::CommSet>(const conduit_cpp::Node &node,
                                                    Ioss::DatabaseIO        *dbase)
    {
      return new Ioss::CommSet(dbase, node[getValuePath(detail::NAME)].as_string(),
                               node[getValuePath(detail::ENTITYTYPE)].as_string(),
                               node[getValuePath(detail::ENTITYCOUNT)].as_int64());
    }

  } // namespace detail

  class DatabaseIO::ImplementationT
  {
    conduit_cpp::Node                        Root;
    conduit_cpp::Node                        DBNode;
    mutable Ioss::Map                        NodeMap;
    std::map<std::string, Ioss::SideBlock *> sideBlocks;
    char                                     read_db_field_separator;

  public:
    conduit_cpp::Node &databaseNode() { return this->DBNode; }
    void              *catalystConduitNode() { return conduit_cpp::c_node(&this->DBNode); }
    void               setDatabaseNode(conduit_node *c_node)
    {
      this->DBNode = conduit_cpp::Node();
      conduit_node_set_external_node(conduit_cpp::c_node(&this->DBNode), c_node);
    }
    conduit_cpp::Node &root() { return this->Root; }

    void print()
    {
      auto &node = this->DBNode;
      node.print_detailed();
    }

    bool defineModel(Ioss::Region *region)
    {
      if (!region->model_defined()) {
        std::ostringstream errmsg;
        errmsg << "Catalyst Write in defineModel(): model isn't defined in region"
               << "\n";
        IOSS_ERROR(errmsg);
      }

      auto &node = this->DBNode;
      node       = conduit_cpp::Node();

      node[detail::getAPISizePath()].set_int8(region->get_database()->int_byte_size_api());
      node[detail::SURFACESPLITTYPE].set_int8(region->get_database()->get_surface_split_type());
      RegionContainer rc;
      rc.push_back(region);
      this->defineEntityGroup(node[detail::REGION], rc);
      this->defineEntityGroup(node[detail::NODEBLOCKS], region->get_node_blocks());
      this->defineEntityGroup(node[detail::EDGEBLOCKS], region->get_edge_blocks());
      this->defineEntityGroup(node[detail::FACEBLOCKS], region->get_face_blocks());
      this->defineEntityGroup(node[detail::ELEMENTBLOCKS], region->get_element_blocks());
      this->defineEntityGroup(node[detail::SIDESETS], region->get_sidesets());
      this->defineEntityGroup(node[detail::NODESETS], region->get_nodesets());
      this->defineEntityGroup(node[detail::EDGESETS], region->get_edgesets());
      this->defineEntityGroup(node[detail::FACESETS], region->get_facesets());
      this->defineEntityGroup(node[detail::ELEMENTSETS], region->get_elementsets());
      this->defineEntityGroup(node[detail::STRUCTUREDBLOCKS], region->get_structured_blocks());
      this->defineEntityGroup(node[detail::ASSEMBLIES], region->get_assemblies());
      this->defineEntityGroup(node[detail::BLOBS], region->get_blobs());
      this->defineEntityGroup(node[detail::COMMSETS], region->get_commsets());
      return true;
    }

    void        readZoneConnectivity(conduit_cpp::Node &&parent, Ioss::StructuredBlock *sb);
    void        readBoundaryConditions(conduit_cpp::Node &&parent, Ioss::StructuredBlock *sb);
    Ioss::IJK_t readIJK(conduit_cpp::Node &&parent);

    bool readModel(Ioss::Region *region);

    bool readTime(Ioss::Region *region)
    {
      auto &node = this->DBNode;
      if (node.has_path(getTimePath())) {
        region->add_state(node[getTimePath()].as_float64());
      }
      return true;
    }

    std::vector<double> getTime()
    {
      std::vector<double> times;
      auto               &node = this->DBNode;
      if (node.has_path(getTimePath())) {
        times.push_back(node[getTimePath()].as_float64());
      }
      return times;
    }

    int64_t putField(const std::string &containerName, const Ioss::GroupingEntity *entityGroup,
                     const Ioss::Field &field, void *data, size_t data_size, bool deep_copy)
    {
      const auto groupName      = getName(entityGroup);
      const auto num_to_get     = field.verify(data_size);
      const auto num_components = field.raw_storage()->component_count();
      if (num_to_get > 0) {
        auto &&node = this->DBNode[getFieldPath(containerName, groupName, field.get_name())];
        node[detail::ROLE].set(static_cast<std::int8_t>(field.get_role()));
        node[detail::TYPE].set(static_cast<std::int8_t>(field.get_type()));
        node[detail::COUNT].set(static_cast<std::int64_t>(field.verify(data_size)));
        node[detail::INDEX].set(static_cast<std::int64_t>(field.get_index()));
        node[detail::COMPONENTCOUNT].set(static_cast<std::int64_t>(num_components));
        node[detail::STORAGE].set(field.raw_storage()->name());
        switch (field.get_type()) {
        case Ioss::Field::BasicType::DOUBLE:
          if (deep_copy) {
            node[detail::VALUE].set(static_cast<double *>(data), num_to_get * num_components);
          }
          else {
            node[detail::VALUE].set_external(static_cast<double *>(data),
                                             num_to_get * num_components);
          }
          break;
        case Ioss::Field::BasicType::INT32:
          if (deep_copy) {
            node[detail::VALUE].set(static_cast<std::int32_t *>(data), num_to_get * num_components);
          }
          else {
            node[detail::VALUE].set_external(static_cast<std::int32_t *>(data),
                                             num_to_get * num_components);
          }
          break;
        case Ioss::Field::BasicType::INT64:
          if (deep_copy) {
            node[detail::VALUE].set(static_cast<std::int64_t *>(data), num_to_get * num_components);
          }
          else {
            node[detail::VALUE].set_external(static_cast<std::int64_t *>(data),
                                             num_to_get * num_components);
          }
          break;
        case Ioss::Field::BasicType::CHARACTER:
          if (deep_copy) {
            node[detail::VALUE].set(static_cast<std::int8_t *>(data), num_to_get * num_components);
          }
          else {
            node[detail::VALUE].set_external(static_cast<std::int8_t *>(data),
                                             num_to_get * num_components);
          }
          break;
        default:
          std::ostringstream errmsg;
          fmt::print(errmsg, "ERROR in {} {}: {} ({}), unsupported field type: {}\n", __func__,
                     containerName, field.get_name(), num_to_get, field.type_string());
          IOSS_ERROR(errmsg);
        }
      }
      return num_to_get;
    }

    int64_t getField(const std::string &containerName, const Ioss::GroupingEntity *entityGroup,
                     const Ioss::Field &field, void *data, size_t data_size)
    {
      const auto groupName      = getName(entityGroup);
      auto       num_to_get     = field.verify(data_size);
      const auto num_components = field.raw_storage()->component_count();
      if (num_to_get > 0) {
        auto path =
            getFieldPath(containerName, groupName, field.get_name()) + detail::FS + detail::VALUE;
        const auto &&node = this->DBNode[path];
        switch (field.get_type()) {
        case Ioss::Field::BasicType::DOUBLE:
          std::copy_n(reinterpret_cast<const double *>(node.element_ptr(0)),
                      num_to_get * num_components, reinterpret_cast<double *>(data));
          break;

        case Ioss::Field::BasicType::INT32:
          std::copy_n(reinterpret_cast<const int32_t *>(node.element_ptr(0)),
                      num_to_get * num_components, reinterpret_cast<int32_t *>(data));
          break;

        case Ioss::Field::BasicType::INT64:
          std::copy_n(reinterpret_cast<const int64_t *>(node.element_ptr(0)),
                      num_to_get * num_components, reinterpret_cast<int64_t *>(data));
          break;

        case Ioss::Field::BasicType::CHARACTER:
          std::copy_n(reinterpret_cast<const char *>(node.element_ptr(0)),
                      num_to_get * num_components, reinterpret_cast<char *>(data));
          break;
        default:
          std::ostringstream errmsg;
          fmt::print(errmsg, "ERROR in {} {}: {} ({}), unsupported field type: {}\n", __func__,
                     containerName, field.get_name(), num_to_get, field.type_string());
          IOSS_ERROR(errmsg);
        }
      }
      return num_to_get;
    }

    int64_t getFieldZeroCopy(const std::string          &containerName,
                             const Ioss::GroupingEntity *entityGroup, const Ioss::Field &field,
                             void **data, size_t *data_size)

    {
      *data      = nullptr;
      *data_size = 0;
      if (!hasField(containerName, entityGroup, field.get_name())) {
        fmt::print(Ioss::OUTPUT(), "WARNING in {} : {}\n", __func__,
                   "field not available, " + field.get_name() + ", in container " + containerName +
                       "\n");
        return -1;
      }

      const auto groupName      = getName(entityGroup);
      auto       num_to_get     = field.verify(0);
      const auto num_components = field.raw_storage()->component_count();
      if (num_to_get > 0) {
        auto path =
            getFieldPath(containerName, groupName, field.get_name()) + detail::FS + detail::VALUE;

        const auto &&node = this->DBNode[path];
        *data_size        = num_to_get * num_components;
        switch (field.get_type()) {
        case Ioss::Field::BasicType::DOUBLE:
          *data = const_cast<double *>(node.as_double_ptr());
          break;

        case Ioss::Field::BasicType::INT32:
          *data = const_cast<int32_t *>(node.as_int32_ptr());
          break;

        case Ioss::Field::BasicType::INT64:
          *data = const_cast<int64_t *>(node.as_int64_ptr());
          break;

        case Ioss::Field::BasicType::CHARACTER:
          *data = const_cast<char *>(node.as_char_ptr());
          break;
        default:
          std::ostringstream errmsg;
          fmt::print(errmsg, "ERROR in {} {}: {} ({}), unsupported field type: {}\n", __func__,
                     containerName, field.get_name(), num_to_get, field.type_string());
          IOSS_ERROR(errmsg);
        }
      }
      return num_to_get;
    }

    int64_t getMeshModelCoordinates(const std::string          &containerName,
                                    const Ioss::GroupingEntity *entityGroup,
                                    const Ioss::Field &field, void *data, size_t data_size)
    {
      const auto groupName      = getName(entityGroup);
      auto       num_to_get     = field.verify(data_size);
      const auto num_components = field.raw_storage()->component_count();
      if (num_to_get > 0) {
        auto path = getPropertyPath(containerName, groupName, detail::COMPONENTDEGREE) +
                    detail::FS + detail::VALUE;
        int64_t component_degree = this->DBNode[path].as_int64();
        double *rdata            = static_cast<double *>(data);

        auto coord_lambda = [&](const std::string &coord_name, int ordinal) {
          path = getFieldPath(containerName, groupName, coord_name) + detail::FS + detail::COUNT;
          int64_t count = this->DBNode[path].as_int64();

          path = getFieldPath(containerName, groupName, coord_name) + detail::FS + detail::VALUE;
          const double *mesh_coords =
              reinterpret_cast<const double *>(this->DBNode[path].element_ptr(0));

          for (size_t i = 0; i < count; i++) {
            rdata[component_degree * i + ordinal] = mesh_coords[i];
          }
        };

        coord_lambda(detail::MESHMODCOX, 0);

        if (component_degree >= 2) {
          coord_lambda(detail::MESHMODCOY, 1);
        }

        if (component_degree == 3) {
          coord_lambda(detail::MESHMODCOZ, 2);
        }
      }
      return num_to_get;
    }

    bool hasField(const std::string &containerName, const Ioss::GroupingEntity *entityGroup,
                  const std::string &fieldName)
    {
      const auto groupName = getName(entityGroup);
      return this->DBNode.has_path(getFieldPath(containerName, groupName, fieldName));
    }

    std::string getFieldPath(const std::string &containerName, const std::string &groupName,
                             const std::string &fieldName)
    {
      return containerName + detail::FS + groupName + detail::FS + detail::FIELDS + detail::FS +
             fieldName;
    }

    bool hasProperty(const std::string &containerName, const Ioss::GroupingEntity *entityGroup,
                     const std::string &propertyName)
    {
      const auto groupName = getName(entityGroup);
      return this->DBNode.has_path(getPropertyPath(containerName, groupName, propertyName));
    }

    std::string getPropertyPath(const std::string &containerName, const std::string &groupName,
                                const std::string &propertyName)
    {
      return containerName + detail::FS + groupName + detail::FS + detail::PROPERTIES + detail::FS +
             propertyName;
    }

    std::string getName(const Ioss::GroupingEntity *entityGroup)
    {
      std::string retVal = entityGroup->name();
      if (dynamic_cast<const Ioss::Region *>(entityGroup) != nullptr) {
        retVal = detail::REGION_ZERO;
      }
      else if (retVal.empty()) {
        retVal = entityGroup->generic_name();
      }
      std::replace(retVal.begin(), retVal.end(), detail::FS_CHAR, detail::DASH_CHAR);
      return retVal;
    }

    std::string getTimePath() { return detail::DATABASE + detail::FS + detail::TIME; }

    int64_t getStructuredBlockIDS(const Ioss::StructuredBlock *sb, const Ioss::Field &field,
                                  void *data, size_t data_size)
    {
      auto num_to_get = field.verify(data_size);

      if (num_to_get > 0) {
        switch (field.get_type()) {
        case Ioss::Field::BasicType::INT32:
          copyIDS(sb, field, reinterpret_cast<int32_t *>(data));
          break;

        case Ioss::Field::BasicType::INT64:
          copyIDS(sb, field, reinterpret_cast<int64_t *>(data));
          break;
        default:
          std::ostringstream errmsg;
          fmt::print(errmsg, "ERROR in {}: {} ({}), unsupported field type: {}\n", __func__,
                     field.get_name(), num_to_get, field.type_string());
          IOSS_ERROR(errmsg);
        }
      }
      return num_to_get;
    }

    template <typename INT_t>
    void copyIDS(const Ioss::StructuredBlock *sb, const Ioss::Field &field, INT_t *data)
    {
      if (field.get_name() == detail::CELLIDS) {
        sb->get_cell_ids(data, true);
      }
      else {
        sb->get_cell_node_ids(data, true);
      }
    }

    Ioss::Map &get_node_map(const Ioss::DatabaseIO *dbase) const
    {
      if (this->NodeMap.defined()) {
        return this->NodeMap;
      }

      auto nbone_path = detail::NODEBLOCKS + detail::FS + detail::NODEBLOCKONE + detail::FS +
                        detail::FIELDS + detail::FS + detail::IDS;
      auto &&idsNode  = this->DBNode[nbone_path];
      auto   node_ids = const_cast<void *>(idsNode[detail::VALUE].element_ptr(0));
      this->NodeMap.set_size(idsNode[detail::COUNT].as_int64());
      if (idsNode[detail::TYPE].as_int8() == Ioss::Field::BasicType::INT32) {
        this->NodeMap.set_map(reinterpret_cast<int32_t *>(node_ids),
                              idsNode[detail::COUNT].as_int64(), 0);
      }
      if (idsNode[detail::TYPE].as_int8() == Ioss::Field::BasicType::INT64) {
        this->NodeMap.set_map(reinterpret_cast<int64_t *>(node_ids),
                              idsNode[detail::COUNT].as_int64(), 0);
      }

      this->NodeMap.set_defined(true);
      return this->NodeMap;
    }

  private:
    template <typename GroupingEntityT>
    bool defineEntityGroup(conduit_cpp::Node                     parent,
                           const std::vector<GroupingEntityT *> &container)
    {
      for (auto group : container) {
        this->addProperties(parent[getName(group)], group);
      }
      return true;
    }

    bool defineEntityGroup(conduit_cpp::Node parent, const Ioss::SideSetContainer &container)
    {
      for (auto group : container) {
        this->addProperties(parent[getName(group)], group);
        for (auto sb : group->get_side_blocks()) {
          parent[getName(group) + detail::FS + detail::SIDEBLOCKS].append().set(sb->name());
        }
        auto &node = this->DBNode;
        this->defineEntityGroup(node[detail::SIDEBLOCKS], group->get_side_blocks());
      }
      return true;
    }

    bool defineEntityGroup(conduit_cpp::Node parent, const Ioss::AssemblyContainer &container)
    {
      for (auto group : container) {
        this->addProperties(parent[getName(group)], group);
        parent[getName(group) + detail::FS + detail::MEMBER_TYPE].set(group->get_member_type());
        for (auto as : group->get_members()) {
          parent[getName(group) + detail::FS + detail::MEMBERS].append().set(as->name());
        }
      }
      return true;
    }

    bool defineEntityGroup(conduit_cpp::Node                     parent,
                           const Ioss::StructuredBlockContainer &container)
    {
      for (auto group : container) {
        this->addProperties(parent[getName(group)], group);
        conduit_cpp::Node n;
        for (auto zc : group->m_zoneConnectivity) {
          defineZoneConnectivity(n, zc);
        }
        parent[getName(group) + detail::FS + detail::ZONECONNECTIVITY].set(n);

        n.set_node(conduit_cpp::Node());
        for (auto bc : group->m_boundaryConditions) {
          defineBoundaryCondition(n, bc);
        }
        parent[getName(group) + detail::FS + detail::BOUNDARYCONDS].set(n);

        parent[getName(group) + detail::FS + detail::BLOCKLOCNODEIND].set(
            group->m_blockLocalNodeIndex);

        n.set_node(conduit_cpp::Node());
        for (auto gm : group->m_globalIdMap) {
          conduit_cpp::Node m;
          m[detail::KEY]   = gm.first;
          m[detail::VALUE] = gm.second;
          m.append().set(n);
        }
        parent[getName(group) + detail::FS + detail::GLOBALIDMAP].set(n);
      }
      return true;
    }

    void defineZoneConnectivity(conduit_cpp::Node parent, Ioss::ZoneConnectivity &zc)
    {
      conduit_cpp::Node n;
      n[detail::CONNECTIONNAME] = zc.m_connectionName;
      n[detail::DONORNAME]      = zc.m_donorName;
      n[detail::TRANSFORM]      = defineIJK(zc.m_transform);
      n[detail::OWNERRANGEBEG]  = defineIJK(zc.m_ownerRangeBeg);
      n[detail::OWNERRANGEEND]  = defineIJK(zc.m_ownerRangeEnd);
      n[detail::OWNEROFFSET]    = defineIJK(zc.m_ownerOffset);
      n[detail::DONORRANGEBEG]  = defineIJK(zc.m_donorRangeBeg);
      n[detail::DONORRANGEEND]  = defineIJK(zc.m_donorRangeEnd);
      n[detail::DONOROFFSET]    = defineIJK(zc.m_donorOffset);
      n[detail::OWNERZONE]      = zc.m_ownerZone;
      n[detail::DONORZONE]      = zc.m_donorZone;
      n[detail::FROMDECOMP]     = zc.m_fromDecomp;
      parent.append().set(n);
    }

    void defineBoundaryCondition(conduit_cpp::Node parent, Ioss::BoundaryCondition &bc)
    {
      conduit_cpp::Node n;
      n[detail::BCNAME]   = bc.m_bcName;
      n[detail::FAMNAME]  = bc.m_famName;
      n[detail::RANGEBEG] = defineIJK(bc.m_rangeBeg);
      n[detail::RANGEEND] = defineIJK(bc.m_rangeEnd);
      n[detail::FACE]     = bc.m_face;
      parent.append().set(n);
    }

    conduit_cpp::Node defineIJK(Ioss::IJK_t &a)
    {
      conduit_cpp::Node n;
      for (auto v : a) {
        n.append().set(v);
      }
      return n;
    }

    template <typename GroupingEntityT>
    bool addProperties(conduit_cpp::Node parent, GroupingEntityT *entityGroup)
    {
      Ioss::NameList names;
      // skip implicit properties.
      entityGroup->property_describe(Ioss::Property::INTERNAL, &names);
      entityGroup->property_describe(Ioss::Property::EXTERNAL, &names);
      entityGroup->property_describe(Ioss::Property::ATTRIBUTE, &names);
      entityGroup->property_describe(Ioss::Property::IMPLICIT, &names);

      auto &&propertiesNode = parent[detail::PROPERTIES];
      for (const auto &name : names) {
        auto property = entityGroup->get_property(name);

        auto &&node = propertiesNode[name];
        node[detail::TYPE].set(static_cast<std::int8_t>(property.get_type()));
        node[detail::ORIGIN].set(static_cast<std::int8_t>(property.get_origin()));
        switch (property.get_type()) {
        case Ioss::Property::BasicType::REAL: node[detail::VALUE].set(property.get_real()); break;

        case Ioss::Property::BasicType::INTEGER: node[detail::VALUE].set(property.get_int()); break;

        case Ioss::Property::BasicType::STRING:
          node[detail::VALUE].set(property.get_string());
          break;

        case Ioss::Property::BasicType::VEC_INTEGER:
          node[detail::VALUE].set(property.get_vec_int());
          break;

        case Ioss::Property::BasicType::VEC_DOUBLE:
          node[detail::VALUE].set(property.get_vec_double());
          break;

        case Ioss::Property::BasicType::POINTER:
        case Ioss::Property::BasicType::INVALID:
        default: return false;
        }
      }
      return true;
    }

    template <typename GroupingEntityT>
    bool readEntityGroup(conduit_cpp::Node &&parent, Ioss::Region *region)
    {
      for (conduit_index_t idx = 0, max = parent.number_of_children(); idx < max; ++idx) {
        auto &&child = parent[idx];
        auto   block = detail::createEntityGroup<GroupingEntityT>(child, region->get_database());
        region->add(block);
        this->readProperties(child[detail::PROPERTIES], block);

        // read fields (meta-data only)
        this->readFields(child[detail::FIELDS], block);
      }
      return true;
    }

    template <typename GroupingEntityT>
    bool readProperties(const conduit_cpp::Node &&parent, GroupingEntityT *block) const
    {
      for (conduit_index_t idx = 0, max = parent.number_of_children(); idx < max; ++idx) {
        auto     &&child  = parent[idx];
        const auto name   = child.name();
        const auto origin = static_cast<Ioss::Property::Origin>(child[detail::ORIGIN].as_int8());
        if (block->property_exists(name) && block->get_property(name).is_implicit()) {
          continue;
        }
        switch (child[detail::TYPE].as_int8()) {
        // TODO: missing origin
        case Ioss::Property::BasicType::REAL:
          block->property_add(Ioss::Property(name, child[detail::VALUE].as_float64(), origin));
          break;

        case Ioss::Property::BasicType::INTEGER:
          block->property_add(Ioss::Property(name, child[detail::VALUE].as_int64(), origin));
          break;

        case Ioss::Property::BasicType::STRING:
          block->property_add(Ioss::Property(name, child[detail::VALUE].as_string(), origin));
          break;

        case Ioss::Property::BasicType::VEC_INTEGER: {
          std::vector<int> v(child[detail::VALUE].as_int_ptr(),
                             child[detail::VALUE].as_int_ptr() +
                                 child[detail::VALUE].number_of_children());
          block->property_add(Ioss::Property(name, v, origin));
          break;
        }

        case Ioss::Property::BasicType::VEC_DOUBLE: {
          std::vector<double> v(child[detail::VALUE].as_double_ptr(),
                                child[detail::VALUE].as_double_ptr() +
                                    child[detail::VALUE].number_of_children());
          block->property_add(Ioss::Property(name, v, origin));
          break;
        }
        }
      }
      return true;
    }

    std::vector<std::string> getScalarNamesFromNonScalarField(const Ioss::Field &field) const
    {
      int                      ncomp = field.get_component_count(Ioss::Field::InOut::INPUT);
      std::vector<std::string> fnames;
      for (int i = 1; i <= ncomp; i++) {
        if (read_db_field_separator == '\0') {
          fnames.push_back(
              field.get_component_name(i, Ioss::Field::InOut::INPUT, read_db_field_separator));
        }
        else {
          fnames.push_back(field.get_component_name(i, Ioss::Field::InOut::INPUT));
        }
      }
      return fnames;
    }

    template <typename T>
    std::vector<T> getInterweavedScalarDataFromConduitNode(const std::vector<std::string> fnames,
                                                           conduit_cpp::Node &node) const
    {
      int            ncomp   = fnames.size();
      auto         &&t_node  = node[fnames[0] + detail::FS + detail::VALUE];
      int            num_get = t_node.number_of_elements();
      std::vector<T> vals(ncomp * num_get);

      for (int i = 0; i < num_get; i++) {
        for (int j = 0; j < ncomp; j++) {
          std::string path_to_value = fnames[j] + detail::FS + detail::VALUE;
          auto      &&child_value   = node[path_to_value];
          vals[i * ncomp + j]       = reinterpret_cast<const T *>(child_value.element_ptr(0))[i];
        }
      }
      return vals;
    }

    template <typename T>
    void addFieldNodeAndDataToConduitNode(const Ioss::Field &field, void *data,
                                          conduit_cpp::Node &node) const
    {
      int ncomp              = field.get_component_count(Ioss::Field::InOut::INPUT);
      int num_get            = field.raw_count();
      node[field.get_name()] = conduit_cpp::Node();
      auto &&f_node          = node[field.get_name()];
      f_node[detail::ROLE].set(static_cast<std::int8_t>(field.get_role()));
      f_node[detail::TYPE].set(static_cast<std::int8_t>(field.get_type()));
      f_node[detail::COUNT].set(static_cast<std::int64_t>(field.raw_count()));
      f_node[detail::INDEX].set(static_cast<std::int64_t>(field.get_index()));
      f_node[detail::COMPONENTCOUNT].set(static_cast<std::int64_t>(ncomp));
      f_node[detail::STORAGE].set(field.raw_storage()->name());
      f_node[detail::VALUE].set(static_cast<T *>(data), ncomp * num_get);
    }

    void combineScalarFieldsInConduitNodeToNonScalarField(const Ioss::Field &field,
                                                          conduit_cpp::Node &node) const
    {
      std::vector<std::string> fnames = this->getScalarNamesFromNonScalarField(field);

      switch (field.get_type()) {
      case Ioss::Field::BasicType::DOUBLE:
        this->addFieldNodeAndDataToConduitNode<double>(
            field, this->getInterweavedScalarDataFromConduitNode<double>(fnames, node).data(),
            node);
        break;

      case Ioss::Field::BasicType::INT32:
        this->addFieldNodeAndDataToConduitNode<std::int32_t>(
            field, this->getInterweavedScalarDataFromConduitNode<std::int32_t>(fnames, node).data(),
            node);
        break;

      case Ioss::Field::BasicType::INT64:
        this->addFieldNodeAndDataToConduitNode<std::int64_t>(
            field, this->getInterweavedScalarDataFromConduitNode<std::int64_t>(fnames, node).data(),
            node);
        break;

      case Ioss::Field::BasicType::CHARACTER:
        this->addFieldNodeAndDataToConduitNode<std::int8_t>(
            field, this->getInterweavedScalarDataFromConduitNode<char>(fnames, node).data(), node);
        break;
      default:
        std::ostringstream errmsg;
        fmt::print(errmsg, "ERROR in {} on read: {}, unsupported field type: {}\n", __func__,
                   field.get_name(), field.type_string());
        IOSS_ERROR(errmsg);
      }

      // Remove Related Scalars from Conduit Node
      for (int i = 0; i < fnames.size(); i++) {
        node.remove(fnames[i]);
      }
    }

    template <typename GroupingEntityT>
    bool readFields(conduit_cpp::Node &&parent, GroupingEntityT *block) const
    {
      // Assumption: count = entity_count (in block)
      Ioss::DatabaseIO *dbase           = block->get_database();
      Ioss::EntityType  b_t             = block->type();
      bool              is_entity_block = false;
      if (b_t == Ioss::EntityType::ELEMENTBLOCK || b_t == Ioss::EntityType::EDGEBLOCK ||
          b_t == Ioss::EntityType::FACEBLOCK || b_t == Ioss::EntityType::NODEBLOCK ||
          b_t == Ioss::EntityType::SIDEBLOCK || b_t == Ioss::EntityType::STRUCTUREDBLOCK) {
        is_entity_block = true;
      }
      Ioss::NameList field_names;
      field_names.reserve(parent.number_of_children());
      size_t entity_count = 0;

      for (conduit_index_t idx = 0, max = parent.number_of_children(); idx < max; ++idx) {
        auto     &&child   = parent[idx];
        const auto name    = child.name();
        const auto type    = static_cast<Ioss::Field::BasicType>(child[detail::TYPE].as_int8());
        const auto role    = static_cast<Ioss::Field::RoleType>(child[detail::ROLE].as_int8());
        const auto count   = child[detail::COUNT].as_int64();
        const auto index   = child[detail::INDEX].as_int64();
        const auto storage = child[detail::STORAGE].as_string();
        if (!block->field_exists(name)) {
          if (storage == IOSS_SCALAR() && role == Ioss::Field::TRANSIENT && is_entity_block) {
            // Add to get_fields() call
            field_names.push_back(name);
            if (entity_count == 0)
              entity_count = count;
          }
          else {
            block->field_add(
                Ioss::Field(name, type, storage, role, count, index).set_zero_copy_enabled());
          }
        }
        else {
          // Verify field details.
          auto field_block = block->get_fieldref(name);
          if (!field_block.has_transform()) {
            block->get_fieldref(name).set_zero_copy_enabled();
          }
          auto field_conduit =
              Ioss::Field(name, type, storage, role, count, index).set_zero_copy_enabled();
          if (field_block != field_conduit) {
            std::ostringstream errmsg;
            errmsg << "Catalyst Read: Field '" << name << "' from conduit "
                   << "already exists in block '" << block->name().c_str() << "' of type '"
                   << block->type_string() << "' and differs from it\n";
            IOSS_ERROR(errmsg);
          }
        }
      }

      // Apply Exodus Properties to Scalar Fields in Entity Blocks
      if (!field_names.empty()) {
        std::vector<Ioss::Field> fields;
        Ioss::Utils::get_fields(entity_count, field_names, Ioss::Field::TRANSIENT, dbase, nullptr,
                                fields);
        for (const auto &field : fields) {
          block->field_add(field.set_zero_copy_enabled());
        }

        for (const auto &field : fields) {
          if (field.raw_storage()->name() != IOSS_SCALAR()) {
            this->combineScalarFieldsInConduitNodeToNonScalarField(field, parent);
          }
        }
      }

      return true;
    }
  };

  template <>
  bool DatabaseIO::ImplementationT::readEntityGroup<Ioss::Region>(conduit_cpp::Node &&parent,
                                                                  Ioss::Region       *region)
  {
    for (conduit_index_t idx = 0, max = parent.number_of_children(); idx < max; ++idx) {
      auto &&child = parent[idx];

      this->readProperties(child[detail::PROPERTIES], region);

      // read fields (meta-data only)
      this->readFields(child[detail::FIELDS], region);
    }
    return true;
  }

  template <>
  bool DatabaseIO::ImplementationT::readEntityGroup<Ioss::SideBlock>(conduit_cpp::Node &&parent,
                                                                     Ioss::Region       *region)
  {
    sideBlocks.clear();
    for (conduit_index_t idx = 0, max = parent.number_of_children(); idx < max; ++idx) {
      auto &&child = parent[idx];
      auto   block = detail::createEntityGroup<Ioss::SideBlock>(child, region->get_database());
      if (sideBlocks.find(block->name()) == sideBlocks.end()) {
        sideBlocks[block->name()] = block;
      }
      else {
        std::ostringstream errmsg;
        fmt::print(errmsg, "ERROR in {} {}: side block name used twice.\n", __func__,
                   block->name());
        IOSS_ERROR(errmsg);
      }

      this->readProperties(child[detail::PROPERTIES], block);

      // read fields (meta-data only)
      this->readFields(child[detail::FIELDS], block);
    }
    return true;
  }

  template <>
  bool DatabaseIO::ImplementationT::readEntityGroup<Ioss::SideSet>(conduit_cpp::Node &&parent,
                                                                   Ioss::Region       *region)
  {
    for (conduit_index_t idx = 0, max = parent.number_of_children(); idx < max; ++idx) {
      auto &&child = parent[idx];
      auto   block = detail::createEntityGroup<Ioss::SideSet>(child, region->get_database());
      for (int i = 0; i < child[detail::SIDEBLOCKS].number_of_children(); i++) {
        auto name = child[detail::SIDEBLOCKS].child(i).as_string();
        if (sideBlocks.find(name) != sideBlocks.end()) {
          block->add(sideBlocks[name]);
        }
        else {
          std::ostringstream errmsg;
          fmt::print(errmsg, "ERROR in {} {}: side block name not available.\n", __func__, name);
          IOSS_ERROR(errmsg);
        }
      }
      region->add(block);
      this->readProperties(child[detail::PROPERTIES], block);

      // read fields (meta-data only)
      this->readFields(child[detail::FIELDS], block);
    }
    sideBlocks.clear();
    return true;
  }

  template <>
  bool DatabaseIO::ImplementationT::readEntityGroup<Ioss::Assembly>(conduit_cpp::Node &&parent,
                                                                    Ioss::Region       *region)
  {
    for (conduit_index_t idx = 0, max = parent.number_of_children(); idx < max; ++idx) {
      auto &&child       = parent[idx];
      auto   block       = detail::createEntityGroup<Ioss::Assembly>(child, region->get_database());
      auto   member_type = child[detail::MEMBER_TYPE].as_int();
      for (int i = 0; i < child[detail::MEMBERS].number_of_children(); i++) {
        auto                  name = child[detail::MEMBERS].child(i).as_string();
        Ioss::GroupingEntity *ge   = nullptr;
        switch (member_type) {
        case Ioss::EntityType::NODEBLOCK: ge = region->get_node_block(name); break;
        case Ioss::EntityType::EDGEBLOCK: ge = region->get_edge_block(name); break;
        case Ioss::EntityType::EDGESET: ge = region->get_edgeset(name); break;
        case Ioss::EntityType::FACEBLOCK: ge = region->get_face_block(name); break;
        case Ioss::EntityType::ELEMENTBLOCK: ge = region->get_element_block(name); break;
        case Ioss::EntityType::NODESET: ge = region->get_nodeset(name); break;
        case Ioss::EntityType::FACESET: ge = region->get_faceset(name); break;
        case Ioss::EntityType::ELEMENTSET: ge = region->get_elementset(name); break;
        case Ioss::EntityType::SIDESET: ge = region->get_sideset(name); break;
        case Ioss::EntityType::COMMSET: ge = region->get_commset(name); break;
        case Ioss::EntityType::SIDEBLOCK: ge = region->get_sideblock(name); break;
        case Ioss::EntityType::ASSEMBLY: ge = region->get_assembly(name); break;
        case Ioss::EntityType::BLOB: ge = region->get_blob(name); break;
        default:
          std::ostringstream errmsg;
          fmt::print(stderr, "ERROR in {} {}: unknown grouping entity type.\n", __func__, name);
          IOSS_ERROR(errmsg);
        }
        if (ge) {
          block->add(ge);
        }
        else {
          std::ostringstream errmsg;
          fmt::print(errmsg, "ERROR in {} {}: grouping entity not found.\n", __func__, name);
          IOSS_ERROR(errmsg);
        }
      }
      region->add(block);
      this->readProperties(child[detail::PROPERTIES], block);

      // read fields (meta-data only)
      this->readFields(child[detail::FIELDS], block);
    }
    return true;
  }

  template <>
  bool
  DatabaseIO::ImplementationT::readEntityGroup<Ioss::StructuredBlock>(conduit_cpp::Node &&parent,
                                                                      Ioss::Region       *region)
  {
    for (conduit_index_t idx = 0, max = parent.number_of_children(); idx < max; ++idx) {
      auto &&child = parent[idx];
      auto block = detail::createEntityGroup<Ioss::StructuredBlock>(child, region->get_database());
      region->add(block);
      auto parent = block->get_node_block().get_property(detail::IOSSCONTAINEDIN);
      this->readProperties(child[detail::PROPERTIES], block);
      this->readProperties(
          child[getName(&block->get_node_block()) + detail::FS + detail::PROPERTIES],
          &block->get_node_block());
      block->get_node_block().property_add(parent);

      // read fields (meta-data only)
      this->readFields(child[detail::FIELDS], block);
      this->readFields(child[getName(&block->get_node_block()) + detail::FS + detail::FIELDS],
                       &block->get_node_block());

      readZoneConnectivity(child[detail::ZONECONNECTIVITY], block);
      readBoundaryConditions(child[detail::BOUNDARYCONDS], block);

      conduit_uint64 *my_vals = child[detail::BLOCKLOCNODEIND].as_uint64_ptr();
      block->m_blockLocalNodeIndex.clear();
      for (int i = 0; i < child[detail::BLOCKLOCNODEIND].number_of_children(); i++) {
        block->m_blockLocalNodeIndex.push_back(my_vals[i]);
      }

      conduit_cpp::Node &&n = child[detail::GLOBALIDMAP];
      block->m_globalIdMap.clear();
      for (conduit_index_t i = 0, m = n.number_of_children(); i < m; ++i) {
        auto &&c = n[i];
        block->m_globalIdMap.push_back(
            std::pair<size_t, size_t>(c[detail::KEY].as_int(), c[detail::VALUE].as_int()));
      }
    }
    return true;
  }

  void DatabaseIO::ImplementationT::readZoneConnectivity(conduit_cpp::Node    &&parent,
                                                         Ioss::StructuredBlock *sb)
  {
    for (conduit_index_t idx = 0, max = parent.number_of_children(); idx < max; ++idx) {
      auto                 &&child = parent[idx];
      Ioss::ZoneConnectivity zc(
          child[detail::CONNECTIONNAME].as_string(), child[detail::OWNERZONE].as_int(),
          child[detail::DONORNAME].as_string(), child[detail::DONORZONE].as_int(),
          readIJK(child[detail::TRANSFORM]), readIJK(child[detail::OWNERRANGEBEG]),
          readIJK(child[detail::OWNERRANGEEND]), readIJK(child[detail::OWNEROFFSET]),
          readIJK(child[detail::DONORRANGEBEG]), readIJK(child[detail::DONORRANGEEND]),
          readIJK(child[detail::DONOROFFSET]));
      zc.m_fromDecomp = child[detail::FROMDECOMP].as_int();
      sb->m_zoneConnectivity.push_back(zc);
    }
  }

  void DatabaseIO::ImplementationT::readBoundaryConditions(conduit_cpp::Node    &&parent,
                                                           Ioss::StructuredBlock *sb)
  {
    for (conduit_index_t idx = 0, max = parent.number_of_children(); idx < max; ++idx) {

      auto                  &&child = parent[idx];
      Ioss::BoundaryCondition bc(
          child[detail::BCNAME].as_string(), child[detail::FAMNAME].as_string(),
          readIJK(child[detail::RANGEBEG]), readIJK(child[detail::RANGEEND]));
      bc.m_face = child[detail::FACE].as_int();
      sb->m_boundaryConditions.push_back(bc);
    }
  }

  Ioss::IJK_t DatabaseIO::ImplementationT::readIJK(conduit_cpp::Node &&parent)
  {
    Ioss::IJK_t a{{0, 0, 0}};
    for (auto i = 0; i < parent.number_of_children(); i++) {
      a[i] = parent[i].as_int();
    }
    return a;
  }

  bool DatabaseIO::ImplementationT::readModel(Ioss::Region *region)
  {
    auto &node = this->DBNode;
    region->get_database()->set_int_byte_size_api(
        static_cast<Ioss::DataSize>(node[detail::getAPISizePath()].as_int8()));
    const auto write_split_type =
        static_cast<Ioss::SurfaceSplitType>(node[detail::SURFACESPLITTYPE].as_int8());
    if (write_split_type != region->get_database()->get_surface_split_type()) {
      static_cast<DatabaseIO *>(region->get_database())->set_split_type_changed(true);
    }
    read_db_field_separator = region->get_database()->get_field_separator();

    if (node.has_path(getTimePath())) {
      region->add_state(node[getTimePath()].to_float64());
    }
    this->readEntityGroup<Ioss::Region>(node[detail::REGION], region);
    this->readEntityGroup<Ioss::NodeBlock>(node[detail::NODEBLOCKS], region);
    this->readEntityGroup<Ioss::ElementBlock>(node[detail::ELEMENTBLOCKS], region);
    this->readEntityGroup<Ioss::EdgeBlock>(node[detail::EDGEBLOCKS], region);
    this->readEntityGroup<Ioss::FaceBlock>(node[detail::FACEBLOCKS], region);

    bool surface_split_changed =
        static_cast<DatabaseIO *>(region->get_database())->split_type_changed();
    if (!surface_split_changed) {
      this->readEntityGroup<Ioss::SideBlock>(node[detail::SIDEBLOCKS], region);
      this->readEntityGroup<Ioss::SideSet>(node[detail::SIDESETS], region);
    }

    this->readEntityGroup<Ioss::NodeSet>(node[detail::NODESETS], region);
    this->readEntityGroup<Ioss::EdgeSet>(node[detail::EDGESETS], region);
    this->readEntityGroup<Ioss::FaceSet>(node[detail::FACESETS], region);
    this->readEntityGroup<Ioss::ElementSet>(node[detail::ELEMENTSETS], region);
    this->readEntityGroup<Ioss::StructuredBlock>(node[detail::STRUCTUREDBLOCKS], region);
    this->readEntityGroup<Ioss::Assembly>(node[detail::ASSEMBLIES], region);
    this->readEntityGroup<Ioss::Blob>(node[detail::BLOBS], region);
    this->readEntityGroup<Ioss::CommSet>(node[detail::COMMSETS], region);
    return true;
  }

  DatabaseIO::DatabaseIO(Ioss::Region *region, const std::string &filename,
                         Ioss::DatabaseUsage db_usage, Ioss_MPI_Comm communicator,
                         const Ioss::PropertyManager &props)
      : Ioss::DatabaseIO(region, filename, db_usage, communicator, props),
        Impl(new DatabaseIO::ImplementationT()), useDeepCopy(true)

  {
    dbState = Ioss::STATE_UNKNOWN;

    bool shallowCopy = false;
    if (Ioss::Utils::check_set_bool_property(properties, detail::SHALLOWCOPYFIELDS, shallowCopy)) {
      this->useDeepCopy = !shallowCopy;
    }

    if (is_input()) {
      auto &pm = get_property_manager();
      if (pm.exists(detail::CATCONDNODE)) {
        auto c_node_ptr =
            reinterpret_cast<conduit_node *>(pm.get(detail::CATCONDNODE).get_pointer());
        this->Impl->setDatabaseNode(c_node_ptr);
      }
      else {
        int timestep = 0;
        if (pm.exists(detail::CATREADTIMESTEP)) {
          timestep = pm.get(detail::CATREADTIMESTEP).get_int();
        }
        else if (const char *ts = std::getenv(detail::CATREADTIMESTEP.c_str())) {
          timestep = std::stoi(std::string(ts));
        }

        int proc_count = util().parallel_size();
        int my_proc    = util().parallel_rank();
        if (properties.exists("processor_count") && properties.exists("my_processor")) {
          proc_count = properties.get("processor_count").get_int();
          my_proc    = properties.get("my_processor").get_int();
        }

        std::ostringstream path;
        path << get_catalyst_dump_dir() << detail::EXECUTE_INVC << timestep
             << detail::PARAMS_CONDUIT_BIN << proc_count << detail::DOT << my_proc;
        auto &root  = this->Impl->root();
        auto &dbase = this->Impl->databaseNode();
        conduit_node_load(conduit_cpp::c_node(&root), path.str().c_str(), "conduit_bin");
        auto dp = CatalystManager::getInstance().getCatDataPath(props);
        conduit_node_set_external_node(conduit_cpp::c_node(&dbase),
                                       conduit_node_fetch(conduit_cpp::c_node(&root), dp.c_str()));
      }
    }
    else {
      catPipeID = CatalystManager::getInstance().initialize(props, this->util());
    }
  }

  DatabaseIO::~DatabaseIO()
  {
    if (!is_input()) {
      CatalystManager::getInstance().finalize(catPipeID);
    }
  }

  bool DatabaseIO::begin_nl(Ioss::State state)
  {
    this->dbState = state;
    return true;
  }

  bool DatabaseIO::end_nl(Ioss::State state)
  {
    if (this->dbState != state) {
      std::ostringstream errmsg;
      errmsg << "Catalyst: dbState != state in end_nl"
             << "\n";
      IOSS_ERROR(errmsg);
    }

    if (!is_input()) {
      auto region = this->get_region();
      if (region == nullptr) {
        std::ostringstream errmsg;
        errmsg << "Catalyst: region is nullptr in end_nl"
               << "\n";
        IOSS_ERROR(errmsg);
      }

      auto &impl = (*this->Impl.get());
      switch (state) {
      case Ioss::STATE_DEFINE_MODEL:
        // here the basic structure for the model is defined i.e.
        // number of blocks/sets/names etc.
        if (!impl.defineModel(region)) {
          return false;
        }
        break;

      case Ioss::STATE_MODEL:
        // here the model has meshdata e.g mesh fields, ids, coordinates etc.
        break;

      case Ioss::STATE_DEFINE_TRANSIENT: break;

      case Ioss::STATE_TRANSIENT: break;

      case Ioss::STATE_LAST_ENTRY: break;
      case Ioss::STATE_UNKNOWN:
      case Ioss::STATE_INVALID:
      case Ioss::STATE_READONLY:
      case Ioss::STATE_CLOSED: break;
      }
    }

    dbState = Ioss::STATE_UNKNOWN;
    return true;
  }

  bool DatabaseIO::begin_state_nl(int state, double time) { return true; }

  // common
  bool DatabaseIO::end_state_nl(int state, double time)
  {
    if (this->is_input()) {}
    else {
      // invoke catalyst.
      auto &impl      = (*this->Impl.get());
      auto &dbaseNode = this->Impl->databaseNode();
      dbaseNode[this->Impl->getTimePath()].set_float64(time);
      conduit_cpp::Node node;
      CatalystManager::getInstance().execute(catPipeID, state, time, impl.databaseNode());
    }
    return true;
  }

  unsigned DatabaseIO::entity_field_support() const
  {
    return Ioss::NODEBLOCK | Ioss::EDGEBLOCK | Ioss::FACEBLOCK | Ioss::ELEMENTBLOCK |
           Ioss::COMMSET | Ioss::NODESET | Ioss::EDGESET | Ioss::FACESET | Ioss::ELEMENTSET |
           Ioss::SIDESET | Ioss::SIDEBLOCK | Ioss::STRUCTUREDBLOCK | Ioss::ASSEMBLY | Ioss::REGION |
           Ioss::BLOB;
  }

  void DatabaseIO::read_meta_data_nl()
  {
    auto region = this->get_region();
    if (region == nullptr) {
      std::ostringstream errmsg;
      errmsg << "Catalyst: region is nullptr in read_meta_data_nl()"
             << "\n";
      IOSS_ERROR(errmsg);
    }

    auto &impl = (*this->Impl.get());
    impl.readModel(region);
  }

  void DatabaseIO::get_step_times_nl()
  {
    auto region = this->get_region();
    if (region == nullptr) {
      std::ostringstream errmsg;
      errmsg << "Catalyst: region is nullptr in get_step_times_nl()"
             << "\n";
      IOSS_ERROR(errmsg);
    }

    auto &impl = (*this->Impl.get());
    impl.readTime(region);
  }

  std::vector<double> DatabaseIO::get_db_step_times_nl()
  {
    auto &impl = (*this->Impl.get());
    return impl.getTime();
  }

  void *DatabaseIO::get_catalyst_conduit_node()
  {
    auto &impl = (*this->Impl.get());
    return impl.catalystConduitNode();
  }

  void DatabaseIO::print_catalyst_conduit_node()
  {
    auto &impl = (*this->Impl.get());
    impl.print();
  }

  std::string DatabaseIO::get_catalyst_dump_dir() const
  {
    std::string retVal;
    auto        catalystDumpDir = std::getenv(detail::CATDUMPDIR.c_str());
    if (catalystDumpDir) {
      retVal = catalystDumpDir;
    }
    if (!retVal.empty() && retVal.back() != detail::FS_CHAR) {
      retVal += detail::FS_CHAR;
    }
    return retVal;
  }

  int64_t DatabaseIO::put_field_internal(const Ioss::Region *reg, const Ioss::Field &field,
                                         void *data, size_t data_size) const
  {
    auto &impl = (*this->Impl.get());
    return impl.putField(detail::REGION, reg, field, data, data_size, this->deep_copy());
  }

  int64_t DatabaseIO::put_field_internal(const Ioss::NodeBlock *nb, const Ioss::Field &field,
                                         void *data, size_t data_size) const
  {
    auto       &impl      = (*this->Impl.get());
    std::string blockPath = detail::NODEBLOCKS;
    if (nb->is_nonglobal_nodeblock()) {
      blockPath = detail::STRUCTUREDBLOCKS + detail::FS + impl.getName(nb->contained_in());
    }
    return impl.putField(blockPath, nb, field, data, data_size, this->deep_copy());
  }

  int64_t DatabaseIO::put_field_internal(const Ioss::EdgeBlock *eb, const Ioss::Field &field,
                                         void *data, size_t data_size) const
  {
    auto &impl = (*this->Impl.get());
    return impl.putField(detail::EDGEBLOCKS, eb, field, data, data_size, this->deep_copy());
  }

  int64_t DatabaseIO::put_field_internal(const Ioss::FaceBlock *fb, const Ioss::Field &field,
                                         void *data, size_t data_size) const
  {
    auto &impl = (*this->Impl.get());
    return impl.putField(detail::FACEBLOCKS, fb, field, data, data_size, this->deep_copy());
  }

  int64_t DatabaseIO::put_field_internal(const Ioss::ElementBlock *eb, const Ioss::Field &field,
                                         void *data, size_t data_size) const
  {
    auto &impl = (*this->Impl.get());
    return impl.putField(detail::ELEMENTBLOCKS, eb, field, data, data_size, this->deep_copy());
  }

  int64_t DatabaseIO::put_field_internal(const Ioss::SideBlock *sb, const Ioss::Field &field,
                                         void *data, size_t data_size) const
  {
    auto &impl = (*this->Impl.get());
    return impl.putField(detail::SIDEBLOCKS, sb, field, data, data_size, this->deep_copy());
  }

  int64_t DatabaseIO::put_field_internal(const Ioss::NodeSet *ns, const Ioss::Field &field,
                                         void *data, size_t data_size) const
  {
    auto &impl = (*this->Impl.get());
    return impl.putField(detail::NODESETS, ns, field, data, data_size, this->deep_copy());
  }

  int64_t DatabaseIO::put_field_internal(const Ioss::EdgeSet *es, const Ioss::Field &field,
                                         void *data, size_t data_size) const
  {
    auto &impl = (*this->Impl.get());
    return impl.putField(detail::EDGESETS, es, field, data, data_size, this->deep_copy());
  }

  int64_t DatabaseIO::put_field_internal(const Ioss::FaceSet *fs, const Ioss::Field &field,
                                         void *data, size_t data_size) const
  {
    auto &impl = (*this->Impl.get());
    return impl.putField(detail::FACESETS, fs, field, data, data_size, this->deep_copy());
  }

  int64_t DatabaseIO::put_field_internal(const Ioss::ElementSet *es, const Ioss::Field &field,
                                         void *data, size_t data_size) const
  {
    auto &impl = (*this->Impl.get());
    return impl.putField(detail::ELEMENTSETS, es, field, data, data_size, this->deep_copy());
  }

  int64_t DatabaseIO::put_field_internal(const Ioss::SideSet *ss, const Ioss::Field &field,
                                         void *data, size_t data_size) const
  {
    auto &impl = (*this->Impl.get());
    return impl.putField(detail::SIDESETS, ss, field, data, data_size, this->deep_copy());
  }

  int64_t DatabaseIO::put_field_internal(const Ioss::CommSet *cs, const Ioss::Field &field,
                                         void *data, size_t data_size) const
  {
    auto &impl = (*this->Impl.get());
    return impl.putField(detail::COMMSETS, cs, field, data, data_size, this->deep_copy());
  }

  int64_t DatabaseIO::put_field_internal(const Ioss::Assembly *as, const Ioss::Field &field,
                                         void *data, size_t data_size) const
  {
    auto &impl = (*this->Impl.get());
    return impl.putField(detail::ASSEMBLIES, as, field, data, data_size, this->deep_copy());
  }

  int64_t DatabaseIO::put_field_internal(const Ioss::Blob *bl, const Ioss::Field &field, void *data,
                                         size_t data_size) const
  {
    auto &impl = (*this->Impl.get());
    return impl.putField(detail::BLOBS, bl, field, data, data_size, this->deep_copy());
  }

  int64_t DatabaseIO::put_field_internal(const Ioss::StructuredBlock *sb, const Ioss::Field &field,
                                         void *data, size_t data_size) const
  {
    auto &impl = (*this->Impl.get());
    return impl.putField(detail::STRUCTUREDBLOCKS, sb, field, data, data_size, this->deep_copy());
  }

  int64_t DatabaseIO::get_field_internal(const Ioss::Region *reg, const Ioss::Field &field,
                                         void *data, size_t data_size) const
  {
    auto &impl = (*this->Impl.get());
    return impl.getField(detail::REGION, reg, field, data, data_size);
  }

  int64_t DatabaseIO::get_field_internal(const Ioss::NodeBlock *nb, const Ioss::Field &field,
                                         void *data, size_t data_size) const
  {
    auto &impl = (*this->Impl.get());

    std::string blockPath = detail::NODEBLOCKS;
    if (nb->is_nonglobal_nodeblock()) {
      blockPath = detail::STRUCTUREDBLOCKS + detail::FS + impl.getName(nb->contained_in());
    }

    if (impl.hasField(blockPath, nb, field.get_name())) {
      return impl.getField(blockPath, nb, field, data, data_size);
    }
    else if ((field.get_name() == detail::MESHMODCO) &&
             (impl.hasField(blockPath, nb, detail::MESHMODCOX) &&
              impl.hasField(blockPath, nb, detail::MESHMODCOY) &&
              impl.hasField(blockPath, nb, detail::MESHMODCOZ))) {
      return impl.getMeshModelCoordinates(blockPath, nb, field, data, data_size);
    }
    else {
      fmt::print(Ioss::OUTPUT(), "WARNING in {} : {}\n", __func__,
                 "field not available, " + field.get_name() + ", in container " + blockPath + "\n");
      return -1;
    }
  }

  int64_t DatabaseIO::get_field_internal(const Ioss::ElementBlock *eb, const Ioss::Field &field,
                                         void *data, size_t data_size) const
  {
    auto &impl = (*this->Impl.get());
    if (impl.hasField(detail::ELEMENTBLOCKS, eb, field.get_name())) {
      return impl.getField(detail::ELEMENTBLOCKS, eb, field, data, data_size);
    }
    else {
      if (field.get_name() == detail::CONNECTIVITYRAW &&
          impl.hasField(detail::ELEMENTBLOCKS, eb, detail::CONNECTIVITY)) {
        // maybe the data has 'connectivity' provided, so we convert it to 'connectivity_raw'.
        auto count =
            this->get_field_internal(eb, eb->get_field(detail::CONNECTIVITY), data, data_size);
        if (count <= 0) {
          return count;
        }

        impl.get_node_map(this).reverse_map_data(
            data, field, field.verify(data_size) * field.raw_storage()->component_count());
        return count;
      }
      else if (field.get_name() == detail::CONNECTIVITY &&
               impl.hasField(detail::ELEMENTBLOCKS, eb, detail::CONNECTIVITYRAW)) {
        // maybe the data has 'connectivity_raw' is provided, so we convert it to 'connectivity.
        auto count =
            this->get_field_internal(eb, eb->get_field(detail::CONNECTIVITYRAW), data, data_size);
        if (count <= 0) {
          return count;
        }

        impl.get_node_map(this).map_data(
            data, field, field.verify(data_size) * field.raw_storage()->component_count());
        return count;
      }

      fmt::print(Ioss::OUTPUT(), "WARNING in {} : {}\n", __func__,
                 "field not available, " + field.get_name() + ", in container " +
                     detail::ELEMENTBLOCKS + "\n");
      return -1;
    }
  }

  int64_t DatabaseIO::get_field_internal(const Ioss::EdgeBlock *eb, const Ioss::Field &field,
                                         void *data, size_t data_size) const
  {
    auto &impl = (*this->Impl.get());
    return impl.getField(detail::EDGEBLOCKS, eb, field, data, data_size);
  }

  int64_t DatabaseIO::get_field_internal(const Ioss::FaceBlock *fb, const Ioss::Field &field,
                                         void *data, size_t data_size) const
  {
    auto &impl = (*this->Impl.get());
    return impl.getField(detail::FACEBLOCKS, fb, field, data, data_size);
  }
  int64_t DatabaseIO::get_field_internal(const Ioss::SideBlock *sb, const Ioss::Field &field,
                                         void *data, size_t data_size) const
  {
    if (split_type_changed()) {
      return -1;
    }
    auto &impl = (*this->Impl.get());
    return impl.getField(detail::SIDEBLOCKS, sb, field, data, data_size);
  }
  int64_t DatabaseIO::get_field_internal(const Ioss::NodeSet *ns, const Ioss::Field &field,
                                         void *data, size_t data_size) const
  {
    auto &impl = (*this->Impl.get());
    return impl.getField(detail::NODESETS, ns, field, data, data_size);
  }
  int64_t DatabaseIO::get_field_internal(const Ioss::EdgeSet *ns, const Ioss::Field &field,
                                         void *data, size_t data_size) const
  {
    auto &impl = (*this->Impl.get());
    return impl.getField(detail::EDGESETS, ns, field, data, data_size);
  }
  int64_t DatabaseIO::get_field_internal(const Ioss::FaceSet *ns, const Ioss::Field &field,
                                         void *data, size_t data_size) const
  {
    auto &impl = (*this->Impl.get());
    return impl.getField(detail::FACESETS, ns, field, data, data_size);
  }
  int64_t DatabaseIO::get_field_internal(const Ioss::ElementSet *ns, const Ioss::Field &field,
                                         void *data, size_t data_size) const
  {
    auto &impl = (*this->Impl.get());
    return impl.getField(detail::ELEMENTSETS, ns, field, data, data_size);
  }
  int64_t DatabaseIO::get_field_internal(const Ioss::SideSet *ss, const Ioss::Field &field,
                                         void *data, size_t data_size) const
  {
    if (split_type_changed()) {
      return -1;
    }
    auto &impl = (*this->Impl.get());
    return impl.getField(detail::SIDESETS, ss, field, data, data_size);
  }
  int64_t DatabaseIO::get_field_internal(const Ioss::CommSet *cs, const Ioss::Field &field,
                                         void *data, size_t data_size) const
  {
    auto &impl = (*this->Impl.get());
    return impl.getField(detail::COMMSETS, cs, field, data, data_size);
  }
  int64_t DatabaseIO::get_field_internal(const Ioss::Assembly *as, const Ioss::Field &field,
                                         void *data, size_t data_size) const
  {
    auto &impl = (*this->Impl.get());
    return impl.getField(detail::ASSEMBLIES, as, field, data, data_size);
  }
  int64_t DatabaseIO::get_field_internal(const Ioss::Blob *bl, const Ioss::Field &field, void *data,
                                         size_t data_size) const
  {
    auto &impl = (*this->Impl.get());
    return impl.getField(detail::BLOBS, bl, field, data, data_size);
  }
  int64_t DatabaseIO::get_field_internal(const Ioss::StructuredBlock *sb, const Ioss::Field &field,
                                         void *data, size_t data_size) const
  {
    std::string blockPath = detail::STRUCTUREDBLOCKS;
    auto       &impl      = (*this->Impl.get());
    if (impl.hasField(blockPath, sb, field.get_name())) {
      return impl.getField(blockPath, sb, field, data, data_size);
    }
    else if (field.get_name() == detail::CELLIDS || field.get_name() == detail::CELLNODEIDS) {
      return impl.getStructuredBlockIDS(sb, field, data, data_size);
    }
    else if ((field.get_name() == detail::MESHMODCO) &&
             (impl.hasField(blockPath, sb, detail::MESHMODCOX) &&
              impl.hasField(blockPath, sb, detail::MESHMODCOY) &&
              impl.hasField(blockPath, sb, detail::MESHMODCOZ))) {
      return impl.getMeshModelCoordinates(blockPath, sb, field, data, data_size);
    }
    else {
      fmt::print(Ioss::OUTPUT(), "WARNING in {} : {}\n", __func__,
                 "field not available, " + field.get_name() + ", in container " +
                     detail::STRUCTUREDBLOCKS + "\n");
      return -1;
    }
  }

  int64_t DatabaseIO::get_zc_field_internal(const Ioss::Region *reg, const Ioss::Field &field,
                                            void **data, size_t *data_size) const
  {
    auto &impl = (*this->Impl.get());
    return impl.getFieldZeroCopy(detail::REGION, reg, field, data, data_size);
  }
  int64_t DatabaseIO::get_zc_field_internal(const Ioss::NodeBlock *nb, const Ioss::Field &field,
                                            void **data, size_t *data_size) const
  {

    auto       &impl      = (*this->Impl.get());
    std::string blockPath = detail::NODEBLOCKS;
    if (nb->is_nonglobal_nodeblock()) {
      blockPath = detail::STRUCTUREDBLOCKS + detail::FS + impl.getName(nb->contained_in());
    }
    return impl.getFieldZeroCopy(blockPath, nb, field, data, data_size);
  }
  int64_t DatabaseIO::get_zc_field_internal(const Ioss::EdgeBlock *eb, const Ioss::Field &field,
                                            void **data, size_t *data_size) const
  {
    auto &impl = (*this->Impl.get());
    return impl.getFieldZeroCopy(detail::EDGEBLOCKS, eb, field, data, data_size);
  }
  int64_t DatabaseIO::get_zc_field_internal(const Ioss::FaceBlock *fb, const Ioss::Field &field,
                                            void **data, size_t *data_size) const
  {
    auto &impl = (*this->Impl.get());
    return impl.getFieldZeroCopy(detail::FACEBLOCKS, fb, field, data, data_size);
  }
  int64_t DatabaseIO::get_zc_field_internal(const Ioss::ElementBlock *eb, const Ioss::Field &field,
                                            void **data, size_t *data_size) const
  {
    auto &impl = (*this->Impl.get());
    return impl.getFieldZeroCopy(detail::ELEMENTBLOCKS, eb, field, data, data_size);
  }
  int64_t DatabaseIO::get_zc_field_internal(const Ioss::SideBlock *sb, const Ioss::Field &field,
                                            void **data, size_t *data_size) const
  {
    if (split_type_changed()) {
      return -1;
    }
    auto &impl = (*this->Impl.get());
    return impl.getFieldZeroCopy(detail::SIDEBLOCKS, sb, field, data, data_size);
  }
  int64_t DatabaseIO::get_zc_field_internal(const Ioss::NodeSet *ns, const Ioss::Field &field,
                                            void **data, size_t *data_size) const
  {
    auto &impl = (*this->Impl.get());
    return impl.getFieldZeroCopy(detail::NODESETS, ns, field, data, data_size);
  }
  int64_t DatabaseIO::get_zc_field_internal(const Ioss::EdgeSet *es, const Ioss::Field &field,
                                            void **data, size_t *data_size) const
  {
    auto &impl = (*this->Impl.get());
    return impl.getFieldZeroCopy(detail::EDGESETS, es, field, data, data_size);
  }
  int64_t DatabaseIO::get_zc_field_internal(const Ioss::FaceSet *fs, const Ioss::Field &field,
                                            void **data, size_t *data_size) const
  {
    auto &impl = (*this->Impl.get());
    return impl.getFieldZeroCopy(detail::FACESETS, fs, field, data, data_size);
  }
  int64_t DatabaseIO::get_zc_field_internal(const Ioss::ElementSet *es, const Ioss::Field &field,
                                            void **data, size_t *data_size) const
  {
    auto &impl = (*this->Impl.get());
    return impl.getFieldZeroCopy(detail::ELEMENTSETS, es, field, data, data_size);
  }
  int64_t DatabaseIO::get_zc_field_internal(const Ioss::SideSet *ss, const Ioss::Field &field,
                                            void **data, size_t *data_size) const
  {
    if (split_type_changed()) {
      return -1;
    }
    auto &impl = (*this->Impl.get());
    return impl.getFieldZeroCopy(detail::SIDESETS, ss, field, data, data_size);
  }
  int64_t DatabaseIO::get_zc_field_internal(const Ioss::CommSet *cs, const Ioss::Field &field,
                                            void **data, size_t *data_size) const
  {
    auto &impl = (*this->Impl.get());
    return impl.getFieldZeroCopy(detail::COMMSETS, cs, field, data, data_size);
  }
  int64_t DatabaseIO::get_zc_field_internal(const Ioss::Assembly *as, const Ioss::Field &field,
                                            void **data, size_t *data_size) const
  {
    auto &impl = (*this->Impl.get());
    return impl.getFieldZeroCopy(detail::ASSEMBLIES, as, field, data, data_size);
  }
  int64_t DatabaseIO::get_zc_field_internal(const Ioss::Blob *bl, const Ioss::Field &field,
                                            void **data, size_t *data_size) const
  {
    auto &impl = (*this->Impl.get());
    return impl.getFieldZeroCopy(detail::BLOBS, bl, field, data, data_size);
  }
  int64_t DatabaseIO::get_zc_field_internal(const Ioss::StructuredBlock *sb,
                                            const Ioss::Field &field, void **data,
                                            size_t *data_size) const
  {
    auto &impl = (*this->Impl.get());
    return impl.getFieldZeroCopy(detail::STRUCTUREDBLOCKS, sb, field, data, data_size);
  }

} // namespace Iocatalyst
