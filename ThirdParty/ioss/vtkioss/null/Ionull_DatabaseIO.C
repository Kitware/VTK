// Copyright(C) 1999-2024 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include "Ioss_CodeTypes.h"
#include "null/Ionull_DatabaseIO.h"

#include "Ioss_DBUsage.h"
#include "Ioss_DatabaseIO.h"
#include "Ioss_EntityType.h"
#include "Ioss_Field.h"
#include "Ioss_State.h"

namespace Ioss {
  class Assembly;
  class Blob;
  class PropertyManager;
} // namespace Ioss

namespace Ionull {
  DatabaseIO::DatabaseIO(Ioss::Region *region, const std::string &filename,
                         Ioss::DatabaseUsage db_usage, Ioss_MPI_Comm communicator,
                         const Ioss::PropertyManager &props)
      : Ioss::DatabaseIO(region, filename, db_usage, communicator, props)
  {
  }

  void DatabaseIO::read_meta_data_nl() {}

  unsigned DatabaseIO::entity_field_support() const
  {
    return Ioss::NODEBLOCK | Ioss::EDGEBLOCK | Ioss::FACEBLOCK | Ioss::ELEMENTBLOCK |
           Ioss::NODESET | Ioss::EDGESET | Ioss::FACESET | Ioss::ELEMENTSET | Ioss::SIDESET |
           Ioss::SIDEBLOCK | Ioss::REGION | Ioss::SUPERELEMENT | Ioss::ASSEMBLY | Ioss::BLOB |
           Ioss::STRUCTUREDBLOCK;
  }

  bool DatabaseIO::begin_nl(Ioss::State /* state */) { return true; }

  bool DatabaseIO::end_nl(Ioss::State /* state */) { return true; }

  bool DatabaseIO::begin_state_nl(int /* state */, double) { return true; }

  bool DatabaseIO::end_state_nl(int /* state */, double) { return true; }

  int64_t DatabaseIO::put_field_internal(const Ioss::Region *, const Ioss::Field &field, void *,
                                         size_t data_size) const
  {
    return field.verify(data_size);
  }

  int64_t DatabaseIO::put_field_internal(const Ioss::NodeBlock *, const Ioss::Field &field, void *,
                                         size_t data_size) const
  {
    return field.verify(data_size);
  }

  int64_t DatabaseIO::put_field_internal(const Ioss::Assembly *, const Ioss::Field &field, void *,
                                         size_t data_size) const
  {
    return field.verify(data_size);
  }

  int64_t DatabaseIO::put_field_internal(const Ioss::ElementBlock *, const Ioss::Field &field,
                                         void *, size_t data_size) const
  {
    return field.verify(data_size);
  }

  int64_t DatabaseIO::put_field_internal(const Ioss::StructuredBlock *, const Ioss::Field &field,
                                         void *, size_t data_size) const
  {
    return field.verify(data_size);
  }

  int64_t DatabaseIO::put_field_internal(const Ioss::FaceBlock *, const Ioss::Field &field, void *,
                                         size_t data_size) const
  {
    return field.verify(data_size);
  }

  int64_t DatabaseIO::put_field_internal(const Ioss::EdgeBlock *, const Ioss::Field &field, void *,
                                         size_t data_size) const
  {
    return field.verify(data_size);
  }

  int64_t DatabaseIO::put_field_internal(const Ioss::ElementSet *, const Ioss::Field &field, void *,
                                         size_t data_size) const
  {
    return field.verify(data_size);
  }

  int64_t DatabaseIO::put_field_internal(const Ioss::CommSet *, const Ioss::Field &field, void *,
                                         size_t data_size) const
  {
    return field.verify(data_size);
  }

  int64_t DatabaseIO::put_field_internal(const Ioss::EdgeSet *, const Ioss::Field &field, void *,
                                         size_t data_size) const
  {
    return field.verify(data_size);
  }

  int64_t DatabaseIO::put_field_internal(const Ioss::FaceSet *, const Ioss::Field &field, void *,
                                         size_t data_size) const
  {
    return field.verify(data_size);
  }

  int64_t DatabaseIO::put_field_internal(const Ioss::NodeSet *, const Ioss::Field &field, void *,
                                         size_t data_size) const
  {
    return field.verify(data_size);
  }

  int64_t DatabaseIO::put_field_internal(const Ioss::SideSet *, const Ioss::Field &field, void *,
                                         size_t data_size) const
  {
    return field.verify(data_size);
  }

  int64_t DatabaseIO::put_field_internal(const Ioss::SideBlock *, const Ioss::Field &field, void *,
                                         size_t data_size) const
  {
    return field.verify(data_size);
  }

  int64_t DatabaseIO::put_field_internal(const Ioss::Blob *, const Ioss::Field &field, void *,
                                         size_t data_size) const
  {
    return field.verify(data_size);
  }

} // namespace Ionull
