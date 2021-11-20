// Copyright(C) 1999-2020 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#ifndef IOSS_Ioss_Map_h
#define IOSS_Ioss_Map_h

#include "vtk_ioss_mangle.h"

#include <Ioss_CodeTypes.h>
#include <cstddef> // for size_t
#include <cstdint> // for int64_t
#include <string>  // for string
#include <vector>  // for vector

#define MAP_USE_HOPSCOTCH
#if defined MAP_USE_STD
#include <unordered_map>
#elif defined MAP_USE_HOPSCOTCH
#include <bhopscotch_map.h>
#elif defined MAP_USE_ROBIN
#include <robin_map.h>
#endif

namespace Ioss {
  class Field;
} // namespace Ioss

namespace Ioss {

  using MapContainer = std::vector<int64_t>;
#if defined MAP_USE_STD
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

  class Map
  {
  public:
    Map() = default;
    Map(std::string entity_type, std::string file_name, int processor)
        : m_entityType(std::move(entity_type)), m_filename(std::move(file_name)),
          m_myProcessor(processor)
    {
    }
    Map(const Map &from) = delete;
    Map &operator=(const Map &from) = delete;
    ~Map()                          = default;

    void   set_size(size_t entity_count);
    size_t size() const { return m_map.empty() ? 0 : m_map.size() - 1; }

    void set_is_sequential(bool yesno) { m_map[0] = yesno ? -1 : 1; }

    // Determines whether the input map is sequential (m_map[i] == i)
    bool is_sequential(bool check_all = false) const;

    int64_t global_to_local(int64_t global, bool must_exist = true) const;

    template <typename INT>
    bool set_map(INT *ids, size_t count, size_t offset, bool in_define_mode = true);

    void set_default(size_t count, size_t offset = 0);

    void build_reverse_map();
    void build_reverse_map_no_lock();
    void build_reverse_map(int64_t num_to_get, int64_t offset);

    void release_memory(); //! Release memory for all maps.

    void reverse_map_data(void *data, const Ioss::Field &field, size_t count) const;
    void map_data(void *data, const Ioss::Field &field, size_t count) const;
    void map_implicit_data(void *data, const Ioss::Field &field, size_t count, size_t offset) const;

    template <typename T>
    size_t map_field_to_db_scalar_order(T *variables, std::vector<double> &db_var,
                                        size_t begin_offset, size_t count, size_t stride,
                                        size_t offset);

    const MapContainer &map() const { return m_map; }
    MapContainer       &map() { return m_map; }

    bool defined() const { return m_defined; }
    void set_defined(bool yes_no) { m_defined = yes_no; }

    bool reorders() const { return !m_reorder.empty(); }

  private:
    template <typename INT> void reverse_map_data(INT *data, size_t count) const;
    template <typename INT> void map_data(INT *data, size_t count) const;
    template <typename INT> void map_implicit_data(INT *data, size_t count, size_t offset) const;

    int64_t global_to_local__(int64_t global, bool must_exist = true) const;
    void    build_reorder_map__(int64_t start, int64_t count);
    void    build_reverse_map__(int64_t num_to_get, int64_t offset);

#if defined(IOSS_THREADSAFE)
    mutable std::mutex m_;
#endif
    MapContainer        m_map{};
    MapContainer        m_reorder{};
    ReverseMapContainer m_reverse{};
    std::string         m_entityType{"unknown"}; // node, element, edge, face
    std::string         m_filename{"undefined"}; // For error messages only.
    int64_t             m_offset{-1};            // local to global offset if m_map is sequential.
    int                 m_myProcessor{0};        // For error messages...
    bool m_defined{false}; // For use by some clients; not all, so don't read too much into value...
  };
} // namespace Ioss

#endif // IOSS_Ioss_Map_h
