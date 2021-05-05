// Copyright(C) 1999-2020 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include "Ioss_BoundingBox.h"  // for AxisAlignedBoundingBox
#include "Ioss_EntityBlock.h"  // for EntityBlock
#include "Ioss_FieldManager.h" // for FieldManager
#include <Ioss_DatabaseIO.h>   // for DatabaseIO
#include <Ioss_ElementBlock.h>
#include <Ioss_Field.h>    // for Field, etc
#include <Ioss_Property.h> // for Property
#include <cstddef>         // for size_t
#include <string>          // for string
#include <vector>          // for vector

namespace Ioss {
  class Field;

  /** \brief Create an element block.
   *
   *  \param[in] io_database The database associated with the region containing the element block.
   *  \param[in] my_name The element block's name.
   *  \param[in] element_type The name of the element topology type for the element block.
   *  \param[in] number_elements The number of elements in the element block.
   */
  ElementBlock::ElementBlock(DatabaseIO *io_database, const std::string &my_name,
                             const std::string &element_type, int64_t number_elements)
      : EntityBlock(io_database, my_name, element_type, number_elements)
  {
    // The 1..global_element_count id.  In a parallel-decomposed run,
    // if maps the element back to its implicit position in the serial
    // undecomposed mesh file.  This is ONLY provided for backward-
    // compatibility and should not be used unless absolutely required.
    fields.add(Ioss::Field("implicit_ids", field_int_type(), "scalar", Ioss::Field::MESH,
                           number_elements));
  }

  ElementBlock::~ElementBlock() = default;

  Property ElementBlock::get_implicit_property(const std::string &my_name) const
  {
    return EntityBlock::get_implicit_property(my_name);
  }

  int64_t ElementBlock::internal_get_field_data(const Field &field, void *data,
                                                size_t data_size) const
  {
    return get_database()->get_field(this, field, data, data_size);
  }

  int64_t ElementBlock::internal_put_field_data(const Field &field, void *data,
                                                size_t data_size) const
  {
    return get_database()->put_field(this, field, data, data_size);
  }

  void ElementBlock::get_block_adjacencies(std::vector<std::string> &block_adjacency) const
  {
    get_database()->get_block_adjacencies(this, block_adjacency);
  }

  AxisAlignedBoundingBox ElementBlock::get_bounding_box() const
  {
    return get_database()->get_bounding_box(this);
  }

  bool ElementBlock::operator==(const ElementBlock &rhs) const
  {
    return EntityBlock::operator==(rhs);
  }

  bool Ioss::ElementBlock::operator!=(const ElementBlock &rhs) const { return !(*this == rhs); }

  bool ElementBlock::equal(const ElementBlock &rhs) const { return EntityBlock::equal(rhs); }
} // namespace Ioss
