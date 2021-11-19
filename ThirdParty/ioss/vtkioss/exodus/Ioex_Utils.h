/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
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

#define EXU_USE_HOPSCOTCH
#if defined EXU_USE_HOPSCOTCH
#include <hopscotch_map.h>
#elif defined EXU_USE_ROBIN
#include <robin_map.h>
#endif

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

  using NameTopoKey = std::pair<std::string, const Ioss::ElementTopology *>;
  struct NameTopoKeyCompare
  {
    bool operator()(const NameTopoKey &lhs, const NameTopoKey &rhs) const
    {
      assert(lhs.second != nullptr);
      assert(rhs.second != nullptr);
      return lhs.first < rhs.first ||
             (!(rhs.first < lhs.first) && lhs.second->name() < rhs.second->name());
    }
  };

  struct NameTopoKeyHash
  {
    size_t operator()(const NameTopoKey &name_topo) const
    {
      return std::hash<std::string>{}(name_topo.first) +
             std::hash<size_t>{}((size_t)name_topo.second);
    }
  };

#if defined EXU_USE_HOPSCOTCH
  using TopologyMap = tsl::hopscotch_map<NameTopoKey, int, NameTopoKeyHash>;
#elif defined EXU_USE_ROBIN
  using TopologyMap = tsl::robin_map<NameTopoKey, int, NameTopoKeyHash>;
#else
  // This is the original method that was used in IOSS prior to using hopscotch or robin map.
  using TopologyMap = std::map<NameTopoKey, int, NameTopoKeyCompare>;
#endif

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

  void exodus_error(int exoid, int lineno, const char *function, const char *filename);
  void exodus_error(int exoid, int lineno, const char *function, const char *filename,
                    const std::string &extra);

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

  bool filter_node_list(Ioss::Int64Vector                &nodes,
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
                                      Ioex::TopologyMap     &side_map,
                                      Ioss::SurfaceSplitType split_type,
                                      const std::string     &surface_name);

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
