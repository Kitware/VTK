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

#include <Ioss_Field.h>
#include <Ioss_Transform.h>
#include <Ioss_Utils.h>
#include <Ioss_VariableType.h>
#include <cstddef>
#include <cstdint>
#include <fmt/ostream.h>
#include <iostream>
#include <string>
#include <vector>

#include <Ioss_CodeTypes.h>

namespace {
  size_t internal_get_size(Ioss::Field::BasicType type, size_t count,
                           const Ioss::VariableType *storage);

  std::string type_string(Ioss::Field::BasicType type)
  {
    switch (type) {
    case Ioss::Field::REAL: return std::string("real");
    case Ioss::Field::INTEGER: return std::string("integer");
    case Ioss::Field::INT64: return std::string("64-bit integer");
    case Ioss::Field::COMPLEX: return std::string("complex");
    case Ioss::Field::STRING: return std::string("string");
    case Ioss::Field::CHARACTER: return std::string("char");
    case Ioss::Field::INVALID: return std::string("invalid");
    default: return std::string("internal error");
    }
  }

  void error_message(const Ioss::Field &field, Ioss::Field::BasicType requested_type)
  {
    std::ostringstream errmsg;
    fmt::print(errmsg,
               "ERROR: For field named '{}', code requested value of type '{}', but field type is "
               "'{}'. Types must match\n",
               field.get_name(), type_string(requested_type), type_string(field.get_type()));
    IOSS_ERROR(errmsg);
  }
} // namespace

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
    : name_(std::move(name)), rawCount_(value_count), transCount_(value_count), size_(0),
      index_(index), type_(type), role_(role)
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

Ioss::Field::Field(const Ioss::Field &from) = default;

Ioss::Field &Ioss::Field::operator=(const Field &from) = default;

Ioss::Field::~Field() = default;

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
      Ioss::Field *new_this = const_cast<Ioss::Field *>(this);
      new_this->reset_type(the_type);
    }
    else {
      error_message(*this, the_type);
    }
  }
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
    Ioss::Field *new_this = const_cast<Ioss::Field *>(this);
    new_this->size_       = internal_get_size(type_, rawCount_, rawStorage_);

    new_this->transCount_   = rawCount_;
    new_this->transStorage_ = rawStorage_;
    for (auto my_transform : transforms_) {
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

bool Ioss::Field::add_transform(Transform *my_transform)
{
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

  for (auto my_transform : transforms_) {
    my_transform->execute(*this, data);

    transStorage_ = my_transform->output_storage(transStorage_);
    transCount_   = my_transform->output_count(transCount_);
  }
  return true;
}

namespace {
  size_t internal_get_size(Ioss::Field::BasicType type, size_t count,
                           const Ioss::VariableType *storage)
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
    // Calculate size of the storage type
    size_t storage_size = storage->component_count();

    return basic_size * storage_size * count;
  }
} // namespace
