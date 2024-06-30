// Copyright(C) 1999-2024 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include <assert.h>
#include <cmath>
#include "vtk_fmt.h"
#include VTK_FMT(fmt/ostream.h)
#include <iosfwd>
#include <stdint.h>
#include <stdlib.h>
#include <string>
#include <vector>

#include "Ioss_Assembly.h"
#include "Ioss_CommSet.h"
#include "Ioss_Compare.h"
#include "Ioss_CoordinateFrame.h"
#include "Ioss_DataPool.h"
#include "Ioss_EdgeBlock.h"
#include "Ioss_EdgeSet.h"
#include "Ioss_ElementBlock.h"
#include "Ioss_ElementSet.h"
#include "Ioss_EntityType.h"
#include "Ioss_FaceBlock.h"
#include "Ioss_FaceSet.h"
#include "Ioss_Field.h"
#include "Ioss_GroupingEntity.h"
#include "Ioss_IOFactory.h"
#include "Ioss_MeshCopyOptions.h"
#include "Ioss_MeshType.h"
#include "Ioss_NodeBlock.h"
#include "Ioss_NodeSet.h"
#include "Ioss_Property.h"
#include "Ioss_Region.h"
#include "Ioss_SideBlock.h"
#include "Ioss_SideSet.h"
#include "Ioss_StructuredBlock.h"
#include "Ioss_Utils.h"

/* These messages indicate a structural difference between the files
 * being compared.  Use Ioss::WarnOut().
 */
#define COUNT_MISMATCH "{} count mismatch ({} vs. {})"
#define NOTFOUND_1     "{} '{}' not found in input #1"
#define NOTFOUND_2     "{} '{}' not found in input #2"

/* These messages indicate a value difference between the files
 * being compared.  Use Ioss::OUTPUT().
 */
#define VALUE_MISMATCH "{} value mismatch ({} vs. {})"

/* These messages indicate a value difference between the files
 * being compared.  Use Ioss::OUTPUT().
 */
#define ATTRIBUTE_FIELD_VALUE_MISMATCH      "\n\nATTRIBUTE FIELD data mismatch ({})"
#define COMMUNICATION_FIELD_VALUE_MISMATCH  "\n\nCOMMUNICATION FIELD data mismatch ({})"
#define MESH_FIELD_VALUE_MISMATCH           "\n\nMESH FIELD data mismatch ({})"
#define TRANSIENT_FIELD_VALUE_MISMATCH      "\n\nTRANSIENT FIELD data mismatch ({})"
#define TRANSIENT_FIELD_STEP_VALUE_MISMATCH "\n\nTRANSIENT FIELD data mismatch ({} at step {}):"

// For compare_database...
namespace {
  double rel_tolerance = 0.0;
  double abs_tolerance = 0.0;
  double tol_floor     = 0.0;

  bool compare_properties(const Ioss::GroupingEntity *ige_1, const Ioss::GroupingEntity *ige_2,
                          std::ostringstream &buf);
  bool compare_qa_info(const Ioss::Region &input_region_1, const Ioss::Region &input_region_2,
                       std::ostringstream &buf);
  bool compare_nodeblock(const Ioss::Region &input_region_1, const Ioss::Region &input_region_2);
  bool compare_assemblies(const Ioss::Region &input_region_1, const Ioss::Region &input_region_2);
  bool compare_elementblocks(const Ioss::Region &input_region_1,
                             const Ioss::Region &input_region_2);
  bool compare_edgeblocks(const Ioss::Region &input_region_1, const Ioss::Region &input_region_2);
  bool compare_faceblocks(const Ioss::Region &input_region_1, const Ioss::Region &input_region_2);
  bool compare_structuredblocks(const Ioss::Region &input_region_1,
                                const Ioss::Region &input_region_2);
  bool compare_nodesets(const Ioss::Region &input_region_1, const Ioss::Region &input_region_2);
  bool compare_edgesets(const Ioss::Region &input_region_1, const Ioss::Region &input_region_2);
  bool compare_facesets(const Ioss::Region &input_region_1, const Ioss::Region &input_region_2);
  bool compare_elemsets(const Ioss::Region &input_region_1, const Ioss::Region &input_region_2);
  bool compare_sidesets(const Ioss::Region &input_region_1, const Ioss::Region &input_region_2);
  bool compare_commsets(const Ioss::Region &input_region_1, const Ioss::Region &input_region_2);
  bool compare_coordinate_frames(const Ioss::Region &input_region_1,
                                 const Ioss::Region &input_region_2);
  template <typename T>
  bool compare_fields(const std::vector<T *> &in_entities_1, const std::vector<T *> &in_entities_2,
                      Ioss::Field::RoleType role, std::ostringstream &buf);

  bool compare_fields(const std::vector<Ioss::StructuredBlock *> &in_entities_1,
                      const std::vector<Ioss::StructuredBlock *> &in_entities_2,
                      Ioss::Field::RoleType role, std::ostringstream &buf);

  bool compare_fields(const Ioss::GroupingEntity *ige_1, const Ioss::GroupingEntity *ige_2,
                      Ioss::Field::RoleType role, std::ostringstream &buf);
  template <typename T1, typename T2>
  bool compare_field_data(const std::vector<T1 *> &in_entities_1,
                          const std::vector<T2 *> &in_entities_2, Ioss::DataPool &pool,
                          Ioss::Field::RoleType role, const Ioss::MeshCopyOptions &options,
                          std::ostringstream &buf);
  bool compare_field_data(const std::vector<Ioss::StructuredBlock *> &in_entities_1,
                          const std::vector<Ioss::StructuredBlock *> &in_entities_2,
                          Ioss::DataPool &pool, Ioss::Field::RoleType role,
                          const Ioss::MeshCopyOptions &options, std::ostringstream &buf);
  bool compare_field_data(const Ioss::GroupingEntity *ige_1, const Ioss::GroupingEntity *ige_2,
                          Ioss::DataPool &pool, Ioss::Field::RoleType role,
                          const Ioss::MeshCopyOptions &options, std::ostringstream &buf,
                          const std::string &prefix = "");
  bool compare_field_data_internal(const Ioss::GroupingEntity *ige_1,
                                   const Ioss::GroupingEntity *ige_2, Ioss::DataPool &in_pool,
                                   const std::string           &field_name,
                                   const Ioss::MeshCopyOptions &options, std::ostringstream &buf);
} // namespace

bool Ioss::Compare::compare_database(Ioss::Region &input_region_1, Ioss::Region &input_region_2,
                                     const Ioss::MeshCopyOptions &options)
{
  bool     overall_result = true;
  bool     rc;
  DataPool data_pool;

  rel_tolerance = options.rel_tolerance;
  abs_tolerance = options.abs_tolerance;
  tol_floor     = options.tol_floor;

  // COMPARE all properties of input database...
  {
    std::ostringstream buf;
    fmt::print(buf, "PROPERTIES mismatch ({})\n", input_region_1.name());
    if (!compare_properties(&input_region_1, &input_region_2, buf)) {
      overall_result = false;
      fmt::print(Ioss::OUTPUT(), "{}", buf.str());
    }
  }

  if (!options.ignore_qa_info) {
    std::ostringstream buf;
    fmt::print(buf, "\nQA INFO mismatch\n");
    if (!compare_qa_info(input_region_1, input_region_2, buf)) {
      overall_result = false;
      fmt::print(Ioss::OUTPUT(), "{}", buf.str());
    }
  }

  {
    std::ostringstream buf;
    fmt::print(buf, "\nNODEBLOCK mismatch\n");
    if (!compare_nodeblock(input_region_1, input_region_2)) {
      overall_result = false;
      fmt::print(Ioss::OUTPUT(), "{}", buf.str());
    }
  }

  {
    std::ostringstream buf;
    fmt::print(buf, "\nEDGEBLOCK mismatch\n");
    if (!compare_edgeblocks(input_region_1, input_region_2)) {
      overall_result = false;
      fmt::print(Ioss::OUTPUT(), "{}", buf.str());
    }
  }
  {
    std::ostringstream buf;
    fmt::print(buf, "\nFACEBLOCK mismatch\n");
    if (!compare_faceblocks(input_region_1, input_region_2)) {
      overall_result = false;
      fmt::print(Ioss::OUTPUT(), "{}", buf.str());
    }
  }
  {
    std::ostringstream buf;
    fmt::print(buf, "\nELEMENTBLOCK mismatch\n");
    if (!compare_elementblocks(input_region_1, input_region_2)) {
      overall_result = false;
      fmt::print(Ioss::OUTPUT(), "{}", buf.str());
    }
  }

  {
    std::ostringstream buf;
    fmt::print(buf, "\nSTRUCTUREDBLOCK mismatch\n");
    if (!compare_structuredblocks(input_region_1, input_region_2)) {
      overall_result = false;
      fmt::print(Ioss::OUTPUT(), "{}", buf.str());
    }
  }

  {
    std::ostringstream buf;
    fmt::print(buf, "\nNODESET mismatch\n");
    if (!compare_nodesets(input_region_1, input_region_2)) {
      overall_result = false;
      fmt::print(Ioss::OUTPUT(), "{}", buf.str());
    }
  }

  {
    std::ostringstream buf;
    fmt::print(buf, "\nEDGESET mismatch\n");
    if (!compare_edgesets(input_region_1, input_region_2)) {
      overall_result = false;
      fmt::print(Ioss::OUTPUT(), "{}", buf.str());
    }
  }

  {
    std::ostringstream buf;
    fmt::print(buf, "\nFACESET mismatch\n");
    if (!compare_facesets(input_region_1, input_region_2)) {
      overall_result = false;
      fmt::print(Ioss::OUTPUT(), "{}", buf.str());
    }
  }

  {
    std::ostringstream buf;
    fmt::print(buf, "\nELEMSET mismatch\n");
    if (!compare_elemsets(input_region_1, input_region_2)) {
      overall_result = false;
      fmt::print(Ioss::OUTPUT(), "{}", buf.str());
    }
  }

  {
    std::ostringstream buf;
    fmt::print(buf, "\nSIDESET mismatch\n");
    if (!compare_sidesets(input_region_1, input_region_2)) {
      overall_result = false;
      fmt::print(Ioss::OUTPUT(), "{}", buf.str());
    }
  }

  {
    std::ostringstream buf;
    fmt::print(buf, "\nCOMMSET mismatch\n");
    if (!compare_commsets(input_region_1, input_region_2)) {
      overall_result = false;
      fmt::print(Ioss::OUTPUT(), "{}", buf.str());
    }
  }

  {
    std::ostringstream buf;
    fmt::print(buf, "\nCOORDINATE FRAME mismatch\n");
    if (!compare_coordinate_frames(input_region_1, input_region_2)) {
      overall_result = false;
      fmt::print(Ioss::OUTPUT(), "{}", buf.str());
    }
  }

  {
    std::ostringstream buf;
    fmt::print(buf, "\nASSEMBLY mismatch\n");
    if (!compare_assemblies(input_region_1, input_region_2)) {
      overall_result = false;
      fmt::print(Ioss::OUTPUT(), "{}", buf.str());
    }
  }

  bool node_major = input_region_2.node_major();

  if (!node_major) {
    {
      std::ostringstream buf;
      fmt::print(buf, MESH_FIELD_VALUE_MISMATCH " (node_major = {})", "element blocks", node_major);
      rc = compare_field_data(input_region_1.get_element_blocks(),
                              input_region_2.get_element_blocks(), data_pool, Ioss::Field::MESH,
                              options, buf);
      if (!rc) {
        fmt::print(Ioss::OUTPUT(), "{}", buf.str());
        overall_result = false;
      }
    }

    {
      std::ostringstream buf;
      fmt::print(buf, ATTRIBUTE_FIELD_VALUE_MISMATCH " (node_major = {})", "element blocks",
                 node_major);

      rc = compare_field_data(input_region_1.get_element_blocks(),
                              input_region_2.get_element_blocks(), data_pool,
                              Ioss::Field::ATTRIBUTE, options, buf);
      if (!rc) {
        fmt::print(Ioss::OUTPUT(), "{}", buf.str());
        overall_result = false;
      }
    }
  }

  if (input_region_1.mesh_type() != Ioss::MeshType::STRUCTURED) {
    assert(input_region_2.mesh_type() != Ioss::MeshType::STRUCTURED);
    {
      std::ostringstream buf;
      fmt::print(buf, MESH_FIELD_VALUE_MISMATCH, "node blocks");

      rc = compare_field_data(input_region_1.get_node_blocks(), input_region_2.get_node_blocks(),
                              data_pool, Ioss::Field::MESH, options, buf);
      if (!rc) {
        overall_result = false;
        fmt::print(Ioss::OUTPUT(), "{}", buf.str());
      }
    }
    {
      std::ostringstream buf;
      fmt::print(buf, ATTRIBUTE_FIELD_VALUE_MISMATCH, "node blocks");
      rc = compare_field_data(input_region_1.get_node_blocks(), input_region_2.get_node_blocks(),
                              data_pool, Ioss::Field::ATTRIBUTE, options, buf);
      if (!rc) {
        overall_result = false;
        fmt::print(Ioss::OUTPUT(), "{}", buf.str());
      }
    }
  }
  if (node_major) {
    {
      std::ostringstream buf;
      fmt::print(buf, MESH_FIELD_VALUE_MISMATCH " (node_major = {})", "element blocks", node_major);
      rc = compare_field_data(input_region_1.get_element_blocks(),
                              input_region_2.get_element_blocks(), data_pool, Ioss::Field::MESH,
                              options, buf);
      if (!rc) {
        overall_result = false;
        fmt::print(Ioss::OUTPUT(), "{}", buf.str());
      }
    }
    {
      std::ostringstream buf;
      fmt::print(buf, ATTRIBUTE_FIELD_VALUE_MISMATCH " (node_major = {})", "element blocks",
                 node_major);
      rc = compare_field_data(input_region_1.get_element_blocks(),
                              input_region_2.get_element_blocks(), data_pool,
                              Ioss::Field::ATTRIBUTE, options, buf);
      if (!rc) {
        overall_result = false;
        fmt::print(Ioss::OUTPUT(), "{}", buf.str());
      }
    }
  }

  {
    std::ostringstream buf;
    fmt::print(buf, MESH_FIELD_VALUE_MISMATCH, "structured blocks");
    rc = compare_field_data(input_region_1.get_structured_blocks(),
                            input_region_2.get_structured_blocks(), data_pool, Ioss::Field::MESH,
                            options, buf);
    if (!rc) {
      overall_result = false;
      fmt::print(Ioss::OUTPUT(), "{}", buf.str());
    }
  }

  {
    std::ostringstream buf;
    fmt::print(buf, ATTRIBUTE_FIELD_VALUE_MISMATCH, "structured blocks");
    rc = compare_field_data(input_region_1.get_structured_blocks(),
                            input_region_2.get_structured_blocks(), data_pool,
                            Ioss::Field::ATTRIBUTE, options, buf);
    if (!rc) {
      overall_result = false;
      fmt::print(Ioss::OUTPUT(), "{}", buf.str());
    }
  }

  {
    std::ostringstream buf;
    fmt::print(buf, MESH_FIELD_VALUE_MISMATCH, "edge blocks");
    rc = compare_field_data(input_region_1.get_edge_blocks(), input_region_2.get_edge_blocks(),
                            data_pool, Ioss::Field::MESH, options, buf);
    if (!rc) {
      overall_result = false;
      fmt::print(Ioss::OUTPUT(), "{}", buf.str());
    }
  }

  {
    std::ostringstream buf;
    fmt::print(buf, ATTRIBUTE_FIELD_VALUE_MISMATCH, "edge blocks");
    rc = compare_field_data(input_region_1.get_edge_blocks(), input_region_2.get_edge_blocks(),
                            data_pool, Ioss::Field::ATTRIBUTE, options, buf);
    if (!rc) {
      overall_result = false;
      fmt::print(Ioss::OUTPUT(), "{}", buf.str());
    }
  }

  {
    std::ostringstream buf;
    fmt::print(buf, MESH_FIELD_VALUE_MISMATCH, "face blocks");
    rc = compare_field_data(input_region_1.get_face_blocks(), input_region_2.get_face_blocks(),
                            data_pool, Ioss::Field::MESH, options, buf);
    if (!rc) {
      overall_result = false;
      fmt::print(Ioss::OUTPUT(), "{}", buf.str());
    }
  }

  {
    std::ostringstream buf;
    fmt::print(buf, ATTRIBUTE_FIELD_VALUE_MISMATCH, "face blocks");
    rc = compare_field_data(input_region_1.get_face_blocks(), input_region_2.get_face_blocks(),
                            data_pool, Ioss::Field::ATTRIBUTE, options, buf);
    if (!rc) {
      overall_result = false;
      fmt::print(Ioss::OUTPUT(), "{}", buf.str());
    }
  }

  {
    std::ostringstream buf;
    fmt::print(buf, MESH_FIELD_VALUE_MISMATCH, "element sets");
    rc = compare_field_data(input_region_1.get_elementsets(), input_region_2.get_elementsets(),
                            data_pool, Ioss::Field::MESH, options, buf);
    if (!rc) {
      overall_result = false;
      fmt::print(Ioss::OUTPUT(), "{}", buf.str());
    }
  }

  {
    std::ostringstream buf;
    fmt::print(buf, ATTRIBUTE_FIELD_VALUE_MISMATCH, "element sets");
    rc = compare_field_data(input_region_1.get_elementsets(), input_region_2.get_elementsets(),
                            data_pool, Ioss::Field::ATTRIBUTE, options, buf);
    if (!rc) {
      overall_result = false;
      fmt::print(Ioss::OUTPUT(), "{}", buf.str());
    }
  }

  {
    std::ostringstream buf;
    fmt::print(buf, MESH_FIELD_VALUE_MISMATCH, "comm sets");
    rc = compare_field_data(input_region_1.get_commsets(), input_region_2.get_commsets(), data_pool,
                            Ioss::Field::MESH, options, buf);
    if (!rc) {
      overall_result = false;
      fmt::print(Ioss::OUTPUT(), "{}", buf.str());
    }
  }

  {
    std::ostringstream buf;
    fmt::print(buf, ATTRIBUTE_FIELD_VALUE_MISMATCH, "comm sets");
    rc = compare_field_data(input_region_1.get_commsets(), input_region_2.get_commsets(), data_pool,
                            Ioss::Field::ATTRIBUTE, options, buf);
    if (!rc) {
      overall_result = false;
      fmt::print(Ioss::OUTPUT(), "{}", buf.str());
    }
  }

  {
    std::ostringstream buf;
    fmt::print(buf, COMMUNICATION_FIELD_VALUE_MISMATCH, "comm sets");
    rc = compare_field_data(input_region_1.get_commsets(), input_region_2.get_commsets(), data_pool,
                            Ioss::Field::COMMUNICATION, options, buf);
    if (!rc) {
      overall_result = false;
      fmt::print(Ioss::OUTPUT(), "{}", buf.str());
    }
  }
  // Side Sets
  if (input_region_1.mesh_type() == Ioss::MeshType::UNSTRUCTURED) {
    // This should have already been checked.
    assert(input_region_2.mesh_type() == Ioss::MeshType::UNSTRUCTURED);

    const auto &in_fss_1 = input_region_1.get_sidesets();
    const auto &in_fss_2 = input_region_2.get_sidesets();

    // This should have already been checked.
    assert(in_fss_1.size() == in_fss_2.size());

    for (const auto &ifs : in_fss_1) {
      const std::string &name = ifs->name();

      // Find matching output sideset
      typename std::vector<Ioss::SideSet *>::const_iterator it;
      for (it = in_fss_2.begin(); it != in_fss_2.end(); ++it) {
        if (name == (*it)->name()) {
          break;
        }
      }

      if (it == in_fss_2.end()) {
        fmt::print(Ioss::WarnOut(), NOTFOUND_2, "SIDESET", name);
        continue;
      }

      {
        std::ostringstream buf;
        fmt::print(buf, MESH_FIELD_VALUE_MISMATCH, "side sets");
        rc = compare_field_data(ifs, (*it), data_pool, Ioss::Field::MESH, options, buf);
        if (!rc) {
          overall_result = false;
          fmt::print(Ioss::OUTPUT(), "{}", buf.str());
        }
      }

      {
        std::ostringstream buf;
        fmt::print(buf, ATTRIBUTE_FIELD_VALUE_MISMATCH, "side sets");
        rc = compare_field_data(ifs, (*it), data_pool, Ioss::Field::ATTRIBUTE, options, buf);
        if (!rc) {
          overall_result = false;
          fmt::print(Ioss::OUTPUT(), "{}", buf.str());
        }
      }
      const auto &in_sbs_1 = ifs->get_side_blocks();
      const auto &in_sbs_2 = (*it)->get_side_blocks();

      // This should have already been checked.
      assert(in_sbs_1.size() == in_sbs_2.size());

      for (const auto &isb : in_sbs_1) {
        const std::string &sbname = isb->name();

        // Find matching output sideblock
        typename std::vector<Ioss::SideBlock *>::const_iterator iter;
        for (iter = in_sbs_2.begin(); iter != in_sbs_2.end(); ++iter) {
          if (sbname == (*iter)->name()) {
            break;
          }
        }
        if (iter == in_sbs_2.end()) {
          fmt::print(Ioss::WarnOut(), NOTFOUND_2, "SIDEBLOCK", name);
          continue;
        }

        {
          std::ostringstream buf;
          fmt::print(buf, MESH_FIELD_VALUE_MISMATCH, "side blocks");
          rc = compare_field_data(isb, (*iter), data_pool, Ioss::Field::MESH, options, buf);
          if (!rc) {
            overall_result = false;
            fmt::print(Ioss::OUTPUT(), "{}", buf.str());
          }
        }

        {
          std::ostringstream buf;
          fmt::print(buf, ATTRIBUTE_FIELD_VALUE_MISMATCH, "side blocks");
          rc = compare_field_data(isb, (*iter), data_pool, Ioss::Field::ATTRIBUTE, options, buf);
          if (!rc) {
            overall_result = false;
            fmt::print(Ioss::OUTPUT(), "{}", buf.str());
          }
        }
      }
    }
  }

  // This should have already been checked
  assert(input_region_1.property_exists("state_count") ==
         input_region_2.property_exists("state_count"));

  if (input_region_1.property_exists("state_count") &&
      input_region_1.get_property("state_count").get_int() > 0) {

    // For each 'TRANSIENT' field in the node blocks and element
    // blocks, transfer to the output node and element blocks.
    {
      std::ostringstream buf;
      fmt::print(buf, TRANSIENT_FIELD_VALUE_MISMATCH, "region");
      rc = compare_fields(&input_region_1, &input_region_2, Ioss::Field::REDUCTION, buf);
      if (!rc) {
        overall_result = false;
        fmt::print(Ioss::OUTPUT(), "{}", buf.str());
      }
    }
    {
      std::ostringstream buf;
      fmt::print(buf, TRANSIENT_FIELD_VALUE_MISMATCH, "node blocks");
      rc = compare_fields(input_region_1.get_node_blocks(), input_region_2.get_node_blocks(),
                          Ioss::Field::TRANSIENT, buf);
      if (!rc) {
        overall_result = false;
        fmt::print(Ioss::OUTPUT(), "{}", buf.str());
      }
    }
    {
      std::ostringstream buf;
      fmt::print(buf, TRANSIENT_FIELD_VALUE_MISMATCH, "edge blocks");
      rc = compare_fields(input_region_1.get_edge_blocks(), input_region_2.get_edge_blocks(),
                          Ioss::Field::TRANSIENT, buf);
      if (!rc) {
        overall_result = false;
        fmt::print(Ioss::OUTPUT(), "{}", buf.str());
      }
    }

    {
      std::ostringstream buf;
      fmt::print(buf, TRANSIENT_FIELD_VALUE_MISMATCH, "face blocks");
      rc = compare_fields(input_region_1.get_face_blocks(), input_region_2.get_face_blocks(),
                          Ioss::Field::TRANSIENT, buf);
      if (!rc) {
        overall_result = false;
        fmt::print(Ioss::OUTPUT(), "{}", buf.str());
      }
    }

    {
      std::ostringstream buf;
      fmt::print(buf, TRANSIENT_FIELD_VALUE_MISMATCH, "element blocks");
      rc = compare_fields(input_region_1.get_element_blocks(), input_region_2.get_element_blocks(),
                          Ioss::Field::TRANSIENT, buf);
      if (!rc) {
        overall_result = false;
        fmt::print(Ioss::OUTPUT(), "{}", buf.str());
      }
    }

    {
      std::ostringstream buf;
      fmt::print(buf, TRANSIENT_FIELD_VALUE_MISMATCH, "structured blocks");
      rc = compare_fields(input_region_1.get_structured_blocks(),
                          input_region_2.get_structured_blocks(), Ioss::Field::TRANSIENT, buf);
      if (!rc) {
        overall_result = false;
        fmt::print(Ioss::OUTPUT(), "{}", buf.str());
      }
    }

    {
      std::ostringstream buf;
      fmt::print(buf, TRANSIENT_FIELD_VALUE_MISMATCH, "node sets");
      rc = compare_fields(input_region_1.get_nodesets(), input_region_2.get_nodesets(),
                          Ioss::Field::TRANSIENT, buf);
      if (!rc) {
        overall_result = false;
        fmt::print(Ioss::OUTPUT(), "{}", buf.str());
      }
    }

    {
      std::ostringstream buf;
      fmt::print(buf, TRANSIENT_FIELD_VALUE_MISMATCH, "edge sets");
      rc = compare_fields(input_region_1.get_edgesets(), input_region_2.get_edgesets(),
                          Ioss::Field::TRANSIENT, buf);
      if (!rc) {
        overall_result = false;
        fmt::print(Ioss::OUTPUT(), "{}", buf.str());
      }
    }

    {
      std::ostringstream buf;
      fmt::print(buf, TRANSIENT_FIELD_VALUE_MISMATCH, "face sets");
      rc = compare_fields(input_region_1.get_facesets(), input_region_2.get_facesets(),
                          Ioss::Field::TRANSIENT, buf);
      if (!rc) {
        overall_result = false;
        fmt::print(Ioss::OUTPUT(), "{}", buf.str());
      }
    }

    {
      std::ostringstream buf;
      fmt::print(buf, TRANSIENT_FIELD_VALUE_MISMATCH, "element sets");
      rc = compare_fields(input_region_1.get_elementsets(), input_region_2.get_elementsets(),
                          Ioss::Field::TRANSIENT, buf);
      if (!rc) {
        overall_result = false;
        fmt::print(Ioss::OUTPUT(), "{}", buf.str());
      }
    }
    // Side Sets
    {
      const auto &in_sss_1 = input_region_1.get_sidesets();
      const auto &in_sss_2 = input_region_2.get_sidesets();
      for (const auto &iss : in_sss_1) {
        const std::string &name = iss->name();

        // Find matching output sideset
        typename std::vector<Ioss::SideSet *>::const_iterator it;
        for (it = in_sss_2.begin(); it != in_sss_2.end(); ++it) {
          if (name == (*it)->name()) {
            break;
          }
        }
        if (it == in_sss_2.end()) {
          // fmt::print(Ioss::WarnOut(), NOTFOUND_2, "SIDESET", name);
          continue;
        }

        {
          {
            std::ostringstream buf;
            fmt::print(buf, TRANSIENT_FIELD_VALUE_MISMATCH, "side sets");
            rc = compare_fields(iss, (*it), Ioss::Field::TRANSIENT, buf);
            if (!rc) {
              overall_result = false;
              fmt::print(Ioss::OUTPUT(), "{}", buf.str());
            }
          }
          const auto &in_sbs_1 = iss->get_side_blocks();
          const auto &in_sbs_2 = (*it)->get_side_blocks();
          if (in_sbs_1.size() != in_sbs_2.size()) {
            fmt::print(Ioss::WarnOut(), COUNT_MISMATCH, "SIDEBLOCK", in_sbs_1.size(),
                       in_sbs_2.size());
            continue;
          }

          for (const auto &isb : in_sbs_1) {
            // Find matching output sideblock
            const std::string &sbname = isb->name();

            typename std::vector<Ioss::SideBlock *>::const_iterator iter;
            for (iter = in_sbs_2.begin(); iter != in_sbs_2.end(); ++iter) {
              if (sbname == (*iter)->name()) {
                break;
              }
            }
            if (iter == in_sbs_2.end()) {
              // fmt::print(Ioss::WarnOut(), NOTFOUND_2, "SIDEBLOCK", sbname);
              continue;
            }

            {
              std::ostringstream buf;
              fmt::print(buf, TRANSIENT_FIELD_VALUE_MISMATCH, "side blocks");
              rc = compare_fields(isb, (*iter), Ioss::Field::TRANSIENT, buf);
              if (!rc) {
                overall_result = false;
                fmt::print(Ioss::OUTPUT(), "{}", buf.str());
              }
            }
          }
        }
      }
    }

    int in_step_count_1 = input_region_1.get_property("state_count").get_int();
    int in_step_count_2 = input_region_2.get_property("state_count").get_int();

    for (int istep = 1; istep <= in_step_count_1; istep++) {
      double in_time_1 = input_region_1.get_state_time(istep);

      if (in_time_1 < options.minimum_time) {
        continue;
      }
      if (in_time_1 > options.maximum_time) {
        break;
      }

      if (istep > in_step_count_2) {
        break;
      }

      input_region_1.begin_state(istep);
      input_region_2.begin_state(istep);

      {
        std::ostringstream buf;
        fmt::print(buf, TRANSIENT_FIELD_STEP_VALUE_MISMATCH, "region", istep);
        rc = compare_field_data(&input_region_1, &input_region_2, data_pool, Ioss::Field::REDUCTION,
                                options, buf);
        if (!rc) {
          overall_result = false;
          fmt::print(Ioss::OUTPUT(), "{}", buf.str());
        }
      }
      // This should have already been checked
      assert(input_region_1.mesh_type() == input_region_2.mesh_type());

      if (input_region_1.mesh_type() != Ioss::MeshType::STRUCTURED) {
        {
          std::ostringstream buf;
          fmt::print(buf, TRANSIENT_FIELD_STEP_VALUE_MISMATCH, "node blocks", istep);
          rc =
              compare_field_data(input_region_1.get_node_blocks(), input_region_2.get_node_blocks(),
                                 data_pool, Ioss::Field::TRANSIENT, options, buf);
          if (!rc) {
            overall_result = false;
            fmt::print(Ioss::OUTPUT(), "{}", buf.str());
          }
        }
      }
      {
        std::ostringstream buf;
        fmt::print(buf, TRANSIENT_FIELD_STEP_VALUE_MISMATCH, "edge blocks", istep);
        rc = compare_field_data(input_region_1.get_edge_blocks(), input_region_2.get_edge_blocks(),
                                data_pool, Ioss::Field::TRANSIENT, options, buf);
        if (!rc) {
          overall_result = false;
          fmt::print(Ioss::OUTPUT(), "{}", buf.str());
        }
      }
      {
        std::ostringstream buf;
        fmt::print(buf, TRANSIENT_FIELD_STEP_VALUE_MISMATCH, "face blocks", istep);
        rc = compare_field_data(input_region_1.get_face_blocks(), input_region_2.get_face_blocks(),
                                data_pool, Ioss::Field::TRANSIENT, options, buf);
        if (!rc) {
          overall_result = false;
          fmt::print(Ioss::OUTPUT(), "{}", buf.str());
        }
      }
      {
        std::ostringstream buf;
        fmt::print(buf, TRANSIENT_FIELD_STEP_VALUE_MISMATCH, "element blocks", istep);
        rc = compare_field_data(input_region_1.get_element_blocks(),
                                input_region_2.get_element_blocks(), data_pool,
                                Ioss::Field::TRANSIENT, options, buf);
        if (!rc) {
          overall_result = false;
          fmt::print(Ioss::OUTPUT(), "{}", buf.str());
        }
      }
      {
        std::ostringstream buf;
        fmt::print(buf, TRANSIENT_FIELD_STEP_VALUE_MISMATCH, "structured blocks", istep);
        rc = compare_field_data(input_region_1.get_structured_blocks(),
                                input_region_2.get_structured_blocks(), data_pool,
                                Ioss::Field::TRANSIENT, options, buf);
        if (!rc) {
          overall_result = false;
          fmt::print(Ioss::OUTPUT(), "{}", buf.str());
        }
      }
      {
        std::ostringstream buf;
        fmt::print(buf, TRANSIENT_FIELD_STEP_VALUE_MISMATCH, "node sets", istep);
        rc = compare_field_data(input_region_1.get_nodesets(), input_region_2.get_nodesets(),
                                data_pool, Ioss::Field::TRANSIENT, options, buf);
        if (!rc) {
          overall_result = false;
          fmt::print(Ioss::OUTPUT(), "{}", buf.str());
        }
      }
      {
        std::ostringstream buf;
        fmt::print(buf, TRANSIENT_FIELD_STEP_VALUE_MISMATCH, "edge sets", istep);
        rc = compare_field_data(input_region_1.get_edgesets(), input_region_2.get_edgesets(),
                                data_pool, Ioss::Field::TRANSIENT, options, buf);
        if (!rc) {
          overall_result = false;
          fmt::print(Ioss::OUTPUT(), "{}", buf.str());
        }
      }
      {
        std::ostringstream buf;
        fmt::print(buf, TRANSIENT_FIELD_STEP_VALUE_MISMATCH, "face sets", istep);
        rc = compare_field_data(input_region_1.get_facesets(), input_region_2.get_facesets(),
                                data_pool, Ioss::Field::TRANSIENT, options, buf);
        if (!rc) {
          overall_result = false;
          fmt::print(Ioss::OUTPUT(), "{}", buf.str());
        }
      }
      {
        std::ostringstream buf;
        fmt::print(buf, TRANSIENT_FIELD_STEP_VALUE_MISMATCH, "element sets", istep);
        rc = compare_field_data(input_region_1.get_elementsets(), input_region_2.get_elementsets(),
                                data_pool, Ioss::Field::TRANSIENT, options, buf);
        if (!rc) {
          overall_result = false;
          fmt::print(Ioss::OUTPUT(), "{}", buf.str());
        }
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
        for (it = in_sss_2.begin(); it != in_sss_2.end(); ++it) {
          if (name == (*it)->name()) {
            break;
          }
        }

        if (it == in_sss_2.end()) {
          // fmt::print(Ioss::WarnOut(), NOTFOUND_2, "SIDESET", name);
          continue;
        }

        {
          {
            std::ostringstream buf;
            fmt::print(buf, TRANSIENT_FIELD_VALUE_MISMATCH, "side sets");
            rc = compare_field_data(iss, (*it), data_pool, Ioss::Field::TRANSIENT, options, buf);
            if (!rc) {
              overall_result = false;
              fmt::print(Ioss::OUTPUT(), "{}", buf.str());
            }
          }
          const auto &in_sbs_1 = iss->get_side_blocks();
          const auto &in_sbs_2 = (*it)->get_side_blocks();
          if (in_sbs_1.size() != in_sbs_2.size()) {
            fmt::print(Ioss::WarnOut(), COUNT_MISMATCH, "SIDEBLOCK", in_sbs_1.size(),
                       in_sbs_2.size());
            continue;
          }

          for (const auto &isb : in_sbs_1) {
            // Find matching output sideblock
            const std::string &sbname = isb->name();

            typename std::vector<Ioss::SideBlock *>::const_iterator iter;
            for (iter = in_sbs_2.begin(); iter != in_sbs_2.end(); ++iter) {
              if (sbname == (*iter)->name()) {
                break;
              }
            }
            if (iter == in_sbs_2.end()) {
              // fmt::print(Ioss::WarnOut(), NOTFOUND_2, "SIDESET", name);
              continue;
            }

            {
              std::ostringstream buf;
              fmt::print(buf, TRANSIENT_FIELD_VALUE_MISMATCH, "side sets");
              rc =
                  compare_field_data(isb, (*iter), data_pool, Ioss::Field::TRANSIENT, options, buf);
              if (!rc) {
                overall_result = false;
                fmt::print(Ioss::OUTPUT(), "{}", buf.str());
              }
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
  bool compare_properties(const Ioss::GroupingEntity *ige_1, const Ioss::GroupingEntity *ige_2,
                          std::ostringstream &buf)
  {
    bool overall_result = true;

    Ioss::NameList ige_properties_1 = ige_1->property_describe();

    for (const auto &property : ige_properties_1) {
      if (!ige_2->property_exists(property)) {
        // BASED on existing code in transfer_properties(), different databases can result
        // in a different set of properties without affecting their equivalence.  As a result,
        // we'll skip properties that they don't have in common.
        continue;
      }

      if (property == "database_name") {
        // IGNORE the database name.  This is generally the filename; we don't care whether
        // the filenames match.
        continue;
      }

      // ALLOW the regions to have different names (when copying between databases, io_shell
      // will create "region_1" (input) and "region_2" (output))
      if (ige_1->type() == Ioss::REGION && property == "name") {
        continue;
      }

      const Ioss::Property &ige_property_1 = ige_1->get_property(property);
      const Ioss::Property &ige_property_2 = ige_2->get_property(property);
      if (ige_property_1 != ige_property_2) {
        if (ige_property_1.get_type() == Ioss::Property::STRING) {
          auto p1_value = ige_property_1.get_string();
          auto p2_value = ige_property_2.get_string();
          fmt::print(buf, "\tPROPERTY value mismatch [STRING] ({}): ('{}' vs '{}')\n", property,
                     p1_value, p2_value);
        }
        else if (ige_property_1.get_type() == Ioss::Property::INTEGER) {
          fmt::print(buf, "\tPROPERTY value mismatch [INTEGER] ({}): ({} vs {})\n", property,
                     ige_property_1.get_int(), ige_property_2.get_int());
        }
        else {
          fmt::print(buf, "PROPERTY value mismatch ({}): unsupported type\n", property);
        }

        overall_result = false;
      }
    }

    return overall_result;
  }

  bool compare_qa_info(const Ioss::Region &input_region_1, const Ioss::Region &input_region_2,
                       std::ostringstream &buf)
  {
    bool overall_result = true;

    const Ioss::NameList &in_information_records_1 = input_region_1.get_information_records();
    const Ioss::NameList &in_information_records_2 = input_region_2.get_information_records();

    if (in_information_records_1.size() != in_information_records_2.size()) {
      fmt::print(Ioss::WarnOut(), COUNT_MISMATCH, "INFORMATION RECORD",
                 in_information_records_1.size(), in_information_records_2.size());
    }

    for (const auto &information_record : in_information_records_1) {
      auto it = std::find(in_information_records_2.begin(), in_information_records_2.end(),
                          information_record);
      if (it == in_information_records_2.end()) {
        // INFORMATION RECORD was not found
        fmt::print(Ioss::WarnOut(), NOTFOUND_2, "INFORMATION RECORD", information_record);
      }
    }

    for (const auto &information_record : in_information_records_2) {
      auto it = std::find(in_information_records_1.begin(), in_information_records_1.end(),
                          information_record);
      if (it == in_information_records_1.end()) {
        // INFORMATION RECORD was not found
        fmt::print(Ioss::WarnOut(), NOTFOUND_1, "INFORMATION RECORD", information_record);
      }
    }

    // Each QA record consists of four strings.  For now, require identical ordering
    // (i.e., records in the same order) for equality.
    const Ioss::NameList &in_qa_1 = input_region_1.get_qa_records();
    const Ioss::NameList &in_qa_2 = input_region_2.get_qa_records();

    bool printed = false;
    if (in_qa_1.size() != in_qa_2.size()) {
      fmt::print(Ioss::WarnOut(), COUNT_MISMATCH, "QA RECORD", in_qa_1.size(), in_qa_2.size());
      printed = true;
    }

    // CHECK for missing QA records and COMPARE existing records
    for (const auto &in_qa_record_1 : in_qa_1) {
      auto it = std::find(in_qa_2.begin(), in_qa_2.end(), in_qa_record_1);
      if (it == in_qa_2.end()) {
        // QA RECORD was not found
        fmt::print(Ioss::WarnOut(), NOTFOUND_2, "QA RECORD", in_qa_record_1);
        printed = true;
        continue;
      }

      if (in_qa_record_1 != *it) {
        fmt::print(buf, VALUE_MISMATCH, "QA RECORD", in_qa_record_1, (*it));
        printed        = true;
        overall_result = false;
      }
    }

    for (const auto &in_qa_record_2 : in_qa_2) {
      auto it = std::find(in_qa_1.begin(), in_qa_1.end(), in_qa_record_2);
      if (it == in_qa_1.end()) {
        // QA RECORD was not found
        fmt::print(Ioss::WarnOut(), NOTFOUND_1, "QA RECORD", in_qa_record_2);
        printed = true;
      }
    }

    if (printed) {
      fmt::print(Ioss::WarnOut(false), "\n");
    }
    return overall_result;
  }

  template <typename T>
  bool compare_entities(const std::vector<T *> &in_sets_1, const std::vector<T *> &in_sets_2,
                        const std::string &uc_type, const std::string &type)
  {
    bool overall_result = true;

    if (in_sets_1.size() != in_sets_2.size()) {
      fmt::print(Ioss::WarnOut(), COUNT_MISMATCH, uc_type, in_sets_1.size(), in_sets_2.size());
      return false;
    }

    if (!in_sets_1.empty()) {
      bool name_not_found = false;
      for (const auto &in_set_1 : in_sets_1) {
        const auto &name = in_set_1->name();
        // find a set in `in_sets_2` with the same name.
        // if found, compare for equality...
        bool found = false;
        for (const auto &in_set_2 : in_sets_2) {
          if (in_set_2->name() == name) {
            found = true;
            if (!in_set_1->equal(*in_set_2)) {
              overall_result = false;
            }
            break;
          }
        }
        if (!found) {
          name_not_found = true;
          fmt::print(Ioss::WarnOut(), NOTFOUND_2, type, in_set_1->name());
          overall_result = false;
        }
      }
      if (name_not_found) {
        // If get here, then there is at least one entity in set 1 which is not in set 2.
        // Since set_1.size() == set_2.size(), then there is at least one entity in set 2 which is
        // not in set_1. Find it/them and print out the namns...
        for (const auto &in_set_2 : in_sets_2) {
          const auto &name = in_set_2->name();
          // find a set in `in_sets_1` with the same name.
          bool found = false;
          for (const auto &in_set_1 : in_sets_1) {
            if (in_set_1->name() == name) {
              found = true;
              break;
            }
          }
          if (!found) {
            fmt::print(Ioss::WarnOut(), NOTFOUND_1, type, in_set_2->name());
            overall_result = false;
          }
        }
      }
    }

    return overall_result;
  }

  bool compare_nodeblock(const Ioss::Region &input_region_1, const Ioss::Region &input_region_2)
  {
    const Ioss::NodeBlockContainer &in_nbs_1 = input_region_1.get_node_blocks();
    const Ioss::NodeBlockContainer &in_nbs_2 = input_region_2.get_node_blocks();
    return compare_entities(in_nbs_1, in_nbs_2, "NODEBLOCK", "nodeblock");
  }

  bool compare_elementblocks(const Ioss::Region &input_region_1, const Ioss::Region &input_region_2)
  {
    const auto &in_ebs_1 = input_region_1.get_element_blocks();
    const auto &in_ebs_2 = input_region_2.get_element_blocks();
    return compare_entities(in_ebs_1, in_ebs_2, "ELEMENTBLOCK", "element block");
  }

  bool compare_edgeblocks(const Ioss::Region &input_region_1, const Ioss::Region &input_region_2)
  {
    const auto &in_ebs_1 = input_region_1.get_edge_blocks();
    const auto &in_ebs_2 = input_region_2.get_edge_blocks();
    return compare_entities(in_ebs_1, in_ebs_2, "EDGEBLOCK", "edge block");
  }

  bool compare_faceblocks(const Ioss::Region &input_region_1, const Ioss::Region &input_region_2)
  {
    const auto &in_fbs_1 = input_region_1.get_face_blocks();
    const auto &in_fbs_2 = input_region_2.get_face_blocks();
    return compare_entities(in_fbs_1, in_fbs_2, "FACEBLOCK", "face block");
  }

  bool compare_structuredblocks(const Ioss::Region &input_region_1,
                                const Ioss::Region &input_region_2)
  {
    bool overall_result = true;

    const auto &in_blocks_1 = input_region_1.get_structured_blocks();
    const auto &in_blocks_2 = input_region_2.get_structured_blocks();

    if (in_blocks_1.size() != in_blocks_2.size()) {
      fmt::print(Ioss::WarnOut(), COUNT_MISMATCH, "STRUCTUREDBLOCK", in_blocks_1.size(),
                 in_blocks_2.size());
      return false;
    }

    for (const auto &in_block_1 : in_blocks_1) {
      const auto &name  = in_block_1->name();
      bool        found = false;
      for (const auto &in_block_2 : in_blocks_2) {
        if (in_block_2->name() == name) {
          found = true;
          if (!in_block_1->equal(*in_block_2)) {
            overall_result = false;
          }
          const auto &nb1 = in_block_1->get_node_block();
          const auto &nb2 = in_block_2->get_node_block();
          if (!nb1.equal(nb2)) {
            overall_result = false;
          }
          break;
        }
      }
      if (!found) {
        fmt::print(Ioss::WarnOut(), NOTFOUND_2, "", in_block_1->name());
        overall_result = false;
      }
    }
    return overall_result;
  }

  bool compare_nodesets(const Ioss::Region &input_region_1, const Ioss::Region &input_region_2)
  {
    const auto &in_nss_1 = input_region_1.get_nodesets();
    const auto &in_nss_2 = input_region_2.get_nodesets();
    return compare_entities(in_nss_1, in_nss_2, "NODESET", "nodeset");
  }

  bool compare_edgesets(const Ioss::Region &input_region_1, const Ioss::Region &input_region_2)
  {
    const auto &in_ess_1 = input_region_1.get_edgesets();
    const auto &in_ess_2 = input_region_2.get_edgesets();
    return compare_entities(in_ess_1, in_ess_2, "EDGESET", "edgeset");
  }

  bool compare_facesets(const Ioss::Region &input_region_1, const Ioss::Region &input_region_2)
  {
    const auto &in_fss_1 = input_region_1.get_facesets();
    const auto &in_fss_2 = input_region_2.get_facesets();
    return compare_entities(in_fss_1, in_fss_2, "FACESET", "faceset");
  }

  bool compare_elemsets(const Ioss::Region &input_region_1, const Ioss::Region &input_region_2)
  {
    const auto &in_ess_1 = input_region_1.get_elementsets();
    const auto &in_ess_2 = input_region_2.get_elementsets();
    return compare_entities(in_ess_1, in_ess_2, "ELEMSET", "elemset");
  }

  bool compare_sidesets(const Ioss::Region &input_region_1, const Ioss::Region &input_region_2)
  {
    const auto &in_sss_1 = input_region_1.get_sidesets();
    const auto &in_sss_2 = input_region_2.get_sidesets();
    return compare_entities(in_sss_1, in_sss_2, "SIDESET", "sideset");
  }

  bool compare_commsets(const Ioss::Region &input_region_1, const Ioss::Region &input_region_2)
  {
    const auto &in_css_1 = input_region_1.get_commsets();
    const auto &in_css_2 = input_region_2.get_commsets();
    return compare_entities(in_css_1, in_css_2, "COMMSET", "commset");
  }

  bool compare_assemblies(const Ioss::Region &input_region_1, const Ioss::Region &input_region_2)
  {
    const auto &in_assem_1 = input_region_1.get_assemblies();
    const auto &in_assem_2 = input_region_2.get_assemblies();
    return compare_entities(in_assem_1, in_assem_2, "ASSEMBLY", "assembly");
  }

  bool compare_coordinate_frames(const Ioss::Region &input_region_1,
                                 const Ioss::Region &input_region_2)
  {
    bool overall_result = true;

    const auto &in_cfs_1 = input_region_1.get_coordinate_frames();
    const auto &in_cfs_2 = input_region_2.get_coordinate_frames();

    if (in_cfs_1.size() != in_cfs_2.size()) {
      fmt::print(Ioss::WarnOut(), COUNT_MISMATCH, "COORDINATE FRAME", in_cfs_1.size(),
                 in_cfs_2.size());
      return false;
    }

    for (const auto &in_cf_1 : in_cfs_1) {
      bool found = false;
      for (const auto &in_cf_2 : in_cfs_2) {
        if (in_cf_1.id() == in_cf_2.id()) {
          found = true;
          if (!in_cf_1.equal(in_cf_2)) {
            overall_result = false;
          }
          break;
        }
      }
      if (!found) {
        fmt::print(Ioss::WarnOut(), NOTFOUND_2, "COORDINATE FRAME", in_cf_1.id());
        overall_result = false;
      }
    }
    return overall_result;
  }

  template <typename T>
  bool compare_fields(const std::vector<T *> &in_entities_1, const std::vector<T *> &in_entities_2,
                      Ioss::Field::RoleType role, std::ostringstream &buf)
  {
    bool overall_result = true;

    if (in_entities_1.size() != in_entities_2.size()) {
      std::string type = "ENTITY";
      if (!in_entities_1.empty()) {
        type = in_entities_1[0]->type_string();
      }
      else {
        type = in_entities_2[0]->type_string();
      }
      fmt::print(Ioss::WarnOut(), COUNT_MISMATCH, type, in_entities_1.size(), in_entities_2.size());
      return false;
    }

    for (const auto &in_entity_1 : in_entities_1) {
      const std::string &name = in_entity_1->name();

      typename std::vector<T *>::const_iterator it;
      for (it = in_entities_2.begin(); it != in_entities_2.end(); ++it) {
        if (name == (*it)->name()) {
          break;
        }
      }
      if (it == in_entities_2.end()) {
        fmt::print(Ioss::WarnOut(), NOTFOUND_2, in_entity_1->type_string(), name);
        overall_result = false;
        continue;
      }

      overall_result &= compare_fields(in_entity_1, (*it), role, buf);
    }

    return overall_result;
  }

  bool compare_fields(const std::vector<Ioss::StructuredBlock *> &in_entities_1,
                      const std::vector<Ioss::StructuredBlock *> &in_entities_2,
                      Ioss::Field::RoleType role, std::ostringstream &buf)
  {
    bool overall_result = true;

    if (in_entities_1.size() != in_entities_2.size()) {
      std::string type = "ENTITY";
      if (!in_entities_1.empty()) {
        type = in_entities_1[0]->type_string();
      }
      else {
        type = in_entities_2[0]->type_string();
      }
      fmt::print(Ioss::WarnOut(), COUNT_MISMATCH, type, in_entities_1.size(), in_entities_2.size());
      return false;
    }

    for (const auto &in_entity_1 : in_entities_1) {
      const std::string &name = in_entity_1->name();

      typename std::vector<Ioss::StructuredBlock *>::const_iterator it;
      for (it = in_entities_2.begin(); it != in_entities_2.end(); ++it) {
        if (name == (*it)->name()) {
          break;
        }
      }
      if (it == in_entities_2.end()) {
        fmt::print(Ioss::WarnOut(), NOTFOUND_2, in_entity_1->type_string(), name);
        overall_result = false;
        continue;
      }

      overall_result &= compare_fields(in_entity_1, (*it), role, buf);

      if (role == Ioss::Field::TRANSIENT) {
        const auto &nb1 = in_entity_1->get_node_block();
        const auto &nb2 = (*it)->get_node_block();
        overall_result &= compare_fields(&nb1, &nb2, role, buf);
      }
    }
    return overall_result;
  }

  bool compare_fields(const Ioss::GroupingEntity *ige_1, const Ioss::GroupingEntity *ige_2,
                      Ioss::Field::RoleType role, std::ostringstream &buf)
  {
    // Check for transient fields...
    Ioss::NameList in_fields_1 = ige_1->field_describe(role);
    Ioss::NameList in_fields_2 = ige_2->field_describe(role);

    if (in_fields_1.size() != in_fields_2.size()) {
      fmt::print(Ioss::WarnOut(), COUNT_MISMATCH, "FIELD", in_fields_1.size(), in_fields_2.size());
      return false;
    }

    bool result = true;
    // Iterate through results fields and transfer to output
    // database...  If a prefix is specified, only transfer fields
    // whose names begin with the prefix
    for (const auto &field_name : in_fields_1) {
      const Ioss::Field &ige_field_1 = ige_1->get_field(field_name);
      const Ioss::Field &ige_field_2 = ige_2->get_field(field_name);
      if (!ige_field_1.equal(ige_field_2)) {
        fmt::print(buf, "\n\tFIELD ({}) mismatch", field_name);
        result = false;
      }
    }
    return result;
  }

  template <typename T1, typename T2>
  bool compare_field_data(const std::vector<T1 *> &in_entities_1,
                          const std::vector<T2 *> &in_entities_2, Ioss::DataPool &pool,
                          Ioss::Field::RoleType role, const Ioss::MeshCopyOptions &options,
                          std::ostringstream &buf)
  {
    bool overall_result = true;

    if (in_entities_1.size() != in_entities_2.size()) {
      std::string type = "ENTITY";
      if (!in_entities_1.empty()) {
        type = in_entities_1[0]->type_string();
      }
      else {
        type = in_entities_2[0]->type_string();
      }
      fmt::print(Ioss::WarnOut(), COUNT_MISMATCH, type, in_entities_1.size(), in_entities_2.size());
      return false;
    }

    for (const auto &in_entity_1 : in_entities_1) {
      const auto &name = in_entity_1->name();

      auto it = in_entities_2.begin();
      for (; it != in_entities_2.end(); ++it) {
        if (name == (*it)->name()) {
          break;
        }
      }
      if (it == in_entities_2.end()) {
        //        fmt::print(Ioss::WarnOut(), NOTFOUND_2, in_entity_1->type_string(), name);
        overall_result = false;
        continue;
      }

      overall_result &= compare_field_data(in_entity_1, (*it), pool, role, options, buf);
    }

    return overall_result;
  }

  bool compare_field_data(const std::vector<Ioss::StructuredBlock *> &in_entities_1,
                          const std::vector<Ioss::StructuredBlock *> &in_entities_2,
                          Ioss::DataPool &pool, Ioss::Field::RoleType role,
                          const Ioss::MeshCopyOptions &options, std::ostringstream &buf)
  {
    bool overall_result = true;

    if (in_entities_1.size() != in_entities_2.size()) {
      std::string type = "ENTITY";
      if (!in_entities_1.empty()) {
        type = in_entities_1[0]->type_string();
      }
      else {
        type = in_entities_2[0]->type_string();
      }
      fmt::print(Ioss::WarnOut(), COUNT_MISMATCH, type, in_entities_1.size(), in_entities_2.size());
      return false;
    }

    for (const auto &in_entity_1 : in_entities_1) {
      const std::string &name = in_entity_1->name();

      std::vector<Ioss::StructuredBlock *>::const_iterator it;
      for (it = in_entities_2.begin(); it != in_entities_2.end(); ++it) {
        if (name == (*it)->name()) {
          break;
        }
      }
      if (it == in_entities_2.end()) {
        //        fmt::print(Ioss::WarnOut(), NOTFOUND_2, in_entity_1->type_string(), name);
        overall_result = false;
        continue;
      }

      overall_result &= compare_field_data(in_entity_1, (*it), pool, role, options, buf);

      if (role == Ioss::Field::TRANSIENT) {
        const auto &nb1 = in_entity_1->get_node_block();
        const auto &nb2 = (*it)->get_node_block();
        overall_result &= compare_field_data(&nb1, &nb2, pool, role, options, buf);
      }
    }

    return overall_result;
  }

  bool compare_field_data(const Ioss::GroupingEntity *ige_1, const Ioss::GroupingEntity *ige_2,
                          Ioss::DataPool &pool, Ioss::Field::RoleType role,
                          const Ioss::MeshCopyOptions &options, std::ostringstream &buf,
                          const std::string &prefix)
  {
    bool overall_result = true;

    // Iterate through the `role` fields of the input
    // database and compare to second database.
    Ioss::NameList in_state_fields_1 = ige_1->field_describe(role);

    for (const auto &field_name : in_state_fields_1) {
      // All of the 'Ioss::EntityBlock' derived classes have a
      // 'connectivity' field, but it is only interesting on the
      // Ioss::ElementBlock class. On the other classes, it just
      // generates overhead...
      if (field_name == "connectivity" && ige_1->type() != Ioss::ELEMENTBLOCK) {
        assert(ige_2->type() != Ioss::ELEMENTBLOCK);
        continue;
      }
      if (field_name == "ids" && ige_1->type() == Ioss::COMMSET) {
        assert(ige_2->type() == Ioss::COMMSET);
        continue;
      }
      if (Ioss::Utils::substr_equal(prefix, field_name)) {
        if (ige_2->field_exists(field_name)) {
          overall_result &=
              compare_field_data_internal(ige_1, ige_2, pool, field_name, options, buf);
        }
      }
    }

    return overall_result;
  }

  template <typename T1, typename T2>
  bool compare_field_data(T1 *data1, T2 *data2, size_t count, size_t component_count,
                          const std::string &field_name, const std::string &entity_name,
                          std::ostringstream &buf)
  {
    int  width = Ioss::Utils::number_width(count - 1);
    bool first = true;
    for (size_t i = 0; i < count * component_count; i++) {
      if (data1[i] != data2[i]) {
        auto idx = i / component_count;
        auto cmp = i % component_count;
        if (first) {
          fmt::print(buf,
                     "\n\tFIELD ({}) on {} -- mismatch at [index.component]:\n\t\t[{:{}}.{}]: "
                     "{}\tvs. {}",
                     field_name, entity_name, idx + 1, width, cmp, data1[i], data2[i]);
          first = false;
        }
        else {
          fmt::print(buf, "\n\t\t[{:{}}.{}]: {}\tvs. {}", idx + 1, width, cmp, data1[i], data2[i]);
        }
      }
    }
    return first;
  }

  template <>
  bool compare_field_data(double *data1, double *data2, size_t count, size_t component_count,
                          const std::string &field_name, const std::string &entity_name,
                          std::ostringstream &buf)
  {
    int  width = Ioss::Utils::number_width(count - 1);
    bool first = true;
    for (size_t i = 0; i < count * component_count; i++) {
      if (data1[i] != data2[i]) {
        double abs_data1 = std::abs(data1[i]);
        double abs_data2 = std::abs(data2[i]);
        if (abs_data1 > tol_floor || abs_data2 > tol_floor) {
          auto idx      = i / component_count;
          auto cmp      = i % component_count;
          auto abs_diff = std::abs(data1[i] - data2[i]);
          auto max      = std::max(abs_data1, abs_data2);
          auto rel_diff = max != 0.0 ? std::abs(data1[i] - data2[i]) / max : 0.0;
          if (abs_diff >= abs_tolerance && rel_diff >= rel_tolerance) {
            if (first) {
              fmt::print(buf,
                         "\n\tFIELD ({}) on {} -- mismatch at [index.component]:\n\t\t[{:{}}.{}]: "
                         "{:20.13e}\tvs. {:20.13e}\tabs: {:12.5e},\trel: {:12.5e}",
                         field_name, entity_name, idx + 1, width, cmp, data1[i], data2[i], abs_diff,
                         rel_diff);
              first = false;
            }
            else {
              fmt::print(
                  buf, "\n\t\t[{:{}}.{}]: {:20.13e}\tvs. {:20.13e}\tabs: {:12.5e},\trel: {:12.5e}",
                  idx + 1, width, cmp, data1[i], data2[i], abs_diff, rel_diff);
            }
          }
        }
      }
    }
    return first;
  }

  bool compare_field_data_internal(const Ioss::GroupingEntity *ige_1,
                                   const Ioss::GroupingEntity *ige_2, Ioss::DataPool &in_pool,
                                   const std::string           &field_name,
                                   const Ioss::MeshCopyOptions &options, std::ostringstream &buf)
  {
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

    size_t icount = ige_1->get_field(field_name).raw_count();
    size_t ocount = ige_2->get_field(field_name).raw_count();

    if (icount != ocount) {
      fmt::print(buf, "\n\tFIELD count mismatch for field '{}', ({} vs. {}) on {}", field_name,
                 icount, ocount, ige_1->name());
      return false;
    }

    size_t isize = ige_1->get_field(field_name).get_size();
    size_t osize = ige_2->get_field(field_name).get_size();

    Ioss::DataPool in_pool_2;
    if (options.data_storage_type == 1 || options.data_storage_type == 2) {
      if (in_pool.data.size() < isize) {
        in_pool.data.resize(isize);
      }
      if (in_pool_2.data.size() < osize) {
        in_pool_2.data.resize(osize);
      }
    }

    assert(in_pool.data.size() >= isize);
    assert(in_pool_2.data.size() >= osize);

    switch (options.data_storage_type) {
    case 1: {
      ige_1->get_field_data(field_name, Data(in_pool.data), isize);
      ige_2->get_field_data(field_name, Data(in_pool_2.data), osize);
      const Ioss::Field &field  = ige_1->get_field(field_name);
      const Ioss::Field &field2 = ige_2->get_field(field_name);

      switch (field.get_type()) {
      case Ioss::Field::REAL:
        return compare_field_data(
            (double *)Data(in_pool.data), (double *)Data(in_pool_2.data), field.raw_count(),
            field.get_component_count(Ioss::Field::InOut::OUTPUT), field_name, ige_1->name(), buf);
      case Ioss::Field::INTEGER:
        switch (field2.get_type()) {
        case Ioss::Field::INTEGER:
          return compare_field_data((int *)Data(in_pool.data), (int *)Data(in_pool_2.data),
                                    field.raw_count(),
                                    field.get_component_count(Ioss::Field::InOut::OUTPUT),
                                    field_name, ige_1->name(), buf);
        case Ioss::Field::INT64:
          return compare_field_data((int *)Data(in_pool.data), (int64_t *)Data(in_pool_2.data),
                                    field.raw_count(),
                                    field.get_component_count(Ioss::Field::InOut::OUTPUT),
                                    field_name, ige_1->name(), buf);
        default:
          fmt::print(Ioss::WarnOut(), "Field data_storage type {} not recognized for field {}.",
                     field.type_string(), field_name);
          return false;
        }
      case Ioss::Field::INT64:
        switch (field2.get_type()) {
        case Ioss::Field::INTEGER:
          return compare_field_data((int64_t *)Data(in_pool.data), (int *)Data(in_pool_2.data),
                                    field.raw_count(),
                                    field.get_component_count(Ioss::Field::InOut::OUTPUT),
                                    field_name, ige_1->name(), buf);
        case Ioss::Field::INT64:
          return compare_field_data((int64_t *)Data(in_pool.data), (int64_t *)Data(in_pool_2.data),
                                    field.raw_count(),
                                    field.get_component_count(Ioss::Field::InOut::OUTPUT),
                                    field_name, ige_1->name(), buf);
        default:
          fmt::print(Ioss::WarnOut(), "Field data_storage type {} not recognized for field {}.",
                     field.type_string(), field_name);
          return false;
        }
      default:
        fmt::print(Ioss::WarnOut(), "Field data_storage type {} not recognized for field {}.",
                   field.type_string(), field_name);
        return false;
      }
    }
    default:
      if (field_name == "mesh_model_coordinates") {
        fmt::print(Ioss::WarnOut(), "data_storage option not recognized.");
      }
    }
    return false;
  }
} // namespace
