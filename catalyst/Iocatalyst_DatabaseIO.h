// Copyright(C) 1999-2020, 2022 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#pragma once

#include "iocatalyst_export.h"

#include "Ioss_EntitySet.h"
#include "Ioss_Region.h"  // for Region, SideSetContainer, etc
#include "Ioss_SideSet.h" // for SideBlockContainer, SideSet
#include <Ioss_DBUsage.h>
#include <Ioss_DatabaseIO.h>

#include "Ioss_Field.h" // for Field, etc

#include <memory> // for std::unique_ptr

/** \brief A namespace for the Catalyst 2.0 database format.
 */
namespace Iocatalyst {
  class IOCATALYST_EXPORT DatabaseIO : public Ioss::DatabaseIO
  {
    using Superclass = Ioss::DatabaseIO;

  public:
    DatabaseIO(Ioss::Region *region, const std::string &filename, Ioss::DatabaseUsage db_usage,
               Ioss_MPI_Comm communicator, const Ioss::PropertyManager &props);
    ~DatabaseIO() override;

    // Check capabilities of input/output database...  Returns an
    // unsigned int with the supported Ioss::EntityTypes or'ed
    // together. If "return_value & Ioss::EntityType" is set, then the
    // database supports that type (e.g. return_value & Ioss::FACESET)
    unsigned entity_field_support() const override;

    /** If there is a single block of nodes in the model, then it is
     *  considered a node_major() database.  If instead the nodes are
     * local to each element block or structured block, then it is
     *   not a node_major database.  Exodus is node major, CGNS is not.
     */
    bool node_major() const override { return true; }

    // Do anything that might be needed to the database prior to it
    // being closed and destructed.
    void finalize_database() const override {}

    /** Return a string specifying underlying format of database (exodus, cgns, ...) */
    const std::string get_format() const { return "CATALYST2"; }

    /** \brief Determine whether the database needs information about process ownership of nodes.
     *
     *  \returns True if database needs information about process ownership of nodes.
     */
    bool needs_shared_node_information() const override { return false; }

    bool internal_edges_available() const override { return false; }
    bool internal_faces_available() const override { return false; }

    /** \brief Get the length of the longest name in the database file.
     *
     *  \returns The length, or 0 for unlimited.
     */
    int  maximum_symbol_length() const override { return 0; } // Default is unlimited...
    void set_maximum_symbol_length(int /* requested_symbol_size */) override {
    } // Default does nothing...

    int int_byte_size_db() const override { return 8; } //! Returns 4 or 8

    /**
     * \returns True is fields must be deep-copied. Set 'SHALLOW_COPY_FIELDS' on
     * database properties to avoid deep-copying of fields. Default is to
     * shallow copy.
     */
    bool deep_copy() const { return this->useDeepCopy; }

  private:
    bool open_group__(const std::string & /* group_name */) override { return false; }
    bool create_subgroup__(const std::string & /* group_name */) override { return false; }

    bool begin__(Ioss::State state) override;
    bool end__(Ioss::State state) override;

    void read_meta_data__() override;
    void get_step_times__() override;

    bool begin_state__(int state, double time) override;
    bool end_state__(int state, double time) override;

    void
    compute_block_membership__(Ioss::SideBlock * /* efblock */,
                               std::vector<std::string> & /* block_membership */) const override
    {
    }

    int64_t get_field_internal(const Ioss::Region *reg, const Ioss::Field &field, void *data,
                               size_t data_size) const override
    {
      return -1;
    }
    int64_t get_field_internal(const Ioss::NodeBlock *nb, const Ioss::Field &field, void *data,
                               size_t data_size) const override;
    int64_t get_field_internal(const Ioss::EdgeBlock *nb, const Ioss::Field &field, void *data,
                               size_t data_size) const override;
    int64_t get_field_internal(const Ioss::FaceBlock *nb, const Ioss::Field &field, void *data,
                               size_t data_size) const override;
    int64_t get_field_internal(const Ioss::ElementBlock *eb, const Ioss::Field &field, void *data,
                               size_t data_size) const override;
    int64_t get_field_internal(const Ioss::SideBlock *fb, const Ioss::Field &field, void *data,
                               size_t data_size) const override;
    int64_t get_field_internal(const Ioss::NodeSet *ns, const Ioss::Field &field, void *data,
                               size_t data_size) const override;
    int64_t get_field_internal(const Ioss::EdgeSet *ns, const Ioss::Field &field, void *data,
                               size_t data_size) const override;
    int64_t get_field_internal(const Ioss::FaceSet *ns, const Ioss::Field &field, void *data,
                               size_t data_size) const override;
    int64_t get_field_internal(const Ioss::ElementSet *ns, const Ioss::Field &field, void *data,
                               size_t data_size) const override;
    int64_t get_field_internal(const Ioss::SideSet *fs, const Ioss::Field &field, void *data,
                               size_t data_size) const override;
    int64_t get_field_internal(const Ioss::CommSet *cs, const Ioss::Field &field, void *data,
                               size_t data_size) const override;
    int64_t get_field_internal(const Ioss::Assembly * /*as*/, const Ioss::Field & /*field*/,
                               void * /*data*/, size_t /*data_size*/) const override;
    int64_t get_field_internal(const Ioss::Blob * /*bl*/, const Ioss::Field & /*field*/,
                               void * /*data*/, size_t /*data_size*/) const override;
    int64_t get_field_internal(const Ioss::StructuredBlock * /*sb*/, const Ioss::Field & /*field*/,
                               void * /*data*/, size_t /*data_size*/) const override;

    int64_t put_field_internal(const Ioss::Region *reg, const Ioss::Field &field, void *data,
                               size_t data_size) const override
    {
      return -1;
    }
    int64_t put_field_internal(const Ioss::NodeBlock *nb, const Ioss::Field &field, void *data,
                               size_t data_size) const override;
    int64_t put_field_internal(const Ioss::EdgeBlock *nb, const Ioss::Field &field, void *data,
                               size_t data_size) const override;
    int64_t put_field_internal(const Ioss::FaceBlock *nb, const Ioss::Field &field, void *data,
                               size_t data_size) const override;
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
    int64_t put_field_internal(const Ioss::SideSet *fs, const Ioss::Field &field, void *data,
                               size_t data_size) const override;
    int64_t put_field_internal(const Ioss::CommSet *cs, const Ioss::Field &field, void *data,
                               size_t data_size) const override;
    int64_t put_field_internal(const Ioss::Assembly * /*as*/, const Ioss::Field & /*field*/,
                               void * /*data*/, size_t /*data_size*/) const override;
    int64_t put_field_internal(const Ioss::Blob * /*bl*/, const Ioss::Field & /*field*/,
                               void * /*data*/, size_t /*data_size*/) const override;
    int64_t put_field_internal(const Ioss::StructuredBlock * /*sb*/, const Ioss::Field & /*field*/,
                               void * /*data*/, size_t /*data_size*/) const override;

    class ImplementationT;
    std::unique_ptr<ImplementationT> Impl;
    bool                             useDeepCopy;
  };
} // namespace Iocatalyst
