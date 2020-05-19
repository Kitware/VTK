/*
 * Copyright(C) 1999-2017, 2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of NTESS nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef IOEX_UTILS_H
#define IOEX_UTILS_H

#include "vtk_ioss_mangle.h"

#include <Ioss_CoordinateFrame.h>
#include <Ioss_ElementBlock.h>
#include <Ioss_ElementTopology.h>
#include <Ioss_Utils.h>

#include <cassert>
#include <vtk_exodusII.h>
#include <set>
#include <string>
#include <vector>

// Contains code that is common between the file-per-processor and
// parallel exodus and base exodus classes.

namespace Ioss {
  class GroupingEntity;
  using CoordinateFrameContainer = std::vector<CoordinateFrame>;
} // namespace Ioss

namespace Ioex {
  using EntityIdSet = std::set<std::pair<int64_t, int64_t>>;
  using SideSetSet  = std::set<std::string>;
  using SideSetMap  = std::map<std::string, const std::string, std::less<const std::string>>;

  struct TopologyMapCompare
  {
    bool operator()(const std::pair<std::string, const Ioss::ElementTopology *> &lhs,
                    const std::pair<std::string, const Ioss::ElementTopology *> &rhs) const
    {
      assert(lhs.second != nullptr);
      assert(rhs.second != nullptr);
      return lhs.first < rhs.first ||
             (!(rhs.first < lhs.first) && lhs.second->name() < rhs.second->name());
    }
  };

  using TopologyMap =
      std::map<std::pair<std::string, const Ioss::ElementTopology *>, int, TopologyMapCompare>;

  const char *Version();
  bool        check_processor_info(int exodusFilePtr, int processor_count, int processor_id);

  Ioss::EntityType map_exodus_type(ex_entity_type type);
  ex_entity_type   map_exodus_type(Ioss::EntityType type);

  void update_last_time_attribute(int exodusFilePtr, double value);
  bool read_last_time_attribute(int exodusFilePtr, double *value);

  bool    type_match(const std::string &type, const char *substring);
  int64_t extract_id(const std::string &name_id);
  bool    set_id(const Ioss::GroupingEntity *entity, ex_entity_type type, Ioex::EntityIdSet *idset);
  int64_t get_id(const Ioss::GroupingEntity *entity, ex_entity_type type, Ioex::EntityIdSet *idset);
  void    decode_surface_name(Ioex::SideSetMap &fs_map, Ioex::SideSetSet &fs_set,
                              const std::string &name);
  void    fix_bad_name(char *name);

  void exodus_error(int exoid, int lineno, const char *function, const char *filename,
                    const std::string &extra = {});

  int add_map_fields(int exoid, Ioss::ElementBlock *block, int64_t my_element_count,
                     size_t name_length);

  void add_coordinate_frames(int exoid, Ioss::Region *region);
  void write_coordinate_frames(int exoid, const Ioss::CoordinateFrameContainer &frames);

  bool find_displacement_field(Ioss::NameList &fields, const Ioss::GroupingEntity *block, int ndim,
                               std::string *disp_name);

  std::string get_entity_name(int exoid, ex_entity_type type, int64_t id,
                              const std::string &basename, int length, bool &db_has_name);

  void filter_element_list(Ioss::Region *region, Ioss::Int64Vector &elements,
                           Ioss::Int64Vector &sides, bool remove_omitted_elements);

  bool filter_node_list(Ioss::Int64Vector &               nodes,
                        const std::vector<unsigned char> &node_connectivity_status);

  template <typename T>
  void filter_node_list(T *data, std::vector<T> &dbvals,
                        const std::vector<int64_t> &active_node_index)
  {
    for (size_t i = 0; i < active_node_index.size(); i++) {
      data[i] = dbvals[active_node_index[i]];
    }
  }

  void filter_element_list(Ioss::Region *region, Ioss::Int64Vector &elements,
                           Ioss::Int64Vector &sides, bool remove_omitted_elements);

  void separate_surface_element_sides(Ioss::Int64Vector &element, Ioss::Int64Vector &sides,
                                      Ioss::Region *region, Ioex::TopologyMap &topo_map,
                                      Ioex::TopologyMap &    side_map,
                                      Ioss::SurfaceSplitType split_type,
                                      const std::string &    surface_name);

  void                       write_reduction_attributes(int exoid, const Ioss::GroupingEntity *ge);
  template <typename T> void write_reduction_attributes(int exoid, const std::vector<T *> &entities)
  {
    // For the entity, write all "reduction attributes"
    for (const auto &ge : entities) {
      write_reduction_attributes(exoid, ge);
    }
  }
} // namespace Ioex
#endif
