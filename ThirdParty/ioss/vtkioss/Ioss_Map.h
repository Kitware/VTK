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

#ifndef IOSS_Ioss_Map_h
#define IOSS_Ioss_Map_h

#include "vtk_ioss_mangle.h"

#include <Ioss_CodeTypes.h>
#include <cstddef> // for size_t
#include <cstdint> // for int64_t
#include <string>  // for string
#include <utility> // for pair
#include <vector>  // for vector
namespace Ioss {
  class Field;
} // namespace Ioss

namespace Ioss {

  using MapContainer        = std::vector<int64_t>;
  using IdPair              = std::pair<int64_t, int64_t>;
  using ReverseMapContainer = std::vector<IdPair>;

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
    MapContainer &      map() { return m_map; }

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
    void    verify_no_duplicate_ids(std::vector<Ioss::IdPair> &reverse_map);

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
