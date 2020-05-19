// Copyright(C) 1999-2017 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials provided
//       with the distribution.
//
//     * Neither the name of NTESS nor the names of its
//       contributors may be used to endorse or promote products derived
//       from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef IOSS_Iogs_DatabaseIO_h
#define IOSS_Iogs_DatabaseIO_h

#include "vtk_ioss_mangle.h"

#include "Ioss_State.h" // for State
#include <Ioss_CodeTypes.h>
#include <Ioss_DBUsage.h>    // for DatabaseUsage
#include <Ioss_DatabaseIO.h> // for DatabaseIO
#include <Ioss_IOFactory.h>  // for IOFactory
#include <Ioss_Map.h>        // for Map
#include <cstddef>           // for size_t
#include <cstdint>           // for int64_t
#include <string>            // for string
#include <vector>            // for vector

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
} // namespace Ioss

/** \brief A namespace for the gen_struc database format.
 */
namespace Iogs {

  class IOFactory : public Ioss::IOFactory
  {
  public:
    static const IOFactory *factory();

  private:
    IOFactory();
    Ioss::DatabaseIO *make_IO(const std::string &filename, Ioss::DatabaseUsage db_usage,
                              MPI_Comm                     communicator,
                              const Ioss::PropertyManager &props) const override;
  };

  class DatabaseIO : public Ioss::DatabaseIO
  {
  public:
    DatabaseIO(Ioss::Region *region, const std::string &filename, Ioss::DatabaseUsage db_usage,
               MPI_Comm communicator, const Ioss::PropertyManager &props);
    DatabaseIO(const DatabaseIO &from) = delete;
    DatabaseIO &operator=(const DatabaseIO &from) = delete;

    ~DatabaseIO() override;

    const std::string get_format() const override { return "Generated_Structured"; }

    // Check capabilities of input/output database...  Returns an
    // unsigned int with the supported Ioss::EntityTypes or'ed
    // together. If "return_value & Ioss::EntityType" is set, then the
    // database supports that type (e.g. return_value & Ioss::FACESET)
    unsigned entity_field_support() const override;

    int int_byte_size_db() const override { return int_byte_size_api(); }

    const GeneratedMesh *get_gen_struc_mesh() const { return m_generatedMesh; }

    void setGeneratedMesh(Iogs::GeneratedMesh *generatedMesh) { m_generatedMesh = generatedMesh; }

    const std::vector<std::string> &get_sideset_names() const { return m_sideset_names; }

  private:
    void read_meta_data__() override;

    bool begin__(Ioss::State state) override;
    bool end__(Ioss::State state) override;

    bool begin_state__(int state, double time) override;

    void get_step_times__() override;
    void get_nodeblocks();
    void get_structured_blocks();
    void get_nodesets();
    void get_sidesets();
    void get_commsets();

    const Ioss::Map &get_node_map() const;
    const Ioss::Map &get_element_map() const;

    int64_t get_field_internal(const Ioss::Region *reg, const Ioss::Field &field, void *data,
                               size_t data_size) const override;
    int64_t get_field_internal(const Ioss::NodeBlock *nb, const Ioss::Field &field, void *data,
                               size_t data_size) const override;
    int64_t get_field_internal(const Ioss::EdgeBlock *nb, const Ioss::Field &field, void *data,
                               size_t data_size) const override;
    int64_t get_field_internal(const Ioss::FaceBlock *nb, const Ioss::Field &field, void *data,
                               size_t data_size) const override;
    int64_t get_field_internal(const Ioss::ElementBlock * /* eb */, const Ioss::Field & /* field */,
                               void * /* data */, size_t /* data_size */) const override
    {
      return -1;
    }

    int64_t get_field_internal(const Ioss::StructuredBlock *sb, const Ioss::Field &field,
                               void *data, size_t data_size) const override;
    int64_t get_field_internal(const Ioss::SideBlock *ef_blk, const Ioss::Field &field, void *data,
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
    int64_t get_field_internal(const Ioss::Assembly* /*sb*/, const Ioss::Field & /*field*/,
                               void * /*data*/, size_t /*data_size*/) const override
    {
      return 0;
    }

    int64_t get_field_internal(const Ioss::Blob * /*sb*/, const Ioss::Field & /*field*/,
                               void * /*data*/, size_t /*data_size*/) const override
    {
      return 0;
    }

    int64_t put_field_internal(const Ioss::Region *reg, const Ioss::Field &field, void *data,
                               size_t data_size) const override;
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
    int64_t put_field_internal(const Ioss::StructuredBlock * /* sb */,
                               const Ioss::Field & /* field */, void * /* data */,
                               size_t /* data_size */) const override
    {
      return -1;
    }
    int64_t put_field_internal(const Ioss::Assembly* /*sb*/, const Ioss::Field & /*field*/,
                               void * /*data*/, size_t /*data_size*/) const override
    {
      return 0;
    }

    int64_t put_field_internal(const Ioss::Blob * /*sb*/, const Ioss::Field & /*field*/,
                               void * /*data*/, size_t /*data_size*/) const override
    {
      return 0;
    }

    void add_transient_fields(Ioss::GroupingEntity *entity);

    GeneratedMesh *          m_generatedMesh{nullptr};
    std::vector<std::string> m_sideset_names{};

    double currentTime{0.0};
    int    spatialDimension{3};

    int elementBlockCount{0};
    int nodesetCount{0};
    int sidesetCount{0};

    bool m_useVariableDf{true};
  };
} // namespace Iogs
#endif // IOSS_Iogs_DatabaseIO_h
