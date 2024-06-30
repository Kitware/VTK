// Copyright(C) 1999-2021, 2023, 2024 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include "Ioss_DatabaseIO.h"
#include "Ioss_Field.h"
#include "Ioss_NodeBlock.h"
#include "Ioss_Property.h"
#include <cassert>
#include <cstddef>
#include <string>

#include "Ioss_BoundingBox.h"
#include "Ioss_CodeTypes.h"
#include "Ioss_EntityBlock.h"
#include "Ioss_FieldManager.h"
#include "Ioss_PropertyManager.h"

/** \brief Create a node block.
 *
 *  \param[in] io_database The database associated with the region containing the node block.
 *  \param[in] my_name The node block's name.
 *  \param[in] node_count The number of nodes in the node block.
 *  \param[in] degrees_of_freedom The number of degrees of freedom (or coordinates) per node.
 */
Ioss::NodeBlock::NodeBlock(Ioss::DatabaseIO *io_database, const std::string &my_name,
                           int64_t node_count, int64_t degrees_of_freedom)
    : Ioss::EntityBlock(io_database, my_name, "node", node_count)
{
  properties.add(Ioss::Property("component_degree", static_cast<int>(degrees_of_freedom)));

  std::string vector_name;
  assert(degrees_of_freedom == 1 || degrees_of_freedom == 2 || degrees_of_freedom == 3);

  if (degrees_of_freedom == 1) {
    vector_name = IOSS_SCALAR();
  }
  else if (degrees_of_freedom == 2) {
    vector_name = IOSS_VECTOR_2D();
  }
  else if (degrees_of_freedom == 3) {
    vector_name = IOSS_VECTOR_3D();
  }

  fields.add(Ioss::Field("mesh_model_coordinates", Ioss::Field::REAL, vector_name,
                         Ioss::Field::MESH, node_count));

  // Permit access 1-coordinate at a time
  fields.add(Ioss::Field("mesh_model_coordinates_x", Ioss::Field::REAL, IOSS_SCALAR(),
                         Ioss::Field::MESH, node_count));
  if (degrees_of_freedom > 1) {
    fields.add(Ioss::Field("mesh_model_coordinates_y", Ioss::Field::REAL, IOSS_SCALAR(),
                           Ioss::Field::MESH, node_count));
  }

  if (degrees_of_freedom > 2) {
    fields.add(Ioss::Field("mesh_model_coordinates_z", Ioss::Field::REAL, IOSS_SCALAR(),
                           Ioss::Field::MESH, node_count));
  }

  fields.add(Ioss::Field("node_connectivity_status", Ioss::Field::CHARACTER, IOSS_SCALAR(),
                         Ioss::Field::MESH, node_count));

  // The 1..global_node_count id.  In a parallel-decomposed run,
  // if maps the node back to its implicit position in the serial
  // undecomposed mesh file.  This is ONLY provided for backward-
  // compatibility and should not be used unless absolutely required.
  fields.add(
      Ioss::Field("implicit_ids", field_int_type(), IOSS_SCALAR(), Ioss::Field::MESH, node_count));

  fields.add(Ioss::Field("owning_processor", Ioss::Field::INT32, IOSS_SCALAR(), Ioss::Field::MESH,
                         node_count));
}

Ioss::NodeBlock::NodeBlock(const Ioss::NodeBlock &other) = default;

Ioss::Property Ioss::NodeBlock::get_implicit_property(const std::string &my_name) const
{
  return Ioss::EntityBlock::get_implicit_property(my_name);
}

int64_t Ioss::NodeBlock::internal_get_field_data(const Ioss::Field &field, void *data,
                                                 size_t data_size) const
{
  return get_database()->get_field(this, field, data, data_size);
}

int64_t Ioss::NodeBlock::internal_put_field_data(const Ioss::Field &field, void *data,
                                                 size_t data_size) const
{
  return get_database()->put_field(this, field, data, data_size);
}

int64_t Ioss::NodeBlock::internal_get_zc_field_data(const Field &field, void **data,
                                                    size_t *data_size) const
{
  return get_database()->get_zc_field(this, field, data, data_size);
}

Ioss::AxisAlignedBoundingBox Ioss::NodeBlock::get_bounding_box() const
{
  return get_database()->get_bounding_box(this);
}

bool Ioss::NodeBlock::operator==(const Ioss::NodeBlock &rhs) const
{
  return Ioss::EntityBlock::operator==(rhs);
}

bool Ioss::NodeBlock::operator!=(const Ioss::NodeBlock &rhs) const { return !(*this == rhs); }

bool Ioss::NodeBlock::equal(const Ioss::NodeBlock &rhs) const
{
  return Ioss::EntityBlock::equal(rhs);
}
