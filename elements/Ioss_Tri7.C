// Copyright(C) 1999-2021 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include "Ioss_ElementVariableType.h" // for ElementVariableType
#include "Ioss_Tri7.h"
#include <cassert> // for assert
#include <string>

// ========================================================================
// Define a variable type for storage of this elements connectivity
#include "Ioss_CodeTypes.h"       // for IntVector
#include "Ioss_ElementTopology.h" // for ElementTopology
#include "Ioss_Utils.h"

namespace Ioss {
  const char *Tri7::name = "tri7";
  class St_Tri7 : public ElementVariableType
  {
  public:
    static void factory() { static St_Tri7 registerThis; }

  protected:
    St_Tri7() : ElementVariableType(Ioss::Tri7::name, 7) {}
  };
} // namespace Ioss
//------------------------------------------------------------------------
namespace {
  struct Constants
  {
    static const int nnode     = 7;
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

void Ioss::Tri7::factory()
{
  static Ioss::Tri7 registerThis;
  Ioss::St_Tri7::factory();
}

Ioss::Tri7::Tri7() : Ioss::ElementTopology(Ioss::Tri7::name, "Triangle_7")
{
  Ioss::ElementTopology::alias(Ioss::Tri7::name, "triangle7");
  Ioss::ElementTopology::alias(Ioss::Tri7::name, "Solid_Tri_7_2D");
  Ioss::ElementTopology::alias(Ioss::Tri7::name, "Face_Tri_7_3D");
  Ioss::ElementTopology::alias(Ioss::Tri7::name, "TRIANGLE_7_2D");
  Ioss::ElementTopology::alias(Ioss::Tri7::name, "triface7");
}

int Ioss::Tri7::parametric_dimension() const { return 2; }
int Ioss::Tri7::spatial_dimension() const { return 2; }
int Ioss::Tri7::order() const { return 2; }

int Ioss::Tri7::number_corner_nodes() const { return 3; }
int Ioss::Tri7::number_nodes() const { return Constants::nnode; }
int Ioss::Tri7::number_edges() const { return Constants::nedge; }
int Ioss::Tri7::number_faces() const { return Constants::nface; }

int Ioss::Tri7::number_nodes_edge(int /* edge */) const { return Constants::nedgenode; }
int Ioss::Tri7::number_nodes_face(int /* face */) const { return Constants::nfacenode; }
int Ioss::Tri7::number_edges_face(int /* face */) const { return Constants::nfaceedge; }

Ioss::IntVector Ioss::Tri7::edge_connectivity(int edge_number) const
{
  assert(edge_number > 0 && edge_number <= number_edges());
  Ioss::IntVector connectivity(Constants::nedgenode);

  for (int i = 0; i < Constants::nedgenode; i++) {
    connectivity[i] = Constants::edge_node_order[edge_number - 1][i];
  }

  return connectivity;
}

Ioss::IntVector Ioss::Tri7::face_connectivity(int /* face_number */) const
{
  Ioss::IntVector connectivity;
  return connectivity;
}

Ioss::IntVector Ioss::Tri7::element_connectivity() const
{
  Ioss::IntVector connectivity(number_nodes());

  for (int i = 0; i < number_nodes(); i++) {
    connectivity[i] = i;
  }

  return connectivity;
}

Ioss::ElementTopology *Ioss::Tri7::face_type(int /* face_number */) const
{
  return (Ioss::ElementTopology *)nullptr;
}

Ioss::ElementTopology *Ioss::Tri7::edge_type(int edge_number) const
{
  assert(edge_number >= 0 && edge_number <= number_edges());
  IOSS_ASSERT_USED(edge_number);
  return Ioss::ElementTopology::factory("edge3");
}
