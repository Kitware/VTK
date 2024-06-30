// Copyright(C) 1999-2021 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include "Ioss_ElementVariableType.h" // for ElementVariableType
#include "Ioss_Shell4.h"
#include <cassert> // for assert
#include <string>

//------------------------------------------------------------------------
// Define a variable type for storage of this elements connectivity
#include "Ioss_CodeTypes.h"       // for IntVector
#include "Ioss_ElementTopology.h" // for ElementTopology
#include "Ioss_Utils.h"

namespace Ioss {
  const char *Shell4::name = "shell4";
  class St_Shell4 : public ElementVariableType
  {
  public:
    static void factory() { static St_Shell4 registerThis; }

  protected:
    St_Shell4() : ElementVariableType(Ioss::Shell4::name, 4) {}
  };
} // namespace Ioss
// ========================================================================
namespace {
  struct Constants
  {
    static const int nnode     = 4;
    static const int nedge     = 4;
    static const int nedgenode = 2;
    static const int nface     = 2;
    static const int nfacenode = 4;
    static const int nfaceedge = 4;
    static int       edge_node_order[nedge][nedgenode];
    static int       face_node_order[nface][nfacenode];
    static int       face_edge_order[nface][nfaceedge];
    static int       nodes_per_face[nface + 1];
    static int       edges_per_face[nface + 1];
  };

  // Edge numbers are zero-based [0..number_edges)
  int Constants::edge_node_order[nedge][nedgenode] = // [edge][edge_node]
      {{0, 1}, {1, 2}, {2, 3}, {3, 0}};

  // Face numbers are zero-based [0..number_faces)
  int Constants::face_node_order[nface][nfacenode] = // [face][face_node]
      {{0, 1, 2, 3}, {0, 3, 2, 1}};

  int Constants::face_edge_order[nface][nfaceedge] = // [face][face_edge]
      {{0, 1, 2, 3}, {3, 2, 1, 0}};

  // face 0 returns number of nodes for all faces if homogeneous
  //        returns -1 if faces have differing topology
  int Constants::nodes_per_face[nface + 1] = {4, 4, 4};

  // face 0 returns number of edges for all faces if homogeneous
  //        returns -1 if faces have differing topology
  int Constants::edges_per_face[nface + 1] = {4, 4, 4};
} // namespace

void Ioss::Shell4::factory()
{
  static Ioss::Shell4 registerThis;
  Ioss::St_Shell4::factory();
}

Ioss::Shell4::Shell4() : Ioss::ElementTopology(Ioss::Shell4::name, "ShellQuadrilateral_4")
{
  Ioss::ElementTopology::alias(Ioss::Shell4::name, "shell");
  Ioss::ElementTopology::alias(Ioss::Shell4::name, "Shell_Quad_4_3D");
  Ioss::ElementTopology::alias(Ioss::Shell4::name, "SHELL_QUADRILATERAL_4");
}

int Ioss::Shell4::parametric_dimension() const { return 2; }
int Ioss::Shell4::spatial_dimension() const { return 3; }
int Ioss::Shell4::order() const { return 1; }

int Ioss::Shell4::number_corner_nodes() const { return 4; }
int Ioss::Shell4::number_nodes() const { return Constants::nnode; }
int Ioss::Shell4::number_edges() const { return Constants::nedge; }
int Ioss::Shell4::number_faces() const { return Constants::nface; }

int Ioss::Shell4::number_nodes_edge(int /* edge */) const { return Constants::nedgenode; }

int Ioss::Shell4::number_nodes_face(int face) const
{
  // face is 1-based.  0 passed in for all faces.
  assert(face >= 0 && face <= number_faces());
  return Constants::nodes_per_face[face];
}

int Ioss::Shell4::number_edges_face(int face) const
{
  // face is 1-based.  0 passed in for all faces.
  assert(face >= 0 && face <= number_faces());
  return Constants::edges_per_face[face];
}

Ioss::IntVector Ioss::Shell4::edge_connectivity(int edge_number) const
{
  assert(edge_number > 0 && edge_number <= Constants::nedge);
  Ioss::IntVector connectivity(Constants::nedgenode);

  for (int i = 0; i < Constants::nedgenode; i++) {
    connectivity[i] = Constants::edge_node_order[edge_number - 1][i];
  }

  return connectivity;
}

Ioss::IntVector Ioss::Shell4::face_connectivity(int face_number) const
{
  assert(face_number > 0 && face_number <= number_faces());
  Ioss::IntVector connectivity(Constants::nodes_per_face[face_number]);

  for (int i = 0; i < Constants::nodes_per_face[face_number]; i++) {
    connectivity[i] = Constants::face_node_order[face_number - 1][i];
  }

  return connectivity;
}

Ioss::IntVector Ioss::Shell4::element_connectivity() const
{
  Ioss::IntVector connectivity(number_nodes());
  for (int i = 0; i < number_nodes(); i++) {
    connectivity[i] = i;
  }
  return connectivity;
}

Ioss::ElementTopology *Ioss::Shell4::face_type(int face_number) const
{
  assert(face_number >= 0 && face_number <= number_faces());
  IOSS_ASSERT_USED(face_number);
  //  return Ioss::ElementTopology::factory("quadface4");
  return Ioss::ElementTopology::factory("quad4");
}

Ioss::ElementTopology *Ioss::Shell4::edge_type(int edge_number) const
{
  assert(edge_number >= 0 && edge_number <= number_edges());
  IOSS_ASSERT_USED(edge_number);
  return Ioss::ElementTopology::factory("edge2");
}

Ioss::IntVector Ioss::Shell4::face_edge_connectivity(int face_number) const
{
  assert(face_number > 0 && face_number <= Constants::nface);

  int             nface_edge = number_edges_face(face_number);
  Ioss::IntVector fcon(nface_edge);

  for (int i = 0; i < nface_edge; i++) {
    fcon[i] = Constants::face_edge_order[face_number - 1][i];
  }

  return fcon;
}
