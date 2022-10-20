// Copyright(C) 1999-2022 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#pragma once

#include "iocgns_export.h"

#include <Ioss_CodeTypes.h>
#include <Ioss_DatabaseIO.h>
#include <Ioss_ElementTopology.h>
#include <Ioss_FaceGenerator.h>
#include <Ioss_Region.h>
#include <Ioss_SideBlock.h>
#include <Ioss_SideSet.h>
#include <Ioss_StructuredBlock.h>
#include <Ioss_Utils.h>
#include <cgns/Iocgns_Defines.h>

#include <vtk_cgns.h> // xxx(kitware)
#include VTK_CGNS(cgnslib.h)

#include <ostream>
#include <string>

// Used in Iocgns_DatabaseIO.C and Iocgns_ParallelDatabase.C
// non-Member function -- can't access m_cgnsFilePtr; make sure cgns_file_ptr is passed in...
#define CGCHECK(funcall)                                                                           \
  if ((funcall) != CG_OK) {                                                                        \
    Iocgns::Utils::cgns_error(cgns_file_ptr, __FILE__, __func__, __LINE__, myProcessor);           \
  }

// Member function -- can access m_cgnsFilePtr
#define CGCHECKM(funcall)                                                                          \
  if ((funcall) != CG_OK) {                                                                        \
    Iocgns::Utils::cgns_error(m_cgnsFilePtr, __FILE__, __func__, __LINE__, myProcessor);           \
  }

#define CGCHECKNP(funcall)                                                                         \
  if ((funcall) != CG_OK) {                                                                        \
    Iocgns::Utils::cgns_error(cgns_file_ptr, __FILE__, __func__, __LINE__, -1);                    \
  }

// Used in Iocgns_Decomposition.C
#define CGCHECK2(funcall)                                                                          \
  if ((funcall) != CG_OK) {                                                                        \
    Iocgns::Utils::cgns_error(filePtr, __FILE__, __func__, __LINE__, m_decomposition.m_processor); \
  }

namespace Iocgns {
  class StructuredZoneData;

  struct IOCGNS_EXPORT ZoneBC
  {
    ZoneBC(std::string bc_name, std::array<cgsize_t, 2> &point_range)
        : name(std::move(bc_name)), range_beg(point_range[0]), range_end(point_range[1])
    {
    }

    std::string name;
    cgsize_t    range_beg;
    cgsize_t    range_end;
  };

  class IOCGNS_EXPORT Utils
  {
  public:
    Utils()  = default;
    ~Utils() = default;

    static std::pair<std::string, int> decompose_name(const std::string &name, bool is_parallel);
    static std::string                 decompose_sb_name(const std::string &name);

    static size_t index(const Ioss::Field &field);

    static void cgns_error(int cgnsid, const char *file, const char *function, int lineno,
                           int processor);

    static void update_db_zone_property(int cgns_file_ptr, const Ioss::Region *region,
                                        int myProcessor, bool is_parallel, bool is_parallel_io);
    static int  get_db_zone(const Ioss::GroupingEntity *entity);
    static void set_field_index(const Ioss::Field &field, size_t index,
                                CGNS_ENUMT(GridLocation_t) location);
    static bool is_cell_field(const Ioss::Field &field);

    template <typename INT>
    static void map_cgns_connectivity(const Ioss::ElementTopology *topo, size_t element_count,
                                      INT *idata)
    {
      // Map from CGNS to IOSS/Exodus/Patran order...
      switch (topo->shape()) {
      case Ioss::ElementShape::HEX:
        switch (topo->number_nodes()) {
        case 8:
        case 20: break;
        case 27: {
          // nodes 1 through 20 are the same...
          //
          // ioss: 21, 22, 23, 24, 25, 26, 27 [zero-based: 20, 21, 22, 23, 24, 25, 26]
          // cgns: 27, 21, 26, 25, 23, 22, 24 [zero-based: 26, 20, 25, 24, 22, 21, 23]
          static std::array<int, 7> hex27_map{26, 20, 25, 24, 22, 21, 23};
          for (size_t i = 0; i < element_count; i++) {
            size_t             con_beg = 27 * i; // start of connectivity for i'th element.
            std::array<int, 7> reorder;
            for (size_t j = 0; j < 7; j++) {
              reorder[j] = idata[con_beg + hex27_map[j]];
            }

            for (size_t j = 0; j < 7; j++) {
              idata[con_beg + 20 + j] = reorder[j];
            }
          }
        }
        }
        break;
      default:
          // do nothing cgns ordering matches ioss/exodus/patran (or not handled yet... todo: add
          // error checking)
          ;
      }
    }

    template <typename INT>
    static void unmap_cgns_connectivity(const Ioss::ElementTopology *topo, size_t element_count,
                                        INT *idata)
    {
      // Map from IOSS/Exodus/Patran to CGNS order...
      switch (topo->shape()) {
      case Ioss::ElementShape::HEX:
        switch (topo->number_nodes()) {
        case 8:
        case 20: break;
        case 27: {
          // nodes 1 through 20 are the same...
          //
          // ioss: 21, 22, 23, 24, 25, 26, 27 [zero-based: 20, 21, 22, 23, 24, 25, 26]
          // cgns: 27, 21, 26, 25, 23, 22, 24 [zero-based: 26, 20, 25, 24, 22, 21, 23]
          static std::array<int, 7> hex27_map{26, 20, 25, 24, 22, 21, 23};
          for (size_t i = 0; i < element_count; i++) {
            size_t             con_beg = 27 * i; // start of connectivity for i'th element.
            std::array<int, 7> reorder;
            for (size_t j = 0; j < 7; j++) {
              reorder[j] = idata[con_beg + 20 + j];
            }

            for (size_t j = 0; j < 7; j++) {
              idata[con_beg + hex27_map[j]] = reorder[j];
            }
          }
        }
        }
        break;
      default:
          // do nothing cgns ordering matches ioss/exodus/patran (or not handled yet... todo: add
          // error checking)
          ;
      }
    }

    template <typename INT>
    static void map_cgns_face_to_ioss(const Ioss::ElementTopology *parent_topo, size_t num_to_get,
                                      INT *idata)
    {
      // The {topo}_map[] arrays map from CGNS face# to IOSS face#.
      // See http://cgns.github.io/CGNS_docs_current/sids/conv.html#unstructgrid
      // NOTE: '0' for first entry is to account for 1-based face numbering.

      switch (parent_topo->shape()) {
      case Ioss::ElementShape::HEX: {
        static int hex_map[] = {0, 5, 1, 2, 3, 4, 6};
        for (size_t i = 0; i < num_to_get; i++) {
          idata[2 * i + 1] = hex_map[idata[2 * i + 1]];
        }
      } break;

      case Ioss::ElementShape::TET: {
        static int tet_map[] = {0, 4, 1, 2, 3};
        for (size_t i = 0; i < num_to_get; i++) {
          idata[2 * i + 1] = tet_map[idata[2 * i + 1]];
        }
      } break;

      case Ioss::ElementShape::PYRAMID: {
        static int pyr_map[] = {0, 5, 1, 2, 3, 4};
        for (size_t i = 0; i < num_to_get; i++) {
          idata[2 * i + 1] = pyr_map[idata[2 * i + 1]];
        }
      } break;

      case Ioss::ElementShape::WEDGE:
#if 0
          static int wed_map[] = {0, 1, 2, 3, 4, 5}; // Same
          // Not needed -- maps 1 to 1
          for (size_t i=0; i < num_to_get; i++) {
            idata[2*i+1] = wed_map[idata[2*i+1]];
          }
#endif
        break;
      default:;
      }
    }

    static void map_ioss_face_to_cgns(const Ioss::ElementTopology *parent_topo, size_t num_to_get,
                                      CGNSIntVector &data)
    {
      // The {topo}_map[] arrays map from CGNS face# to IOSS face#.
      // See http://cgns.github.io/CGNS_docs_current/sids/conv.html#unstructgrid
      // NOTE: '0' for first entry is to account for 1-based face numbering.

      switch (parent_topo->shape()) {
      case Ioss::ElementShape::HEX: {
        static int hex_map[] = {0, 2, 3, 4, 5, 1, 6};
        for (size_t i = 0; i < num_to_get; i++) {
          data[num_to_get * 2 + i] = hex_map[data[num_to_get * 2 + i]];
        }
      } break;

      case Ioss::ElementShape::TET: {
        static int tet_map[] = {0, 2, 3, 4, 1};
        for (size_t i = 0; i < num_to_get; i++) {
          data[num_to_get * 2 + i] = tet_map[data[num_to_get * 2 + i]];
        }
      } break;

      case Ioss::ElementShape::PYRAMID: {
        static int pyr_map[] = {0, 2, 3, 4, 5, 1};
        for (size_t i = 0; i < num_to_get; i++) {
          data[num_to_get * 2 + i] = pyr_map[data[num_to_get * 2 + i]];
        }
      } break;

      case Ioss::ElementShape::WEDGE:
#if 0
          static int wed_map[] = {0, 1, 2, 3, 4, 5}; // Same
          // Not needed -- maps 1 to 1
          for (size_t i=0; i < num_to_get; i++) {
            data[num_to_get * 2 + i] = wed_map[data[num_to_get * 2 + i]];
          }
#endif
        break;
      default:;
      }
    }

    static std::vector<ZoneBC> parse_zonebc_sideblocks(int cgns_file_ptr, int base, int zone,
                                                       int myProcessor);

    static void
    generate_boundary_faces(Ioss::Region                                  *region,
                            std::map<std::string, Ioss::FaceUnorderedSet> &boundary_faces,
                            Ioss::Field::BasicType                         field_type);

    static void write_flow_solution_metadata(int file_ptr, int base_ptr, Ioss::Region *region,
                                             int state, const int *vertex_solution_index,
                                             const int *cell_center_solution_index,
                                             bool       is_parallel_io);
    static int  find_solution_index(int cgns_file_ptr, int base, int zone, int step,
                                    CGNS_ENUMT(GridLocation_t) location);
    static Ioss::MeshType check_mesh_type(int cgns_file_ptr);

    static void output_assembly(int file_ptr, const Ioss::Assembly *assembly, bool is_parallel_io,
                                bool appending = false);
    static void output_assemblies(int file_ptr, const Ioss::Region &region, bool is_parallel_io);

    static void   write_state_meta_data(int file_ptr, const Ioss::Region &region,
                                        bool is_parallel_io);
    static size_t common_write_meta_data(int file_ptr, const Ioss::Region &region,
                                         std::vector<size_t> &zone_offset, bool is_parallel);
    static size_t resolve_nodes(Ioss::Region &region, int my_processor, bool is_parallel);
    static std::vector<std::vector<std::pair<size_t, size_t>>>
    resolve_processor_shared_nodes(Ioss::Region &region, int my_processor);

    static CGNS_ENUMT(ElementType_t) map_topology_to_cgns(const std::string &name);
    static std::string map_cgns_to_topology_type(CGNS_ENUMT(ElementType_t) type);
    static void        add_sidesets(int cgns_file_ptr, Ioss::DatabaseIO *db);
    static void        add_assemblies(int cgns_file_ptr, Ioss::DatabaseIO *db);
    static void add_to_assembly(int cgns_file_ptr, Ioss::Region *region, Ioss::EntityBlock *block,
                                int base, int zone);

    static void add_structured_boundary_conditions(int cgns_file_ptr, Ioss::StructuredBlock *block,
                                                   bool is_parallel_io);
    static void add_structured_boundary_conditions_fpp(int                    cgns_file_ptr,
                                                       Ioss::StructuredBlock *block);
    static void add_structured_boundary_conditions_pio(int                    cgns_file_ptr,
                                                       Ioss::StructuredBlock *block);

    static void finalize_database(int cgns_file_ptr, const std::vector<double> &timesteps,
                                  Ioss::Region *region, int myProcessor, bool is_parallel_io);
    static int  get_step_times(int cgns_file_ptr, std::vector<double> &timesteps,
                               Ioss::Region *region, double timeScaleFactor, int myProcessor);
    static void add_transient_variables(int cgns_file_ptr, const std::vector<double> &timesteps,
                                        Ioss::Region *region, int myProcessor, bool is_parallel_io);

    static void set_line_decomposition(int cgns_file_ptr, const std::string &line_decomposition,
                                       std::vector<Iocgns::StructuredZoneData *> &zones, int rank,
                                       bool verbose);
    static void decompose_model(std::vector<Iocgns::StructuredZoneData *> &zones, int proc_count,
                                int rank, double load_balance_threshold, bool verbose);
    static int  pre_split(std::vector<Iocgns::StructuredZoneData *> &zones, double avg_work,
                          double load_balance, int proc_rank, int proc_count, bool verbose);
    static void assign_zones_to_procs(std::vector<Iocgns::StructuredZoneData *> &zones,
                                      std::vector<size_t> &work_vector, bool verbose);
    static std::string show_config();

    template <typename INT>
    static void generate_block_faces(Ioss::ElementTopology *topo, size_t num_elem,
                                     const std::vector<INT> &connectivity,
                                     Ioss::FaceUnorderedSet &boundary,
                                     const std::vector<INT> &zone_local_zone_global);
  };
} // namespace Iocgns
