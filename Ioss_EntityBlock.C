// Copyright(C) 1999-2021 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include <Ioss_DatabaseIO.h>
#include <Ioss_ElementTopology.h>
#include <Ioss_EntityBlock.h>
#include <Ioss_Field.h>
#include <Ioss_Property.h>
#include <cstddef>
#include "vtk_fmt.h"
#include VTK_FMT(fmt/ostream.h)
#include <ostream>
#include <string>

#include "Ioss_FieldManager.h"
#include "Ioss_GroupingEntity.h"
#include "Ioss_PropertyManager.h"
#include "Ioss_Utils.h"

/** \brief Constructor adds "name" and "entity_count" properties to the entity
 *         and specifies the topology type for the entity block.
 *
 *  \param[in] io_database The database associated with the block.
 *  \param[in] my_name The block name.
 *  \param[in] entity_type The topology type for the block.
 *  \param[in] entity_cnt The number of subentities in the block.
 *
 */
Ioss::EntityBlock::EntityBlock(Ioss::DatabaseIO *io_database, const std::string &my_name,
                               const std::string &entity_type, size_t entity_cnt)
    : Ioss::GroupingEntity(io_database, my_name, entity_cnt)

{
  // The 'true' means it is ok for the factory to return
  // nullptr.  This is done here just so we can output a better
  // error message.
  topology_ = ElementTopology::factory(entity_type, true);
  if (topology_ == nullptr) {
    std::ostringstream errmsg;
    fmt::print(errmsg, "ERROR: The topology type '{}' is not supported on '{}' in file '{}'",
               entity_type, name(), io_database->get_filename());
    IOSS_ERROR(errmsg);
  }

  if (topology()->master_element_name() != entity_type && topology()->name() != entity_type) {
    // Maintain original element type on output database if possible.
    properties.add(Ioss::Property("original_topology_type", entity_type));
  }

  properties.add(Ioss::Property(this, "topology_node_count", Ioss::Property::INTEGER));
  properties.add(Ioss::Property(this, "topology_type", Ioss::Property::STRING));
  fields.add(Ioss::Field("connectivity", field_int_type(), topology_->name(), Ioss::Field::MESH,
                         entity_cnt));

  // Returns connectivity in local id space
  fields.add(Ioss::Field("connectivity_raw", field_int_type(), topology()->name(),
                         Ioss::Field::MESH, entity_cnt));
}

/** \brief Calculate and get an implicit property.
 *
 *  These are calcuated from data stored in the EntityBlock instead of having
 *  an explicit value assigned. An example would be 'topology_node_count' for an ElementBlock.
 *  Note that even though this is a pure virtual function, an implementation
 *  is provided to return properties that are common to all 'block'-type grouping entities.
 *  Derived classes should call 'EntityBlock::get_implicit_property'
 *  if the requested property is not specific to their type.
 */
Ioss::Property Ioss::EntityBlock::get_implicit_property(const std::string &my_name) const
{
  if (my_name == "topology_node_count") {
    return Ioss::Property(my_name, topology()->number_nodes());
  }
  if (my_name == "topology_type") {
    return Ioss::Property(my_name, topology()->name());
  }

  return Ioss::GroupingEntity::get_implicit_property(my_name);
}

bool Ioss::EntityBlock::equal_(const Ioss::EntityBlock &rhs, const bool quiet) const
{
  /* COMPARE element topologies */
  if (*(this->topology_) != *(rhs.topology_)) {
    if (!quiet) {
      fmt::print(Ioss::OUTPUT(), "EntityBlock: TOPOLOGY mismatch\n");
    }
    return false;
  }

  if (this->idOffset != rhs.idOffset) {
    if (!quiet) {
      fmt::print(Ioss::OUTPUT(), "EntityBlock: idOffset mismatch ({} vs. {})\n", this->idOffset,
                 rhs.idOffset);
    }
    return false;
  }

  if (!Ioss::GroupingEntity::equal_(rhs, quiet)) {
    if (!quiet) {
      fmt::print(Ioss::OUTPUT(), "EntityBlock: GroupingEntity mismatch\n");
    }
    return false;
  }

  return true;
}

bool Ioss::EntityBlock::operator==(const Ioss::EntityBlock &rhs) const { return equal_(rhs, true); }

bool Ioss::EntityBlock::operator!=(const Ioss::EntityBlock &rhs) const { return !(*this == rhs); }

bool Ioss::EntityBlock::equal(const Ioss::EntityBlock &rhs) const { return equal_(rhs, false); }
