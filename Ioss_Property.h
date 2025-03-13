// Copyright(C) 1999-2024 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#pragma once

#include "Ioss_CodeTypes.h"
#include "ioss_export.h"
#include "vtk_ioss_mangle.h"

#include <cstdint> // for int64_t
#include <string>  // for string
#include <variant>
#include <vector>

namespace Ioss {
  class GroupingEntity;
} // namespace Ioss

namespace Ioss {

  /** \brief A named value that has a known type.
   *
   */
  class IOSS_EXPORT Property
  {
  public:
    enum BasicType { INVALID = -1, REAL, INTEGER, POINTER, STRING, VEC_INTEGER, VEC_DOUBLE };
    enum Origin {
      INTERNAL = -1, ///< Property is for internal use
      IMPLICIT, ///< Property is calculated on the fly based on current state of entity containing
                // property
      EXTERNAL, ///< Property was created by client
      ATTRIBUTE ///< Property created from an Exodus or Database Attribute
    };

    Property(std::string name, int64_t value, Origin origin = INTERNAL);
    Property(std::string name, int value, Origin origin = INTERNAL);
    Property(std::string name, double value, Origin origin = INTERNAL);
    Property(std::string name, const std::string &value, Origin origin = INTERNAL);
    Property(std::string name, const char *value, Origin origin = INTERNAL);
    Property(std::string name, void *value, Origin origin = INTERNAL);
    Property(std::string name, const std::vector<int> &value, Origin origin = INTERNAL);
    Property(std::string name, const std::vector<double> &value, Origin origin = INTERNAL);

    // To set implicit property
    Property(const GroupingEntity *ge, std::string name, BasicType type);

    IOSS_NODISCARD std::string get_string() const;
    IOSS_NODISCARD int64_t     get_int() const;
    IOSS_NODISCARD double      get_real() const;
    IOSS_NODISCARD void       *get_pointer() const;
    IOSS_NODISCARD std::vector<double> get_vec_double() const;
    IOSS_NODISCARD std::vector<int> get_vec_int() const;

    void                  set_origin(Origin origin) { origin_ = origin; }
    IOSS_NODISCARD Origin get_origin() const { return origin_; }

    /** \brief Tells whether the property is calculated, rather than stored.
     *
     * \returns True if property is calculated; False if it is stored directly.
     */
    IOSS_NODISCARD bool is_implicit() const { return origin_ == IMPLICIT; }

    /** \brief Tells whether the property is stored, rather than calculated.
     *
     * \returns True if property is stored directly; False if it is calculated.
     */
    IOSS_NODISCARD bool is_explicit() const { return origin_ != IMPLICIT; }

    /** Tells whether the property has a valid type (currently REAL, INTEGER, POINTER, or STRING)
     *
     *  \returns True if the property type is valid.
     */
    IOSS_NODISCARD bool is_valid() const { return type_ != INVALID; }

    /** Tells whether the property has an invalid type (currently not one of REAL, INTEGER, POINTER,
     * or STRING)
     *
     *  \returns True if the property type is invalid.
     */
    IOSS_NODISCARD bool is_invalid() const { return type_ == INVALID; }

    /** \brief Get the property name.
     *
     *  \returns The property name.
     */
    IOSS_NODISCARD std::string get_name() const { return name_; }

    /** \brief Get the property type.
     *
     *  \returns The property type.
     */
    IOSS_NODISCARD BasicType get_type() const { return type_; }

    IOSS_NODISCARD bool operator!=(const Ioss::Property &rhs) const;
    IOSS_NODISCARD bool operator==(const Ioss::Property &rhs) const;

#if 0
    friend void swap(Ioss::Property &first, Ioss::Property &second) noexcept
    {
      using std::swap;
      swap(first.name_, second.name_);
      swap(first.type_, second.type_);
      swap(first.origin_, second.origin_);
      swap(first.data_, second.data_);
    }
#endif
  private:
    std::string name_{};
    BasicType   type_{INVALID};

    /// True if property is calculated rather than stored.
    /// False if property is stored in 'data_'
    Origin origin_{INTERNAL};

    bool get_value(int64_t *value) const;
    bool get_value(double *value) const;
    bool get_value(std::string *value) const;
    bool get_value(void *&value) const;
    bool get_value(std::vector<double> *value) const;
    bool get_value(std::vector<int> *value) const;

    /// The actual value of the property.  Use 'type_' to
    /// discriminate the actual type of the property.
    std::variant<std::string, const Ioss::GroupingEntity *, double, int64_t, std::vector<double>,
                 std::vector<int>, void *>
        data_;
  };
} // namespace Ioss
