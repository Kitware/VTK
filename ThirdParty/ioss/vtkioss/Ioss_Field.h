// Copyright(C) 1999-2022 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#pragma once

#include "ioss_export.h"

#include "vtk_ioss_mangle.h"

#include <Ioss_CodeTypes.h>
#include <cstddef> // for size_t
#include <string>  // for string
#include <vector>  // for vector
namespace Ioss {
  class GroupingEntity;
  class Transform;
  class VariableType;

  /** \brief Holds metadata for bulk data associated with a GroupingEntity.
   */
  class IOSS_EXPORT Field
  {
  public:
    /** \brief The basic data type held in the field.
     */
    enum BasicType {
      INVALID = -1,
      REAL    = 1,
      DOUBLE  = 1,
      INTEGER = 4,
      INT32   = 4,
      INT64   = 8,
      COMPLEX,
      STRING,
      CHARACTER
    };

    enum class InOut { INPUT, OUTPUT };

    static Ioss::Field::BasicType get_field_type(char /*dummy*/) { return CHARACTER; }
    static Ioss::Field::BasicType get_field_type(double /*dummy*/) { return DOUBLE; }
    static Ioss::Field::BasicType get_field_type(int /*dummy*/) { return INTEGER; }
    static Ioss::Field::BasicType get_field_type(unsigned int /*dummy*/) { return INTEGER; }
    static Ioss::Field::BasicType get_field_type(int64_t /*dummy*/) { return INT64; }
    static Ioss::Field::BasicType get_field_type(uint64_t /*dummy*/) { return INT64; }
    static Ioss::Field::BasicType get_field_type(Complex /*dummy*/) { return COMPLEX; }
    static Ioss::Field::BasicType get_field_type(const std::string & /*dummy*/) { return STRING; }

    /* \brief Categorizes the type of information held in the field.
     */
    enum RoleType {
      INTERNAL,
      MESH,      /**< A field which is used to define the basic geometry
                      or topology of the model and is not normally transient
                      in nature. Examples would be element connectivity or
                      nodal coordinates. */
      ATTRIBUTE, /**< A field which is used to define an attribute on an
                      EntityBlock derived class. Examples would be thickness
                      of the elements in a shell element block or the radius
                      of particles in a particle element block. */
      MAP,
      COMMUNICATION,
      MESH_REDUCTION, /**< A field which summarizes some non-transient data
                         about an entity (\sa REDUCTION). This could be an
                         offset applied to an element block, or the units
                         system of a model or the name of the solid model
                         which this entity is modelling... */
      INFORMATION = MESH_REDUCTION,
      REDUCTION, /**< A field which typically summarizes some transient data
                      about an entity. The size of this field is typically not
                      proportional to the number of entities in a GroupingEntity.
                      An example would be average displacement over a group of
                      nodes or the kinetic energy of a model. This data is also
                      transient. */
      TRANSIENT  /**< A field which is typically calculated at multiple steps
                      or times in an analysis. These are typically "results"
                      data. Examples would be nodal displacement or element
                      stress. */
    };

    Field();

    // Create a field named 'name' that contains values of type 'type'
    // in a storage format of type 'storage'.  There are 'value_count'
    // items in the field. If `value_count==0`, then the correct size
    // will be set when the field is added to a `GroupingEntity`
    Field(std::string name, BasicType type, const std::string &storage, RoleType role,
          size_t value_count = 0, size_t index = 0);

    Field(std::string name, BasicType type, const std::string &storage, int copies, RoleType role,
          size_t value_count = 0, size_t index = 0);

    Field(std::string name, BasicType type, const VariableType *storage, RoleType role,
          size_t value_count = 0, size_t index = 0);

    Field(const Ioss::Field &from)      = default;
    Field &operator=(const Field &from) = default;
    ~Field()                            = default;

    // Compare two fields (used for STL container)
    bool operator<(const Field &other) const;

    bool operator==(const Ioss::Field &rhs) const;
    bool operator!=(const Ioss::Field &rhs) const;
    bool equal(const Ioss::Field &rhs) const;

    bool is_valid() const { return type_ != INVALID; }
    bool is_invalid() const { return type_ == INVALID; }

    const std::string &get_name() const { return name_; }
    std::string       &get_name() { return name_; }

    /** \brief Get name of the 'component_indexth` component (1-based)
     *
     * \param[in] component_index 1-based index of the component to be named
     * \param[in] in_out Is the field being read or written
     * \param[in] suffix optional suffix separator to be used if the separator
     *            on the field is set to '1' which means 'unset'
     * \returns name of the specified component
     */
    std::string get_component_name(int component_index, InOut in_out, char suffix = 1) const;
    int         get_component_count(InOut in_out) const;

    void set_suffix_separator(char suffix_separator) { suffixSeparator_ = suffix_separator; }
    char get_suffix_separator() const { return suffixSeparator_; }
    void set_suffices_uppercase(bool true_false) { sufficesUppercase_ = true_false; }
    bool get_suffices_uppercase() const { return sufficesUppercase_; }

    /** \brief Get the basic data type of the data held in the field.
     *
     * \returns the basic data type of the data held in the field.
     */
    BasicType get_type() const { return type_; }

    const VariableType *raw_storage() const { return rawStorage_; }
    const VariableType *transformed_storage() const { return transStorage_; }

    size_t raw_count() const { return rawCount_; }           // Number of items in field
    size_t transformed_count() const { return transCount_; } // Number of items in field

    size_t get_size() const; // data size (in bytes) required to hold entire field

    /** \brief Get the role (MESH, ATTRIBUTE, TRANSIENT, REDUCTION, etc.) of the data in the field.
     *
     * \returns The RoleType of the data in the field.
     */
    RoleType get_role() const { return role_; }

    size_t get_index() const { return index_; }
    void   set_index(size_t index) const { index_ = index; }

    void reset_count(size_t new_count);  // new number of items in field
    void reset_type(BasicType new_type); // new type of items in field.

    // Verify that data_size is valid.  Return value is the maximum
    // number of entities to get ('RawCount')
    // Throws runtime error if data_size too small.
    size_t verify(size_t data_size) const;

    // Verify that the type 'the_type' matches the field's type.
    // throws exception if the types don't match.
    void check_type(BasicType the_type) const;

    bool               is_type(BasicType the_type) const { return the_type == type_; }

    std::string        type_string() const;
    static std::string type_string(BasicType type);

    std::string        role_string() const;
    static std::string role_string(RoleType role);

    bool add_transform(Transform *my_transform);
    bool transform(void *data);
    bool has_transform() const { return !transforms_.empty(); }

  private:
    std::string name_;

    size_t rawCount_{};   // Count of items in field before transformation
    size_t transCount_{}; // Count of items in field after transformed
    size_t size_{};       // maximum data size (in bytes) required to hold entire field
    mutable size_t
        index_{}; // Optional flag that can be used by a client to indicate an ordering.
                  // Unused by field itself.  Used by some DatabaeIO objects to set ordering.
    BasicType type_{INVALID};
    RoleType  role_{INTERNAL};

    const VariableType *rawStorage_{};   // Storage type of raw field
    const VariableType *transStorage_{}; // Storage type after transformation

    std::vector<Transform *> transforms_;
    char                     suffixSeparator_{1}; // Value = 1 means unset; use database default.
    bool sufficesUppercase_{false}; // True if the suffices are uppercase on database...

    bool equal_(const Ioss::Field &rhs, bool quiet) const;
  };
} // namespace Ioss
