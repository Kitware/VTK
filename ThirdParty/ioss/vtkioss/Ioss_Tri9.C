// Copyright(C) 1999-2020 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

// ========================================================================
// Define a variable type for storage of this elements connectivity
#include "Ioss_CodeTypes.h"           // for IntVector
#include "Ioss_ElementTopology.h"     // for ElementTopology
#include <Ioss_ElementVariableType.h> // for ElementVariableType
#include <Ioss_Tri9.h>
#include <cassert> // for assert
#include <cstddef> // for nullptr

namespace Ioss {
  const char *Tri9::name = "tri9";
  class St_Tri9 : public ElementVariableType
  {
  public:
    static void factory() { static St_Tri9 registerThis; }

  protected:
    St_Tri9() : ElementVariableType(Ioss::Tri9::name, 9) {}
  };
} // namespace Ioss
//------------------------------------------------------------------------
namespace {
  struct Constants
  {
    static const int nnode     = 9;
    static const int nedge     = 3;
    static const int nedgenode = 4;
    static const int nface     = 0;
    static const int nfacenode = 0;
    static const int nfaceedge = 0;
    static int       edge_node_order[nedge][nedgenode];
  };

  // Edge numbers are zero-based [0..number_edges)
  int Constants::edge_node_order[nedge][nedgenode] = // [edge][edge_node]
      {{0, 1, 3, 4}, {1, 2, 5, 6}, {2, 0, 7, 8}};
} // namespace

void Ioss::Tri9::factory()
{
  static Ioss::Tri9 registerThis;
  Ioss::St_Tri9::factory();
}

Ioss::Tri9::Tri9() : Ioss::ElementTopology(Ioss::Tri9::name, "Triangle_9")
{
  Ioss::ElementTopology::alias(Ioss::Tri9::name, "triangle9");
  Ioss::ElementTopology::alias(Ioss::Tri9::name, "Solid_Tri_9_2D");
  Ioss::ElementTopology::alias(Ioss::Tri9::name, "Face_Tri_9_3D");
  Ioss::ElementTopology::alias(Ioss::Tri9::name, "TRIANGLE_9_2D");
  Ioss::ElementTopology::alias(Ioss::Tri9::name, "triface9");
}

Ioss::Tri9::~Tri9() = default;

int Ioss::Tri9::parametric_dimension() const { return 2; }
int Ioss::Tri9::spatial_dimension() const { return 2; }
int Ioss::Tri9::order() const { return 3; }

int Ioss::Tri9::number_corner_nodes() const { return 3; }
int Ioss::Tri9::number_nodes() const { return Constants::nnode; }
int Ioss::Tri9::number_edges() const { return Constants::nedge; }
int Ioss::Tri9::number_faces() const { return Constants::nface; }

int Ioss::Tri9::number_nodes_edge(int /* edge */) const { return Constants::nedgenode; }
int Ioss::Tri9::number_nodes_face(int /* face */) const { return Constants::nfacenode; }
int Ioss::Tri9::number_edges_face(int /* face */) const { return Constants::nfaceedge; }

Ioss::IntVector Ioss::Tri9::edge_connectivity(int edge_number) const
{
  assert(edge_number > 0 && edge_number <= number_edges());
  Ioss::IntVector connectivity(Constants::nedgenode);

  for (int i = 0; i < Constants::nedgenode; i++) {
    connectivity[i] = Constants::edge_node_order[edge_number - 1][i];
  }

  return connectivity;
}

Ioss::IntVector Ioss::Tri9::face_connectivity(int /* face_number */) const
{
  Ioss::IntVector connectivity;
  return connectivity;
}

Ioss::IntVector Ioss::Tri9::element_connectivity() const
{
  Ioss::IntVector connectivity(number_nodes());

  for (int i = 0; i < number_nodes(); i++) {
    connectivity[i] = i;
  }

  return connectivity;
}

Ioss::ElementTopology *Ioss::Tri9::face_type(int /* face_number */) const
{
  return (Ioss::ElementTopology *)nullptr;
}

Ioss::ElementTopology *Ioss::Tri9::edge_type(int edge_number) const
{
  assert(edge_number >= 0 && edge_number <= number_edges());
  return Ioss::ElementTopology::factory("edge4");
}
