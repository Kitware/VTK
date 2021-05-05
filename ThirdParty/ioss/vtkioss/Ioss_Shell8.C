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
#include <Ioss_Shell8.h>
#include <cassert> // for assert

namespace Ioss {
  const char *Shell8::name = "shell8";
  class St_Shell8 : public ElementVariableType
  {
  public:
    static void factory() { static St_Shell8 registerThis; }

  protected:
    St_Shell8() : ElementVariableType(Ioss::Shell8::name, 8) {}
  };
} // namespace Ioss
// ========================================================================
namespace {
  struct Constants
  {
    static const int nnode     = 8;
    static const int nedge     = 4;
    static const int nedgenode = 3;
    static const int nface     = 2;
    static const int nfacenode = 8;
    static const int nfaceedge = 4;
    static int       edge_node_order[nedge][nedgenode];
    static int       face_node_order[nface][nfacenode];
    static int       face_edge_order[nface][nfaceedge];
    static int       nodes_per_face[nface + 1];
    static int       edges_per_face[nface + 1];
  };

  // Edge numbers are zero-based [0..number_edges)
  int Constants::edge_node_order[nedge][nedgenode] = // [edge][edge_node]
      {{0, 1, 4}, {1, 2, 5}, {2, 3, 6}, {3, 0, 7}};

  // Face numbers are zero-based [0..number_faces)
  int Constants::face_node_order[nface][nfacenode] = // [face][face_node]
      {{0, 1, 2, 3, 4, 5, 6, 7}, {0, 3, 2, 1, 7, 6, 5, 4}};

  int Constants::face_edge_order[nface][nfaceedge] = // [face][face_edge]
      {{0, 1, 2, 3}, {3, 2, 1, 0}};

  // face 0 returns number of nodes for all faces if homogeneous
  //        returns -1 if faces have differing topology
  int Constants::nodes_per_face[nface + 1] = {8, 8, 8};

  // face 0 returns number of edges for all faces if homogeneous
  //        returns -1 if faces have differing topology
  int Constants::edges_per_face[nface + 1] = {4, 4, 4};
} // namespace

void Ioss::Shell8::factory()
{
  static Ioss::Shell8 registerThis;
  Ioss::St_Shell8::factory();
}

Ioss::Shell8::Shell8() : Ioss::ElementTopology(Ioss::Shell8::name, "ShellQuadrilateral_8")
{
  Ioss::ElementTopology::alias(Ioss::Shell8::name, "Shell_Quad_8_3D");
  Ioss::ElementTopology::alias(Ioss::Shell8::name, "SHELL_QUADRILATERAL_8");
}

Ioss::Shell8::~Shell8() = default;

int Ioss::Shell8::parametric_dimension() const { return 2; }
int Ioss::Shell8::spatial_dimension() const { return 3; }
int Ioss::Shell8::order() const { return 2; }

int Ioss::Shell8::number_corner_nodes() const { return 4; }
int Ioss::Shell8::number_nodes() const { return Constants::nnode; }
int Ioss::Shell8::number_edges() const { return Constants::nedge; }
int Ioss::Shell8::number_faces() const { return Constants::nface; }

int Ioss::Shell8::number_nodes_edge(int /* edge */) const { return Constants::nedgenode; }

int Ioss::Shell8::number_nodes_face(int face) const
{
  // face is 1-based.  0 passed in for all faces.
  assert(face >= 0 && face <= number_faces());
  return Constants::nodes_per_face[face];
}

int Ioss::Shell8::number_edges_face(int face) const
{
  // face is 1-based.  0 passed in for all faces.
  assert(face >= 0 && face <= number_faces());
  return Constants::edges_per_face[face];
}

Ioss::IntVector Ioss::Shell8::edge_connectivity(int edge_number) const
{
  assert(edge_number > 0 && edge_number <= Constants::nedge);
  Ioss::IntVector connectivity(Constants::nedgenode);

  for (int i = 0; i < Constants::nedgenode; i++) {
    connectivity[i] = Constants::edge_node_order[edge_number - 1][i];
  }

  return connectivity;
}

Ioss::IntVector Ioss::Shell8::face_connectivity(int face_number) const
{
  assert(face_number > 0 && face_number <= number_faces());
  Ioss::IntVector connectivity(Constants::nodes_per_face[face_number]);

  for (int i = 0; i < Constants::nodes_per_face[face_number]; i++) {
    connectivity[i] = Constants::face_node_order[face_number - 1][i];
  }

  return connectivity;
}

Ioss::IntVector Ioss::Shell8::element_connectivity() const
{
  Ioss::IntVector connectivity(number_nodes());
  for (int i = 0; i < number_nodes(); i++) {
    connectivity[i] = i;
  }
  return connectivity;
}

Ioss::ElementTopology *Ioss::Shell8::face_type(int face_number) const
{
  assert(face_number >= 0 && face_number <= number_faces());
  //  return Ioss::ElementTopology::factory("quadface8");
  return Ioss::ElementTopology::factory("quad8");
}

Ioss::ElementTopology *Ioss::Shell8::edge_type(int edge_number) const
{
  assert(edge_number >= 0 && edge_number <= number_edges());
  return Ioss::ElementTopology::factory("edge3");
}

Ioss::IntVector Ioss::Shell8::face_edge_connectivity(int face_number) const
{
  assert(face_number > 0 && face_number <= Constants::nface);

  int             nface_edge = number_edges_face(face_number);
  Ioss::IntVector fcon(nface_edge);

  for (int i = 0; i < nface_edge; i++) {
    fcon[i] = Constants::face_edge_order[face_number - 1][i];
  }

  return fcon;
}
