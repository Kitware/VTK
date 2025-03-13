// Copyright(C) 1999-2020, 2022, 2023, 2024 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#pragma once

#include "Ioss_CodeTypes.h"
#include "Ioss_DBUsage.h"    // for DatabaseUsage
#include "Ioss_DatabaseIO.h" // for DatabaseIO
#include "Ioss_IOFactory.h"  // for IOFactory
#include "Ioss_Map.h"        // for Map
#include <cstddef>           // for size_t
#include <cstdint>           // for int64_t
#include <string>            // for string
#include <vector>            // for vector

#include "Ioss_State.h" // for State
#include "iogs_export.h"
#include "vtk_ioss_mangle.h"

namespace Iogs {
  class GeneratedMesh;
} // namespace Iogs
namespace Ioss {
  class CommSet;
  class EdgeBlock;
  class EdgeSet;
  class ElementBlock;
  class ElementSet;
  class FaceBlock;
  class FaceSet;
  class Field;
  class GroupingEntity;
  class NodeBlock;
  class NodeSet;
  class PropertyManager;
  class Region;
  class SideBlock;
  class SideSet;
  class StructuredBlock;
} // namespace Ioss

namespace Ioss {
  class EntityBlock;
  class Assembly;
  class Blob;
  class Map;
} // namespace Ioss

/** \brief A namespace for the gen_struc database format.
 */
namespace Iogs {

  class IOGS_EXPORT IOFactory : public Ioss::IOFactory
  {
  public:
    static const IOFactory *factory();

  private:
    IOFactory();
    IOSS_NODISCARD Ioss::DatabaseIO *make_IO(const std::string           &filename,
                                             Ioss::DatabaseUsage          db_usage,
                                             Ioss_MPI_Comm                communicator,
                                             const Ioss::PropertyManager &props) const override;
  };

  class IOGS_EXPORT DatabaseIO : public Ioss::DatabaseIO
  {
  public:
    DatabaseIO(Ioss::Region *region, const std::string &filename, Ioss::DatabaseUsage db_usage,
               Ioss_MPI_Comm communicator, const Ioss::PropertyManager &props);

    ~DatabaseIO() override;

    IOSS_NODISCARD std::string get_format() const override { return "Generated_Structured"; }

    // Check capabilities of input/output database...  Returns an
    // unsigned int with the supported Ioss::EntityTypes or'ed
    // together. If "return_value & Ioss::EntityType" is set, then the
    // database supports that type (e.g. return_value & Ioss::FACESET)
    IOSS_NODISCARD unsigned entity_field_support() const override;

    IOSS_NODISCARD int int_byte_size_db() const override { return int_byte_size_api(); }

    IOSS_NODISCARD const GeneratedMesh *get_gen_struc_mesh() const { return m_generatedMesh; }

    void setGeneratedMesh(Iogs::GeneratedMesh *generatedMesh) { m_generatedMesh = generatedMesh; }

    IOSS_NODISCARD const Ioss::NameList &get_sideset_names() const { return m_sideset_names; }

  private:
    void read_meta_data_nl() override;

    bool begin_nl(Ioss::State state) override;
    bool end_nl(Ioss::State state) override;

    bool begin_state_nl(int state, double time) override;

    void                get_step_times_nl() override;
    std::vector<double> get_db_step_times_nl() override;

    void get_nodeblocks();
    void get_structured_blocks();
    void get_nodesets();
    void get_sidesets();
    void get_commsets();

    IOSS_NODISCARD const Ioss::Map &get_node_map() const;
    IOSS_NODISCARD const Ioss::Map &get_element_map() const;

    IOSS_NOOP_GFI(Ioss::ElementBlock)
    IOSS_NOOP_GFI(Ioss::EdgeBlock)
    IOSS_NOOP_GFI(Ioss::FaceBlock)
    IOSS_NOOP_GFI(Ioss::NodeSet)
    IOSS_NOOP_GFI(Ioss::EdgeSet)
    IOSS_NOOP_GFI(Ioss::FaceSet)
    IOSS_NOOP_GFI(Ioss::ElementSet)
    IOSS_NOOP_GFI(Ioss::SideSet)
    IOSS_NOOP_GFI(Ioss::Assembly)
    IOSS_NOOP_GFI(Ioss::Blob)

    // Input only database -- these will never be called...
    IOSS_NOOP_PFI(Ioss::Region)
    IOSS_NOOP_PFI(Ioss::NodeBlock)
    IOSS_NOOP_PFI(Ioss::EdgeBlock)
    IOSS_NOOP_PFI(Ioss::FaceBlock)
    IOSS_NOOP_PFI(Ioss::ElementBlock)
    IOSS_NOOP_PFI(Ioss::StructuredBlock)
    IOSS_NOOP_PFI(Ioss::SideBlock)
    IOSS_NOOP_PFI(Ioss::NodeSet)
    IOSS_NOOP_PFI(Ioss::EdgeSet)
    IOSS_NOOP_PFI(Ioss::FaceSet)
    IOSS_NOOP_PFI(Ioss::ElementSet)
    IOSS_NOOP_PFI(Ioss::SideSet)
    IOSS_NOOP_PFI(Ioss::CommSet)
    IOSS_NOOP_PFI(Ioss::Assembly)
    IOSS_NOOP_PFI(Ioss::Blob)

    int64_t get_field_internal(const Ioss::Region *reg, const Ioss::Field &field, void *data,
                               size_t data_size) const override;
    int64_t get_field_internal(const Ioss::NodeBlock *nb, const Ioss::Field &field, void *data,
                               size_t data_size) const override;
    int64_t get_field_internal(const Ioss::StructuredBlock *sb, const Ioss::Field &field,
                               void *data, size_t data_size) const override;
    int64_t get_field_internal(const Ioss::SideBlock *sb, const Ioss::Field &field, void *data,
                               size_t data_size) const override;
    int64_t get_field_internal(const Ioss::CommSet *cs, const Ioss::Field &field, void *data,
                               size_t data_size) const override;

    void add_transient_fields(Ioss::GroupingEntity *entity);

    GeneratedMesh *m_generatedMesh{nullptr};
    Ioss::NameList m_sideset_names{};

    double currentTime{0.0};
    int    spatialDimension{3};

    int elementBlockCount{0};
    int nodesetCount{0};
    int sidesetCount{0};

    bool m_useVariableDf{true};
  };
} // namespace Iogs
