// Copyright(C) 1999-2025 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include "Ioss_Field.h"
#include "Ioss_Transform.h"
#include "Ioss_Utils.h"
#include "Ioss_VariableType.h"
#include <cstddef>
#include <cstdint>
#include "vtk_fmt.h"
#include VTK_FMT(fmt/format.h)
#include VTK_FMT(fmt/ostream.h)
#include VTK_FMT(fmt/ranges.h)
#include <iostream>
#include <string>
#include <vector>

#include "Ioss_CodeTypes.h"

namespace {
  size_t internal_get_size(Ioss::Field::BasicType type, size_t count,
                           const Ioss::VariableType *storage);
  size_t internal_get_basic_size(Ioss::Field::BasicType type);

  void error_message(const Ioss::Field &field, Ioss::Field::BasicType requested_type)
  {
    std::ostringstream errmsg;
    fmt::print(errmsg,
               "ERROR: For field named '{}', code requested value of type '{}', but field type is "
               "'{}'. Types must match\n",
               field.get_name(), Ioss::Field::type_string(requested_type),
               Ioss::Field::type_string(field.get_type()));
    IOSS_ERROR(errmsg);
  }

} // namespace

namespace Ioss {
  std::ostream &operator<<(std::ostream &os, const Field &fld)
  {
    Ioss::NameList components(fld.get_component_count(Field::InOut::INPUT));
    for (size_t i = 0; i < components.size(); i++) {
      components[i] = fld.get_component_name(i + 1, Field::InOut::INPUT, 1);
    }
    auto storage = fld.raw_storage()->name();
    if (storage == "scalar") {
      fmt::print(os, "\tField: {}, Storage: {}\t{}\t{}\n", fld.get_name(),
                 fld.raw_storage()->name(), fld.type_string(), fld.role_string());
    }
    else {
      fmt::print(os,
                 "\tField: {}, Storage: {} ({}),\t{},\t{}, Sep1: '{}', Sep2: '{}'\n"
                 "\t\t\tComponents ({}): {}\n",
                 fld.get_name(), fld.raw_storage()->name(), fld.raw_storage()->type_string(),
                 fld.type_string(), fld.role_string(), fld.get_suffix_separator(0),
                 fld.get_suffix_separator(1), fld.get_component_count(Field::InOut::INPUT),
                 fmt::join(components, ", "));
    }
    return os;
  }
} // namespace Ioss

/** \brief Create an empty field.
 */
Ioss::Field::Field() { rawStorage_ = transStorage_ = Ioss::VariableType::factory("invalid"); }

/** \brief Create a field.
 *
 *  \param[in] name The name of the field
 *  \param[in] type The basic data type of data held in the field.
 *  \param[in] storage The storage class of the data (ConstructedVariableType,
 * CompositeVariableType, etc)
 *  \param[in] role The category of information held in the field (MESH, ATTRIBUTE, TRANSIENT,
 * REDUCTION, etc)
 *  \param[in] value_count The number of items in the field.
 *  \param[in] index
 *
 */
Ioss::Field::Field(std::string name, const Ioss::Field::BasicType type, const std::string &storage,
                   const Ioss::Field::RoleType role, size_t value_count, size_t index)
    : name_(std::move(name)), rawCount_(value_count), transCount_(value_count), index_(index),
      type_(type), role_(role)
{
  rawStorage_ = transStorage_ = Ioss::VariableType::factory(storage);
  size_                       = internal_get_size(type_, rawCount_, rawStorage_);
}

/** \brief Create a field.
 *
 *  \param[in] name The name of the field
 *  \param[in] type The basic data type of data held in the field.
 *  \param[in] storage The storage class of the data (ConstructedVariableType,
 * CompositeVariableType, etc)
 *  \param[in] copies The number of variables to be combined in a CompositeVariableType field.
 *  \param[in] role The category of information held in the field (MESH, ATTRIBUTE, TRANSIENT,
 * REDUCTION, etc)
 *  \param[in] value_count The number of items in the field.
 *  \param[in] index
 *
 */
Ioss::Field::Field(std::string name, const Ioss::Field::BasicType type, const std::string &storage,
                   int copies, const Ioss::Field::RoleType role, size_t value_count, size_t index)
    : name_(std::move(name)), rawCount_(value_count), transCount_(value_count), index_(index),
      type_(type), role_(role)
{
  rawStorage_ = transStorage_ = Ioss::VariableType::factory(storage, copies);
  size_                       = internal_get_size(type_, rawCount_, rawStorage_);
}

/** \brief Create a field.
 *
 *  \param[in] name The name of the field
 *  \param[in] type The basic data type of data held in the field.
 *  \param[in] storage The storage class of the data (ConstructedVariableType,
 * CompositeVariableType, etc)
 *  \param[in] role The category of information held in the field (MESH, ATTRIBUTE, TRANSIENT,
 * REDUCTION, etc)
 *  \param[in] value_count The number of items in the field.
 *  \param[in] index
 *
 */
Ioss::Field::Field(std::string name, const Ioss::Field::BasicType type,
                   const Ioss::VariableType *storage, const Ioss::Field::RoleType role,
                   size_t value_count, size_t index)
    : name_(std::move(name)), rawCount_(value_count), transCount_(value_count), index_(index),
      type_(type), role_(role), rawStorage_(storage), transStorage_(storage)
{
  size_ = internal_get_size(type_, rawCount_, rawStorage_);
}

/** \brief Create a field.
 *
 *  \param[in] name The name of the field
 *  \param[in] type The basic data type of data held in the field.
 *  \param[in] storage The storage class of the data (ConstructedVariableType,
 * CompositeVariableType, etc)
 *  \param[in] secondary The secondary storage class of the data (typically "basis") [For a
 * ComposedVariableType field] \param[in] role The category of information held in the field (MESH,
 * ATTRIBUTE, TRANSIENT, REDUCTION, etc) \param[in] value_count The number of items in the field.
 *  \param[in] index
 *
 */
Ioss::Field::Field(std::string name, BasicType type, const std::string &storage,
                   const std::string &secondary, RoleType role, size_t value_count, size_t index)
    : name_(std::move(name)), rawCount_(value_count), transCount_(value_count), index_(index),
      type_(type), role_(role)
{
  rawStorage_ = transStorage_ = Ioss::VariableType::factory(storage, secondary);
  size_                       = internal_get_size(type_, rawCount_, rawStorage_);
}

int Ioss::Field::get_component_count(Ioss::Field::InOut in_out) const
{
  const auto *storage = (in_out == InOut::INPUT) ? raw_storage() : transformed_storage();
  return storage->component_count();
}

std::string Ioss::Field::get_component_name(int component_index, InOut in_out, char suffix) const
{
  char suffix_separator0 = get_suffix_separator(0);
  if (suffix_separator0 == 1) {
    suffix_separator0 = suffix != 1 ? suffix : '_';
  }
  char suffix_separator1 = get_suffix_separator(1);
  if (suffix_separator1 == 1) {
    suffix_separator1 = suffix != 1 ? suffix : '_';
  }
  const auto *storage = (in_out == InOut::INPUT) ? raw_storage() : transformed_storage();
  return storage->label_name(get_name(), component_index, suffix_separator0, suffix_separator1,
                             get_suffices_uppercase());
}

/* \brief Verify that data_size is valid.
 *
 * If return value >= 0, then it is the maximum number of
 * entities to get 'count'
 * Throws runtime error if data_size too small.
 *
 * \param[in] data_size The data size to test
 * \returns The maximum number of entities to get 'count'
 *
 */
size_t Ioss::Field::verify(size_t data_size) const
{
  if (data_size > 0) {
    // Check sufficient storage
    size_t required = get_size();
    if (required > data_size) {
      std::ostringstream errmsg;
      fmt::print(errmsg,
                 "Field {} requires {} bytes to store its data. Only {} bytes were provided.\n",
                 name_, required, data_size);
      IOSS_ERROR(errmsg);
    }
  }
  return rawCount_;
}

void Ioss::Field::check_type(BasicType the_type) const
{
  if (type_ != the_type) {
    if ((the_type == Ioss::Field::INTEGER && type_ == Ioss::Field::REAL) ||
        (the_type == Ioss::Field::INT64 && type_ == Ioss::Field::REAL)) {
      // If Ioss created the field by reading the database, it may
      // think the field is a real but it is really an integer.  Make
      // sure that the field type is correct here...
      auto *new_this = const_cast<Ioss::Field *>(this);
      new_this->reset_type(the_type);
    }
    else {
      error_message(*this, the_type);
    }
  }
}

const Ioss::Field &Ioss::Field::set_zero_copy_enabled(bool true_false) const
{
  if (has_transform()) {
    std::ostringstream errmsg;
    fmt::print(errmsg,
               "Field {} is being set to `zero_copy_enabled`; however, it contains 1 or more "
               "transforms which is not allowed.\n",
               name_);
    IOSS_ERROR(errmsg);
  }
  zeroCopyable_ = true_false;
  return *this;
}

void Ioss::Field::reset_count(size_t new_count)
{
  if (transCount_ == rawCount_) {
    transCount_ = new_count;
  }
  rawCount_ = new_count;
  size_     = 0;
}

void Ioss::Field::reset_type(Ioss::Field::BasicType new_type)
{
  type_ = new_type;
  size_ = 0;
}

// Return number of bytes required to store entire field
size_t Ioss::Field::get_size() const
{
  if (size_ == 0) {
    auto *new_this  = const_cast<Ioss::Field *>(this);
    new_this->size_ = internal_get_size(type_, rawCount_, rawStorage_);

    new_this->transCount_   = rawCount_;
    new_this->transStorage_ = rawStorage_;
    for (const auto &my_transform : transforms_) {
      new_this->transCount_   = my_transform->output_count(transCount_);
      new_this->transStorage_ = my_transform->output_storage(transStorage_);
      size_t size             = internal_get_size(type_, transCount_, transStorage_);
      if (size > size_) {
        new_this->size_ = size;
      }
    }
  }
  return size_;
}

size_t Ioss::Field::get_basic_size() const
{
  // Calculate size of the low-level data type
  return internal_get_basic_size(type_);
}

bool Ioss::Field::add_transform(Transform *my_transform)
{
  if (zero_copy_enabled()) {
    std::ostringstream errmsg;
    fmt::print(errmsg,
               "Field {} is currently set to `zero_copy_enabled` which does not support adding a "
               "transform.  The transform has *not* been added to this field.\n",
               name_);
    IOSS_ERROR(errmsg);
  }

  const Ioss::VariableType *new_storage = my_transform->output_storage(transStorage_);
  size_t                    new_count   = my_transform->output_count(transCount_);

  if (new_storage != nullptr && new_count > 0) {
    transStorage_ = new_storage;
    transCount_   = new_count;
  }
  else {
    return false;
  }

  if (transCount_ < rawCount_) {
    role_ = REDUCTION;
  }

  size_t size = internal_get_size(type_, transCount_, transStorage_);
  if (size > size_) {
    size_ = size;
  }

  transforms_.push_back(my_transform);
  return true;
}

bool Ioss::Field::transform(void *data)
{
  transStorage_ = rawStorage_;
  transCount_   = rawCount_;

  for (auto &my_transform : transforms_) {
    my_transform->execute(*this, data);

    transStorage_ = my_transform->output_storage(transStorage_);
    transCount_   = my_transform->output_count(transCount_);
  }
  return true;
}

bool Ioss::Field::equal_(const Ioss::Field &rhs, bool quiet) const
{
  bool is_same = true;
  if (!Ioss::Utils::str_equal(this->name_, rhs.name_)) {
    if (!quiet) {
      fmt::print(Ioss::OUTPUT(), "\tFIELD name mismatch ({} v. {})\n", this->name_, rhs.name_);
    }
    is_same = false;
  }

  if (this->type_ != rhs.type_) {
    if (!quiet) {
      fmt::print(Ioss::OUTPUT(), "\tFIELD {} type mismatch ({} v. {})\n", this->name_,
                 this->type_string(), rhs.type_string());
    }
    is_same = false;
  }

  if (this->role_ != rhs.role_) {
    if (!quiet) {
      fmt::print(Ioss::OUTPUT(), "\tFIELD {} role mismatch ({} v. {})\n", this->name_,
                 this->role_string(), rhs.role_string());
    }
    is_same = false;
  }

  if (this->rawCount_ != rhs.rawCount_) {
    if (!quiet) {
      fmt::print(Ioss::OUTPUT(), "\tFIELD {} rawCount mismatch ({} v. {})\n", this->name_,
                 this->rawCount_, rhs.rawCount_);
    }
    is_same = false;
  }

  if (this->transCount_ != rhs.transCount_) {
    if (!quiet) {
      fmt::print(Ioss::OUTPUT(), "\tFIELD {} transCount mismatch ({} v. {})\n", this->name_,
                 this->transCount_, rhs.transCount_);
    }
    is_same = false;
  }

  if (this->get_size() != rhs.get_size()) {
    if (!quiet) {
      fmt::print(Ioss::OUTPUT(), "\tFIELD {} size mismatch ({} v. {})\n", this->name_,
                 this->get_size(), rhs.get_size());
    }
    is_same = false;
  }

  if (!quiet) {
    if (this->get_suffices_uppercase() != rhs.get_suffices_uppercase()) {
      fmt::print(Ioss::OUTPUT(), "\tFIELD {} suffices_uppercase mismatch ({} v. {})\n", this->name_,
                 this->get_suffices_uppercase(), rhs.get_suffices_uppercase());
      is_same = false;
    }
  }

  if (!quiet) {
    if (this->zero_copy_enabled() != rhs.zero_copy_enabled()) {
      fmt::print(Ioss::OUTPUT(), "\tFIELD {} zero_copy_enabled mismatch ({} v. {})\n", this->name_,
                 this->zero_copy_enabled(), rhs.zero_copy_enabled());
      is_same = false;
    }
  }

  return is_same;
}

bool Ioss::Field::operator==(const Ioss::Field &rhs) const { return equal_(rhs, true); }

bool Ioss::Field::operator!=(const Ioss::Field &rhs) const { return !(*this == rhs); }

bool Ioss::Field::equal(const Ioss::Field &rhs) const { return equal_(rhs, false); }

std::string Ioss::Field::type_string() const { return type_string(get_type()); }

std::string Ioss::Field::type_string(Ioss::Field::BasicType type)
{
  switch (type) {
  case Ioss::Field::REAL: return {"real"};
  case Ioss::Field::INTEGER: return {"integer"};
  case Ioss::Field::INT64: return {"64-bit integer"};
  case Ioss::Field::COMPLEX: return {"complex"};
  case Ioss::Field::STRING: return {"string"};
  case Ioss::Field::CHARACTER: return {"char"};
  case Ioss::Field::INVALID: return {"invalid"};
  }
  return {"internal error"};
}

std::string Ioss::Field::role_string() const { return role_string(get_role()); }

std::string Ioss::Field::role_string(Ioss::Field::RoleType role)
{
  switch (role) {
  case Ioss::Field::INTERNAL: return {"Internal"};
  case Ioss::Field::MAP: return {"Map"};
  case Ioss::Field::MESH: return {"Mesh"};
  case Ioss::Field::ATTRIBUTE: return {"Attribute"};
  case Ioss::Field::COMMUNICATION: return {"Communication"};
  case Ioss::Field::MESH_REDUCTION: return {"Mesh Reduction"};
  case Ioss::Field::REDUCTION: return {"Reduction"};
  case Ioss::Field::TRANSIENT: return {"Transient"};
  }
  return {"internal error"};
}

namespace {
  size_t internal_get_basic_size(Ioss::Field::BasicType type)
  {
    // Calculate size of the low-level data type
    size_t basic_size = 0;
    switch (type) {
    case Ioss::Field::REAL: basic_size = sizeof(double); break;
    case Ioss::Field::INTEGER: basic_size = sizeof(int); break;
    case Ioss::Field::INT64: basic_size = sizeof(int64_t); break;
    case Ioss::Field::COMPLEX: basic_size = sizeof(Complex); break;
    case Ioss::Field::STRING: basic_size = sizeof(std::string *); break;
    case Ioss::Field::CHARACTER: basic_size = sizeof(char); break;
    case Ioss::Field::INVALID: basic_size = 0; break;
    }

    return basic_size;
  }

  size_t internal_get_size(Ioss::Field::BasicType type, size_t count,
                           const Ioss::VariableType *storage)
  {
    // Calculate size of the low-level data type
    size_t basic_size = internal_get_basic_size(type);

    // Calculate size of the storage type
    size_t storage_size = storage->component_count();

    return basic_size * storage_size * count;
  }
} // namespace
