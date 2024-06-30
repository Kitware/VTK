// Copyright(C) 1999-2021 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include "Ioss_ElementVariableType.h" // for ElementVariableType
#include "Ioss_Quad6.h"
#include <cassert> // for assert
#include <string>

//------------------------------------------------------------------------
// Define a variable type for storage of this elements connectivity
#include "Ioss_CodeTypes.h"       // for IntVector
#include "Ioss_ElementTopology.h" // for ElementTopology

namespace Ioss {
  const char *Quad6::name = "quad6";
  class St_Quad6 : public ElementVariableType
  {
  public:
    static void factory() { static St_Quad6 registerThis; }

  protected:
    St_Quad6() : ElementVariableType(Ioss::Quad6::name, 6) {}
  };
} // namespace Ioss
// ========================================================================

namespace {
  struct Constants
  {
    static const int nnode     = 6;
    static const int nedge     = 4;
    static const int nedgenode = 3;
    static const int nface     = 0;
    static int       edge_node_order[nedge][nedgenode];
  };

  // Edge numbers are zero-based [0..number_edges)
  int Constants::edge_node_order[nedge][nedgenode] = // [edge][edge_node]
      {{0, 1, 4}, {1, 2, -1}, {2, 3, 5}, {3, 0, -1}};
} // namespace

void Ioss::Quad6::factory()
{
  static Ioss::Quad6 registerThis;
  Ioss::St_Quad6::factory();
}

Ioss::Quad6::Quad6() : Ioss::ElementTopology(Ioss::Quad6::name, "Quadrilateral_6")
{
  Ioss::ElementTopology::alias(Ioss::Quad6::name, "Solid_Quad_6_2D");
  Ioss::ElementTopology::alias(Ioss::Quad6::name, "QUADRILATERAL_6_2D");
  Ioss::ElementTopology::alias(Ioss::Quad6::name, "Face_Quad_6_3D");
  Ioss::ElementTopology::alias(Ioss::Quad6::name, "quadface6");
}

int Ioss::Quad6::parametric_dimension() const { return 2; }
int Ioss::Quad6::spatial_dimension() const { return 2; }
int Ioss::Quad6::order() const { return 2; }

int Ioss::Quad6::number_corner_nodes() const { return 4; }
int Ioss::Quad6::number_nodes() const { return Constants::nnode; }
int Ioss::Quad6::number_edges() const { return Constants::nedge; }
int Ioss::Quad6::number_faces() const { return Constants::nface; }

int Ioss::Quad6::number_nodes_edge(int edge) const
{
  if (edge == 0) {
    return -1;
  }
  if (edge == 1 || edge == 3) {
    return 3;
  }
  return 2;
}

int Ioss::Quad6::number_nodes_face(int /* face */) const { return 0; }
int Ioss::Quad6::number_edges_face(int /* face */) const { return 0; }

Ioss::IntVector Ioss::Quad6::edge_connectivity(int edge_number) const
{
  assert(edge_number > 0 && edge_number <= number_edges());
  Ioss::IntVector connectivity(number_nodes_edge(edge_number));
  assert(edge_number > 0 && edge_number <= Constants::nedge);

  for (int i = 0; i < number_nodes_edge(edge_number); i++) {
    connectivity[i] = Constants::edge_node_order[edge_number - 1][i];
  }

  return connectivity;
}

Ioss::IntVector Ioss::Quad6::face_connectivity(int /* face_number */) const
{
  Ioss::IntVector connectivity;
  return connectivity;
}

Ioss::IntVector Ioss::Quad6::element_connectivity() const
{
  Ioss::IntVector connectivity(number_nodes());
  for (int i = 0; i < number_nodes(); i++) {
    connectivity[i] = i;
  }
  return connectivity;
}

Ioss::ElementTopology *Ioss::Quad6::face_type(int /* face_number */) const
{
  return (Ioss::ElementTopology *)nullptr;
}

Ioss::ElementTopology *Ioss::Quad6::edge_type(int edge_number) const
{
  // edge_number == 0 returns topology for all edges if
  // all edges are the same topology; otherwise, returns nullptr
  // edge_number is 1-based.
  assert(edge_number >= 0 && edge_number <= number_edges());

  if (edge_number == 0) {
    return (Ioss::ElementTopology *)nullptr;
  }
  if (edge_number == 1 || edge_number == 3) {
    return Ioss::ElementTopology::factory("edge3");
  }
  return Ioss::ElementTopology::factory("edge2");
}
