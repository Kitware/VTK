// Copyright(C) 1999-2024 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#pragma once

#include "ioss_export.h"
#include "vtk_ioss_mangle.h"

#include "Ioss_CodeTypes.h"       // for Complex
#include "Ioss_DatabaseIO.h"      // for DatabaseIO
#include "Ioss_EntityType.h"      // for EntityType
#include "Ioss_Field.h"           // for Field, Field::RoleType, etc
#include "Ioss_FieldManager.h"    // for FieldManager, NameList
#include "Ioss_Property.h"        // for Property
#include "Ioss_PropertyManager.h" // for PropertyManager
#include "Ioss_State.h"           // for State
#include "Ioss_VariableType.h"    // for component_count()
#include <cstddef>                // for size_t, nullptr
#include <cstdint>                // for int64_t
#include <string>                 // for string
#include <vector>                 // for vector

#ifdef SEACAS_HAVE_KOKKOS
#include <Kokkos_Core.hpp> // for Kokkos::View
#endif

namespace Ioss {

  /** \brief Base class for all 'grouping' entities.
   *  The following derived classes are typical:
   *
   *  -- NodeSet  -- grouping of nodes (0d topology)
   *
   *  -- EdgeSet  -- grouping of edges (1d topology)
   *
   *  -- FaceSet  -- grouping of faces (2d topology) [Surface]
   *
   *  Similarly, there is:
   *
   *  -- NodeBlock -- grouping of 'similar' nodes (same degree of freedom, ...)
   *
   *  -- ElementBlock -- grouping of 'similar' elements (same element topology,
   *                     attributes, ...)
   *     0d, 1d, 2d, 3d topology possible -- e.g., sphere, bar, quad, hex
   *
   *  A Region is also a grouping entity, except that its list of subentites
   *  are other GroupingEntities. That is, it maintains a list of NodeBlocks,
   *  ElementBlocks, NodeLists, CommLists and Surfaces. [Similar to the
   *  "Composite Pattern" in Design Patterns]  All interface to GroupingEntities
   *  is through the Region class; clients of the IO subsystem have no direct
   *  access to the underlying GroupingEntities (other than the Region).
   *
   *  Each GroupingEntity contains:
   *
   *  -- name
   *
   *  -- MeshEntities of the specified topological dimension
   *
   *  -- Optional attributes, either global (applied to the groupingentity), or
   *     unique value(s) to be applied to each subentity.
   *
   *  -- Data items
   */
  class IOSS_EXPORT GroupingEntity
  {
  public:
    friend class Property;

    GroupingEntity() = default;
    GroupingEntity(DatabaseIO *io_database, const std::string &my_name, int64_t entity_count);
    GroupingEntity(const GroupingEntity &other);
    GroupingEntity &operator=(const GroupingEntity &rhs) = delete;

    virtual ~GroupingEntity();

    IOSS_NODISCARD State get_state() const;

    IOSS_NODISCARD DatabaseIO *get_database() const;
    void                       set_database(DatabaseIO *io_database);
    void                       reset_database(DatabaseIO *io_database);
    virtual void               delete_database();

    /** Return the GroupingEntity pointer of the "object" that this
     *  entity is contained in.  For example, a SideBlock would
     *  return the SideSet that "owns" the SideBlock.
     *  Most GroupingEntities would return the containing Region
     *  A region would return itself(?)
     *  A NodeBlock containing the subset of nodes in a StructuredBlock
     *  would return that StructuredBlock.
     */
    IOSS_NODISCARD virtual const GroupingEntity *contained_in() const;

    /** \brief Get name of entity.
     *
     *  This short-circuits the process of getting the name via the property.
     *  \returns The same information as: entity->get_property("name").get_string()
     */
    IOSS_NODISCARD const std::string &name() const { return entityName; }

    /** \brief Set the name of the entity.
     *
     *  \param[in] new_name The new name of the entity.
     */
    void set_name(const std::string &new_name) { entityName = new_name; }

    /** \brief Get a generated name based on the type of the entity and the id.
     *
     *  For example, element block 10 would return "block_10"
     *  This is the default name if no name is assigned in the mesh database.
     *  \returns The generic name.
     */
    IOSS_NODISCARD std::string generic_name() const;

    /** Determine whether a name is an alias for this entity.
     *
     *  \param[in] my_name Determine whether this name is an alias for this entity.
     *  \returns True if input name is an alias for this entity.
     */
    IOSS_NODISCARD bool is_alias(const std::string &my_name) const;

    /** \brief Get list of blocks that the entities in this GroupingEntity "touch".
     *
     * For a SideSet, returns a list of the element blocks that the
     * elements in the set belong to.
     * For others, it returns an empty vector.
     * Entries are pushed onto the "block_members" vector, so it will be
     * appended to if it is not empty at entry to the function.
     */
    virtual void block_membership(Ioss::NameList & /* block_members */) {}

    IOSS_NODISCARD std::string get_filename() const;

    /** \brief Get the name of the particular type of entity.
     *
     *  \returns The name of the particular type of entity.
     */
    IOSS_NODISCARD virtual std::string type_string() const = 0;

    /** \brief Get a short name of the particular type of entity.
     *
     *  \returns The short name of the particular type of entity.
     */
    IOSS_NODISCARD virtual std::string short_type_string() const = 0;

    /** \brief What does this entity contain
     *
     *  \returns The name of the thing this entity contains.
     */
    IOSS_NODISCARD virtual std::string contains_string() const = 0;

    /** \brief Get the EntityType, which indicates the particular type of GroupingEntity this is.
     *
     *  \returns The particular EntityType of this GroupingEntity.
     */
    IOSS_NODISCARD virtual EntityType type() const = 0;

    // ========================================================================
    //                                PROPERTIES
    // ========================================================================
    // Property-related information....
    // Just forward it through to the property manager...
    void                    property_add(const Property &new_prop);
    void                    property_erase(const std::string &property_name);
    IOSS_NODISCARD bool     property_exists(const std::string &property_name) const;
    IOSS_NODISCARD Property get_property(const std::string &property_name) const;
    IOSS_NODISCARD int64_t  get_optional_property(const std::string &property,
                                                  int64_t            optional_value) const;
    IOSS_NODISCARD std::string get_optional_property(const std::string &property_name,
                                                     const std::string &optional_value) const;
    IOSS_NODISCARD NameList    property_describe() const;
    int                        property_describe(NameList *names) const;
    IOSS_NODISCARD NameList    property_describe(Ioss::Property::Origin origin) const;
    int                   property_describe(Ioss::Property::Origin origin, NameList *names) const;
    IOSS_NODISCARD size_t property_count() const;
    /** Add a property, or change its value if it already exists with
        a different value */
    void property_update(const std::string &property, int64_t value) const;
    void property_update(const std::string &property, const std::string &value) const;

    // ========================================================================
    //                                FIELDS
    // ========================================================================
    // Just forward these through to the field manager...
    void                        field_add(Field new_field);
    void                        field_erase(const std::string &field_name);
    void                        field_erase(Field::RoleType role);
    IOSS_NODISCARD bool         field_exists(const std::string &field_name) const;
    IOSS_NODISCARD Field        get_field(const std::string &field_name) const;
    IOSS_NODISCARD const Field &get_fieldref(const std::string &field_name) const;
    int                         field_describe(NameList *names) const;
    IOSS_NODISCARD NameList     field_describe() const;
    int                         field_describe(Field::RoleType role, NameList *names) const;
    IOSS_NODISCARD NameList     field_describe(Field::RoleType role) const;
    IOSS_NODISCARD size_t       field_count() const;
    IOSS_NODISCARD size_t       field_count(Field::RoleType role) const;

    IOSS_NODISCARD bool check_for_duplicate(const Ioss::Field &new_field) const;

    // Put this fields data into 'data'.

    // Returns number of entities for which the field was read.
    // Assumes 'data' is large enough to hold all values.
    int64_t get_field_data(const std::string &field_name, void *data, size_t data_size) const;

    int64_t put_field_data(const std::string &field_name, void *data, size_t data_size) const;

    // Zero-copy API.  *IF* a field is zero-copyable, then this function will set the `data`
    // pointer to point to a chunk of memory of size `data_size` bytes containing the field
    // data for the specified field.  If the field is not zero-copyable, then the  `data`
    // pointer will point to `nullptr` and `data_size` will be 0.
    int64_t get_field_data(const std::string &field_name, void **data, size_t *data_size) const;

    // Put this fields data into the specified std::vector space.
    // Returns number of entities for which the field was read.
    // Resizes 'data' to size needed to hold all values.
    template <typename T>
    int64_t get_field_data(const std::string &field_name, std::vector<T> &data) const;

    template <typename T>
    int64_t put_field_data(const std::string &field_name, const std::vector<T> &data) const;
    template <typename T>
    int64_t put_field_data(const std::string &field_name, std::vector<T> &data) const;

#ifdef SEACAS_HAVE_KOKKOS
    // Get and put this field's data into the specified Kokkos::View.
    // Returns the number of entities for which the field was read.
    // Resizes 'data' to size needed to hold all values;
    // however, any Views that were previously created referencing the same
    // underlying memory allocation as 'data' will remain the original size.
    template <typename T, typename... Args>
    int64_t get_field_data(const std::string &field_name, Kokkos::View<T *, Args...> &data) const;

    template <typename T, typename... Args>
    int64_t get_field_data(const std::string &field_name, Kokkos::View<T **, Args...> &data) const;

    template <typename T, typename... Args>
    int64_t put_field_data(const std::string &field_name, Kokkos::View<T *, Args...> &data) const;

    template <typename T, typename... Args>
    int64_t put_field_data(const std::string &field_name, Kokkos::View<T **, Args...> &data) const;
#endif

    /** Get the number of bytes used to store the INT data type
     *
     *  \returns The number of bytes.
     */
    IOSS_NODISCARD Ioss::Field::BasicType field_int_type() const
    {
      if (database_ == nullptr || get_database()->int_byte_size_api() == 4) {
        return Ioss::Field::INT32;
      }

      return Ioss::Field::INT64;
    }

    IOSS_NODISCARD unsigned int hash() const { return hash_; }

    IOSS_NODISCARD int64_t entity_count() const { return get_property("entity_count").get_int(); }

    // COMPARE GroupingEntities
    IOSS_NODISCARD bool operator!=(const GroupingEntity &rhs) const;
    IOSS_NODISCARD bool operator==(const GroupingEntity &rhs) const;
    IOSS_NODISCARD bool equal(const GroupingEntity &rhs) const;

  protected:
    void count_attributes() const;

    bool set_state(State new_state)
    {
      entityState = new_state;
      return true;
    }

    // Protected to give access to Region which is the only
    // class that should delete the database. May have to make
    // private and provide friend...
    void really_delete_database();

    // Handle implicit properties -- These are calculated from data stored
    // in the grouping entity instead of having an explicit value assigned.
    // An example would be 'element_block_count' for a region.
    // Note that even though this is a pure virtual function, an implementation
    // is provided to return properties that are common to all grouping entities.
    // Derived classes should call 'GroupingEntity::get_implicit_property'
    // if the requested property is not specific to their type.
    IOSS_NODISCARD virtual Property get_implicit_property(const std::string &my_name) const = 0;

    PropertyManager properties;
    FieldManager    fields;

    virtual int64_t internal_get_field_data(const Field &field, void *data,
                                            size_t data_size = 0) const = 0;
    virtual int64_t internal_put_field_data(const Field &field, void *data,
                                            size_t data_size = 0) const = 0;

    virtual int64_t internal_get_zc_field_data(const Field &field, void **data,
                                               size_t *data_size) const = 0;

    int64_t entityCount = 0;

#if defined(IOSS_THREADSAFE)
    mutable std::mutex m_;
#endif

    IOSS_NODISCARD bool equal_(const GroupingEntity &rhs, bool quiet) const;

  private:
    void verify_field_exists(const std::string &field_name, const std::string &inout) const;

    std::string entityName{};

    DatabaseIO *database_ = nullptr;

    mutable int64_t attributeCount = 0;
    State           entityState    = STATE_CLOSED;
    unsigned int    hash_          = 0;
  };
} // namespace Ioss

/** \brief Add a property to the entity's property manager.
 *
 *  \param[in] new_prop The property to add.
 */
inline void Ioss::GroupingEntity::property_add(const Ioss::Property &new_prop)
{
  properties.add(new_prop);
}

/** \brief Remove a property from the entity's property manager.
 *
 *  Assumes that the property with the given name already exists in the property manager.
 *
 *  \param[in] property_name The name of the property to remove.
 *
 */
inline void Ioss::GroupingEntity::property_erase(const std::string &property_name)
{
  properties.erase(property_name);
}

/** \brief Checks if a property exists in the entity's property manager.
 *
 *  \param[in] property_name The property to check
 *  \returns True if the property exists.
 */
inline bool Ioss::GroupingEntity::property_exists(const std::string &property_name) const
{
  return properties.exists(property_name);
}

/** \brief Get the Property from the property manager associated with the entity.
 *
 *  \param[in] property_name The name of the property to get
 *  \returns The property
 *
 */
inline Ioss::Property Ioss::GroupingEntity::get_property(const std::string &property_name) const
{
  return properties.get(property_name);
}

inline int64_t Ioss::GroupingEntity::get_optional_property(const std::string &property_name,
                                                           int64_t            optional_value) const
{
  return properties.get_optional(property_name, optional_value);
}

inline std::string
Ioss::GroupingEntity::get_optional_property(const std::string &property_name,
                                            const std::string &optional_value) const
{
  return properties.get_optional(property_name, optional_value);
}

/** \brief Get the names of all properties in the property manager for this entity.
 *
 * \returns The property names in the property manager.
 */
inline Ioss::NameList Ioss::GroupingEntity::property_describe() const
{
  return properties.describe();
}

/** \brief Get the names of all properties in the property manager for this entity.
 *
 * \param[out] names All the property names in the property manager.
 * \returns The number of properties extracted from the property manager.
 */
inline int Ioss::GroupingEntity::property_describe(NameList *names) const
{
  return properties.describe(names);
}

inline Ioss::NameList Ioss::GroupingEntity::property_describe(Ioss::Property::Origin origin) const
{
  return properties.describe(origin);
}

inline int Ioss::GroupingEntity::property_describe(Ioss::Property::Origin origin,
                                                   NameList              *names) const
{
  return properties.describe(origin, names);
}

/** \brief Get the number of properties defined in the property manager for this entity.
 *
 *  \returns The number of properties.
 */
inline size_t Ioss::GroupingEntity::property_count() const { return properties.count(); }

// ------------------------------------------------------------------------

/** \brief Remove all fields of type `role` from the entity's field manager.
 *
 * \param[in] role Remove all fields (if any) of type `role`
 */
inline void Ioss::GroupingEntity::field_erase(Ioss::Field::RoleType role) { fields.erase(role); }

/** \brief Remove a field from the entity's field manager.
 *
 * Assumes that a field with the given name exists in the field manager.
 *
 * \param[in] field_name The name of the field to remove.
 */
inline void Ioss::GroupingEntity::field_erase(const std::string &field_name)
{
  fields.erase(field_name);
}

/** \brief Checks if a field with a given name exists in the entity's field manager.
 *
 * \param[in] field_name The name of the field to check for.
 * \returns True if the field exists in the entity's field manager.
 *
 */
inline bool Ioss::GroupingEntity::field_exists(const std::string &field_name) const
{
  return fields.exists(field_name);
}

/** \brief Get a field from the entity's field manager.
 *
 *  \param[in] field_name The name of the field to get.
 *  \returns The field object.
 *
 */
inline Ioss::Field Ioss::GroupingEntity::get_field(const std::string &field_name) const
{
  return fields.get(field_name);
}

/** \brief Get a reference to a field from the entity's field manager.
 *
 *  \param[in] field_name The name of the field to get.
 *  \returns A reference to the field object.
 *
 */
inline const Ioss::Field &Ioss::GroupingEntity::get_fieldref(const std::string &field_name) const
{
  return fields.getref(field_name);
}

/** \brief Get the names of all fields in the entity's field manager.
 *
 * \returns All field names in the entity's field manager.
 *
 */
inline Ioss::NameList Ioss::GroupingEntity::field_describe() const { return fields.describe(); }

/** \brief Get the names of all fields in the entity's field manager.
 *
 * \param[out] names All field names in the entity's field manager.
 * \returns The number of fields extracted from the entity's field manager.
 *
 */
inline int Ioss::GroupingEntity::field_describe(NameList *names) const
{
  return fields.describe(names);
}

/** \brief Get the names of all fields of a specified RoleType in the entity's field manager.
 *
 * \param[in] role The role type (MESH, ATTRIBUTE, TRANSIENT, REDUCTION, etc.)
 * \returns All field names of the specified RoleType in the entity's field manager.
 *
 */
inline Ioss::NameList Ioss::GroupingEntity::field_describe(Ioss::Field::RoleType role) const
{
  return fields.describe(role);
}

/** \brief Get the names of all fields of a specified RoleType in the entity's field manager.
 *
 * \param[in] role The role type (MESH, ATTRIBUTE, TRANSIENT, REDUCTION, etc.)
 * \param[out] names All field names of the specified RoleType in the entity's field manager.
 * \returns The number of fields extracted from the entity's field manager.
 *
 */
inline int Ioss::GroupingEntity::field_describe(Ioss::Field::RoleType role, NameList *names) const
{
  return fields.describe(role, names);
}

/** \brief Get the number of fields in the entity's field manager.
 *
 *  \returns The number of fields in the entity's field manager.
 */
inline size_t Ioss::GroupingEntity::field_count() const { return fields.count(); }

/** \brief Read type 'T' field data from the database file into memory using a std::vector.
 *
 *  \param[in] field_name The name of the field to read.
 *  \param[out] data The data.
 *  \returns The number of values read.
 *
 */
template <typename T>
int64_t Ioss::GroupingEntity::get_field_data(const std::string &field_name,
                                             std::vector<T>    &data) const
{
  verify_field_exists(field_name, "input");

  Ioss::Field field = get_field(field_name);
  field.check_type(Ioss::Field::get_field_type(static_cast<T>(0)));

  data.resize(field.raw_count() * field.raw_storage()->component_count());
  size_t data_size = data.size() * sizeof(T);
  auto   retval    = internal_get_field_data(field, Data(data), data_size);

  // At this point, transform the field if specified...
  if (retval >= 0) {
    field.transform(Data(data));
  }

  return retval;
}

/** \brief Write type 'T' field data from memory into the database file using a std::vector.
 *
 *  \param[in] field_name The name of the field to write.
 *  \param[in] data The data.
 *  \returns The number of values written.
 *
 */
template <typename T>
int64_t Ioss::GroupingEntity::put_field_data(const std::string    &field_name,
                                             const std::vector<T> &data) const
{
  verify_field_exists(field_name, "output");

  Ioss::Field field = get_field(field_name);
  field.check_type(Ioss::Field::get_field_type(T(0)));
  size_t data_size = data.size() * sizeof(T);
  if (field.has_transform()) {
    // Need non-const data since the transform will change the users data.
    std::vector<T> nc_data(data);
    field.transform(Data(nc_data));
    return internal_put_field_data(field, Data(nc_data), data_size);
  }

  T *my_data = const_cast<T *>(Data(data));
  return internal_put_field_data(field, my_data, data_size);
}

template <typename T>
int64_t Ioss::GroupingEntity::put_field_data(const std::string &field_name,
                                             std::vector<T>    &data) const
{
  verify_field_exists(field_name, "output");

  Ioss::Field field = get_field(field_name);
  field.check_type(Ioss::Field::get_field_type(static_cast<T>(0)));
  size_t data_size = data.size() * sizeof(T);
  T     *my_data   = const_cast<T *>(Data(data));
  field.transform(my_data);
  return internal_put_field_data(field, my_data, data_size);
}

#ifdef SEACAS_HAVE_KOKKOS

/** \brief Read field data from the database file into memory using a 1-D Kokkos:::View.
 *
 *  \tparam T The data type.
 *  \tparam Args The other template arguments for data.
 *  \param[in] field_name The name of the field to read.
 *  \param[out] data The data.
 *  \returns The number of values read.
 *
 */
template <typename T, typename... Args>
int64_t Ioss::GroupingEntity::get_field_data(const std::string          &field_name,
                                             Kokkos::View<T *, Args...> &data) const
{
  typedef Kokkos::View<T *, Args...> ViewType;

  verify_field_exists(field_name, "input");

  Ioss::Field field = get_field(field_name);

  // Resize the view
  auto new_view_size = field.raw_count() * field.raw_storage()->component_count();
  Kokkos::resize(data, new_view_size);
  size_t data_size = new_view_size * sizeof(T);

  // Create a host mirror view. (No memory allocation if data is in HostSpace.)
  typename ViewType::HostMirror host_data = Kokkos::create_mirror_view(data);

  // Extract a pointer to the underlying allocated memory of the host view.
  T *host_data_ptr = host_data.data();

  // Extract the data from disk to the underlying memory pointed to by host_data_ptr.
  auto retval = internal_get_field_data(field, host_data_ptr, data_size);

  // At this point, transform the field if specified...
  if (retval >= 0)
    field.transform(host_data_ptr);

  // Copy the data to the device. (No op if data is in HostSpace.)
  Kokkos::deep_copy(data, host_data);

  return retval;
}

/** \brief Read field data from the database file into memory using a 2-D Kokkos:::View.
 *
 *  \tparam T The data type
 *  \tparam Args The other template arguments for data.
 *  \param[in] field_name The name of the field to read.
 *  \param[out] data The data.
 *  \returns The number of values read.
 *
 */
template <typename T, typename... Args>
int64_t Ioss::GroupingEntity::get_field_data(const std::string           &field_name,
                                             Kokkos::View<T **, Args...> &data) const
{
  typedef Kokkos::View<T **, Args...> ViewType;

  verify_field_exists(field_name, "input");

  Ioss::Field field = get_field(field_name);

  // Resize the view
  int new_view_size_left  = field.raw_count();
  int new_view_size_right = field.raw_storage()->component_count();
  Kokkos::resize(data, new_view_size_left, new_view_size_right);
  size_t data_size = new_view_size_left * new_view_size_right * sizeof(T);

  // Create and allocate an array to hold the data temporarily.
  // This is necessary to ensure the data is placed in the correct
  // location in the 2-D array, avoiding incorrect placement due
  // to Views with padded dimensions.
  T *data_array = new T[data_size];

  // Create a host mirror view. (No memory allocation if data is in HostSpace.)
  typename ViewType::HostMirror host_data = Kokkos::create_mirror_view(data);

  // Extract the data from disk to the underlying memory pointed to by host_data_ptr.
  auto retval = internal_get_field_data(field, data_array, data_size);

  // At this point, transform the field if specified...
  if (retval >= 0)
    field.transform(data_array);

  // Copy the data to the host Mirror view.
  // The host mirror view has the same layout as the device view.
  // For CUDA, this will be LayoutLeft. In this case, the loop order
  // chosen here will be slower than the reversed loop order.
  // However, The time for this extra in-memory copy is small
  // compared with the time to copy from disk into memory.
  for (int i = 0; i < new_view_size_left; ++i) {
    for (int j = 0; j < new_view_size_right; ++j) {
      host_data(i, j) = data_array[new_view_size_right * i + j];
    }
  }

  // Delete the temporary array
  delete[] data_array;

  // Copy the data to the device. (No op if data is in HostSpace.)
  Kokkos::deep_copy(data, host_data);

  return retval;
}

/** \brief Write field data from memory into the database file using a 1-D Kokkos::View.
 *
 *  \tparam T The data type
 *  \tparam Args The other template arguments for data.
 *  \param[in] field_name The name of the field to write.
 *  \param[in] data The data.
 *  \returns The number of values written.
 *
 */
template <typename T, typename... Args>
int64_t Ioss::GroupingEntity::put_field_data(const std::string          &field_name,
                                             Kokkos::View<T *, Args...> &data) const
{
  typedef Kokkos::View<T *, Args...> ViewType;

  verify_field_exists(field_name, "output");

  Ioss::Field field     = get_field(field_name);
  size_t      data_size = field.raw_count() * field.raw_storage()->component_count() * sizeof(T);

  // Create a host mirror view. (No memory allocation if data is in HostSpace.)
  typename ViewType::HostMirror host_data = Kokkos::create_mirror_view(data);

  // Copy the data to the host. (No op if data is in HostSpace.)
  Kokkos::deep_copy(host_data, data);

  // Extract a pointer to the underlying allocated memory of the host view.
  T *host_data_ptr = host_data.data();

  // Transform the field
  field.transform(host_data_ptr);

  // Copy the data to disk from the underlying memory pointed to by host_data_ptr.
  return internal_put_field_data(field, host_data_ptr, data_size);
}

/** \brief Write field data from memory into the database file using a 2-D Kokkos::View.
 *
 *  \tparam T The data type
 *  \tparam Args The other template arguments for data.
 *  \param[in] field_name The name of the field to write.
 *  \param[in] data The data.
 *  \returns The number of values written.
 *
 */
template <typename T, typename... Args>
int64_t Ioss::GroupingEntity::put_field_data(const std::string           &field_name,
                                             Kokkos::View<T **, Args...> &data) const
{
  typedef Kokkos::View<T **, Args...> ViewType;

  verify_field_exists(field_name, "output");

  Ioss::Field field = get_field(field_name);

  int    view_size_left  = data.extent(0);
  int    view_size_right = data.extent(1);
  size_t data_size       = field.raw_count() * field.raw_storage()->component_count() * sizeof(T);

  if (view_size_left * view_size_right * sizeof(T) != data_size) {
    std::ostringstream errmsg;
    errmsg << "\nERROR: View dimensions are inconsistent with field raw count or raw storage "
              "component count"
           << "for field" << field_name << "\n\n";
    IOSS_ERROR(errmsg);
  }

  // Create a host mirror view. (No memory allocation if data is in HostSpace.)
  typename ViewType::HostMirror host_data = Kokkos::create_mirror_view(data);

  // Copy the data to the host. (No op if data is in HostSpace.)
  Kokkos::deep_copy(host_data, data);

  // Create and allocate an array to hold the data temporarily.
  // This is necessary to ensure the data is taken from the correct
  // location in the 2-D array, avoiding incorrect location due
  // to Views with padded dimensions.
  T *data_array = new T[data_size];

  // Copy the data from the host Mirror view.
  // The host mirror view has the same layout as the device view.
  // For CUDA, this will be LayoutLeft. In this case, the loop order
  // chosen here will be slower than the reversed loop order.
  // However, The time for this extra in-memory copy is small
  // compared with the time to copy to disk from memory.
  for (int i = 0; i < view_size_left; ++i) {
    for (int j = 0; j < view_size_right; ++j) {
      data_array[view_size_right * i + j] = host_data(i, j);
    }
  }

  // Transform the field
  field.transform(data_array);

  // Copy the data to disk from the underlying memory pointed to by data_array.
  auto retval = internal_put_field_data(field, data_array, data_size);

  // Delete the temporary array
  delete[] data_array;

  return retval;
}
#endif
