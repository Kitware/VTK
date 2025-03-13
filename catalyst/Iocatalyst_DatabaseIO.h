// Copyright(C) 1999-2020, 2022, 2023 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#pragma once

#include "iocatalyst_export.h"
#include "vtk_ioss_mangle.h"

#include "Ioss_DBUsage.h"
#include "Ioss_DatabaseIO.h"
#include "Ioss_EntitySet.h"
#include "Ioss_Region.h"  // for Region, SideSetContainer, etc
#include "Ioss_SideSet.h" // for SideBlockContainer, SideSet

#include "Iocatalyst_CatalystManager.h"
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

    using RegionContainer = std::vector<Ioss::Region *>;

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
    std::string get_format() const override { return "CATALYST2"; }

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

    void *get_catalyst_conduit_node();

    void print_catalyst_conduit_node();

    std::string get_catalyst_dump_dir() const;

    void set_split_type_changed(bool split_type_changed) { split_type_c = split_type_changed; }
    IOSS_NODISCARD bool split_type_changed() const { return split_type_c; }

  private:
    bool begin_nl(Ioss::State state) override;
    bool end_nl(Ioss::State state) override;

    void                read_meta_data_nl() override;
    void                get_step_times_nl() override;
    std::vector<double> get_db_step_times_nl() override;

    bool begin_state_nl(int state, double time) override;
    bool end_state_nl(int state, double time) override;

    CatalystManager::CatalystPipelineID catPipeID;

    void
    compute_block_membership_nl(Ioss::SideBlock * /* efblock */,
                                std::vector<std::string> & /* block_membership */) const override
    {
    }

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
    int64_t get_field_internal(const Ioss::SideSet *ss, const Ioss::Field &field, void *data,
                               size_t data_size) const override;
    int64_t get_field_internal(const Ioss::CommSet *cs, const Ioss::Field &field, void *data,
                               size_t data_size) const override;
    int64_t get_field_internal(const Ioss::Assembly *as, const Ioss::Field &field, void *data,
                               size_t data_size) const override;
    int64_t get_field_internal(const Ioss::Blob *bl, const Ioss::Field &field, void *data,
                               size_t data_size) const override;
    int64_t get_field_internal(const Ioss::StructuredBlock *sb, const Ioss::Field &field,
                               void *data, size_t data_size) const override;

    int64_t get_zc_field_internal(const Ioss::Region *reg, const Ioss::Field &field, void **data,
                                  size_t *data_size) const override;
    int64_t get_zc_field_internal(const Ioss::NodeBlock *nb, const Ioss::Field &field, void **data,
                                  size_t *data_size) const override;
    int64_t get_zc_field_internal(const Ioss::EdgeBlock *eb, const Ioss::Field &field, void **data,
                                  size_t *data_size) const override;
    int64_t get_zc_field_internal(const Ioss::FaceBlock *fb, const Ioss::Field &field, void **data,
                                  size_t *data_size) const override;
    int64_t get_zc_field_internal(const Ioss::ElementBlock *eb, const Ioss::Field &field,
                                  void **data, size_t *data_size) const override;
    int64_t get_zc_field_internal(const Ioss::SideBlock *sb, const Ioss::Field &field, void **data,
                                  size_t *data_size) const override;
    int64_t get_zc_field_internal(const Ioss::NodeSet *ns, const Ioss::Field &field, void **data,
                                  size_t *data_size) const override;
    int64_t get_zc_field_internal(const Ioss::EdgeSet *es, const Ioss::Field &field, void **data,
                                  size_t *data_size) const override;
    int64_t get_zc_field_internal(const Ioss::FaceSet *fs, const Ioss::Field &field, void **data,
                                  size_t *data_size) const override;
    int64_t get_zc_field_internal(const Ioss::ElementSet *es, const Ioss::Field &field, void **data,
                                  size_t *data_size) const override;
    int64_t get_zc_field_internal(const Ioss::SideSet *ss, const Ioss::Field &field, void **data,
                                  size_t *data_size) const override;
    int64_t get_zc_field_internal(const Ioss::CommSet *cs, const Ioss::Field &field, void **data,
                                  size_t *data_size) const override;
    int64_t get_zc_field_internal(const Ioss::Assembly *as, const Ioss::Field &field, void **data,
                                  size_t *data_size) const override;
    int64_t get_zc_field_internal(const Ioss::Blob *bl, const Ioss::Field &field, void **data,
                                  size_t *data_size) const override;
    int64_t get_zc_field_internal(const Ioss::StructuredBlock *sb, const Ioss::Field &field,
                                  void **data, size_t *data_size) const override;

    int64_t put_field_internal(const Ioss::Region *reg, const Ioss::Field &field, void *data,
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
    int64_t put_field_internal(const Ioss::Assembly *as, const Ioss::Field &field, void *data,
                               size_t data_size) const override;
    int64_t put_field_internal(const Ioss::Blob *bl, const Ioss::Field &field, void *data,
                               size_t data_size) const override;
    int64_t put_field_internal(const Ioss::StructuredBlock *sb, const Ioss::Field &field,
                               void *data, size_t data_size) const override;

    class ImplementationT;
    std::unique_ptr<ImplementationT> Impl;
    bool                             useDeepCopy;
    bool                             split_type_c{false};
  };
} // namespace Iocatalyst
