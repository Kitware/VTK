// Copyright(C) 1999-2021 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include "Ioss_ElementVariableType.h" // for ElementVariableType
#include "Ioss_Tri6.h"
#include <cassert> // for assert
#include <string>

// ========================================================================
// Define a variable type for storage of this elements connectivity
#include "Ioss_CodeTypes.h"       // for IntVector
#include "Ioss_ElementTopology.h" // for ElementTopology
#include "Ioss_Utils.h"

namespace Ioss {
  const char *Tri6::name = "tri6";
  class St_Tri6 : public ElementVariableType
  {
  public:
    static void factory() { static St_Tri6 registerThis; }

  protected:
    St_Tri6() : ElementVariableType(Ioss::Tri6::name, 6) {}
  };
} // namespace Ioss
//------------------------------------------------------------------------
namespace {
  struct Constants
  {
    static const int nnode     = 6;
    static const int nedge     = 3;
    static const int nedgenode = 3;
    static const int nface     = 0;
    static const int nfacenode = 0;
    static const int nfaceedge = 0;
    static int       edge_node_order[nedge][nedgenode];
  };

  // Edge numbers are zero-based [0..number_edges)
  int Constants::edge_node_order[nedge][nedgenode] = // [edge][edge_node]
      {{0, 1, 3}, {1, 2, 4}, {2, 0, 5}};
} // namespace

void Ioss::Tri6::factory()
{
  static Ioss::Tri6 registerThis;
  Ioss::St_Tri6::factory();
}

Ioss::Tri6::Tri6() : Ioss::ElementTopology(Ioss::Tri6::name, "Triangle_6")
{
  Ioss::ElementTopology::alias(Ioss::Tri6::name, "triangle6");
  Ioss::ElementTopology::alias(Ioss::Tri6::name, "Solid_Tri_6_2D");
  Ioss::ElementTopology::alias(Ioss::Tri6::name, "Face_Tri_6_3D");
  Ioss::ElementTopology::alias(Ioss::Tri6::name, "TRIANGLE_6_2D");
  Ioss::ElementTopology::alias(Ioss::Tri6::name, "triface6");
}

int Ioss::Tri6::parametric_dimension() const { return 2; }
int Ioss::Tri6::spatial_dimension() const { return 2; }
int Ioss::Tri6::order() const { return 2; }

int Ioss::Tri6::number_corner_nodes() const { return 3; }
int Ioss::Tri6::number_nodes() const { return Constants::nnode; }
int Ioss::Tri6::number_edges() const { return Constants::nedge; }
int Ioss::Tri6::number_faces() const { return Constants::nface; }

int Ioss::Tri6::number_nodes_edge(int /* edge */) const { return Constants::nedgenode; }
int Ioss::Tri6::number_nodes_face(int /* face */) const { return Constants::nfacenode; }
int Ioss::Tri6::number_edges_face(int /* face */) const { return Constants::nfaceedge; }

Ioss::IntVector Ioss::Tri6::edge_connectivity(int edge_number) const
{
  assert(edge_number > 0 && edge_number <= number_edges());
  Ioss::IntVector connectivity(Constants::nedgenode);

  for (int i = 0; i < Constants::nedgenode; i++) {
    connectivity[i] = Constants::edge_node_order[edge_number - 1][i];
  }

  return connectivity;
}

Ioss::IntVector Ioss::Tri6::face_connectivity(int /* face_number */) const
{
  Ioss::IntVector connectivity;
  return connectivity;
}

Ioss::IntVector Ioss::Tri6::element_connectivity() const
{
  Ioss::IntVector connectivity(number_nodes());

  for (int i = 0; i < number_nodes(); i++) {
    connectivity[i] = i;
  }

  return connectivity;
}

Ioss::ElementTopology *Ioss::Tri6::face_type(int /* face_number */) const
{
  return (Ioss::ElementTopology *)nullptr;
}

Ioss::ElementTopology *Ioss::Tri6::edge_type(int edge_number) const
{
  assert(edge_number >= 0 && edge_number <= number_edges());
  IOSS_ASSERT_USED(edge_number);
  return Ioss::ElementTopology::factory("edge3");
}
