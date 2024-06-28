// Copyright(C) 1999-2020, 2022, 2023, 2024 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#pragma once

#include "Ioss_CodeTypes.h"
#include "Ioss_Field.h"
#include <cstddef> // for size_t
#include <cstdint> // for int64_t
#include <string>  // for string
#include <vector>  // for vector

#include "ioss_export.h"
#include "vtk_ioss_mangle.h"

#define MAP_USE_STD
#if defined MAP_USE_STD
#include <unordered_map>
#elif defined MAP_USE_HOPSCOTCH
#include <bhopscotch_map.h>
#elif defined MAP_USE_ROBIN
#include <robin_map.h>
#endif

namespace Ioss {

  using MapContainer = std::vector<int64_t>;
#if defined MAP_USE_SORTED_VECTOR
  using IdPair              = std::pair<int64_t, int64_t>;
  using ReverseMapContainer = std::vector<IdPair>;
#elif defined MAP_USE_STD
  using ReverseMapContainer = std::unordered_map<int64_t, int64_t>;
#elif defined MAP_USE_HOPSCOTCH
  // The `b` variant requires less-than-comparable key, but is faster
  using ReverseMapContainer = tsl::bhopscotch_map<int64_t, int64_t>;
  // using ReverseMapContainer = tsl::hopscotch_map<int64_t, int64_t>;
  // using ReverseMapContainer = tsl::hopscotch_pg_map<int64_t, int64_t>;
#elif defined MAP_USE_ROBIN
  using ReverseMapContainer = tsl::robin_map<int64_t, int64_t>;
  // using ReverseMapContainer = tsl::robin_pg_map<int64_t, int64_t>;
#endif

  class IOSS_EXPORT Map
  {
  public:
    Map() = default;
    Map(std::string entity_type, std::string file_name, int processor)
        : m_entityType(std::move(entity_type)), m_filename(std::move(file_name)),
          m_myProcessor(processor)
    {
    }
    Map(const Map &from)            = delete;
    Map &operator=(const Map &from) = delete;

    void set_rank(int processor) { m_myProcessor = processor; }

    void                  set_size(size_t entity_count);
    IOSS_NODISCARD size_t size() const { return m_map.empty() ? 0 : m_map.size() - 1; }

    void set_is_sequential(bool yesno) { m_map[0] = yesno ? -1 : 1; }

    // Determines whether the input map is sequential (m_map[i] == i)
    IOSS_NODISCARD bool is_sequential(bool check_all = false) const;

    IOSS_NODISCARD int64_t global_to_local(int64_t global, bool must_exist = true) const;

    template <typename INT>
    bool set_map(INT *ids, size_t count, size_t offset, bool in_define_mode = true);

    void set_default(size_t count, size_t offset = 0);

    void build_reverse_map();
    void build_reverse_map_no_lock();
    void build_reverse_map(int64_t num_to_get, int64_t offset);

    void release_memory(); //! Release memory for all maps.

    void reverse_map_data(void *data, const Ioss::Field &field, size_t count) const;
    void map_data(void *data, const Ioss::Field &field, size_t count) const;
    void map_data(void *data, const Ioss::Field::BasicType type, size_t count) const;
    void map_implicit_data(void *data, const Ioss::Field &field, size_t count, size_t offset) const;

    template <typename T>
    size_t map_field_to_db_scalar_order(T *variables, std::vector<double> &db_var,
                                        size_t begin_offset, size_t count, size_t stride,
                                        size_t offset);

    IOSS_NODISCARD const MapContainer &map() const { return m_map; }
    IOSS_NODISCARD MapContainer       &map() { return m_map; }

    IOSS_NODISCARD bool defined() const { return m_defined; }
    void                set_defined(bool yes_no) { m_defined = yes_no; }

    IOSS_NODISCARD bool reorders() const { return !m_reorder.empty(); }

  private:
    template <typename INT> void reverse_map_data(INT *data, size_t count) const;
    template <typename INT> void map_data(INT *data, size_t count) const;
    template <typename INT> void map_implicit_data(INT *data, size_t count, size_t offset) const;

    int64_t global_to_local_nl(int64_t global, bool must_exist = true) const;
    void    build_reorder_map_nl(int64_t start, int64_t count);
    void    build_reverse_map_nl(int64_t num_to_get, int64_t offset);
#if defined MAP_USE_SORTED_VECTOR
    void verify_no_duplicate_ids(std::vector<Ioss::IdPair> &reverse_map);
#endif
#if defined(IOSS_THREADSAFE)
    mutable std::mutex m_;
#endif
    MapContainer        m_map{};
    MapContainer        m_reorder{};
    ReverseMapContainer m_reverse{};
    std::string         m_entityType{"unknown"}; // node, element, edge, face
    std::string         m_filename{"undefined"}; // For error messages only.
    mutable int64_t     m_offset{-1};            // local to global offset if m_map is sequential.
    int                 m_myProcessor{0};        // For error messages...
    bool m_defined{false}; // For use by some clients; not all, so don't read too much into value...
  };
} // namespace Ioss
