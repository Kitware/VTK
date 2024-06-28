// Copyright(C) 1999-2024 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#pragma once

#include "Iohb_Layout.h"
#include "Ioss_CodeTypes.h"
#include "Ioss_DBUsage.h"
#include "Ioss_DatabaseIO.h"
#include "Ioss_IOFactory.h"
#include "Ioss_State.h"
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <time.h>

#include "iohb_export.h"
#include "vtk_ioss_mangle.h"

namespace Ioss {
  class GroupingEntity;
  class EntityBlock;
  class CommSet;
  class EdgeBlock;
  class EdgeSet;
  class ElementBlock;
  class ElementSet;
  class FaceBlock;
  class FaceSet;
  class Field;
  class NodeBlock;
  class NodeSet;
  class PropertyManager;
  class Region;
  class SideBlock;
  class SideSet;
  class StructuredBlock;
  class Assembly;
  class Blob;
} // namespace Ioss

/** \brief A namespace for the heartbeat database format.
 */
namespace Iohb {
  class Layout;

  enum class Format { DEFAULT = 0, SPYHIS = 1, TEXT, TS_TEXT, CSV, TS_CSV };

  class IOHB_EXPORT IOFactory : public Ioss::IOFactory
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

  class IOHB_EXPORT DatabaseIO : public Ioss::DatabaseIO
  {
  public:
    DatabaseIO(Ioss::Region *region, const std::string &filename, Ioss::DatabaseUsage db_usage,
               Ioss_MPI_Comm communicator, const Ioss::PropertyManager &props);

    ~DatabaseIO() override;

    IOSS_NODISCARD std::string get_format() const override { return "HeartBeat"; }

    // Check capabilities of input/output database...  Returns an
    // unsigned int with the supported Ioss::EntityTypes or'ed
    // together. If "return_value & Ioss::EntityType" is set, then the
    // database supports that type (e.g. return_value & Ioss::FACESET)
    IOSS_NODISCARD unsigned entity_field_support() const override;

    IOSS_NODISCARD int int_byte_size_db() const override { return int_byte_size_api(); }

  private:
    IOSS_NODISCARD int64_t node_global_to_local_nl(int64_t /* global */,
                                                   bool /* must_exist */) const override
    {
      return 0;
    }
    IOSS_NODISCARD int64_t element_global_to_local_nl(int64_t /* global */) const override
    {
      return 0;
    }

    void read_meta_data_nl() override {}

    void flush_database_nl() const override;

    bool begin_nl(Ioss::State state) override;
    bool end_nl(Ioss::State state) override;

    bool begin_state_nl(int state, double time) override;
    bool end_state_nl(int state, double time) override;

    void initialize() const;

    IOSS_NOOP_GFI(Ioss::Region)
    IOSS_NOOP_GFI(Ioss::NodeBlock)
    IOSS_NOOP_GFI(Ioss::EdgeBlock)
    IOSS_NOOP_GFI(Ioss::FaceBlock)
    IOSS_NOOP_GFI(Ioss::ElementBlock)
    IOSS_NOOP_GFI(Ioss::StructuredBlock)
    IOSS_NOOP_GFI(Ioss::SideBlock)
    IOSS_NOOP_GFI(Ioss::NodeSet)
    IOSS_NOOP_GFI(Ioss::EdgeSet)
    IOSS_NOOP_GFI(Ioss::FaceSet)
    IOSS_NOOP_GFI(Ioss::ElementSet)
    IOSS_NOOP_GFI(Ioss::SideSet)
    IOSS_NOOP_GFI(Ioss::CommSet)
    IOSS_NOOP_GFI(Ioss::Assembly)
    IOSS_NOOP_GFI(Ioss::Blob)

    int64_t put_field_internal(const Ioss::Region *region, const Ioss::Field &field, void *data,
                               size_t data_size) const override;

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

    time_t timeLastFlush_{0};
    time_t flushInterval_{10};

    std::ostream           *logStream{nullptr};
    std::unique_ptr<Layout> layout_{};
    std::unique_ptr<Layout> legend_{};

    std::string defaultTsFormat{"[%H:%M:%S]"};
    std::string tsFormat{};
    std::string separator_{", "};
    int         precision_{5};
    int         fieldWidth_{0};
    bool        showLabels{true};
    bool        showLegend{false};
    bool        appendOutput{false};
    bool        addTimeField{false};

    bool   initialized_{false};
    bool   streamNeedsDelete{false};
    Format fileFormat{Format::DEFAULT};
  };
} // namespace Iohb
