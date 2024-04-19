// Copyright(C) 1999-2020, 2022 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#pragma once

#include "ioss_export.h"

#include "vtk_ioss_mangle.h"

#include "Ioss_Super.h"
#include <Ioss_CodeTypes.h>          // for IntVector
#include <Ioss_ElementPermutation.h> // for ElementPermutation
#include <string>                    // for string
namespace Ioss {
  class ElementVariableType;
} // namespace Ioss

// STL Includes

namespace Ioss {

  class IOSS_EXPORT Super : public Ioss::ElementTopology
  {

  public:
    static const char *name;

    static void factory();
    ~Super() override;
    Super(const std::string &my_name, int node_count);

    static void make_super(const std::string &type);

    ElementShape shape() const override { return ElementShape::UNKNOWN; }
    int          spatial_dimension() const override;
    int          parametric_dimension() const override;
    bool         is_element() const override { return true; }
    bool         is_shell() const override { return false; }
    int          order() const override;

    int number_corner_nodes() const override;
    int number_nodes() const override;
    int number_edges() const override;
    int number_faces() const override;

    int number_nodes_edge(int edge = 0) const override;
    int number_nodes_face(int face = 0) const override;
    int number_edges_face(int face = 0) const override;

    Ioss::IntVector edge_connectivity(int edge_number) const override;
    Ioss::IntVector face_connectivity(int face_number) const override;
    Ioss::IntVector element_connectivity() const override;

    Ioss::IntVector face_edge_connectivity(int face_number) const override;

    Ioss::ElementTopology *face_type(int face_number = 0) const override;
    Ioss::ElementTopology *edge_type(int edge_number = 0) const override;

    const std::string &base_topology_permutation_name() const override { return baseTopologyName; }

  protected:
  private:
    int                        nodeCount;
    Ioss::ElementVariableType *storageType{};
    std::string                baseTopologyName{};
    Super(const Super &) = delete;
  };
} // namespace Ioss
