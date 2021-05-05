// Copyright(C) 1999-2020 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include "Ioss_CodeTypes.h"           // for IntVector
#include "Ioss_ElementTopology.h"     // for ElementTopology
#include <Ioss_ElementVariableType.h> // for ElementVariableType
#include <Ioss_TriShell7.h>
#include <cassert> // for assert

//------------------------------------------------------------------------
// Define a variable type for storage of this elements connectivity
namespace Ioss {
  const char *TriShell7::name = "trishell7";
  class St_TriShell7 : public ElementVariableType
  {
  public:
    static void factory() { static St_TriShell7 registerThis; }

  protected:
    St_TriShell7() : ElementVariableType(Ioss::TriShell7::name, 7) {}
  };
} // namespace Ioss
// ========================================================================
namespace {
  struct Constants
  {
    static const int nnode     = 7;
    static const int nedge     = 3;
    static const int nedgenode = 3;
    static const int nface     = 2;
    static const int nfacenode = 7;
    static int       edge_node_order[nedge][nedgenode];
    static int       face_node_order[nface][nfacenode];
    static int       nodes_per_face[nface + 1];
    static int       edges_per_face[nface + 1];
  };

  // Edge numbers are zero-based [0..number_edges)
  int Constants::edge_node_order[nedge][nedgenode] = // [edge][edge_node]
      {{0, 1, 3}, {1, 2, 4}, {2, 0, 5}};

  // Face numbers are zero-based [0..number_faces)
  int Constants::face_node_order[nface][nfacenode] = // [face][face_node]
      {{0, 1, 2, 3, 4, 5, 6}, {0, 2, 1, 5, 4, 3, 6}};

  // face 0 returns number of nodes for all faces if homogeneous
  //        returns -1 if faces have differing topology
  int Constants::nodes_per_face[nface + 1] = {7, 7, 7};

  // face 0 returns number of edges for all faces if homogeneous
  //        returns -1 if faces have differing topology
  int Constants::edges_per_face[nface + 1] = {3, 3, 3};
} // namespace

void Ioss::TriShell7::factory()
{
  static Ioss::TriShell7 registerThis;
  Ioss::St_TriShell7::factory();
}

Ioss::TriShell7::TriShell7() : Ioss::ElementTopology(Ioss::TriShell7::name, "ShellTriangle_7")
{
  Ioss::ElementTopology::alias(Ioss::TriShell7::name, "Shell_Tri_7_3D");
  Ioss::ElementTopology::alias(Ioss::TriShell7::name, "SHELL_TRIANGLE_7");
  Ioss::ElementTopology::alias(Ioss::TriShell7::name, "SHELL7");
}

Ioss::TriShell7::~TriShell7() = default;

int Ioss::TriShell7::parametric_dimension() const { return 2; }
int Ioss::TriShell7::spatial_dimension() const { return 3; }
int Ioss::TriShell7::order() const { return 2; }

int Ioss::TriShell7::number_corner_nodes() const { return 3; }
int Ioss::TriShell7::number_nodes() const { return Constants::nnode; }
int Ioss::TriShell7::number_edges() const { return Constants::nedge; }
int Ioss::TriShell7::number_faces() const { return Constants::nface; }

int Ioss::TriShell7::number_nodes_edge(int /* edge */) const { return Constants::nedgenode; }

int Ioss::TriShell7::number_nodes_face(int face) const
{
  // face is 1-based.  0 passed in for all faces.
  assert(face >= 0 && face <= number_faces());
  return Constants::nodes_per_face[face];
}

int Ioss::TriShell7::number_edges_face(int face) const
{
  // face is 1-based.  0 passed in for all faces.
  assert(face >= 0 && face <= number_faces());
  return Constants::edges_per_face[face];
}

Ioss::IntVector Ioss::TriShell7::edge_connectivity(int edge_number) const
{
  assert(edge_number > 0 && edge_number <= Constants::nedge);
  Ioss::IntVector connectivity(Constants::nedgenode);

  for (int i = 0; i < Constants::nedgenode; i++) {
    connectivity[i] = Constants::edge_node_order[edge_number - 1][i];
  }

  return connectivity;
}

Ioss::IntVector Ioss::TriShell7::face_connectivity(int face_number) const
{
  assert(face_number > 0 && face_number <= number_faces());
  Ioss::IntVector connectivity(Constants::nodes_per_face[face_number]);

  for (int i = 0; i < Constants::nodes_per_face[face_number]; i++) {
    connectivity[i] = Constants::face_node_order[face_number - 1][i];
  }

  return connectivity;
}

Ioss::IntVector Ioss::TriShell7::element_connectivity() const
{
  Ioss::IntVector connectivity(number_nodes());
  for (int i = 0; i < number_nodes(); i++) {
    connectivity[i] = i;
  }
  return connectivity;
}

Ioss::ElementTopology *Ioss::TriShell7::face_type(int face_number) const
{
  assert(face_number >= 0 && face_number <= number_faces());
  //  return Ioss::ElementTopology::factory("triface7");
  return Ioss::ElementTopology::factory("tri7");
}

Ioss::ElementTopology *Ioss::TriShell7::edge_type(int edge_number) const
{
  assert(edge_number >= 0 && edge_number <= number_edges());
  return Ioss::ElementTopology::factory("edge3");
}
