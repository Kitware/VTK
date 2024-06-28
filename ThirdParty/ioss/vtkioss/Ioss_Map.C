// Copyright(C) 1999-2024 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include "Ioss_Field.h" // for Field, etc
#include "Ioss_Map.h"
#include "Ioss_SmartAssert.h"
#include "Ioss_Utils.h" // for IOSS_ERROR
#include <cstddef>      // for size_t
#include "vtk_fmt.h"
#include VTK_FMT(fmt/ostream.h)
#include <numeric>
#include <sstream>
#include <vector> // for vector, vector<>::iterator, etc

#include "Ioss_CodeTypes.h"

// If defined, then only build m_reverseMap when it is used.
#undef USE_LAZY_REVERSE

namespace {
  template <typename INT> bool is_one2one(INT *ids, size_t num_to_get, size_t offset)
  {
    bool one2one    = true;
    INT  map_offset = num_to_get > 0 ? ids[0] - 1 - offset : 0;
    for (size_t i = 0; i < num_to_get; i++) {
      if ((size_t)ids[i] != i + offset + 1 + map_offset) {
        one2one = false;
        break;
      }
    }
    return one2one;
  }
} // namespace

void Ioss::Map::release_memory()
{
  IOSS_FUNC_ENTER(m_);
  MapContainer().swap(m_map);
  MapContainer().swap(m_reorder);
  ReverseMapContainer().swap(m_reverse);
}

// Determines whether the input map is sequential (m_map[i] == i)
bool Ioss::Map::is_sequential(bool check_all) const
{
  // Assumes the_map runs from [1..size) Slot zero will contain -1 if the
  // vector is sequential; 1 if not sequential, and 0 if it has not
  // yet been determined...
  // Once the the_map has been determined to be sequential/not-sequential,
  // slot zero is set appropriately.
  // 'sequential' is defined here to mean i==the_map[i] for all
  // 0<i<the_map.size()

  // Arguably, an empty map is sequential...
  if (m_map.empty()) {
    return true;
  }

  if (!check_all) {
    // Check slot zero...
    if (m_map[0] == -1) {
      return true;
    }
    if (m_map[0] == 1) {
      return false;
    }
  }

  IOSS_FUNC_ENTER(m_);
  auto  &new_map  = const_cast<Ioss::MapContainer &>(m_map);
  size_t map_size = m_map.size();
  if (m_offset == -1 && map_size > 1) {
    m_offset = m_map[1] - 1;
  }
  for (int64_t i = 1; i < (int64_t)map_size; i++) {
    if (m_map[i] != i + m_offset) {
      new_map[0] = 1;
      m_offset   = -1;
      return false;
    }
  }
  new_map[0] = -1;
  return true;
}

void Ioss::Map::set_size(size_t entity_count)
{
  IOSS_FUNC_ENTER(m_);
  if (m_map.empty()) {
    m_map.resize(entity_count + 1);
    set_is_sequential(true);
  }
}

void Ioss::Map::build_reverse_map() { build_reverse_map(m_map.size() - 1, 0); }
void Ioss::Map::build_reverse_map_no_lock() { build_reverse_map_nl(m_map.size() - 1, 0); }
void Ioss::Map::build_reverse_map(int64_t num_to_get, int64_t offset)
{
  IOSS_FUNC_ENTER(m_);
  build_reverse_map_nl(num_to_get, offset);
}

void Ioss::Map::build_reverse_map_nl(int64_t num_to_get, int64_t offset)
{
  // Stored as an unordered map -- key:global_id, value:local_id
  if (!is_sequential()) {
#if defined MAP_USE_SORTED_VECTOR
    ReverseMapContainer new_ids;
    if (m_reverse.empty()) {
      // This is first time that the m_reverse map is being built..
      // m_map is no longer  1-to-1.
      // Just iterate m_map and add all values that are non-zero
      new_ids.reserve(m_map.size() - 1);
      for (size_t i = 1; i < m_map.size(); i++) {
        if (m_map[i] != 0) {
          new_ids.emplace_back(m_map[i], i);
        }
      }
    }
    else {
      new_ids.reserve(num_to_get);
      for (int64_t i = 0; i < num_to_get; i++) {
        int64_t local_id = offset + i + 1;
        new_ids.emplace_back(m_map[local_id], local_id);

        if (m_map[local_id] <= 0) {
          std::ostringstream errmsg;
          fmt::print(
              errmsg,
              "\nERROR: {0} map detected non-positive global id {1} for {0} with local id {2} "
              "on processor {3}.\n",
              m_entityType, m_map[local_id], local_id, m_myProcessor);
          IOSS_ERROR(errmsg);
        }
      }
    }

    // new_ids is a vector of pairs <global_id, local_id>
    Ioss::sort(new_ids);

    int64_t new_id_min = new_ids.empty() ? 0 : new_ids.front().first;
    int64_t old_id_max = m_reverse.empty() ? 0 : m_reverse.back().first;
    if (new_ids.size() + 1 == m_map.size()) {
      SMART_ASSERT(m_reverse.empty() || m_reverse.size() + 1 == m_map.size());
      new_ids.swap(m_reverse);
    }
    else if (new_id_min > old_id_max) {
      m_reverse.insert(m_reverse.end(), new_ids.begin(), new_ids.end());
    }
    else {
      // Copy reverseElementMap to old_ids, empty reverseElementMap.
      ReverseMapContainer old_ids;
      old_ids.swap(m_reverse);
      SMART_ASSERT(m_reverse.empty());

      // Merge old_ids and new_ids to reverseElementMap.
      m_reverse.reserve(old_ids.size() + new_ids.size());
      std::merge(
          old_ids.begin(), old_ids.end(), new_ids.begin(), new_ids.end(),
          std::inserter(m_reverse, m_reverse.begin()),
          [](const Ioss::IdPair &lhs, const Ioss::IdPair &rhs) { return lhs.first < rhs.first; });
    }

    // Check for duplicate ids...
    // Maybe debug only...
    verify_no_duplicate_ids(m_reverse);
#else
    if (m_reverse.empty()) {
      // This is first time that the m_reverse map is being built..
      // m_map is no longer  1-to-1.
      // Just iterate m_map and add all values that are non-zero
      m_reverse.max_load_factor(0.95);
      m_reverse.reserve(m_map.size());
      for (size_t i = 1; i < m_map.size(); i++) {
        if (m_map[i] != 0) {
          bool ok = m_reverse.insert({m_map[i], i}).second;
          if (!ok) {
            std::ostringstream errmsg;
            fmt::print(
                errmsg,
                "\nERROR: Duplicate {0} global id detected on processor {1}, filename '{2}'.\n"
                "       Global id {3} assigned to local {0}s {4} and {5}.\n",
                m_entityType, m_myProcessor, m_filename, m_map[i], i, m_reverse[m_map[i]]);
            IOSS_ERROR(errmsg);
          }
        }
      }
    }
    else {
      for (int64_t i = 0; i < num_to_get; i++) {
        int64_t local_id = offset + i + 1;
        bool    ok       = m_reverse.insert({m_map[local_id], local_id}).second;
        if (!ok) {
          if (local_id != m_reverse[m_map[local_id]]) {
            std::ostringstream errmsg;
            fmt::print(
                errmsg,
                "\nERROR: Duplicate {0} global id detected on processor {1}, filename '{2}'.\n"
                "       Global id {3} assigned to local {0}s {4} and {5}.\n",
                m_entityType, m_myProcessor, m_filename, m_map[local_id], local_id,
                m_reverse[m_map[local_id]]);
            IOSS_ERROR(errmsg);
          }
        }

        if (m_map[local_id] <= 0) {
          std::ostringstream errmsg;
          fmt::print(
              errmsg,
              "\nERROR: {0} map detected non-positive global id {1} for {0} with local id {2} "
              "on processor {3}.\n",
              m_entityType, m_map[local_id], local_id, m_myProcessor);
          IOSS_ERROR(errmsg);
        }
      }
    }
#if IOSS_DEBUG_OUTPUT
    fmt::print("[{}] ({}) Map Size         = {}\n", m_myProcessor, m_entityType, m_map.size());
    fmt::print("[{}] ({}) Size             = {}\n", m_myProcessor, m_entityType, m_reverse.size());
    fmt::print("[{}] ({}) Bucket Count     = {}\n", m_myProcessor, m_entityType,
               m_reverse.bucket_count());
    fmt::print("[{}] ({}) Load Factor      = {}\n", m_myProcessor, m_entityType,
               m_reverse.load_factor());
    fmt::print("[{}] ({}) Max Load Factor  = {}\n\n", m_myProcessor, m_entityType,
               m_reverse.max_load_factor());
#endif
#endif
  }
}

#if defined MAP_USE_SORTED_VECTOR
void Ioss::Map::verify_no_duplicate_ids(std::vector<Ioss::IdPair> &reverse_map)
{
  // Check for duplicate ids...
  auto dup = std::adjacent_find(
      reverse_map.begin(), reverse_map.end(),
      [](const Ioss::IdPair &lhs, const Ioss::IdPair &rhs) { return lhs.first == rhs.first; });
  if (dup != reverse_map.end()) {
    auto               other = dup + 1;
    std::ostringstream errmsg;
    fmt::print(errmsg,
               "\nERROR: Duplicate {0} global id detected on processor {1}, filename '{2}'.\n"
               "       Global id {3} assigned to local {0}s {4} and {5}.\n",
               m_entityType, m_myProcessor, m_filename, (*dup).first, (*dup).second,
               (*other).second);
    IOSS_ERROR(errmsg);
  }
}
#endif

template IOSS_EXPORT bool Ioss::Map::set_map(int *ids, size_t count, size_t offset,
                                             bool in_define_mode);
template IOSS_EXPORT bool Ioss::Map::set_map(int64_t *ids, size_t count, size_t offset,
                                             bool in_define_mode);

template <typename INT>
bool Ioss::Map::set_map(INT *ids, size_t count, size_t offset, bool in_define_mode)
{
  IOSS_FUNC_ENTER(m_);
  if (in_define_mode && is_sequential()) {
    // If the current map is one-to-one, check whether it will be one-to-one
    // after adding these ids...
    bool one2one = is_one2one(ids, count, offset);
    if (one2one) {
      // Further checks on how ids fit into previously set m_map entries (if any)
      if (count > 0) {
        INT tmp_offset = ids[0] - 1 - offset;
        if (tmp_offset < 0 || (m_offset >= 0 && tmp_offset != m_offset)) {
          one2one = false;
        }
      }
    }

    if (!one2one) {
      // Up to this point, the id map has been one-to-one.  Once we
      // apply these `ids` to `m_map`, the map will no
      // longer be one-to-one. The main consequence of this is that we
      // now need an explicit reverseMap.  The reverseMap is built
      // incrementally with the current range of 'ids', but before
      // that can be done, need to build a reverseMap of the current
      // one-to-one data...
      set_is_sequential(false);
#if !defined USE_LAZY_REVERSE
      if (m_map.size() - 1 > count) {
        build_reverse_map_nl(m_map.size() - 1, 0);
      }
#endif
      m_offset = 0;
    }
    else {
      // Map is sequential beginning at ids[0]
      if (count > 0) {
        m_offset = ids[0] - 1 - offset;
      }
    }
  }

  // Determine if `changed` which means an entity was redefined...
  // This is used to determine whether a `reorder` map is needed.
  bool changed = false; // True if redefining an entry
  for (size_t i = 0; i < count; i++) {
    int64_t local_id = offset + i + 1;
    SMART_ASSERT((size_t)local_id < m_map.size())(local_id)(m_map.size());
    if (m_map[local_id] > 0 && m_map[local_id] != ids[i]) {
      changed = true;
      break;
    }
  }

#if defined USE_LAZY_REVERSE
  // Build this now before we redefine an entry
  if (!in_define_mode && changed) {
    build_reverse_map_nl(m_map.size() - 1, 0);
  }
#endif

  for (size_t i = 0; i < count; i++) {
    int64_t local_id = offset + i + 1;
    SMART_ASSERT((size_t)local_id < m_map.size())(local_id)(m_map.size());

    if (ids[i] <= 0) {
      std::ostringstream errmsg;
      fmt::print(errmsg,
                 "\nERROR: {} mapping routines detected non-positive global id {}"
                 " for local id {} on processor {}, filename '{}'.\n",
                 m_entityType, ids[i], local_id, m_myProcessor, m_filename);
      IOSS_ERROR(errmsg);
    }

    m_map[local_id] = ids[i];
    if (local_id != ids[i] - m_offset) {
      set_is_sequential(false);
    }
  }

  if (in_define_mode) {
    if (changed) {
      m_reverse.clear();
    }
#if !defined USE_LAZY_REVERSE
    build_reverse_map_nl(count, offset);
#endif
  }
  else if (changed) {
    // Build the reorderEntityMap which does a direct mapping from
    // the current topologies local order to the local order
    // stored in the database if these two orders are different, that
    // is if the ids order was redefined after the STATE_MODEL
    // phase... This is 0-based and used for
    // remapping output and input TRANSIENT fields.
    build_reorder_map_nl(offset, count);
  }
  return changed;
}

void Ioss::Map::set_default(size_t count, size_t offset)
{
  IOSS_FUNC_ENTER(m_);
  m_map.resize(count + 1);
  std::iota(m_map.begin() + 1, m_map.end(), 1 + offset);
  m_offset = -1 * static_cast<int64_t>(offset);
  set_is_sequential(true);
}

#ifndef DOXYGEN_SKIP_THIS
template IOSS_EXPORT void Ioss::Map::reverse_map_data(int *data, size_t count) const;
template IOSS_EXPORT void Ioss::Map::reverse_map_data(int64_t *data, size_t count) const;
#endif

template <typename INT> void Ioss::Map::reverse_map_data(INT *data, size_t count) const
{
  IOSS_FUNC_ENTER(m_);
  if (!is_sequential()) {
    for (size_t i = 0; i < count; i++) {
      INT global_id = data[i];
      data[i]       = (INT)global_to_local_nl(global_id, true);
    }
  }
  else if (m_offset != 0) {
    for (size_t i = 0; i < count; i++) {
      data[i] -= m_offset;
    }
  }
}

void Ioss::Map::reverse_map_data(void *data, const Ioss::Field &field, size_t count) const
{
  if (field.get_type() == Ioss::Field::INTEGER) {
    int *connect = static_cast<int *>(data);
    reverse_map_data(connect, count);
  }
  else {
    auto *connect = static_cast<int64_t *>(data);
    reverse_map_data(connect, count);
  }
}

#ifndef DOXYGEN_SKIP_THIS
template IOSS_EXPORT void Ioss::Map::map_data(int *data, size_t count) const;
template IOSS_EXPORT void Ioss::Map::map_data(int64_t *data, size_t count) const;
#endif

template <typename INT> void Ioss::Map::map_data(INT *data, size_t count) const
{
  IOSS_FUNC_ENTER(m_);
  if (!is_sequential()) {
    for (size_t i = 0; i < count; i++) {
      data[i] = (INT)m_map[data[i]];
    }
  }
  else if (m_offset != 0) {
    for (size_t i = 0; i < count; i++) {
      data[i] += m_offset;
    }
  }
}

void Ioss::Map::map_data(void *data, const Ioss::Field::BasicType type, size_t count) const
{
  if (type == Ioss::Field::INTEGER) {
    int *datum = static_cast<int *>(data);
    map_data(datum, count);
  }
  else {
    auto *datum = static_cast<int64_t *>(data);
    map_data(datum, count);
  }
}

void Ioss::Map::map_data(void *data, const Ioss::Field &field, size_t count) const
{
  map_data(data, field.get_type(), count);
}

#ifndef DOXYGEN_SKIP_THIS
template IOSS_EXPORT void Ioss::Map::map_implicit_data(int *data, size_t count,
                                                       size_t offset) const;
template IOSS_EXPORT void Ioss::Map::map_implicit_data(int64_t *data, size_t count,
                                                       size_t offset) const;
#endif

template <typename INT>
void Ioss::Map::map_implicit_data(INT *ids, size_t count, size_t offset) const
{
  // Map the "local" ids (offset+1..offset+count) to the global ids. The local
  // ids are implicit
  if (is_sequential()) {
    for (size_t i = 0; i < count; i++) {
      ids[i] = static_cast<INT>(m_offset + offset + 1 + i);
    }
  }
  else {
    for (size_t i = 0; i < count; i++) {
      ids[i] = static_cast<INT>(m_map[offset + 1 + i]);
    }
  }
}

void Ioss::Map::map_implicit_data(void *data, const Ioss::Field &field, size_t count,
                                  size_t offset) const
{
  IOSS_FUNC_ENTER(m_);
  if (field.get_type() == Ioss::Field::INTEGER) {
    map_implicit_data(static_cast<int *>(data), count, offset);
  }
  else {
    map_implicit_data(static_cast<int64_t *>(data), count, offset);
  }
}

template IOSS_EXPORT size_t Ioss::Map::map_field_to_db_scalar_order(double              *variables,
                                                                    std::vector<double> &db_var,
                                                                    size_t begin_offset,
                                                                    size_t count, size_t stride,
                                                                    size_t offset);
template IOSS_EXPORT size_t Ioss::Map::map_field_to_db_scalar_order(int                 *variables,
                                                                    std::vector<double> &db_var,
                                                                    size_t begin_offset,
                                                                    size_t count, size_t stride,
                                                                    size_t offset);
template IOSS_EXPORT size_t Ioss::Map::map_field_to_db_scalar_order(int64_t             *variables,
                                                                    std::vector<double> &db_var,
                                                                    size_t begin_offset,
                                                                    size_t count, size_t stride,
                                                                    size_t offset);

template <typename T>
size_t Ioss::Map::map_field_to_db_scalar_order(T *variables, std::vector<double> &db_var,
                                               size_t begin_offset, size_t count, size_t stride,
                                               size_t offset)
{
  IOSS_FUNC_ENTER(m_);
  size_t num_out = 0;
  if (!m_reorder.empty()) {
    size_t k = offset;
    for (size_t j = begin_offset; j < count * stride; j += stride) {
      // Map to storage location.
      int64_t where = m_reorder[k++] - offset;
      if (where >= 0) {
        SMART_ASSERT(where < (int64_t)count)(where)(count);
        db_var[where] = variables[j];
        num_out++;
      }
    }
  }
  else {
    size_t k = 0;
    for (size_t j = begin_offset; j < count * stride; j += stride) {
      // Map to storage location.
      db_var[k++] = variables[j];
    }
    num_out = count;
  }
  return num_out;
}

void Ioss::Map::build_reorder_map_nl(int64_t start, int64_t count)
{
  // This routine builds a map that relates the current node id order
  // to the original node ordering in affect at the time the file was
  // created. That is, the node map used to define the topology of the
  // model.  Now, if there are changes in node ordering at the
  // application level, we build the node reorder map to map the
  // current order into the original order.  An added complication is
  // that this is more than just a reordering... It may be that the
  // application has 'ghosted' nodes that it doesn't want to put out on
  // the database, so the reorder map must handle a node that is not
  // in the original mesh and map that to an invalid value (currently
  // using -1 as invalid value...)

  // Note: To further add confusion, the reorder map is 0-based
  // and the reverse map and 'map' are 1-based. This is
  // just a consequence of how they are intended to be used...
  //
  // `start` is based on a 0-based array -- start of the reorderMap to build.

  if (m_reorder.empty()) {
    // See if actually need a reorder map first...
    bool need_reorder_map = false;
    if (m_reverse.empty()) {
      need_reorder_map = true;
    }
    else {
      int64_t my_end = start + count;
      for (int64_t i = start; i < my_end; i++) {
        int64_t global_id     = m_map[i + 1];
        int64_t orig_local_id = global_to_local_nl(global_id) - 1;

        // The reordering should only be a permutation of the original
        // ordering within this entity block...
        SMART_ASSERT(orig_local_id >= start && orig_local_id <= my_end)
        (orig_local_id)(start)(my_end);
        if (i != orig_local_id) {
          need_reorder_map = true;
          break;
        }
      }
    }
    if (need_reorder_map) {
      int64_t map_size = m_map.size() - 1;
      m_reorder.resize(map_size);
      // If building a partial reorder map, assume all entries
      // are a direct 1-1 and then let the partial fills overwrite
      // if needed.
      std::iota(m_reorder.begin(), m_reorder.end(), 0);
    }
    else {
      return;
    }
  }

  int64_t my_end = start + count;
  for (int64_t i = start; i < my_end; i++) {
    int64_t global_id     = m_map[i + 1];
    int64_t orig_local_id = global_to_local_nl(global_id) - 1;

    // The reordering should only be a permutation of the original
    // ordering within this entity block...
    SMART_ASSERT(orig_local_id >= start && orig_local_id <= my_end)(orig_local_id)(start)(my_end);
    m_reorder[i] = orig_local_id;
  }
}

// Node and Element mapping functions.  The ExodusII database
// stores ids in a local-id system (1..NUMNP), (1..NUMEL) but
// Sierra wants entities in a global system. These routines
// take care of the mapping from local <-> global

int64_t Ioss::Map::global_to_local(int64_t global, bool must_exist) const
{
  IOSS_FUNC_ENTER(m_);
  return global_to_local_nl(global, must_exist);
}

int64_t Ioss::Map::global_to_local_nl(int64_t global, bool must_exist) const
{
  int64_t local = global;
#if defined USE_LAZY_REVERSE
  if (!is_sequential() && m_reverse.empty() && m_reorder.empty()) {
    auto *new_this = const_cast<Ioss::Map *>(this);
    new_this->build_reverse_map_no_lock();
  }
#endif

  if (!is_sequential() && !m_reverse.empty()) {
    // Possible for !is_sequential() which means non-one-to-one, but
    // reverseMap is empty (which implied one-to-one) if the ORIGINAL mapping defined
    // during dbState == STATE_MODEL was one-to-one, but there is a
    // reordering which is due to new id ordering defined after STATE_MODEL...
#if defined MAP_USE_SORTED_VECTOR
    auto iter = std::lower_bound(
        m_reverse.begin(), m_reverse.end(), global,
        [](const Ioss::IdPair &lhs, int64_t val) -> bool { return lhs.first < val; });
    if (iter != m_reverse.end() && iter->first == global) {
      local = iter->second;
    }
    else {
      local = 0;
    }
#else
    auto iter = m_reverse.find(global);
    if (iter != m_reverse.end()) {
      local = iter->second;
    }
    else {
      local = 0;
    }
#endif
  }
  else if (!must_exist && global > static_cast<int64_t>(m_map.size()) - 1) {
    local = 0;
  }
  else {
    local = global - m_offset;
  }
  if (local > static_cast<int64_t>(m_map.size()) - 1) {
    std::ostringstream errmsg;
    fmt::print(errmsg,
               "ERROR: Ioss Mapping routines detected {0} with global id equal to {1} returns a "
               "local id of {2} which is\n"
               "larger than the local {0} count {5} on processor {3}, filename '{4}'.\n"
               "This should not happen, please report.\n",
               m_entityType, global, local, m_myProcessor, m_filename, m_map.size() - 1);
    IOSS_ERROR(errmsg);
  }
  else if (local <= 0 && must_exist) {
    std::ostringstream errmsg;
    fmt::print(errmsg,
               "ERROR: Ioss Mapping routines could not find a {0} with global id equal to {1} in "
               "the {0} map\n"
               "on processor {2}, filename '{3}'.\n"
               "This should not happen, please report.\n",
               m_entityType, global, m_myProcessor, m_filename);
    IOSS_ERROR(errmsg);
  }
  return local;
}
