// Copyright(C) 1999-2021 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include "Ioss_ElementVariableType.h" // for ElementVariableType
#include "Ioss_Tri3.h"
#include <cassert> // for assert
#include <string>

#include "Ioss_CodeTypes.h"       // for IntVector
#include "Ioss_ElementTopology.h" // for ElementTopology
#include "Ioss_Utils.h"

// ========================================================================
// Define a variable type for storage of this elements connectivity
namespace Ioss {
  const char *Tri3::name = "tri3";
  class St_Tri3 : public ElementVariableType
  {
  public:
    static void factory() { static St_Tri3 registerThis; }

  protected:
    St_Tri3() : ElementVariableType(Ioss::Tri3::name, 3) {}
  };
} // namespace Ioss
//------------------------------------------------------------------------
namespace {
  struct Constants
  {
    static const int nnode     = 3;
    static const int nedge     = 3;
    static const int nedgenode = 2;
    static const int nface     = 0;
    static const int nfacenode = 0;
    static const int nfaceedge = 0;
    static int       edge_node_order[nedge][nedgenode];
  };

  // Edge numbers are zero-based [0..number_edges)
  int Constants::edge_node_order[nedge][nedgenode] = // [edge][edge_node]
      {{0, 1}, {1, 2}, {2, 0}};
} // namespace

void Ioss::Tri3::factory()
{
  static Ioss::Tri3 registerThis;
  Ioss::St_Tri3::factory();
}

Ioss::Tri3::Tri3() : Ioss::ElementTopology(Ioss::Tri3::name, "Triangle_3")
{
  Ioss::ElementTopology::alias(Ioss::Tri3::name, "tri");
  Ioss::ElementTopology::alias(Ioss::Tri3::name, "triangle");
  Ioss::ElementTopology::alias(Ioss::Tri3::name, "triangle3");
  Ioss::ElementTopology::alias(Ioss::Tri3::name, "Solid_Tri_3_2D");
  Ioss::ElementTopology::alias(Ioss::Tri3::name, "Face_Tri_3_3D");
  Ioss::ElementTopology::alias(Ioss::Tri3::name, "triface3");
  Ioss::ElementTopology::alias(Ioss::Tri3::name, "TRIANGLE_3_2D");
}

int Ioss::Tri3::parametric_dimension() const { return 2; }
int Ioss::Tri3::spatial_dimension() const { return 2; }
int Ioss::Tri3::order() const { return 1; }

int Ioss::Tri3::number_corner_nodes() const { return number_nodes(); }
int Ioss::Tri3::number_nodes() const { return Constants::nnode; }
int Ioss::Tri3::number_edges() const { return Constants::nedge; }
int Ioss::Tri3::number_faces() const { return Constants::nface; }

int Ioss::Tri3::number_nodes_edge(int /* edge */) const { return Constants::nedgenode; }
int Ioss::Tri3::number_nodes_face(int /* face */) const { return Constants::nfacenode; }
int Ioss::Tri3::number_edges_face(int /* face */) const { return Constants::nfaceedge; }

Ioss::IntVector Ioss::Tri3::edge_connectivity(int edge_number) const
{
  assert(edge_number > 0 && edge_number <= number_edges());
  Ioss::IntVector connectivity(Constants::nedgenode);

  for (int i = 0; i < Constants::nedgenode; i++) {
    connectivity[i] = Constants::edge_node_order[edge_number - 1][i];
  }

  return connectivity;
}

Ioss::IntVector Ioss::Tri3::face_connectivity(int /* face_number */) const
{
  Ioss::IntVector connectivity;
  return connectivity;
}

Ioss::IntVector Ioss::Tri3::element_connectivity() const
{
  Ioss::IntVector connectivity(number_nodes());

  for (int i = 0; i < number_nodes(); i++) {
    connectivity[i] = i;
  }

  return connectivity;
}

Ioss::ElementTopology *Ioss::Tri3::face_type(int /* face_number */) const
{
  return (Ioss::ElementTopology *)nullptr;
}

Ioss::ElementTopology *Ioss::Tri3::edge_type(int edge_number) const
{
  assert(edge_number >= 0 && edge_number <= number_edges());
  IOSS_ASSERT_USED(edge_number);
  return Ioss::ElementTopology::factory("edge2");
}
