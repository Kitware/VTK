// Copyright(C) 1999-2020 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include <Ioss_DatabaseIO.h>
#include <Ioss_ElementTopology.h>
#include <Ioss_FaceBlock.h>
#include <Ioss_Property.h>
#include <cstddef>
#include <string>

#include "Ioss_EntityBlock.h"
#include "Ioss_PropertyManager.h"

namespace Ioss {
  class Field;
} // namespace Ioss

/** \brief Create a face block.
 *
 *  \param[in] io_database The database associated with the region containing the face block.
 *  \param[in] my_name The face block's name.
 *  \param[in] face_type The name of the face topology type for the face block.
 *  \param[in] number_faces The number of faces in the face block.
 */
Ioss::FaceBlock::FaceBlock(Ioss::DatabaseIO *io_database, const std::string &my_name,
                           const std::string &face_type, int64_t number_faces)
    : Ioss::EntityBlock(io_database, my_name, face_type, number_faces)
{
  if (topology()->master_element_name() != face_type && topology()->name() != face_type) {
    // Maintain original face type on output database if possible.
    properties.add(Ioss::Property("original_face_type", face_type));
  }
}

Ioss::FaceBlock::~FaceBlock() = default;

Ioss::Property Ioss::FaceBlock::get_implicit_property(const std::string &my_name) const
{
  return Ioss::EntityBlock::get_implicit_property(my_name);
}

int64_t Ioss::FaceBlock::internal_get_field_data(const Ioss::Field &field, void *data,
                                                 size_t data_size) const
{
  return get_database()->get_field(this, field, data, data_size);
}

int64_t Ioss::FaceBlock::internal_put_field_data(const Ioss::Field &field, void *data,
                                                 size_t data_size) const
{
  return get_database()->put_field(this, field, data, data_size);
}
