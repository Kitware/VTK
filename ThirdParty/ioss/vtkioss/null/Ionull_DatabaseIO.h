// Copyright(C) 2023, 2024 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

// -*- Mode: c++ -*-
#pragma once

#include "Ioss_DBUsage.h"
#include "Ioss_DatabaseIO.h"
#include <stddef.h>
#include <stdint.h>
#include <string>

#include "Ioss_CodeTypes.h"
#include "Ioss_DataSize.h"
#include "Ioss_State.h"
#include "ionull_export.h"
#include "vtk_ioss_mangle.h"

namespace Ioss {
  class GroupingEntity;
  class Region;
  class EntityBlock;
  class NodeBlock;
  class EdgeBlock;
  class FaceBlock;
  class ElementBlock;
  class EntitySet;
  class Field;
  class NodeSet;
  class EdgeSet;
  class FaceSet;
  class ElementSet;
  class SideBlock;
  class SideSet;
  class StructuredBlock;
  class CommSet;
  class ElementTopology;
  class Assembly;
  class Blob;
  class PropertyManager;
} // namespace Ioss

namespace Ionull {
  class IONULL_EXPORT DatabaseIO : public Ioss::DatabaseIO
  {
  public:
    DatabaseIO(Ioss::Region *region, const std::string &filename, Ioss::DatabaseUsage db_usage,
               Ioss_MPI_Comm communicator, const Ioss::PropertyManager &props);

    IOSS_NODISCARD std::string get_format() const override { return "Null"; }

    // Check capabilities of input/output database...  Returns an
    // unsigned int with the supported Ioss::EntityTypes or'ed
    // together. If "return_value & Ioss::EntityType" is set, then the
    // database supports that type (e.g. return_value & Ioss::FACESET)
    IOSS_NODISCARD unsigned entity_field_support() const override;

    IOSS_NODISCARD int int_byte_size_db() const override { return 8; }
    void               set_int_byte_size_api(Ioss::DataSize) const override {}

    bool begin_nl(Ioss::State state) override;
    bool end_nl(Ioss::State state) override;

    bool begin_state_nl(int state, double time) override;
    bool end_state_nl(int state, double time) override;

    IOSS_NODISCARD bool ok_nl(bool, std::string *, int *) const override { return true; }

  private:
    // Input only database -- these will never be called...
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

    void read_meta_data_nl() override;
    void get_step_times_nl() override {}

    int64_t put_field_internal(const Ioss::Region *reg, const Ioss::Field &field, void *data,
                               size_t data_size) const override;
    int64_t put_field_internal(const Ioss::Blob *blob, const Ioss::Field &field, void *data,
                               size_t data_size) const override;
    int64_t put_field_internal(const Ioss::Assembly *assem, const Ioss::Field &field, void *data,
                               size_t data_size) const override;
    int64_t put_field_internal(const Ioss::NodeBlock *nb, const Ioss::Field &field, void *data,
                               size_t data_size) const override;
    int64_t put_field_internal(const Ioss::EdgeBlock *eb, const Ioss::Field &field, void *data,
                               size_t data_size) const override;
    int64_t put_field_internal(const Ioss::FaceBlock *eb, const Ioss::Field &field, void *data,
                               size_t data_size) const override;
    int64_t put_field_internal(const Ioss::StructuredBlock *sb, const Ioss::Field &field,
                               void *data, size_t data_size) const override;
    int64_t put_field_internal(const Ioss::ElementBlock *eb, const Ioss::Field &field, void *data,
                               size_t data_size) const override;
    int64_t put_field_internal(const Ioss::SideBlock *fb, const Ioss::Field &field, void *data,
                               size_t data_size) const override;
    int64_t put_field_internal(const Ioss::NodeSet *ns, const Ioss::Field &field, void *data,
                               size_t data_size) const override;
    int64_t put_field_internal(const Ioss::EdgeSet *ns, const Ioss::Field &field, void *data,
                               size_t data_size) const override;
    int64_t put_field_internal(const Ioss::FaceSet *ns, const Ioss::Field &field, void *data,
                               size_t data_size) const override;
    int64_t put_field_internal(const Ioss::ElementSet *ns, const Ioss::Field &field, void *data,
                               size_t data_size) const override;
    int64_t put_field_internal(const Ioss::SideSet *ss, const Ioss::Field &field, void *data,
                               size_t data_size) const override;
    int64_t put_field_internal(const Ioss::CommSet *cs, const Ioss::Field &field, void *data,
                               size_t data_size) const override;
  };
} // namespace Ionull
