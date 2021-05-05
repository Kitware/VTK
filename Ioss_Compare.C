// Copyright(C) 1999-2021 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include <Ioss_DataPool.h>
#include <Ioss_MeshCopyOptions.h>
#include <Ioss_SubSystem.h>

#include <fmt/chrono.h>
#include <fmt/format.h>
#include <fmt/ostream.h>

/* These messages indicate a structural difference between the files
 * being compared.  Use Ioss::WARNING().
 */
#define COUNT_MISMATCH "{} count mismatch ({} vs. {})"
#define NOTFOUND_1 "{} ({}) not found in input #1"
#define NOTFOUND_2 "{} ({}) not found in input #2"

/* These messages indicate a value difference between the files
 * being compared.  Use Ioss::OUTPUT().
 */
#define VALUE_MISMATCH "{} value mismatch ({} vs. {})"

/* These messages indicate a value difference between the files
 * being compared.  Use Ioss::OUTPUT().
 */
#define ATTRIBUTE_FIELD_VALUE_MISMATCH "FIELD data ({}): ATTRIBUTE FIELD data mismatch"
#define COMMUNICATION_FIELD_VALUE_MISMATCH "FIELD data ({}): COMMUNICATION FIELD data mismatch"
#define MESH_FIELD_VALUE_MISMATCH "FIELD data ({}): MESH FIELD data mismatch"
#define TRANSIENT_FIELD_VALUE_MISMATCH "FIELD data ({}): TRANSIENT FIELD data mismatch"
#define TRANSIENT_FIELD_STEP_VALUE_MISMATCH                                                        \
  "FIELD data ({} / step {}): TRANSIENT FIELD data mismatch"

// For compare_database...
namespace {
  bool compare_properties(const Ioss::GroupingEntity *ige_1, const Ioss::GroupingEntity *ige_2);
  bool compare_qa_info(const Ioss::Region &input_region_1, const Ioss::Region &input_region_2);
  bool compare_nodeblock(const Ioss::Region &input_region_1, const Ioss::Region &input_region_2,
                         const Ioss::MeshCopyOptions &options);
  bool compare_elementblocks(const Ioss::Region &input_region_1, const Ioss::Region &input_region_2,
                             const Ioss::MeshCopyOptions &options);
  bool compare_edgeblocks(const Ioss::Region &input_region_1, const Ioss::Region &input_region_2,
                          const Ioss::MeshCopyOptions &options);
  bool compare_faceblocks(const Ioss::Region &input_region_1, const Ioss::Region &input_region_2,
                          const Ioss::MeshCopyOptions &options);
  bool compare_structuredblocks(const Ioss::Region &         input_region_1,
                                const Ioss::Region &         input_region_2,
                                const Ioss::MeshCopyOptions &options);
  bool compare_nodesets(const Ioss::Region &input_region_1, const Ioss::Region &input_region_2,
                        const Ioss::MeshCopyOptions &options);
  bool compare_edgesets(const Ioss::Region &input_region_1, const Ioss::Region &input_region_2,
                        const Ioss::MeshCopyOptions &options);
  bool compare_facesets(const Ioss::Region &input_region_1, const Ioss::Region &input_region_2,
                        const Ioss::MeshCopyOptions &options);
  bool compare_elemsets(const Ioss::Region &input_region_1, const Ioss::Region &input_region_2,
                        const Ioss::MeshCopyOptions &options);
  bool compare_sidesets(const Ioss::Region &input_region_1, const Ioss::Region &input_region_2,
                        const Ioss::MeshCopyOptions &options);
  bool compare_commsets(const Ioss::Region &input_region_1, const Ioss::Region &input_region_2,
                        const Ioss::MeshCopyOptions &options);
  bool compare_coordinate_frames(const Ioss::Region &         input_region_1,
                                 const Ioss::Region &         input_region_2,
                                 const Ioss::MeshCopyOptions &options);
  template <typename T>
  bool compare_fields(const std::vector<T *> &in_entities_1, const std::vector<T *> &in_entities_2,
                      const Ioss::Field::RoleType role);

  bool compare_fields(const Ioss::GroupingEntity *ige_1, const Ioss::GroupingEntity *ige_2,
                      const Ioss::Field::RoleType role);
  template <typename T>
  bool compare_field_data(const std::vector<T *> &in_entities_1,
                          const std::vector<T *> &in_entities_2, DataPool &pool,
                          const Ioss::Field::RoleType role, const Ioss::MeshCopyOptions &options);
  bool compare_field_data(const Ioss::GroupingEntity *ige_1, const Ioss::GroupingEntity *ige_2,
                          DataPool &pool, const Ioss::Field::RoleType role,
                          const Ioss::MeshCopyOptions &options, const std::string &prefix = "");
  bool compare_field_data_internal(const Ioss::GroupingEntity *ige_1,
                                   const Ioss::GroupingEntity *ige_2, DataPool &in_pool,
                                   const std::string &          field_name,
                                   const Ioss::MeshCopyOptions &options);
} // namespace

bool Ioss::Compare::compare_database(Ioss::Region &input_region_1, Ioss::Region &input_region_2,
                                     const Ioss::MeshCopyOptions &options)
{
  bool     overall_result = true;
  bool     rc;
  DataPool data_pool;

  // COMPARE all properties of input database...
  if (compare_properties(&input_region_1, &input_region_2) == false) {
    fmt::print(Ioss::OUTPUT(), "PROPERTIES mismatch ({})\n", input_region_1.name());
    overall_result = false;
  }

  if (!options.ignore_qa_info) {
    if (compare_qa_info(input_region_1, input_region_2) == false) {
      fmt::print(Ioss::OUTPUT(), "\nQA INFO mismatch\n");
      overall_result = false;
    }
  }

  if (compare_nodeblock(input_region_1, input_region_2, options) == false) {
    fmt::print(Ioss::OUTPUT(), "\nNODEBLOCK mismatch\n");
    overall_result = false;
  }

  if (compare_edgeblocks(input_region_1, input_region_2, options) == false) {
    fmt::print(Ioss::OUTPUT(), "\nEDGEBLOCK mismatch\n");
    overall_result = false;
  }

  if (compare_faceblocks(input_region_1, input_region_2, options) == false) {
    fmt::print(Ioss::OUTPUT(), "\nFACEBLOCK mismatch\n");
    overall_result = false;
  }

  if (compare_elementblocks(input_region_1, input_region_2, options) == false) {
    fmt::print(Ioss::OUTPUT(), "\nELEMENTBLOCK mismatch\n");
    overall_result = false;
  }

  if (compare_structuredblocks(input_region_1, input_region_2, options) == false) {
    fmt::print(Ioss::OUTPUT(), "\nSTRUCTUREDBLOCK mismatch\n");
    overall_result = false;
  }

  if (compare_nodesets(input_region_1, input_region_2, options) == false) {
    fmt::print(Ioss::OUTPUT(), "\nNODESET mismatch\n");
    overall_result = false;
  }

  if (compare_edgesets(input_region_1, input_region_2, options) == false) {
    fmt::print(Ioss::OUTPUT(), "\nEDGESET mismatch\n");
    overall_result = false;
  }

  if (compare_facesets(input_region_1, input_region_2, options) == false) {
    fmt::print(Ioss::OUTPUT(), "\nFACESET mismatch\n");
    overall_result = false;
  }

  if (compare_elemsets(input_region_1, input_region_2, options) == false) {
    fmt::print(Ioss::OUTPUT(), "\nELEMSET mismatch\n");
    overall_result = false;
  }

  if (compare_sidesets(input_region_1, input_region_2, options) == false) {
    fmt::print(Ioss::OUTPUT(), "\nSIDESET mismatch\n");
    overall_result = false;
  }

  if (compare_commsets(input_region_1, input_region_2, options) == false) {
    fmt::print(Ioss::OUTPUT(), "\nCOMMSET mismatch\n");
    overall_result = false;
  }

  if (compare_coordinate_frames(input_region_1, input_region_2, options) == false) {
    fmt::print(Ioss::OUTPUT(), "\nCOORDINATE FRAME mismatch\n");
    overall_result = false;
  }

  bool node_major = input_region_2.node_major();

  if (!node_major) {
    rc =
        compare_field_data(input_region_1.get_element_blocks(), input_region_2.get_element_blocks(),
                           data_pool, Ioss::Field::MESH, options);
    if (rc == false) {
      fmt::print(Ioss::OUTPUT(), MESH_FIELD_VALUE_MISMATCH " (node_major = {})", "element blocks",
                 node_major);
      overall_result = false;
    }

    rc =
        compare_field_data(input_region_1.get_element_blocks(), input_region_2.get_element_blocks(),
                           data_pool, Ioss::Field::ATTRIBUTE, options);
    if (rc == false) {
      fmt::print(Ioss::OUTPUT(), ATTRIBUTE_FIELD_VALUE_MISMATCH " (node_major = {})",
                 "element blocks", node_major);
      overall_result = false;
    }
  }

  if (input_region_1.mesh_type() != Ioss::MeshType::STRUCTURED) {
    assert(input_region_2.mesh_type() != Ioss::MeshType::STRUCTURED);

    rc = compare_field_data(input_region_1.get_node_blocks(), input_region_2.get_node_blocks(),
                            data_pool, Ioss::Field::MESH, options);
    if (rc == false) {
      fmt::print(Ioss::OUTPUT(), MESH_FIELD_VALUE_MISMATCH, "node blocks");
      overall_result = false;
    }

    rc = compare_field_data(input_region_1.get_node_blocks(), input_region_2.get_node_blocks(),
                            data_pool, Ioss::Field::ATTRIBUTE, options);
    if (rc == false) {
      fmt::print(Ioss::OUTPUT(), ATTRIBUTE_FIELD_VALUE_MISMATCH, "node blocks");
      overall_result = false;
    }
  }
  if (node_major) {
    rc =
        compare_field_data(input_region_1.get_element_blocks(), input_region_2.get_element_blocks(),
                           data_pool, Ioss::Field::MESH, options);
    if (rc == false) {
      fmt::print(Ioss::OUTPUT(), MESH_FIELD_VALUE_MISMATCH " (node_major = {})", "element blocks",
                 node_major);
      overall_result = false;
    }

    rc =
        compare_field_data(input_region_1.get_element_blocks(), input_region_2.get_element_blocks(),
                           data_pool, Ioss::Field::ATTRIBUTE, options);
    if (rc == false) {
      fmt::print(Ioss::OUTPUT(), ATTRIBUTE_FIELD_VALUE_MISMATCH " (node_major = {})",
                 "element blocks", node_major);
      overall_result = false;
    }
  }

  rc = compare_field_data(input_region_1.get_structured_blocks(),
                          input_region_2.get_structured_blocks(), data_pool, Ioss::Field::MESH,
                          options);
  if (rc == false) {
    fmt::print(Ioss::OUTPUT(), MESH_FIELD_VALUE_MISMATCH, "structured blocks");
    overall_result = false;
  }

  rc = compare_field_data(input_region_1.get_structured_blocks(),
                          input_region_2.get_structured_blocks(), data_pool, Ioss::Field::ATTRIBUTE,
                          options);
  if (rc == false) {
    fmt::print(Ioss::OUTPUT(), ATTRIBUTE_FIELD_VALUE_MISMATCH, "structured blocks");
    overall_result = false;
  }

  rc = compare_field_data(input_region_1.get_edge_blocks(), input_region_2.get_edge_blocks(),
                          data_pool, Ioss::Field::MESH, options);
  if (rc == false) {
    fmt::print(Ioss::OUTPUT(), MESH_FIELD_VALUE_MISMATCH, "edge blocks");
    overall_result = false;
  }

  rc = compare_field_data(input_region_1.get_edge_blocks(), input_region_2.get_edge_blocks(),
                          data_pool, Ioss::Field::ATTRIBUTE, options);
  if (rc == false) {
    fmt::print(Ioss::OUTPUT(), ATTRIBUTE_FIELD_VALUE_MISMATCH, "edge blocks");
    overall_result = false;
  }

  rc = compare_field_data(input_region_1.get_face_blocks(), input_region_2.get_face_blocks(),
                          data_pool, Ioss::Field::MESH, options);
  if (rc == false) {
    fmt::print(Ioss::OUTPUT(), MESH_FIELD_VALUE_MISMATCH, "face blocks");
    overall_result = false;
  }

  rc = compare_field_data(input_region_1.get_face_blocks(), input_region_2.get_face_blocks(),
                          data_pool, Ioss::Field::ATTRIBUTE, options);
  if (rc == false) {
    fmt::print(Ioss::OUTPUT(), ATTRIBUTE_FIELD_VALUE_MISMATCH, "face blocks");
    overall_result = false;
  }

  rc = compare_field_data(input_region_1.get_elementsets(), input_region_2.get_elementsets(),
                          data_pool, Ioss::Field::MESH, options);
  if (rc == false) {
    fmt::print(Ioss::OUTPUT(), MESH_FIELD_VALUE_MISMATCH, "element sets");
    overall_result = false;
  }

  rc = compare_field_data(input_region_1.get_elementsets(), input_region_2.get_elementsets(),
                          data_pool, Ioss::Field::ATTRIBUTE, options);
  if (rc == false) {
    fmt::print(Ioss::OUTPUT(), ATTRIBUTE_FIELD_VALUE_MISMATCH, "element sets");
    overall_result = false;
  }

  rc = compare_field_data(input_region_1.get_commsets(), input_region_2.get_commsets(), data_pool,
                          Ioss::Field::MESH, options);
  if (rc == false) {
    fmt::print(Ioss::OUTPUT(), MESH_FIELD_VALUE_MISMATCH, "comm sets");
    overall_result = false;
  }

  rc = compare_field_data(input_region_1.get_commsets(), input_region_2.get_commsets(), data_pool,
                          Ioss::Field::ATTRIBUTE, options);
  if (rc == false) {
    fmt::print(Ioss::OUTPUT(), ATTRIBUTE_FIELD_VALUE_MISMATCH, "comm sets");
    overall_result = false;
  }

  rc = compare_field_data(input_region_1.get_commsets(), input_region_2.get_commsets(), data_pool,
                          Ioss::Field::COMMUNICATION, options);
  if (rc == false) {
    fmt::print(Ioss::OUTPUT(), COMMUNICATION_FIELD_VALUE_MISMATCH, "comm sets");
    overall_result = false;
  }

  // Side Sets
  if (input_region_1.mesh_type() == Ioss::MeshType::UNSTRUCTURED) {
    // This should have already been checked.
    assert(input_region_1.mesh_type() == Ioss::MeshType::UNSTRUCTURED);

    const auto &in_fss_1 = input_region_1.get_sidesets();
    const auto &in_fss_2 = input_region_2.get_sidesets();

    // This should have already been checked.
    assert(in_fss_1.size() == in_fss_2.size());

    for (const auto &ifs : in_fss_1) {
      const std::string &name = ifs->name();

      // Find matching output sideset
      typename std::vector<Ioss::SideSet *>::const_iterator it;
      for (it = in_fss_2.begin(); it != in_fss_2.end(); it++) {
        if (name.compare((*it)->name()) == 0) {
          break;
        }
      }

      if (it == in_fss_2.end()) {
        fmt::print(Ioss::WARNING(), NOTFOUND_2, "SIDESET", name);
        continue;
      }

      rc = compare_field_data(ifs, (*it), data_pool, Ioss::Field::MESH, options);
      if (rc == false) {
        fmt::print(Ioss::OUTPUT(), MESH_FIELD_VALUE_MISMATCH, "side sets");
        overall_result = false;
      }

      rc = compare_field_data(ifs, (*it), data_pool, Ioss::Field::ATTRIBUTE, options);
      if (rc == false) {
        fmt::print(Ioss::OUTPUT(), ATTRIBUTE_FIELD_VALUE_MISMATCH, "side sets");
        overall_result = false;
      }

      const auto &in_sbs_1 = ifs->get_side_blocks();
      const auto &in_sbs_2 = (*it)->get_side_blocks();

      // This should have already been checked.
      assert(in_sbs_1.size() == in_sbs_2.size());

      for (const auto &isb : in_sbs_1) {
        const std::string &sbname = isb->name();

        // Find matching output sideblock
        typename std::vector<Ioss::SideBlock *>::const_iterator iter;
        for (iter = in_sbs_2.begin(); iter != in_sbs_2.end(); iter++) {
          if (sbname.compare((*iter)->name()) == 0) {
            break;
          }
        }
        if (iter == in_sbs_2.end()) {
          fmt::print(Ioss::WARNING(), NOTFOUND_2, "SIDEBLOCK", name);
          continue;
        }

        rc = compare_field_data(isb, (*iter), data_pool, Ioss::Field::MESH, options);
        if (rc == false) {
          fmt::print(Ioss::OUTPUT(), MESH_FIELD_VALUE_MISMATCH, "side blocks");
          overall_result = false;
        }

        rc = compare_field_data(isb, (*iter), data_pool, Ioss::Field::ATTRIBUTE, options);
        if (rc == false) {
          fmt::print(Ioss::OUTPUT(), ATTRIBUTE_FIELD_VALUE_MISMATCH, "side blocks");
          overall_result = false;
        }
      }
    }
  }

  // This should have already been checked
  assert(input_region_1.property_exists("state_count") ==
         input_region_2.property_exists("state_count"));

  // This should have already been checked
  assert(input_region_1.get_property("state_count").get_int() ==
         input_region_2.get_property("state_count").get_int());

  if (input_region_1.property_exists("state_count") &&
      input_region_1.get_property("state_count").get_int() > 0) {

    // For each 'TRANSIENT' field in the node blocks and element
    // blocks, transfer to the output node and element blocks.
    rc = compare_fields(&input_region_1, &input_region_2, Ioss::Field::TRANSIENT);
    if (rc == false) {
      fmt::print(Ioss::OUTPUT(), TRANSIENT_FIELD_VALUE_MISMATCH, "region");
      overall_result = false;
    }

    rc = compare_fields(input_region_1.get_node_blocks(), input_region_2.get_node_blocks(),
                        Ioss::Field::TRANSIENT);
    if (rc == false) {
      fmt::print(Ioss::OUTPUT(), TRANSIENT_FIELD_VALUE_MISMATCH, "node blocks");
      overall_result = false;
    }

    rc = compare_fields(input_region_1.get_edge_blocks(), input_region_2.get_edge_blocks(),
                        Ioss::Field::TRANSIENT);
    if (rc == false) {
      fmt::print(Ioss::OUTPUT(), TRANSIENT_FIELD_VALUE_MISMATCH, "edge blocks");
      overall_result = false;
    }

    rc = compare_fields(input_region_1.get_face_blocks(), input_region_2.get_face_blocks(),
                        Ioss::Field::TRANSIENT);
    if (rc == false) {
      fmt::print(Ioss::OUTPUT(), TRANSIENT_FIELD_VALUE_MISMATCH, "face blocks");
      overall_result = false;
    }

    rc = compare_fields(input_region_1.get_element_blocks(), input_region_2.get_element_blocks(),
                        Ioss::Field::TRANSIENT);
    if (rc == false) {
      fmt::print(Ioss::OUTPUT(), TRANSIENT_FIELD_VALUE_MISMATCH, "element blocks");
      overall_result = false;
    }

    rc = compare_fields(input_region_1.get_structured_blocks(),
                        input_region_2.get_structured_blocks(), Ioss::Field::TRANSIENT);
    if (rc == false) {
      fmt::print(Ioss::OUTPUT(), TRANSIENT_FIELD_VALUE_MISMATCH, "structured blocks");
      overall_result = false;
    }

    rc = compare_fields(input_region_1.get_nodesets(), input_region_2.get_nodesets(),
                        Ioss::Field::TRANSIENT);
    if (rc == false) {
      fmt::print(Ioss::OUTPUT(), TRANSIENT_FIELD_VALUE_MISMATCH, "node sets");
      overall_result = false;
    }

    rc = compare_fields(input_region_1.get_edgesets(), input_region_2.get_edgesets(),
                        Ioss::Field::TRANSIENT);
    if (rc == false) {
      fmt::print(Ioss::OUTPUT(), TRANSIENT_FIELD_VALUE_MISMATCH, "edge sets");
      overall_result = false;
    }

    rc = compare_fields(input_region_1.get_facesets(), input_region_2.get_facesets(),
                        Ioss::Field::TRANSIENT);
    if (rc == false) {
      fmt::print(Ioss::OUTPUT(), TRANSIENT_FIELD_VALUE_MISMATCH, "face sets");
      overall_result = false;
    }

    rc = compare_fields(input_region_1.get_elementsets(), input_region_2.get_elementsets(),
                        Ioss::Field::TRANSIENT);
    if (rc == false) {
      fmt::print(Ioss::OUTPUT(), TRANSIENT_FIELD_VALUE_MISMATCH, "element sets");
      overall_result = false;
    }

    // Side Sets
    {
      const auto &in_sss_1 = input_region_1.get_sidesets();
      const auto &in_sss_2 = input_region_2.get_sidesets();
      for (const auto &iss : in_sss_1) {
        const std::string &name = iss->name();

        // Find matching output sideset
        typename std::vector<Ioss::SideSet *>::const_iterator it;
        for (it = in_sss_2.begin(); it != in_sss_2.end(); it++) {
          if (name.compare((*it)->name()) == 0) {
            break;
          }
        }
        if (it == in_sss_2.end()) {
          fmt::print(Ioss::WARNING(), NOTFOUND_2, "SIDESET", name);
          continue;
        }

        {
          rc = compare_fields(iss, (*it), Ioss::Field::TRANSIENT);
          if (rc == false) {
            fmt::print(Ioss::OUTPUT(), TRANSIENT_FIELD_VALUE_MISMATCH, "side sets");
            overall_result = false;
          }

          const auto &in_sbs_1 = iss->get_side_blocks();
          const auto &in_sbs_2 = (*it)->get_side_blocks();
          if (in_sbs_1.size() != in_sbs_2.size()) {
            fmt::print(Ioss::WARNING(), COUNT_MISMATCH, "SIDEBLOCK", in_sbs_1.size(),
                       in_sbs_2.size());
            continue;
          }

          for (const auto &isb : in_sbs_1) {
            // Find matching output sideblock
            const std::string &sbname = isb->name();

            typename std::vector<Ioss::SideBlock *>::const_iterator iter;
            for (iter = in_sbs_2.begin(); iter != in_sbs_2.end(); iter++) {
              if (sbname.compare((*iter)->name()) == 0) {
                break;
              }
            }
            if (iter == in_sbs_2.end()) {
              fmt::print(Ioss::WARNING(), NOTFOUND_2, "SIDEBLOCK", sbname);
              continue;
            }

            rc = compare_fields(isb, (*iter), Ioss::Field::TRANSIENT);
            if (rc == false) {
              fmt::print(Ioss::OUTPUT(), TRANSIENT_FIELD_VALUE_MISMATCH, "side blocks");
              overall_result = false;
            }
          }
        }
      }
    }

    int in_step_count_1 = input_region_1.get_property("state_count").get_int();

    // This should have already been checked
    assert(in_step_count_1 == input_region_2.get_property("state_count").get_int());

    for (int istep = 1; istep <= in_step_count_1; istep++) {
      double in_time_1 = input_region_1.get_state_time(istep);

      // This should have already been checked
      assert(in_time_1 == input_region_2.get_state_time(istep));

      if (in_time_1 < options.minimum_time) {
        continue;
      }
      if (in_time_1 > options.maximum_time) {
        break;
      }

      input_region_1.begin_state(istep);
      input_region_2.begin_state(istep);

      rc = compare_field_data(&input_region_1, &input_region_2, data_pool, Ioss::Field::TRANSIENT,
                              options);
      if (rc == false) {
        fmt::print(Ioss::OUTPUT(), TRANSIENT_FIELD_STEP_VALUE_MISMATCH, "region", istep);
        overall_result = false;
      }

      // This should have already been checked
      assert(input_region_1.mesh_type() == input_region_2.mesh_type());

      if (input_region_1.mesh_type() != Ioss::MeshType::STRUCTURED) {
        rc = compare_field_data(input_region_1.get_node_blocks(), input_region_2.get_node_blocks(),
                                data_pool, Ioss::Field::TRANSIENT, options);
        if (rc == false) {
          fmt::print(Ioss::OUTPUT(), TRANSIENT_FIELD_STEP_VALUE_MISMATCH, "node blocks", istep);
          overall_result = false;
        }
      }

      rc = compare_field_data(input_region_1.get_edge_blocks(), input_region_2.get_edge_blocks(),
                              data_pool, Ioss::Field::TRANSIENT, options);
      if (rc == false) {
        fmt::print(Ioss::OUTPUT(), TRANSIENT_FIELD_STEP_VALUE_MISMATCH, "edge blocks", istep);
        overall_result = false;
      }

      rc = compare_field_data(input_region_1.get_face_blocks(), input_region_2.get_face_blocks(),
                              data_pool, Ioss::Field::TRANSIENT, options);
      if (rc == false) {
        fmt::print(Ioss::OUTPUT(), TRANSIENT_FIELD_STEP_VALUE_MISMATCH, "face blocks", istep);
        overall_result = false;
      }

      rc = compare_field_data(input_region_1.get_element_blocks(),
                              input_region_2.get_element_blocks(), data_pool,
                              Ioss::Field::TRANSIENT, options);
      if (rc == false) {
        fmt::print(Ioss::OUTPUT(), TRANSIENT_FIELD_STEP_VALUE_MISMATCH, "element blocks", istep);
        overall_result = false;
      }

      rc = compare_field_data(input_region_1.get_structured_blocks(),
                              input_region_2.get_structured_blocks(), data_pool,
                              Ioss::Field::TRANSIENT, options);
      if (rc == false) {
        fmt::print(Ioss::OUTPUT(), TRANSIENT_FIELD_STEP_VALUE_MISMATCH, "structured blocks", istep);
        overall_result = false;
      }

      rc = compare_field_data(input_region_1.get_nodesets(), input_region_2.get_nodesets(),
                              data_pool, Ioss::Field::TRANSIENT, options);
      if (rc == false) {
        fmt::print(Ioss::OUTPUT(), TRANSIENT_FIELD_STEP_VALUE_MISMATCH, "node sets", istep);
        overall_result = false;
      }

      rc = compare_field_data(input_region_1.get_edgesets(), input_region_2.get_edgesets(),
                              data_pool, Ioss::Field::TRANSIENT, options);
      if (rc == false) {
        fmt::print(Ioss::OUTPUT(), TRANSIENT_FIELD_STEP_VALUE_MISMATCH, "edge sets", istep);
        overall_result = false;
      }

      rc = compare_field_data(input_region_1.get_facesets(), input_region_2.get_facesets(),
                              data_pool, Ioss::Field::TRANSIENT, options);
      if (rc == false) {
        fmt::print(Ioss::OUTPUT(), TRANSIENT_FIELD_STEP_VALUE_MISMATCH, "face sets", istep);
        overall_result = false;
      }

      rc = compare_field_data(input_region_1.get_elementsets(), input_region_2.get_elementsets(),
                              data_pool, Ioss::Field::TRANSIENT, options);
      if (rc == false) {
        fmt::print(Ioss::OUTPUT(), TRANSIENT_FIELD_STEP_VALUE_MISMATCH, "element sets", istep);
        overall_result = false;
      }

      // Side Sets
      const auto &in_sss_1 = input_region_1.get_sidesets();
      const auto &in_sss_2 = input_region_2.get_sidesets();

      // This should have already been checked
      assert(in_sss_1.size() == in_sss_2.size());

      for (const auto &iss : in_sss_1) {
        const std::string &name = iss->name();

        // Find matching output sideset
        typename std::vector<Ioss::SideSet *>::const_iterator it;
        for (it = in_sss_2.begin(); it != in_sss_2.end(); it++) {
          if (name.compare((*it)->name()) == 0) {
            break;
          }
        }

        if (it == in_sss_2.end()) {
          fmt::print(Ioss::WARNING(), NOTFOUND_2, "SIDESET", name);
          continue;
        }

        {
          rc = compare_field_data(iss, (*it), data_pool, Ioss::Field::TRANSIENT, options);
          if (rc == false) {
            fmt::print(Ioss::OUTPUT(), TRANSIENT_FIELD_VALUE_MISMATCH, "side sets");
            overall_result = false;
          }

          const auto &in_sbs_1 = iss->get_side_blocks();
          const auto &in_sbs_2 = (*it)->get_side_blocks();
          if (in_sbs_1.size() != in_sbs_2.size()) {
            fmt::print(Ioss::WARNING(), COUNT_MISMATCH, "SIDEBLOCK", in_sbs_1.size(),
                       in_sbs_2.size());
            continue;
          }

          for (const auto &isb : in_sbs_1) {
            // Find matching output sideblock
            const std::string &sbname = isb->name();

            typename std::vector<Ioss::SideBlock *>::const_iterator iter;
            for (iter = in_sbs_2.begin(); iter != in_sbs_2.end(); iter++) {
              if (sbname.compare((*iter)->name()) == 0) {
                break;
              }
            }
            if (iter == in_sbs_2.end()) {
              fmt::print(Ioss::WARNING(), NOTFOUND_2, "SIDESET", name);
              continue;
            }

            rc = compare_field_data(isb, (*iter), data_pool, Ioss::Field::TRANSIENT, options);
            if (rc == false) {
              fmt::print(Ioss::OUTPUT(), TRANSIENT_FIELD_VALUE_MISMATCH, "side sets");
              overall_result = false;
            }
          }
        }
      }
    }
  }

  Ioss::Utils::clear(data_pool.data);

  return overall_result;
}

namespace {
  bool compare_properties(const Ioss::GroupingEntity *ige_1, const Ioss::GroupingEntity *ige_2)
  {
    bool overall_result = true;

    Ioss::NameList ige_properties_1;
    ige_1->property_describe(&ige_properties_1);

    Ioss::NameList ige_properties_2;
    ige_2->property_describe(&ige_properties_2);

    for (const auto &property : ige_properties_1) {
      if (!ige_2->property_exists(property)) {
        // BASED on existing code in transfer_properties(), different databases can result
        // in a different set of properties without affecting their equivalence.  As a result,
        // we'll skip properties that they don't have in common.
        continue;
      }

      if (property.compare("database_name") == 0) {
        // IGNORE the database name.  This is generally the filename; we don't care whether
        // the filenames match.
        continue;
      }

      // ALLOW the regions to have different names (when copying between databases, io_shell
      // will create "region_1" (input) and "region_2" (output))
      if (ige_1->type() == Ioss::REGION && property.compare("name") == 0) {
        continue;
      }

      Ioss::Property ige_property_1 = ige_1->get_property(property);
      Ioss::Property ige_property_2 = ige_2->get_property(property);
      if (ige_property_1 != ige_property_2) {
        if (ige_property_1.get_type() == Ioss::Property::STRING) {
          fmt::print(Ioss::OUTPUT(), "PROPERTY value mismatch ({}): ({} vs {})\n", property,
                     ige_property_1.get_string(), ige_property_2.get_string());
        }
        else if (ige_property_1.get_type() == Ioss::Property::INTEGER) {
          fmt::print(Ioss::OUTPUT(), "PROPERTY value mismatch ({}): ({} vs {})\n", property,
                     ige_property_1.get_int(), ige_property_2.get_int());
        }
        else {
          fmt::print(Ioss::OUTPUT(), "PROPERTY value mismatch ({}): unsupported type\n", property);
        }

        overall_result = false;
      }
    }

    return overall_result;
  }

  bool compare_qa_info(const Ioss::Region &input_region_1, const Ioss::Region &input_region_2)
  {
    bool overall_result = true;

    std::vector<std::string> in_information_records_1 = input_region_1.get_information_records();
    std::vector<std::string> in_information_records_2 = input_region_2.get_information_records();

    if (in_information_records_1.size() != in_information_records_2.size()) {
      fmt::print(Ioss::WARNING(), COUNT_MISMATCH, "INFORMATION RECORD",
                 in_information_records_1.size(), in_information_records_2.size());
    }

    for (const auto &information_record : in_information_records_1) {
      auto it = std::find(in_information_records_2.begin(), in_information_records_2.end(),
                          information_record);
      if (it == in_information_records_2.end()) {
        // INFORMATION RECORD was not found
        fmt::print(Ioss::WARNING(), NOTFOUND_2, "INFORMATION RECORD", information_record);
      }
    }

    for (const auto &information_record : in_information_records_2) {
      auto it = std::find(in_information_records_1.begin(), in_information_records_1.end(),
                          information_record);
      if (it == in_information_records_1.end()) {
        // INFORMATION RECORD was not found
        fmt::print(Ioss::WARNING(), NOTFOUND_1, "INFORMATION RECORD", information_record);
      }
    }

    // Each QA record consists of four strings.  For now, require identical ordering
    // (i.e., records in the same order) for equality.
    const std::vector<std::string> &in_qa_1 = input_region_1.get_qa_records();
    const std::vector<std::string> &in_qa_2 = input_region_2.get_qa_records();

    if (in_qa_1.size() != in_qa_2.size()) {
      fmt::print(Ioss::WARNING(), COUNT_MISMATCH, "QA RECORD", in_qa_1.size(), in_qa_2.size());
    }

    // CHECK for missing QA records and COMPARE existing records
    for (const auto &in_qa_record_1 : in_qa_1) {
      auto it = std::find(in_qa_2.begin(), in_qa_2.end(), in_qa_record_1);
      if (it == in_qa_2.end()) {
        // QA RECORD was not found
        fmt::print(Ioss::WARNING(), NOTFOUND_2, "QA RECORD", in_qa_record_1);
        continue;
      }

      if (in_qa_record_1.compare(*it) != 0) {
        fmt::print(Ioss::OUTPUT(), VALUE_MISMATCH, "QA RECORD", in_qa_record_1, (*it));
        overall_result = false;
      }
    }

    for (const auto &in_qa_record_2 : in_qa_2) {
      auto it = std::find(in_qa_1.begin(), in_qa_1.end(), in_qa_record_2);
      if (it == in_qa_1.end()) {
        // QA RECORD was not found
        fmt::print(Ioss::WARNING(), NOTFOUND_1, "QA RECORD", in_qa_record_2);
      }
    }

    return overall_result;
  }

  bool compare_nodeblock(const Ioss::Region &input_region_1, const Ioss::Region &input_region_2,
                         const Ioss::MeshCopyOptions &options)
  {
    bool overall_result = true;

    Ioss::NodeBlockContainer in_nbs_1 = input_region_1.get_node_blocks();
    Ioss::NodeBlockContainer in_nbs_2 = input_region_2.get_node_blocks();

    if (in_nbs_1.size() != in_nbs_2.size()) {
      fmt::print(Ioss::WARNING(), COUNT_MISMATCH, "NODEBLOCK", in_nbs_1.size(), in_nbs_2.size());
      return false;
    }

    for (const auto &inb : in_nbs_1) {
      auto *nb2 = input_region_2.get_node_block(inb->name());
      if (nb2 == nullptr) {
        fmt::print(Ioss::WARNING(), NOTFOUND_2, "NODEBLOCK", inb->name());
        overall_result = false;
      }
      else if (!inb->equal(*nb2)) {
        fmt::print(Ioss::OUTPUT(), "NODEBLOCK {} mismatch", inb->name());
        overall_result = false;
      }
    }

    return overall_result;
  }

  template <typename T>
  bool compare_blocks(const std::vector<T *> &     in_blocks_1,
                      const std::vector<T *> &     in_blocks_const_2,
                      const Ioss::MeshCopyOptions &options)
  {
    bool overall_result = true;

    if (in_blocks_1.size() != in_blocks_const_2.size()) {
      fmt::print(Ioss::WARNING(), COUNT_MISMATCH, "BLOCK", in_blocks_1.size(),
                 in_blocks_const_2.size());
      return false;
    }

    // COPY the const input vector so that we remove elements as they're matched without
    // affecting the original data structure.
    std::vector<T *> in_blocks_2 = in_blocks_const_2;

    if (!in_blocks_1.empty()) {
      for (const auto &in_block_1 : in_blocks_1) {
        typename std::vector<T *>::const_iterator it;
        for (it = in_blocks_2.begin(); it != in_blocks_2.end(); it++) {
          if (*(*it) == *in_block_1) {
            break;
          }
        }
        if (it == in_blocks_2.end()) {
          fmt::print(Ioss::WARNING(), NOTFOUND_2, "BLOCK", in_block_1->name());
          overall_result = false;
        }
        else {
          // Just to be sure, remove the OUTPUT nodeblock from the container so that we don't
          // inadvertently match against it again
          in_blocks_2.erase(it);
        }
      }
    }

    return overall_result;
  }

  bool compare_elementblocks(const Ioss::Region &input_region_1, const Ioss::Region &input_region_2,
                             const Ioss::MeshCopyOptions &options)
  {
    const auto &in_ebs_1 = input_region_1.get_element_blocks();
    const auto &in_ebs_2 = input_region_2.get_element_blocks();
    if (compare_blocks(in_ebs_1, in_ebs_2, options) == false) {
      fmt::print(Ioss::OUTPUT(), "\nELEMENTBLOCKS mismatch\n");
      return false;
    }
    return true;
  }

  bool compare_edgeblocks(const Ioss::Region &input_region_1, const Ioss::Region &input_region_2,
                          const Ioss::MeshCopyOptions &options)
  {
    const auto &in_ebs_1 = input_region_1.get_edge_blocks();
    const auto &in_ebs_2 = input_region_2.get_edge_blocks();
    if (compare_blocks(in_ebs_1, in_ebs_2, options) == false) {
      fmt::print(Ioss::OUTPUT(), "\nEDGEBLOCKS mismatch\n");
      return false;
    }
    return true;
  }

  bool compare_faceblocks(const Ioss::Region &input_region_1, const Ioss::Region &input_region_2,
                          const Ioss::MeshCopyOptions &options)
  {
    const auto &in_fbs_1 = input_region_1.get_face_blocks();
    const auto &in_fbs_2 = input_region_2.get_face_blocks();
    if (compare_blocks(in_fbs_1, in_fbs_2, options) == false) {
      fmt::print(Ioss::OUTPUT(), "\nFACEBLOCKS mismatch\n");
      return false;
    }
    return true;
  }

  bool compare_structuredblocks(const Ioss::Region &         input_region_1,
                                const Ioss::Region &         input_region_2,
                                const Ioss::MeshCopyOptions &options)
  {
    bool overall_result = true;

    auto in_blocks_1      = input_region_1.get_structured_blocks();
    auto in_blocks_orig_2 = input_region_2.get_structured_blocks();

    // COPY the const input vector so that we can remove elements as they're matched without
    // affecting the original data structure.
    std::vector<Ioss::StructuredBlock *> in_blocks_2 = in_blocks_orig_2;

    if (in_blocks_1.size() != in_blocks_2.size()) {
      fmt::print(Ioss::WARNING(), COUNT_MISMATCH, "STRUCTUREDBLOCK", in_blocks_1.size(),
                 in_blocks_2.size());
      return false;
    }

    if (!in_blocks_1.empty()) {
      for (const auto &in_block_1 : in_blocks_1) {
        std::vector<Ioss::StructuredBlock *>::iterator it;
        for (it = in_blocks_2.begin(); it != in_blocks_2.end(); it++) {
          if (*(*it) == *in_block_1) {
            break;
          }
        }
        if (it == in_blocks_2.end()) {
          fmt::print(Ioss::WARNING(), NOTFOUND_2, "", in_block_1->name());
          overall_result = false;
        }
        else {
          // Just to be sure, remove the OUTPUT nodeblock from the container so that we don't
          // inadvertently match against it again
          in_blocks_2.erase(it);
        }
      }
    }

    return overall_result;
  }

  template <typename T>
  bool compare_sets(const std::vector<T *> &in_sets_1, const std::vector<T *> &in_sets_const_2,
                    const Ioss::MeshCopyOptions &options)
  {
    bool overall_result = true;

    if (in_sets_1.size() != in_sets_const_2.size()) {
      fmt::print(Ioss::WARNING(), COUNT_MISMATCH, "set", in_sets_1.size(), in_sets_const_2.size());
      return false;
    }

    // COPY the const input vector so that we remove elements as they're matched without
    // affecting the original data structure.
    std::vector<T *> in_sets_2 = in_sets_const_2;

    if (!in_sets_1.empty()) {
      for (const auto &in_set_1 : in_sets_1) {
        typename std::vector<T *>::const_iterator it;
        for (it = in_sets_2.begin(); it != in_sets_2.end(); it++) {
          if (*(*it) == *in_set_1) {
            break;
          }
        }
        if (it == in_sets_2.end()) {
          fmt::print(Ioss::WARNING(), NOTFOUND_2, "set", in_set_1->name());
          overall_result = false;
        }
        else {
          // Just to be sure, remove the OUTPUT set from the container so that we don't
          // inadvertently match against it again
          in_sets_2.erase(it);
        }
      }
    }

    return overall_result;
  }

  bool compare_nodesets(const Ioss::Region &input_region_1, const Ioss::Region &input_region_2,
                        const Ioss::MeshCopyOptions &options)
  {
    const auto &in_nss_1 = input_region_1.get_nodesets();
    const auto &in_nss_2 = input_region_2.get_nodesets();
    bool        rc       = compare_sets(in_nss_1, in_nss_2, options);
    if (!rc) {
      fmt::print(Ioss::OUTPUT(), "\nNODESET mismatch\n");
    }

    return rc;
  }

  bool compare_edgesets(const Ioss::Region &input_region_1, const Ioss::Region &input_region_2,
                        const Ioss::MeshCopyOptions &options)
  {
    const auto &in_ess_1 = input_region_1.get_edgesets();
    const auto &in_ess_2 = input_region_2.get_edgesets();
    bool        rc       = compare_sets(in_ess_1, in_ess_2, options);
    if (!rc) {
      fmt::print(Ioss::OUTPUT(), "\nEDGESET mismatch\n");
    }

    return rc;
  }

  bool compare_facesets(const Ioss::Region &input_region_1, const Ioss::Region &input_region_2,
                        const Ioss::MeshCopyOptions &options)
  {
    const auto &in_fss_1 = input_region_1.get_facesets();
    const auto &in_fss_2 = input_region_2.get_facesets();
    bool        rc       = compare_sets(in_fss_1, in_fss_2, options);
    if (!rc) {
      fmt::print(Ioss::OUTPUT(), "\nFACESET mismatch\n");
    }

    return rc;
  }

  bool compare_elemsets(const Ioss::Region &input_region_1, const Ioss::Region &input_region_2,
                        const Ioss::MeshCopyOptions &options)
  {
    const auto &in_ess_1 = input_region_1.get_elementsets();
    const auto &in_ess_2 = input_region_2.get_elementsets();
    bool        rc       = compare_sets(in_ess_1, in_ess_2, options);
    if (!rc) {
      fmt::print(Ioss::OUTPUT(), "\nELEMSET mismatch\n");
    }

    return rc;
  }

  bool compare_sidesets(const Ioss::Region &input_region_1, const Ioss::Region &input_region_2,
                        const Ioss::MeshCopyOptions &options)
  {
    const auto &in_sss_1 = input_region_1.get_sidesets();
    const auto &in_sss_2 = input_region_2.get_sidesets();
    bool        rc       = compare_sets(in_sss_1, in_sss_2, options);
    if (!rc) {
      fmt::print(Ioss::OUTPUT(), "\nSIDESET mismatch\n");
    }

    return rc;
  }

  bool compare_commsets(const Ioss::Region &input_region_1, const Ioss::Region &input_region_2,
                        const Ioss::MeshCopyOptions &options)
  {
    bool overall_result = true;

    const auto &in_css_1      = input_region_1.get_commsets();
    const auto &in_css_orig_2 = input_region_2.get_commsets();

    if (in_css_1.size() != in_css_orig_2.size()) {
      fmt::print(Ioss::WARNING(), COUNT_MISMATCH, "COMMSET", in_css_1.size(), in_css_orig_2.size());
      return false;
    }

    // COPY the const input vector so that we remove elements as they're matched without
    // affecting the original data structure.
    std::vector<Ioss::CommSet *> in_css_2 = in_css_orig_2;

    if (!in_css_1.empty()) {
      for (const auto &in_cs_1 : in_css_1) {
        typename std::vector<Ioss::CommSet *>::const_iterator it;
        for (it = in_css_2.begin(); it != in_css_2.end(); it++) {
          if (*(*it) == *in_cs_1) {
            break;
          }
        }
        if (it == in_css_2.end()) {
          fmt::print(Ioss::WARNING(), NOTFOUND_2, "COMMSET", in_cs_1->name());
          overall_result = false;
        }
        else {
          // Just to be sure, remove the OUTPUT set from the container so that we don't
          // inadvertently match against it again
          in_css_2.erase(it);
        }
      }
    }

    return overall_result;
  }

  bool compare_coordinate_frames(const Ioss::Region &         input_region_1,
                                 const Ioss::Region &         input_region_2,
                                 const Ioss::MeshCopyOptions &options)
  {
    bool overall_result = true;

    const auto &in_cfs_1      = input_region_1.get_coordinate_frames();
    const auto &in_cfs_orig_2 = input_region_2.get_coordinate_frames();

    if (in_cfs_1.size() != in_cfs_orig_2.size()) {
      fmt::print(Ioss::WARNING(), COUNT_MISMATCH, "COORDINATE FRAME", in_cfs_1.size(),
                 in_cfs_orig_2.size());
      return false;
    }

    // COPY the const input vector so that we remove elements as they're matched without
    // affecting the original data structure.
    std::vector<Ioss::CoordinateFrame> in_cfs_2 = in_cfs_orig_2;

    if (!in_cfs_1.empty()) {
      for (const auto &in_cf_1 : in_cfs_1) {
        typename std::vector<Ioss::CoordinateFrame>::const_iterator it;
        for (it = in_cfs_2.begin(); it != in_cfs_2.end(); it++) {
          if ((*it) == in_cf_1) {
            break;
          }
        }
        if (it == in_cfs_2.end()) {
          fmt::print(Ioss::WARNING(), NOTFOUND_2, "COORDINATE FRAME", in_cf_1.id());
          overall_result = false;
        }
        else {
          // Just to be sure, remove the OUTPUT set from the container so that we don't
          // inadvertently match against it again
          in_cfs_2.erase(it);
        }
      }
    }

    return overall_result;
  }

  template <typename T>
  bool compare_fields(const std::vector<T *> &in_entities_1, const std::vector<T *> &in_entities_2,
                      const Ioss::Field::RoleType role)
  {
    bool overall_result = true;

    if (in_entities_1.size() != in_entities_2.size()) {
      fmt::print(Ioss::WARNING(), COUNT_MISMATCH, "ENTITY", in_entities_1.size(),
                 in_entities_2.size());
      return false;
    }

    for (const auto &in_entity_1 : in_entities_1) {
      const std::string &name = in_entity_1->name();

      typename std::vector<T *>::const_iterator it;
      for (it = in_entities_2.begin(); it != in_entities_2.end(); it++) {
        if (name.compare((*it)->name()) == 0) {
          break;
        }
      }
      if (it == in_entities_2.end()) {
        fmt::print(Ioss::WARNING(), NOTFOUND_2, "ENTITY", name);
        overall_result = false;
        continue;
      }

      overall_result &= compare_fields(in_entity_1, (*it), role);
    }

    return overall_result;
  }

  bool compare_fields(const Ioss::GroupingEntity *ige_1, const Ioss::GroupingEntity *ige_2,
                      const Ioss::Field::RoleType role)
  {
    // Check for transient fields...
    Ioss::NameList in_fields_1;
    ige_1->field_describe(role, &in_fields_1);

    Ioss::NameList in_fields_2;
    ige_2->field_describe(role, &in_fields_2);

    if (in_fields_1.size() != in_fields_2.size()) {
      fmt::print(Ioss::WARNING(), COUNT_MISMATCH, "FIELD", in_fields_1.size(), in_fields_2.size());
      return false;
    }

    // Iterate through results fields and transfer to output
    // database...  If a prefix is specified, only transfer fields
    // whose names begin with the prefix
    for (const auto &field_name : in_fields_1) {
      Ioss::Field ige_field_1 = ige_1->get_field(field_name);
      Ioss::Field ige_field_2 = ige_2->get_field(field_name);
      if (ige_field_1 != ige_field_2) {
        fmt::print(Ioss::OUTPUT(), "FIELD ({}) mismatch\n", field_name);
      }
    }

    return true;
  }

  template <typename T>
  bool compare_field_data(const std::vector<T *> &in_entities_1,
                          const std::vector<T *> &in_entities_2, DataPool &pool,
                          const Ioss::Field::RoleType role, const Ioss::MeshCopyOptions &options)
  {
    bool overall_result = true;

    if (in_entities_1.size() != in_entities_2.size()) {
      fmt::print(Ioss::WARNING(), COUNT_MISMATCH, "ENTITY", in_entities_1.size(),
                 in_entities_2.size());
      return false;
    }

    for (const auto &in_entity_1 : in_entities_1) {
      const std::string &name = in_entity_1->name();

      typename std::vector<T *>::const_iterator it;
      for (it = in_entities_2.begin(); it != in_entities_2.end(); it++) {
        if (name.compare((*it)->name()) == 0) {
          break;
        }
      }
      if (it == in_entities_2.end()) {
        fmt::print(Ioss::WARNING(), NOTFOUND_2, "ENTITY", name);
        overall_result = false;
        continue;
      }

      overall_result &= compare_field_data(in_entity_1, (*it), pool, role, options);
    }

    return overall_result;
  }

  bool compare_field_data(const Ioss::GroupingEntity *ige_1, const Ioss::GroupingEntity *ige_2,
                          DataPool &pool, const Ioss::Field::RoleType role,
                          const Ioss::MeshCopyOptions &options, const std::string &prefix)
  {
    bool overall_result = true;

    // Iterate through the TRANSIENT-role fields of the input
    // database and transfer to output database.
    Ioss::NameList in_state_fields_1;
    Ioss::NameList in_state_fields_2;

    ige_1->field_describe(role, &in_state_fields_1);
    ige_2->field_describe(role, &in_state_fields_2);

    // Complication here is that if the 'role' is 'Ioss::Field::MESH',
    // then the 'ids' field must be transferred first...
    if (ige_1->field_exists("ids") != ige_2->field_exists("ids")) {
      fmt::print(Ioss::OUTPUT(),
                 "FIELD data: field MISMATCH --> "
                 "ige_1->field_exists(\"ids\") = {} / ige_2->field_exists(\"ids\") = {}\n",
                 ige_1->field_exists("ids"), ige_2->field_exists("ids"));
      return false;
    }

    if (role == Ioss::Field::MESH && ige_1->field_exists("ids")) {
      assert(ige_2->field_exists("ids"));
      overall_result &= compare_field_data_internal(ige_1, ige_2, pool, "ids", options);
    }

    for (const auto &field_name : in_state_fields_1) {
      // All of the 'Ioss::EntityBlock' derived classes have a
      // 'connectivity' field, but it is only interesting on the
      // Ioss::ElementBlock class. On the other classes, it just
      // generates overhead...
      if (field_name == "connectivity" && ige_1->type() != Ioss::ELEMENTBLOCK) {
        assert(ige_2->type() != Ioss::ELEMENTBLOCK);
        continue;
      }
      if (field_name == "ids") {
        continue;
      }
      if (Ioss::Utils::substr_equal(prefix, field_name)) {
        assert(ige_2->field_exists(field_name));
        overall_result &= compare_field_data_internal(ige_1, ige_2, pool, field_name, options);
      }
    }

    return overall_result;
  }

  bool compare_field_data_internal(const Ioss::GroupingEntity *ige_1,
                                   const Ioss::GroupingEntity *ige_2, DataPool &in_pool,
                                   const std::string &          field_name,
                                   const Ioss::MeshCopyOptions &options)
  {
    size_t isize = ige_1->get_field(field_name).get_size();
    size_t osize = ige_2->get_field(field_name).get_size();

    DataPool in_pool_2;

    if (isize != osize) {
      fmt::print(Ioss::WARNING(), "FIELD size mismatch ({} vs. {})\n", isize, osize);
    }

    if (field_name == "mesh_model_coordinates_x") {
      return true;
    }
    if (field_name == "mesh_model_coordinates_y") {
      return true;
    }
    if (field_name == "mesh_model_coordinates_z") {
      return true;
    }
    if (field_name == "connectivity_raw") {
      return true;
    }
    if (field_name == "element_side_raw") {
      return true;
    }
    if (field_name == "ids_raw") {
      return true;
    }
    if (field_name == "implicit_ids") {
      return true;
    }
    if (field_name == "node_connectivity_status") {
      return true;
    }
    if (field_name == "owning_processor") {
      return true;
    }
    if (field_name == "entity_processor_raw") {
      return true;
    }
    if (field_name == "ids" && ige_1->type() == Ioss::SIDEBLOCK) {
      return true;
    }
    if (field_name == "ids" && ige_1->type() == Ioss::STRUCTUREDBLOCK) {
      return true;
    }
    if (field_name == "cell_ids" && ige_1->type() == Ioss::STRUCTUREDBLOCK) {
      return true;
    }
    if (field_name == "cell_node_ids" && ige_1->type() == Ioss::STRUCTUREDBLOCK) {
      return true;
    }

    if (options.data_storage_type == 1 || options.data_storage_type == 2) {
      if (in_pool.data.size() < isize) {
        in_pool.data.resize(isize);
      }
      if (in_pool_2.data.size() < isize) {
        in_pool_2.data.resize(isize);
      }
    }
    else {
    }

    assert(in_pool.data.size() >= isize);
    assert(in_pool_2.data.size() >= isize);

    switch (options.data_storage_type) {
    case 1:
      ige_1->get_field_data(field_name, in_pool.data.data(), isize);
      ige_2->get_field_data(field_name, in_pool_2.data.data(), isize);
      for (unsigned int i = 0; i < isize; i++) {
        if (in_pool.data[i] != in_pool_2.data[i]) {
          fmt::print(Ioss::OUTPUT(), "FIELD data ({}) mismatch at index[{}]: 0x{} vs. 0x{}",
                     field_name, i, in_pool.data[i], in_pool_2.data[i]);
          return false;
        }
      }
      break;
    default:
      if (field_name == "mesh_model_coordinates") {
        fmt::print(Ioss::WARNING(), "data_storage option not recognized.");
      }
      return false;
    }

    return true;
  }
} // namespace
