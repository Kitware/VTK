// Copyright(C) 1999-2021 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details
#include <Ioss_GroupingEntity.h>
#include <Ioss_Property.h>
#include <Ioss_Utils.h>
#include <algorithm>
#include <cstddef>
#include <fmt/ostream.h>
#include <ostream>
#include <string>

namespace {
  std::string type_string(Ioss::Property::BasicType type)
  {
    switch (type) {
    case Ioss::Property::INVALID: return std::string("invalid");
    case Ioss::Property::REAL: return std::string("real");
    case Ioss::Property::INTEGER: return std::string("integer");
    case Ioss::Property::POINTER: return std::string("pointer");
    case Ioss::Property::STRING: return std::string("string");
    case Ioss::Property::VEC_INTEGER: return std::string("vector<int>");
    case Ioss::Property::VEC_DOUBLE: return std::string("vector<double>");
    default: return std::string("internal error");
    }
  }

  void error_message(const Ioss::Property &property, const std::string &requested_type)
  {
    std::ostringstream errmsg;
    fmt::print(errmsg,
               "ERROR: For property named '{}', code requested value of type '{}', but property "
               "type is '{}'. Types must match\n",
               property.get_name(), requested_type, type_string(property.get_type()));
    IOSS_ERROR(errmsg);
  }
} // namespace

/** \brief Create an INTEGER type property.
 *
 *  \param[in] name The property name.
 *  \param[in] value The property value.
 *  \param[in] origin The origin of the property - IMPLICIT, or EXTERNAL, or ATTRIBUTE
 */
Ioss::Property::Property(std::string name, int value, Origin origin)
    : name_(std::move(name)), type_(INTEGER), origin_(origin)
{
  data_.ival = value;
}

/** \brief Create an INTEGER type property using an int64_t variable.
 *
 *  \param[in] name The property name.
 *  \param[in] value The property value.
 *  \param[in] origin The origin of the property - IMPLICIT, or EXTERNAL, or ATTRIBUTE
 */
Ioss::Property::Property(std::string name, int64_t value, Origin origin)
    : name_(std::move(name)), type_(INTEGER), origin_(origin)
{
  data_.ival = value;
}

/** \brief Create a REAL type property.
 *
 *  \param[in] name The property name.
 *  \param[in] value The property value.
 *  \param[in] origin The origin of the property - IMPLICIT, or EXTERNAL, or ATTRIBUTE
 */
Ioss::Property::Property(std::string name, double value, Origin origin)
    : name_(std::move(name)), type_(REAL), origin_(origin)
{
  data_.rval = value;
}

/** \brief Create a STRING type property.
 *
 *  \param[in] name The property name.
 *  \param[in] value The property value.
 *  \param[in] origin The origin of the property - IMPLICIT, or EXTERNAL, or ATTRIBUTE
 */
Ioss::Property::Property(std::string name, const std::string &value, Origin origin)
    : name_(std::move(name)), type_(STRING), origin_(origin)
{
  data_.sval = new std::string(value);
}

/** \brief Create a VEC_INTEGER type property.
 *
 *  \param[in] name The property name.
 *  \param[in] value The property value.
 *  \param[in] origin The origin of the property - IMPLICIT, or EXTERNAL, or ATTRIBUTE
 */
Ioss::Property::Property(std::string name, const std::vector<int> &value, Origin origin)
    : name_(std::move(name)), type_(VEC_INTEGER), origin_(origin)
{
  data_.ivec = new std::vector<int>(value);
}

/** \brief Create a VEC_DOUBLE type property.
 *
 *  \param[in] name The property name.
 *  \param[in] value The property value.
 *  \param[in] origin The origin of the property - IMPLICIT, or EXTERNAL, or ATTRIBUTE
 */
Ioss::Property::Property(std::string name, const std::vector<double> &value, Origin origin)
    : name_(std::move(name)), type_(VEC_DOUBLE), origin_(origin)
{
  data_.dvec = new std::vector<double>(value);
}

/** \brief Create a STRING type property from const char* argument.
 *
 *  \param[in] name The property name.
 *  \param[in] value The property value.
 *  \param[in] origin The origin of the property - IMPLICIT, or EXTERNAL, or ATTRIBUTE
 */
Ioss::Property::Property(std::string name, const char *value, Origin origin)
    : name_(std::move(name)), type_(STRING), origin_(origin)
{
  data_.sval = new std::string(value);
}

/** \brief Create a POINTER type property.
 *
 *  \param[in] name The property name.
 *  \param[in] value The property value.
 *  \param[in] origin The origin of the property - IMPLICIT, or EXTERNAL, or ATTRIBUTE
 */
Ioss::Property::Property(std::string name, void *value, Origin origin)
    : name_(std::move(name)), type_(POINTER), origin_(origin)
{
  data_.pval = value;
}

/** \brief Set implicit property with a specified type.
 *
 *  \param[in] ge The property value.
 *  \param[in] name The property name.
 *  \param[in] type The property type.
 */
Ioss::Property::Property(const Ioss::GroupingEntity *ge, std::string name, const BasicType type)
    : name_(std::move(name)), type_(type), origin_(IMPLICIT)
{
  data_.ge = ge;
}

/** \brief Copy constructor.
 *
 *  \param[in] from The Ioss::Property to copy
 */
Ioss::Property::Property(const Ioss::Property &from)
    : name_(from.name_), type_(from.type_), origin_(from.origin_)
{
  if (!is_implicit() && type_ == STRING) {
    data_.sval = new std::string(*(from.data_.sval));
  }
  else if (!is_implicit() && type_ == VEC_DOUBLE) {
    data_.dvec = new std::vector<double>(*(from.data_.dvec));
  }
  else if (!is_implicit() && type_ == VEC_INTEGER) {
    data_.ivec = new std::vector<int>(*(from.data_.ivec));
  }
  else {
    data_ = from.data_;
  }
}

Ioss::Property::~Property()
{
  if (!is_implicit() && type_ == STRING) {
    delete data_.sval;
  }
  else if (!is_implicit() && type_ == VEC_DOUBLE) {
    delete data_.dvec;
  }
  else if (!is_implicit() && type_ == VEC_INTEGER) {
    delete data_.ivec;
  }
}

Ioss::Property &Ioss::Property::operator=(Ioss::Property rhs)
{
  std::swap(*this, rhs);
  return *this;
}

bool Ioss::Property::operator==(const Ioss::Property &rhs) const
{
  if (this->name_.compare(rhs.name_) != 0) {
    return false;
  }

  if (this->type_ != rhs.type_) {
    return false;
  }

  switch (this->type_) {
  case INVALID: break;
  case REAL:
    double r_lhs, r_rhs;
    this->get_value(&r_lhs);
    rhs.get_value(&r_rhs);
    if (r_lhs != r_rhs) {
      return false;
    }
    break;
  case INTEGER:
    int64_t i_lhs, i_rhs;
    this->get_value(&i_lhs);
    rhs.get_value(&i_rhs);
    if (i_lhs != i_rhs) {
      return false;
    }
    break;
  case POINTER:
    void *p_lhs, *p_rhs;
    this->get_value(p_lhs);
    rhs.get_value(p_rhs);
    if (p_lhs != p_rhs) {
      return false;
    }
    break;
  case VEC_DOUBLE: {
    auto rh = rhs.get_vec_double();
    auto lh = get_vec_double();
    if (lh != rh) {
      return false;
    }
  } break;
  case VEC_INTEGER: {
    auto rh = rhs.get_vec_int();
    auto lh = get_vec_int();
    if (lh != rh) {
      return false;
    }
  } break;
  case STRING:
    std::string s_lhs, s_rhs;
    this->get_value(&s_lhs);
    rhs.get_value(&s_rhs);
    if (s_lhs.compare(s_rhs) != 0) {
      return false;
    }
    break;
  }

  return true;
}

bool Ioss::Property::operator!=(const Ioss::Property &rhs) const { return !(*this == rhs); }

/** \brief Get the property value if it is of type STRING.
 *
 *  \returns The STRING-type property value
 */
std::string Ioss::Property::get_string() const
{
  std::string value;
  bool        valid = get_value(&value);
  if (!valid) {
    error_message(*this, "string");
  }
  return value;
}

/** \brief Get the property value if it is of type VEC_DOUBLE.
 *
 *  \returns The VEC_DOUBLE-type property value
 */
std::vector<double> Ioss::Property::get_vec_double() const
{
  std::vector<double> value;
  bool                valid = get_value(&value);
  if (!valid) {
    error_message(*this, "vector<double>");
  }
  return value;
}

/** \brief Get the property value if it is of type VEC_INT.
 *
 *  \returns The VEC_INT-type property value
 */
std::vector<int> Ioss::Property::get_vec_int() const
{
  std::vector<int> value;
  bool             valid = get_value(&value);
  if (!valid) {
    error_message(*this, "vector<int>");
  }
  return value;
}

/** \brief Get the property value if it is of type INTEGER.
 *
 *  \returns The INTEGER-type property value
 */
int64_t Ioss::Property::get_int() const
{
  int64_t value;
  bool    valid = get_value(&value);
  if (!valid) {
    error_message(*this, "int");
  }
  return value;
}

/** \brief Get the property value if it is of type REAL.
 *
 *  \returns The REAL-type property value.
 */
double Ioss::Property::get_real() const
{
  double value;
  bool   valid = get_value(&value);
  if (!valid) {
    error_message(*this, "real");
  }
  return value;
}

/** \brief Get the property value if it is of type POINTER.
 *
 *  \returns The POINTER-type property value.
 */
void *Ioss::Property::get_pointer() const
{
  void *value = nullptr;
  bool  valid = get_value(value);
  if (!valid) {
    error_message(*this, "pointer");
  }
  return value;
}

bool Ioss::Property::get_value(int64_t *value) const
{
  bool valid_request = type_ == INTEGER ? true : false;
  if (is_explicit()) {
    *value = data_.ival;
  }
  else {
    const Ioss::GroupingEntity *ge       = data_.ge;
    const Ioss::Property        implicit = ge->get_implicit_property(name_);
    valid_request                        = implicit.get_value(value);
  }
  return valid_request;
}

bool Ioss::Property::get_value(double *value) const
{
  bool valid_request = type_ == REAL ? true : false;
  if (is_explicit()) {
    *value = data_.rval;
  }
  else {
    const Ioss::GroupingEntity *ge       = data_.ge;
    const Ioss::Property        implicit = ge->get_implicit_property(name_);
    valid_request                        = implicit.get_value(value);
  }
  return valid_request;
}

bool Ioss::Property::get_value(std::string *value) const
{
  bool valid_request = type_ == STRING ? true : false;
  if (is_explicit()) {
    *value = *(data_.sval);
  }
  else {
    const Ioss::GroupingEntity *ge       = data_.ge;
    const Ioss::Property        implicit = ge->get_implicit_property(name_);
    valid_request                        = implicit.get_value(value);
  }
  return valid_request;
}

bool Ioss::Property::get_value(std::vector<int> *value) const
{
  bool valid_request = type_ == VEC_INTEGER ? true : false;
  if (is_explicit()) {
    std::copy(data_.ivec->begin(), data_.ivec->end(), std::back_inserter(*value));
  }
  else {
    const Ioss::GroupingEntity *ge       = data_.ge;
    const Ioss::Property        implicit = ge->get_implicit_property(name_);
    valid_request                        = implicit.get_value(value);
  }
  return valid_request;
}

bool Ioss::Property::get_value(std::vector<double> *value) const
{
  bool valid_request = type_ == VEC_DOUBLE ? true : false;
  if (is_explicit()) {
    std::copy(data_.dvec->begin(), data_.dvec->end(), std::back_inserter(*value));
  }
  else {
    const Ioss::GroupingEntity *ge       = data_.ge;
    const Ioss::Property        implicit = ge->get_implicit_property(name_);
    valid_request                        = implicit.get_value(value);
  }
  return valid_request;
}

bool Ioss::Property::get_value(void *&value) const
{
  bool valid_request = type_ == POINTER ? true : false;
  if (is_explicit()) {
    value = data_.pval;
  }
  else {
    const Ioss::GroupingEntity *ge       = data_.ge;
    const Ioss::Property        implicit = ge->get_implicit_property(name_);
    valid_request                        = implicit.get_value(value);
  }
  return valid_request;
}
