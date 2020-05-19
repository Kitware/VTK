// Copyright(C) 1999-2017, 2020 National Technology & Engineering Solutions
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

#ifndef IOSS_Ioss_SideSet_h
#define IOSS_Ioss_SideSet_h

#include "vtk_ioss_mangle.h"

#include "Ioss_EntityType.h"     // for EntityType, etc
#include <Ioss_GroupingEntity.h> // for GroupingEntity
#include <Ioss_Property.h>       // for Property
#include <cstddef>               // for size_t
#include <cstdint>               // for int64_t
#include <string>                // for string
#include <vector>                // for vector

namespace Ioss {
  class DatabaseIO;
  class Field;
  class SideBlock;

  using SideBlockContainer = std::vector<SideBlock *>;

  /** \brief A collection of element sides.
   */
  class SideSet : public GroupingEntity
  {
  public:
    SideSet(DatabaseIO *io_database, const std::string &my_name);
    SideSet(const SideSet &);
    ~SideSet() override;

    std::string type_string() const override { return "SideSet"; }
    std::string short_type_string() const override { return "surface"; }
    std::string contains_string() const override { return "Element/Side pair"; }
    EntityType  type() const override { return SIDESET; }

    bool                      add(SideBlock *side_block);
    const SideBlockContainer &get_side_blocks() const;
    SideBlock *               get_side_block(const std::string &my_name) const;
    size_t                    side_block_count() const { return sideBlocks.size(); }

    size_t     block_count() const { return sideBlocks.size(); }
    SideBlock *get_block(size_t which) const;

    void block_membership(std::vector<std::string> &block_members) override;

    // Handle implicit properties -- These are calcuated from data stored
    // in the grouping entity instead of having an explicit value assigned.
    // An example would be 'element_block_count' for a region.
    Property get_implicit_property(const std::string &my_name) const override;

    int max_parametric_dimension() const;

  protected:
    int64_t internal_get_field_data(const Field &field, void *data,
                                    size_t data_size) const override;

    int64_t internal_put_field_data(const Field &field, void *data,
                                    size_t data_size) const override;

  private:
    SideBlockContainer       sideBlocks;
    std::vector<std::string> blockMembership; // What element blocks do the
                                              // elements in this sideset belong to.
  };
} // namespace Ioss
#endif
