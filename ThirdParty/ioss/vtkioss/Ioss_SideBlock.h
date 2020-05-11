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

#ifndef IOSS_Ioss_SideBlock_h
#define IOSS_Ioss_SideBlock_h

#include "vtk_ioss_mangle.h"

#include <Ioss_ElementBlock.h>
#include <Ioss_EntityBlock.h> // for EntityBlock
#include <Ioss_EntityType.h>  // for EntityType, etc
#include <Ioss_Property.h>    // for Property
#include <Ioss_SideSet.h>
#include <cstddef> // for size_t
#include <cstdint> // for int64_t
#include <string>  // for string
#include <vector>  // for vector
namespace Ioss {
  class DatabaseIO;
} // namespace Ioss
namespace Ioss {
  class ElementTopology;
} // namespace Ioss
namespace Ioss {
  class Field;
} // namespace Ioss

namespace Ioss {

  /** \brief A collection of element sides having the same topology.
   */
  class SideBlock : public EntityBlock
  {
  public:
    friend class SideSet;

    SideBlock(DatabaseIO *io_database, const std::string &my_name, const std::string &side_type,
              const std::string &element_type, size_t side_count);

    SideBlock(const SideBlock &other);

    std::string type_string() const override { return "SideBlock"; }
    std::string short_type_string() const override { return "sideblock"; }
    std::string contains_string() const override { return "Element/Side pair"; }
    EntityType  type() const override { return SIDEBLOCK; }

    const SideSet *             owner() const { return owner_; }
    const Ioss::GroupingEntity *contained_in() const override { return owner_; }

    void block_membership(std::vector<std::string> &block_members) override;

    // Handle implicit properties -- These are calcuated from data stored
    // in the grouping entity instead of having an explicit value assigned.
    // An example would be 'element_block_count' for a region.
    Property get_implicit_property(const std::string &my_name) const override;

    // For faceblock, edgeblock, if they are split by element block, then this
    // will be non-nullptr and is a pointer to the parent element block for this
    // faceblock or edgeblock. Has no meaning for other EntityBlock types or split
    // types.
    const ElementBlock *parent_element_block() const
    {
      return dynamic_cast<const ElementBlock *>(parentBlock_);
    }

    void set_parent_element_block(const ElementBlock *element_block)
    {
      parentBlock_ = element_block;
    }

    const EntityBlock *parent_block() const { return parentBlock_; }
    void               set_parent_block(const EntityBlock *block) { parentBlock_ = block; }

    // Describes the contained entities element block topology
    const ElementTopology *parent_element_topology() const { return parentTopology_; }

    // For faceblock, edgeblock, return whether the surface is applied
    // to the same face/edge for all elements in the surface. If not,
    // return 0; otherwise return the consistent face number.
    int  get_consistent_side_number() const;
    void set_consistent_side_number(int side) { consistentSideNumber = side; }

  protected:
    int64_t internal_get_field_data(const Field &field, void *data,
                                    size_t data_size) const override;

    int64_t internal_put_field_data(const Field &field, void *data,
                                    size_t data_size) const override;

  private:
    const SideSet *    owner_{nullptr};
    ElementTopology *  parentTopology_{nullptr}; // Topology of parent element (if any)
    const EntityBlock *parentBlock_{nullptr};

    // Pointer to the SideSet (if any) that contains this side block.
    std::vector<std::string> blockMembership{}; // What element blocks do the
                                                // elements in this sideset belong to.
    mutable int consistentSideNumber{-1};
  };
} // namespace Ioss
#endif
