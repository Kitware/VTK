// Copyright(C) 1999-2024 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#pragma once

#include "ioss_export.h"
#include "vtk_ioss_mangle.h"

#include "Ioss_CodeTypes.h"       // for IntVector
#include "Ioss_ElementTopology.h" // for ElementTopology

namespace Ioss {
  class IOSS_EXPORT Pyramid18 : public Ioss::ElementTopology
  {

  public:
    static const char *name;

    static void factory();

    IOSS_NODISCARD ElementShape shape() const override { return ElementShape::PYRAMID; }
    IOSS_NODISCARD int          spatial_dimension() const override;
    IOSS_NODISCARD int          parametric_dimension() const override;
    IOSS_NODISCARD bool         is_element() const override { return true; }
    IOSS_NODISCARD bool         is_shell() const override { return false; }
    IOSS_NODISCARD int          order() const override;

    IOSS_NODISCARD int number_corner_nodes() const override;
    IOSS_NODISCARD int number_nodes() const override;
    IOSS_NODISCARD int number_edges() const override;
    IOSS_NODISCARD int number_faces() const override;

    IOSS_NODISCARD int number_nodes_edge(int edge = 0) const override;
    IOSS_NODISCARD int number_nodes_face(int face = 0) const override;
    IOSS_NODISCARD int number_edges_face(int face = 0) const override;

    IOSS_NODISCARD bool faces_similar() const override;

    IOSS_NODISCARD Ioss::IntVector edge_connectivity(int edge_number) const override;
    IOSS_NODISCARD Ioss::IntVector face_connectivity(int face_number) const override;
    IOSS_NODISCARD Ioss::IntVector element_connectivity() const override;
    IOSS_NODISCARD Ioss::IntVector face_edge_connectivity(int face_number) const override;

    IOSS_NODISCARD Ioss::ElementTopology *face_type(int face_number = 0) const override;
    IOSS_NODISCARD Ioss::ElementTopology *edge_type(int edge_number = 0) const override;

  protected:
    Pyramid18();
  };
} // namespace Ioss
