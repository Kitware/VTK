// Copyright(C) 1999-2021 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include "Ioss_ElementVariableType.h" // for ElementVariableType
#include "Ioss_Tri4a.h"
#include <cassert> // for assert
#include <string>

#include "Ioss_CodeTypes.h"       // for IntVector
#include "Ioss_ElementTopology.h" // for ElementTopology

// ========================================================================
// Define a variable type for storage of this elements connectivity
namespace Ioss {
  const char *Tri4a::name = "tri4a";
  class St_Tri4a : public ElementVariableType
  {
  public:
    static void factory() { static St_Tri4a registerThis; }

  protected:
    St_Tri4a() : ElementVariableType(Ioss::Tri4a::name, 4) {}
  };
} // namespace Ioss
//------------------------------------------------------------------------
namespace {
  struct Constants
  {
    static const int nnode     = 4;
    static const int nedge     = 3;
    static const int nedgenode = 3;
    static const int nface     = 0;
    static const int nfacenode = 0;
    static const int nfaceedge = 0;
    static int       nodes_per_edge[nedge + 1];
    static int       edge_node_order[nedge][nedgenode];
  };
  // Edge numbers are zero-based [0..number_edges)
  int Constants::edge_node_order[nedge][nedgenode] = // [edge][edge_node]
      {{0, 1, 3}, {1, 2, -1}, {2, 0, -1}};

  // edge 0 returns number of nodes for all edges if homogeneous
  //        returns -1 if edges have differing topology
  int Constants::nodes_per_edge[nedge + 1] = {-1, 3, 2, 2};
} // namespace

void Ioss::Tri4a::factory()
{
  static Ioss::Tri4a registerThis;
  Ioss::St_Tri4a::factory();
}

Ioss::Tri4a::Tri4a() : Ioss::ElementTopology(Ioss::Tri4a::name, "Triangle_4a") {}

int Ioss::Tri4a::parametric_dimension() const { return 2; }
int Ioss::Tri4a::spatial_dimension() const { return 2; }
int Ioss::Tri4a::order() const { return 2; }

int Ioss::Tri4a::number_corner_nodes() const { return 3; }
int Ioss::Tri4a::number_nodes() const { return Constants::nnode; }
int Ioss::Tri4a::number_edges() const { return Constants::nedge; }
int Ioss::Tri4a::number_faces() const { return Constants::nface; }

int Ioss::Tri4a::number_nodes_edge(int edge) const { return Constants::nodes_per_edge[edge]; }
int Ioss::Tri4a::number_nodes_face(int /* face */) const { return Constants::nfacenode; }
int Ioss::Tri4a::number_edges_face(int /* face */) const { return Constants::nfaceedge; }

bool Ioss::Tri4a::edges_similar() const { return false; }

Ioss::IntVector Ioss::Tri4a::edge_connectivity(int edge_number) const
{
  assert(edge_number > 0 && edge_number <= number_edges());
  Ioss::IntVector connectivity(Constants::nodes_per_edge[edge_number]);

  for (int i = 0; i < Constants::nodes_per_edge[edge_number]; i++) {
    connectivity[i] = Constants::edge_node_order[edge_number - 1][i];
  }

  return connectivity;
}

Ioss::IntVector Ioss::Tri4a::face_connectivity(int /* face_number */) const
{
  Ioss::IntVector connectivity;
  return connectivity;
}

Ioss::IntVector Ioss::Tri4a::element_connectivity() const
{
  Ioss::IntVector connectivity(number_nodes());

  for (int i = 0; i < number_nodes(); i++) {
    connectivity[i] = i;
  }

  return connectivity;
}

Ioss::ElementTopology *Ioss::Tri4a::face_type(int /* face_number */) const
{
  return (Ioss::ElementTopology *)nullptr;
}

Ioss::ElementTopology *Ioss::Tri4a::edge_type(int edge_number) const
{
  assert(edge_number >= 0 && edge_number <= number_edges());
  if (edge_number == 0) {
    return nullptr;
  }
  if (edge_number == 1) {
    return Ioss::ElementTopology::factory("edge3");
  }

  return Ioss::ElementTopology::factory("edge2");
}
