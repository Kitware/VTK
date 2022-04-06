// Copyright(C) 1999-2021 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include <Ioss_DatabaseIO.h>
#include <Ioss_GroupingEntity.h>
#include <Ioss_Property.h>
#include <Ioss_Region.h>
#include <Ioss_Utils.h>
#include <Ioss_VariableType.h>
#include <cassert>
#include <cstddef>
#include "vtk_fmt.h"
#include VTK_FMT(fmt/ostream.h)
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
  int64_t id = get_optional_property("id", 0);
  return Ioss::Utils::encode_entity_name(short_type_string(), id);
}

bool Ioss::GroupingEntity::is_alias(const std::string &my_name) const
{
  Region *region = database_->get_region();
  return region->get_alias(my_name, type()) == entityName;
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
void Ioss::GroupingEntity::field_add(Ioss::Field new_field)
{
  size_t field_size = new_field.raw_count();

  if (new_field.get_role() == Ioss::Field::REDUCTION) {
    if (field_size == 0) {
      new_field.reset_count(1);
    }
    fields.add(new_field);
    return;
  }

  size_t entity_size = entity_count();
  if (field_size == 0 && entity_size != 0) {
    // Set field size to match entity size...
    new_field.reset_count(entity_size);
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
  fields.add(new_field);
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
  Ioss::NameList names = field_describe(role);
  return names.size();
}

void Ioss::GroupingEntity::count_attributes() const
{
  if (attributeCount > 0) {
    return;
  }

  // If the set has a field named "attribute", then the number of
  // attributes is equal to the component count of that field...
  Ioss::NameList results_fields = field_describe(Ioss::Field::ATTRIBUTE);

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

bool Ioss::GroupingEntity::equal_(const Ioss::GroupingEntity &rhs, const bool quiet) const
{
  if (this->entityName.compare(rhs.entityName) != 0) {
    if (!quiet) {
      fmt::print(Ioss::OUTPUT(), "GroupingEntity: entityName mismatch ({} vs. {})\n",
                 this->entityName, rhs.entityName);
    }
    return false;
  }

  if (this->entityCount != rhs.entityCount) {
    if (!quiet) {
      fmt::print(Ioss::OUTPUT(), "GroupingEntity: entityCount mismatch ([] vs. [])\n",
                 this->entityCount, rhs.entityCount);
    }
    return false;
  }

  if (this->attributeCount != rhs.attributeCount) {
    if (!quiet) {
      fmt::print(Ioss::OUTPUT(), "GroupingEntity: attributeCount mismatch ([] vs. [])\n",
                 this->attributeCount, rhs.attributeCount);
    }
    return false;
  }

  if (this->entityState != rhs.entityState) {
    if (!quiet) {
      fmt::print(Ioss::OUTPUT(), "GroupingEntity: entityState mismatch ([] vs. [])\n",
                 this->entityState, rhs.entityState);
    }
    return false;
  }

  if (this->hash_ != rhs.hash_) {
    if (!quiet) {
      fmt::print(Ioss::OUTPUT(), "GroupingEntity: hash_ mismatch ({} vs. {})\n", this->hash_,
                 rhs.hash_);
    }
    return false;
  }

  /* COMPARE properties */
  Ioss::NameList lhs_properties = this->property_describe();
  Ioss::NameList rhs_properties = rhs.property_describe();

  if (lhs_properties.size() != rhs_properties.size()) {
    if (!quiet) {
      fmt::print(Ioss::OUTPUT(), "GroupingEntity: NUMBER of properties are different ({} vs. {})\n",
                 lhs_properties.size(), rhs_properties.size());
    }
    return false;
  }

  for (auto &lhs_property : lhs_properties) {
    auto it = std::find(rhs_properties.begin(), rhs_properties.end(), lhs_property);
    if (it == rhs_properties.end()) {
      if (!quiet) {
        fmt::print(Ioss::OUTPUT(), "WARNING: {}: INPUT property ({}) not found in OUTPUT\n", name(),
                   lhs_property);
      }
      continue;
    }

    if (this->properties.get(lhs_property) != rhs.properties.get(lhs_property)) {
      // EMPIRICALLY, different representations (e.g., CGNS vs. Exodus) of the same mesh
      // can have different values for the "original_block_order" property.
      if (lhs_property.compare("original_block_order") == 0) {
        if (!quiet) {
          fmt::print(Ioss::OUTPUT(),
                     "WARNING: {}: values for \"original_block_order\" DIFFER ({} vs. {})\n",
                     name(), this->properties.get(lhs_property).get_int(),
                     rhs.properties.get(lhs_property).get_int());
        }
      }
      else {
        if (!quiet) {
          fmt::print(Ioss::OUTPUT(), "{}: PROPERTY ({}) mismatch\n", name(), lhs_property);
        }
        return false;
      }
    }
  }

  if (!quiet) {
    for (auto &rhs_property : rhs_properties) {
      auto it = std::find(lhs_properties.begin(), lhs_properties.end(), rhs_property);
      if (it == lhs_properties.end()) {
        fmt::print(Ioss::OUTPUT(), "WARNING: {}: OUTPUT property ({}) not found in INPUT\n", name(),
                   rhs_property);
      }
    }
  }

  /* COMPARE fields */
  Ioss::NameList lhs_fields = this->field_describe();
  Ioss::NameList rhs_fields = rhs.field_describe();

  bool the_same = true;
  if (lhs_fields.size() != rhs_fields.size()) {
    if (!quiet) {
      fmt::print(Ioss::OUTPUT(), "\n{}: NUMBER of fields are different ({} vs. {})\n", name(),
                 lhs_fields.size(), rhs_fields.size());
      the_same = false;
    }
    else {
      return false;
    }
  }

  for (auto &field : lhs_fields) {
    if (!quiet) {
      const auto &f1 = this->fields.get(field);
      if (rhs.field_exists(field)) {
        const auto &f2 = rhs.fields.get(field);
        if (!f1.equal(f2)) {
          fmt::print(Ioss::OUTPUT(), "{}: FIELD ({}) mismatch\n", name(), field);
          the_same = false;
        }
      }
      else {
        fmt::print(Ioss::OUTPUT(), "{}: FIELD ({}) not found in input #2\n", name(), field);
        the_same = false;
      }
    }
    else {
      if (this->fields.get(field) != rhs.fields.get(field)) {
        return false;
      }
    }
  }

  if (rhs_fields.size() > lhs_fields.size()) {
    // See which fields are missing from input #1...
    for (auto &field : rhs_fields) {
      if (!this->field_exists(field)) {
        fmt::print(Ioss::OUTPUT(), "{}: FIELD ({}) not found in input #1\n", name(), field);
        the_same = false;
      }
    }
  }
  return the_same;
}

bool Ioss::GroupingEntity::operator==(const Ioss::GroupingEntity &rhs) const
{
  return equal_(rhs, true);
}

bool Ioss::GroupingEntity::operator!=(const Ioss::GroupingEntity &rhs) const
{
  return !(*this == rhs);
}

bool Ioss::GroupingEntity::equal(const Ioss::GroupingEntity &rhs) const
{
  return equal_(rhs, false);
}
