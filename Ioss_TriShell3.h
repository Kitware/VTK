/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
#ifndef IOSS_Ioss_TriShell3_h
#define IOSS_Ioss_TriShell3_h

#include "vtk_ioss_mangle.h"

#include <Ioss_CodeTypes.h>       // for IntVector
#include <Ioss_ElementTopology.h> // for ElementTopology

// STL Includes

namespace Ioss {
  class TriShell3 : public Ioss::ElementTopology
  {

  public:
    static const char *name;

    static void factory();
    ~TriShell3() override;

    ElementShape shape() const override { return ElementShape::TRI; }
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
    TriShell3();

  private:
    static TriShell3 instance_;

    TriShell3(const TriShell3 &) = delete;
  };
} // namespace Ioss
#endif // IOSS_Ioss_TriShell3_h
