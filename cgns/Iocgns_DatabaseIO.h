// Copyright(C) 1999-2024 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#pragma once

#include "Ioss_CodeTypes.h"
#include "Ioss_DBUsage.h"    // for DatabaseUsage
#include "Ioss_DatabaseIO.h" // for DatabaseIO
#include "Ioss_FaceGenerator.h"
#include "Ioss_IOFactory.h" // for IOFactory
#include "Ioss_Map.h"       // for Map
#include "Ioss_State.h"     // for State
#include "cgns/Iocgns_Defines.h"
#include <vtk_cgns.h> // xxx(kitware)
#include VTK_CGNS(cgnslib.h)
#include VTK_CGNS(cgnstypes.h)
#include <cstddef>  // for size_t
#include <cstdint>  // for int64_t
#include <iostream> // for ostream
#include <map>
#include <string> // for string
#include <vector>

#include "iocgns_export.h"
#include "vtk_ioss_mangle.h"

namespace Ioss {
  class Assembly;
  class Blob;
  class CommSet;
  class EdgeBlock;
  class EdgeSet;
  class ElementBlock;
  class ElementSet;
  class ElementTopology;
  class FaceBlock;
  class FaceSet;
  class Field;
  class GroupingEntity;
  class NodeBlock;
  class NodeSet;
  class Region;
  class SideBlock;
  class SideSet;
  class EntityBlock;
  class StructuredBlock;
  class Map;
  class PropertyManager;
} // namespace Ioss

/** \brief A namespace for the CGNS database format.
 */
namespace Iocgns {

  class IOCGNS_EXPORT DatabaseIO : public Ioss::DatabaseIO
  {
  public:
    enum class entity_type { NODE, ELEM };

    DatabaseIO(Ioss::Region *region, const std::string &filename, Ioss::DatabaseUsage db_usage,
               Ioss_MPI_Comm communicator, const Ioss::PropertyManager &props);

    // Check capabilities of input/output database...  Returns an
    // unsigned int with the supported Ioss::EntityTypes or'ed
    // together. If "return_value & Ioss::EntityType" is set, then the
    // database supports that type (e.g. return_value & Ioss::FACESET)
    IOSS_NODISCARD unsigned entity_field_support() const override;

    ~DatabaseIO() override;

    IOSS_NODISCARD std::string get_format() const override { return "CGNS"; }

    // This isn't quite true since a CGNS library with cgsize_t == 64-bits can read
    // a file with 32-bit ints. However,...
    IOSS_NODISCARD int int_byte_size_db() const override { return CG_SIZEOF_SIZE; }

    IOSS_NODISCARD bool node_major() const override { return false; }

    // Metadata-related functions.
    void write_meta_data();
    void write_results_meta_data();

    IOSS_NODISCARD int get_file_pointer() const override;

  private:
    IOSS_NODISCARD int64_t node_global_to_local_nl(int64_t global, bool must_exist) const override;
    IOSS_NODISCARD int64_t element_global_to_local_nl(int64_t global) const override;

    void read_meta_data_nl() override;

    void open_state_file(int state);
    void free_state_pointer();

    void openDatabase_nl() const override;
    void closeDatabase_nl() const override;

    IOSS_NODISCARD bool begin_nl(Ioss::State state) override;
    IOSS_NODISCARD bool end_nl(Ioss::State state) override;

    IOSS_NODISCARD bool begin_state_nl(int state, double time) override;
    IOSS_NODISCARD bool end_state_nl(int state, double time) override;
    void                flush_database_nl() const override;

    bool                check_valid_file_open(int status) const;
    void                create_structured_block(int base, int zone, size_t &num_node);
    void                create_structured_block_fpp(int base, int num_zones, size_t &num_node);
    size_t              finalize_structured_blocks();
    void                finalize_database() const override;
    void                get_step_times_nl() override;
    std::vector<double> get_db_step_times_nl() override;

    void create_unstructured_block(int base, int zone, size_t &num_node);
    void write_adjacency_data();

    int64_t get_field_internal(const Ioss::Region *reg, const Ioss::Field &field, void *data,
                               size_t data_size) const override;
    int64_t get_field_internal(const Ioss::NodeBlock *nb, const Ioss::Field &field, void *data,
                               size_t data_size) const override;
    int64_t get_field_internal(const Ioss::EdgeBlock *eb, const Ioss::Field &field, void *data,
                               size_t data_size) const override;
    int64_t get_field_internal(const Ioss::FaceBlock *fb, const Ioss::Field &field, void *data,
                               size_t data_size) const override;
    int64_t get_field_internal(const Ioss::ElementBlock *eb, const Ioss::Field &field, void *data,
                               size_t data_size) const override;
    int64_t get_field_internal(const Ioss::StructuredBlock *sb, const Ioss::Field &field,
                               void *data, size_t data_size) const override;
    int64_t get_field_internal(const Ioss::SideBlock *sb, const Ioss::Field &field, void *data,
                               size_t data_size) const override;
    int64_t get_field_internal(const Ioss::NodeSet *ns, const Ioss::Field &field, void *data,
                               size_t data_size) const override;
    int64_t get_field_internal(const Ioss::EdgeSet *es, const Ioss::Field &field, void *data,
                               size_t data_size) const override;
    int64_t get_field_internal(const Ioss::FaceSet *fs, const Ioss::Field &field, void *data,
                               size_t data_size) const override;
    int64_t get_field_internal(const Ioss::ElementSet *es, const Ioss::Field &field, void *data,
                               size_t data_size) const override;
    int64_t get_field_internal(const Ioss::SideSet *fs, const Ioss::Field &field, void *data,
                               size_t data_size) const override;
    int64_t get_field_internal(const Ioss::CommSet *cs, const Ioss::Field &field, void *data,
                               size_t data_size) const override;
    int64_t get_field_internal(const Ioss::Assembly * /*sb*/, const Ioss::Field & /*field*/,
                               void * /*data*/, size_t /*data_size*/) const override
    {
      return 0;
    }

    int64_t get_field_internal(const Ioss::Blob * /*sb*/, const Ioss::Field & /*field*/,
                               void * /*data*/, size_t /*data_size*/) const override
    {
      return 0;
    }

    int64_t get_field_internal_sub_nb(const Ioss::NodeBlock *nb, const Ioss::Field &field,
                                      void *data, size_t data_size) const;

    int64_t put_field_internal(const Ioss::Region *region, const Ioss::Field &field, void *data,
                               size_t data_size) const override;
    int64_t put_field_internal(const Ioss::NodeBlock *nb, const Ioss::Field &field, void *data,
                               size_t data_size) const override;
    int64_t put_field_internal(const Ioss::EdgeBlock *eb, const Ioss::Field &field, void *data,
                               size_t data_size) const override;
    int64_t put_field_internal(const Ioss::FaceBlock *fb, const Ioss::Field &field, void *data,
                               size_t data_size) const override;
    int64_t put_field_internal(const Ioss::ElementBlock *eb, const Ioss::Field &field, void *data,
                               size_t data_size) const override;
    int64_t put_field_internal(const Ioss::SideBlock *sb, const Ioss::Field &field, void *data,
                               size_t data_size) const override;
    int64_t put_field_internal(const Ioss::NodeSet *ns, const Ioss::Field &field, void *data,
                               size_t data_size) const override;
    int64_t put_field_internal(const Ioss::EdgeSet *es, const Ioss::Field &field, void *data,
                               size_t data_size) const override;
    int64_t put_field_internal(const Ioss::FaceSet *fs, const Ioss::Field &field, void *data,
                               size_t data_size) const override;
    int64_t put_field_internal(const Ioss::ElementSet *es, const Ioss::Field &field, void *data,
                               size_t data_size) const override;
    int64_t put_field_internal(const Ioss::SideSet *ss, const Ioss::Field &field, void *data,
                               size_t data_size) const override;
    int64_t put_field_internal(const Ioss::CommSet *cs, const Ioss::Field &field, void *data,
                               size_t data_size) const override;
    int64_t put_field_internal(const Ioss::StructuredBlock *sb, const Ioss::Field &field,
                               void *data, size_t data_size) const override;
    int64_t put_field_internal(const Ioss::Assembly * /*sb*/, const Ioss::Field & /*field*/,
                               void * /*data*/, size_t /*data_size*/) const override
    {
      return 0;
    }

    int64_t put_field_internal(const Ioss::Blob * /*sb*/, const Ioss::Field & /*field*/,
                               void * /*data*/, size_t /*data_size*/) const override
    {
      return 0;
    }

    int64_t put_field_internal_sub_nb(const Ioss::NodeBlock *nb, const Ioss::Field &field,
                                      void *data, size_t data_size) const;

    // ID Mapping functions.
    IOSS_NODISCARD const Ioss::Map &get_map(entity_type type) const;
    IOSS_NODISCARD const Ioss::Map &get_map(Ioss::Map &entity_map, int64_t entityCount,
                                            int64_t file_offset, int64_t file_count,
                                            entity_type type) const;

    mutable int m_cgnsFilePtr{-1};
    mutable int m_cgnsBasePtr{
        -1}; // If using links to file-per-state, the file pointer for "base" file.

    int          m_flushInterval{0}; // Default is no flushing after each timestep
    int          m_currentVertexSolutionIndex{0};
    int          m_currentCellCenterSolutionIndex{0};
    mutable bool m_dbFinalized{false};

    mutable std::vector<size_t> m_zoneOffset; // Offset for local zone/block element ids to global.
    mutable std::vector<size_t>
        m_bcOffset; // The BC Section element offsets in unstructured output.
    mutable std::vector<double>                           m_timesteps;
    std::vector<CGNSIntVector>                            m_blockLocalNodeMap;
    std::map<std::string, int>                            m_zoneNameMap;
    mutable std::map<int, Ioss::Map *>                    m_globalToBlockLocalNodeMap;
    mutable std::map<std::string, Ioss::FaceUnorderedSet> m_boundaryFaces;
  };
} // namespace Iocgns
