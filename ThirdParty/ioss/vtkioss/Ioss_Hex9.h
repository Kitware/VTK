// Copyright(C) 1999-2022 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#pragma once

#include <Ioss_CodeTypes.h>       // for IntVector
#include <Ioss_ElementTopology.h> // for ElementTopology

// STL Includes

namespace Ioss {
  class Hex9 : public Ioss::ElementTopology
  {

  public:
    static const char *name;

    static void factory();
    ~Hex9() override = default;

    ElementShape shape() const override { return ElementShape::HEX; }
    int          spatial_dimension() const override;
    int          parametric_dimension() const override;
    bool         is_element() const override { return true; }
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

  protected:
    Hex9();

  private:
    static Hex9 instance_;

    Hex9(const Hex9 &) = delete;
  };
} // namespace Ioss
