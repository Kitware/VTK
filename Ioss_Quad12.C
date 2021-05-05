// Copyright(C) 1999-2020 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

//------------------------------------------------------------------------
// Define a variable type for storage of this elements connectivity
#include "Ioss_CodeTypes.h"           // for IntVector
#include "Ioss_ElementTopology.h"     // for ElementTopology
#include <Ioss_ElementVariableType.h> // for ElementVariableType
#include <Ioss_Quad12.h>
#include <cassert> // for assert
#include <cstddef> // for nullptr

namespace Ioss {
  const char *Quad12::name = "quad12";
  class St_Quad12 : public ElementVariableType
  {
  public:
    static void factory() { static St_Quad12 registerThis; }

  protected:
    St_Quad12() : ElementVariableType(Ioss::Quad12::name, 12) {}
  };
} // namespace Ioss
// ========================================================================
Ioss::Quad12 Ioss::Quad12::instance_;

namespace {
  struct Constants
  {
    static const int nnode     = 12;
    static const int nedge     = 4;
    static const int nedgenode = 4;
    static const int nface     = 0;
    static int       edge_node_order[nedge][nedgenode];
  };

  // Edge numbers are zero-based [0..number_edges)
  int Constants::edge_node_order[nedge][nedgenode] = // [edge][edge_node]
      {{0, 1, 4, 5}, {1, 2, 6, 7}, {2, 3, 8, 9}, {3, 0, 10, 11}};
} // namespace

void Ioss::Quad12::factory()
{
  static Ioss::Quad12 registerThis;
  Ioss::St_Quad12::factory();
}

Ioss::Quad12::Quad12() : Ioss::ElementTopology(Ioss::Quad12::name, "Quadrilateral_12")
{
  Ioss::ElementTopology::alias(Ioss::Quad12::name, "Solid_Quad_12_2D");
  Ioss::ElementTopology::alias(Ioss::Quad12::name, "QUADRILATERAL_12_2D");
  Ioss::ElementTopology::alias(Ioss::Quad12::name, "Face_Quad_12_3D");
  Ioss::ElementTopology::alias(Ioss::Quad12::name, "quadface12");
}

Ioss::Quad12::~Quad12() = default;

int Ioss::Quad12::parametric_dimension() const { return 2; }
int Ioss::Quad12::spatial_dimension() const { return 2; }
int Ioss::Quad12::order() const { return 2; }

int Ioss::Quad12::number_corner_nodes() const { return 4; }
int Ioss::Quad12::number_nodes() const { return Constants::nnode; }
int Ioss::Quad12::number_edges() const { return Constants::nedge; }
int Ioss::Quad12::number_faces() const { return Constants::nface; }

int Ioss::Quad12::number_nodes_edge(int /* edge */) const { return Constants::nedgenode; }

int Ioss::Quad12::number_nodes_face(int /* face */) const { return 0; }
int Ioss::Quad12::number_edges_face(int /* face */) const { return 0; }

Ioss::IntVector Ioss::Quad12::edge_connectivity(int edge_number) const
{
  assert(edge_number > 0 && edge_number <= number_edges());
  Ioss::IntVector connectivity(Constants::nedgenode);
  assert(edge_number > 0 && edge_number <= Constants::nedge);

  for (int i = 0; i < Constants::nedgenode; i++) {
    connectivity[i] = Constants::edge_node_order[edge_number - 1][i];
  }

  return connectivity;
}

Ioss::IntVector Ioss::Quad12::face_connectivity(int /* face_number */) const
{
  Ioss::IntVector connectivity;
  return connectivity;
}

Ioss::IntVector Ioss::Quad12::element_connectivity() const
{
  Ioss::IntVector connectivity(number_nodes());
  for (int i = 0; i < number_nodes(); i++) {
    connectivity[i] = i;
  }
  return connectivity;
}

Ioss::ElementTopology *Ioss::Quad12::face_type(int /* face_number */) const
{
  return (Ioss::ElementTopology *)nullptr;
}

Ioss::ElementTopology *Ioss::Quad12::edge_type(int edge_number) const
{
  assert(edge_number >= 0 && edge_number <= number_edges());
  return Ioss::ElementTopology::factory("edge4");
}
