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

#include <Ioss_DatabaseIO.h>
#include <Ioss_GroupingEntity.h>
#include <Ioss_Property.h>
#include <Ioss_Region.h>
#include <Ioss_Utils.h>
#include <Ioss_VariableType.h>
#include <cassert>
#include <cstddef>
#include <fmt/ostream.h>
#include <iostream>
#include <string>
#include <vector>

#include "Ioss_CodeTypes.h"
#include "Ioss_EntityType.h"
#include "Ioss_Field.h"
#include "Ioss_FieldManager.h"
#include "Ioss_PropertyManager.h"
#include "Ioss_State.h"

/** \brief Base class constructor adds "name" and "entity_count" properties to the entity.
 *
 *  \param[in] io_database The database associated with the entity.
 *  \param[in] my_name The entity name.
 *  \param[in] entity_cnt The number of subentities in the entity.
 *
 */
Ioss::GroupingEntity::GroupingEntity(Ioss::DatabaseIO *io_database, const std::string &my_name,
                                     int64_t entity_cnt)
    : entityCount(entity_cnt), entityName(my_name), database_(io_database),
      hash_(Ioss::Utils::hash(my_name))
{
  properties.add(Ioss::Property("name", my_name));
  properties.add(Ioss::Property("entity_count", entity_cnt));
  properties.add(Ioss::Property(this, "attribute_count", Ioss::Property::INTEGER));

  if (my_name != "null_entity") {
    Ioss::Field::BasicType int_type = Ioss::Field::INTEGER;
    if (io_database != nullptr) {
      int_type = field_int_type();
    }
    fields.add(Ioss::Field("ids", int_type, "scalar", Ioss::Field::MESH, entity_cnt));
  }
}

Ioss::GroupingEntity::GroupingEntity(const Ioss::GroupingEntity &other)
    : properties(other.properties), fields(other.fields), entityCount(other.entityCount),
      entityName(other.entityName), attributeCount(other.attributeCount),
      entityState(other.entityState), hash_(other.hash_)
{
}

Ioss::GroupingEntity::~GroupingEntity()
{
  // Only deleted by owning entity (Ioss::Region)
  database_ = nullptr;
}

// Default implementation is to do nothing. Redefined in Ioss::Region
// to actually delete the database.
void Ioss::GroupingEntity::delete_database() {}

void Ioss::GroupingEntity::really_delete_database()
{
  delete database_;
  database_ = nullptr;
}

const Ioss::GroupingEntity *Ioss::GroupingEntity::contained_in() const
{
  if (database_ == nullptr) {
    return nullptr;
  }
  return database_->get_region();
}

std::string Ioss::GroupingEntity::generic_name() const
{
  int64_t id = 0;
  if (property_exists("id")) {
    id = get_property("id").get_int();
  }
  return Ioss::Utils::encode_entity_name(short_type_string(), id);
}

bool Ioss::GroupingEntity::is_alias(const std::string &my_name) const
{
  Region *region = database_->get_region();
  return region->get_alias(my_name) == entityName;
}

Ioss::DatabaseIO *Ioss::GroupingEntity::get_database() const
{
  assert(database_ != nullptr);
  return database_;
}

/** \brief Get the file name associated with the database containing this entity.
 *
 *  \returns The file name.
 */
std::string Ioss::GroupingEntity::get_filename() const
{
  // Ok for database_ to be nullptr at this point.
  if (database_ == nullptr) {
    return std::string();
  }

  return database_->get_filename();
}

void Ioss::GroupingEntity::set_database(Ioss::DatabaseIO *io_database)
{
  assert(database_ == nullptr);   // Must be unset if we are setting it.
  assert(io_database != nullptr); // Must be set to valid value
  database_ = io_database;
}

void Ioss::GroupingEntity::reset_database(Ioss::DatabaseIO *io_database)
{
  assert(io_database != nullptr); // Must be set to valid value
  database_ = io_database;
}

// Discuss Data Object functions:
// ---Affect the containing object:
//    open(in string object_name, out ?)
//    close()
//    destroy()
// ---Affect what the object contains:
//    set(in string propertyname, in any property_value)
//    get(in string propertyname, out any property_value)
//    add(in string propertyname);
//    delete(in string propertyname)
//    describe(out vector<Ioss::Properties>)
//

/** \brief Get the current Ioss::State of the entity.
 *
 *  \returns The current state.
 */
Ioss::State Ioss::GroupingEntity::get_state() const { return entityState; }

/** \brief Calculate and get an implicit property.
 *
 *  These are calcuated from data stored in the EntityBlock instead of having
 *  an explicit value assigned. An example would be 'element_block_count' for a region.
 *  Note that even though this is a pure virtual function, an implementation
 *  is provided to return properties that are common to all 'block'-type grouping entities.
 *  Derived classes should call 'GroupingEntity::get_implicit_property'
 *  if the requested property is not specific to their type.
 */
Ioss::Property Ioss::GroupingEntity::get_implicit_property(const std::string &my_name) const
{
  // Handle properties generic to all GroupingEntities.
  // These include:
  if (my_name == "attribute_count") {
    count_attributes();
    return Ioss::Property(my_name, static_cast<int>(attributeCount));
  }

  // End of the line. No property of this name exists.
  std::ostringstream errmsg;
  fmt::print(errmsg, "\nERROR: Property '{}' does not exist on {} {}\n\n", my_name, type_string(),
             name());
  IOSS_ERROR(errmsg);
}

/** \brief Add a field to the entity's field manager.
 *
 *  Assumes that a field with the same name does not already exist.
 *
 *  \param[in] new_field The field to add
 *
 */
void Ioss::GroupingEntity::field_add(const Ioss::Field &new_field)
{
  if (new_field.get_role() == Ioss::Field::REDUCTION) {
    fields.add(new_field);
    return;
  }

  size_t entity_size = entity_count();
  size_t field_size  = new_field.raw_count();
  if (field_size == 0 && entity_size != 0) {
    // Set field size to match entity size...
    Ioss::Field tmp_field(new_field);
    tmp_field.reset_count(entity_size);
    fields.add(tmp_field);
  }
  else if (entity_size != field_size && type() != REGION) {
    std::string        filename = get_database()->get_filename();
    std::ostringstream errmsg;
    fmt::print(errmsg,
               "IO System error: The {} '{}' has a size of {},\nbut the field '{}' which is being "
               "output on that entity has a size of {}\non database '{}'.\nThe sizes must match.  "
               "This is an application error that should be reported.",
               type_string(), name(), entity_size, new_field.get_name(), field_size, filename);
    IOSS_ERROR(errmsg);
  }
  else {
    fields.add(new_field);
  }
}

/** \brief Read field data from the database file into memory using a pointer.
 *
 *  \param[in] field_name The name of the field to read.
 *  \param[out] data The data.
 *  \param[in] data_size The number of bytes of data to be read.
 *  \returns The number of values read.
 *
 */
int64_t Ioss::GroupingEntity::get_field_data(const std::string &field_name, void *data,
                                             size_t data_size) const
{
  verify_field_exists(field_name, "input");

  Ioss::Field field  = get_field(field_name);
  int64_t     retval = internal_get_field_data(field, data, data_size);

  // At this point, transform the field if specified...
  if (retval >= 0) {
    field.transform(data);
  }

  return retval;
}

/** \brief Write field data from memory into the database file using a pointer.
 *
 *  \param[in] field_name The name of the field to write.
 *  \param[in] data The data.
 *  \param[in] data_size The number of bytes of data to be written.
 *  \returns The number of values written.
 *
 */
int64_t Ioss::GroupingEntity::put_field_data(const std::string &field_name, void *data,
                                             size_t data_size) const
{
  verify_field_exists(field_name, "input");

  Ioss::Field field = get_field(field_name);
  field.transform(data);
  return internal_put_field_data(field, data, data_size);
}

/** \brief Get the number of fields with the given role (MESH, ATTRIBUTE, TRANSIENT, REDUCTION,
 * etc.)
 *         in the entity's field manager.
 *
 *  \returns The number of fields with the given role.
 */
size_t Ioss::GroupingEntity::field_count(Ioss::Field::RoleType role) const
{
  IOSS_FUNC_ENTER(m_);
  Ioss::NameList names;
  fields.describe(role, &names);
  return names.size();
}

void Ioss::GroupingEntity::count_attributes() const
{
  if (attributeCount > 0) {
    return;
  }

  // If the set has a field named "attribute", then the number of
  // attributes is equal to the component count of that field...
  NameList results_fields;
  field_describe(Ioss::Field::ATTRIBUTE, &results_fields);

  Ioss::NameList::const_iterator IF;
  int64_t                        attribute_count = 0;
  for (IF = results_fields.begin(); IF != results_fields.end(); ++IF) {
    std::string field_name = *IF;
    if (field_name != "attribute" || results_fields.size() == 1) {
      Ioss::Field field = get_field(field_name);
      attribute_count += field.raw_storage()->component_count();
    }
  }
  attributeCount = attribute_count;
}

void Ioss::GroupingEntity::verify_field_exists(const std::string &field_name,
                                               const std::string &inout) const
{
  if (!field_exists(field_name)) {
    std::string        filename = get_database()->get_filename();
    std::ostringstream errmsg;
    fmt::print(errmsg, "\nERROR: On database '{}', Field '{}' does not exist for {} on {} {}\n\n",
               filename, field_name, inout, type_string(), name());
    IOSS_ERROR(errmsg);
  }
}

void Ioss::GroupingEntity::property_update(const std::string &property, int64_t value) const
{
  if (property_exists(property)) {
    if (get_property(property).get_int() != value) {
      auto *nge = const_cast<Ioss::GroupingEntity *>(this);
      nge->property_erase(property);
      nge->property_add(Ioss::Property(property, value));
    }
  }
  else {
    auto *nge = const_cast<Ioss::GroupingEntity *>(this);
    nge->property_add(Ioss::Property(property, value));
  }
}

void Ioss::GroupingEntity::property_update(const std::string &property,
                                           const std::string &value) const
{
  if (property_exists(property)) {
    if (get_property(property).get_string() != value) {
      auto *nge = const_cast<Ioss::GroupingEntity *>(this);
      nge->property_erase(property);
      nge->property_add(Ioss::Property(property, value));
    }
  }
  else {
    auto *nge = const_cast<Ioss::GroupingEntity *>(this);
    nge->property_add(Ioss::Property(property, value));
  }
}
