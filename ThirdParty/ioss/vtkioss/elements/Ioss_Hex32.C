// Copyright(C) 1999-2021 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include "Ioss_ElementVariableType.h" // for ElementVariableType
#include "Ioss_Hex32.h"
#include <cassert> // for assert
#include <string>

#include "Ioss_CodeTypes.h"       // for IntVector
#include "Ioss_ElementTopology.h" // for ElementTopology
#include "Ioss_Utils.h"

//------------------------------------------------------------------------
// Define a variable type for storage of this elements connectivity
namespace Ioss {
  const char *Hex32::name = "hex32";
  class St_Hex32 : public ElementVariableType
  {
  public:
    static void factory() { static St_Hex32 registerThis; }

  protected:
    St_Hex32() : ElementVariableType(Ioss::Hex32::name, 32) {}
  };
} // namespace Ioss

// ========================================================================
namespace {
  struct Constants
  {
    static const int nnode     = 32;
    static const int nedge     = 12;
    static const int nedgenode = 4;
    static const int nface     = 6;
    static const int nfacenode = 12;
    static const int nfaceedge = 4;
    static int       edge_node_order[nedge][nedgenode];
    static int       face_node_order[nface][nfacenode];
    static int       face_edge_order[nface][nfaceedge];
    static int       nodes_per_face[nface + 1];
    static int       edges_per_face[nface + 1];
  };

  // Edge numbers are zero-based [0..number_edges)
  int Constants::edge_node_order[nedge][nedgenode] = // [edge][edge_node]
      {{0, 1, 8, 9},   {1, 5, 17, 21}, {5, 4, 25, 24}, {4, 0, 20, 16},
       {3, 2, 13, 12}, {2, 6, 18, 22}, {6, 7, 28, 29}, {7, 3, 23, 19},
       {0, 3, 15, 14}, {1, 2, 10, 11}, {5, 6, 26, 27}, {4, 7, 31, 30}};

  // Face numbers are zero-based [0..number_faces)
  int Constants::face_node_order[nface][nfacenode] = // [face][face_node]
      {{0, 1, 5, 4, 8, 9, 17, 21, 25, 24, 20, 16},   {1, 2, 6, 5, 10, 11, 18, 22, 27, 26, 21, 17},
       {2, 3, 7, 6, 12, 13, 19, 23, 29, 28, 22, 18}, {0, 4, 7, 3, 16, 20, 31, 30, 23, 19, 14, 15},
       {0, 3, 2, 1, 15, 14, 13, 12, 11, 10, 9, 8},   {4, 5, 6, 7, 24, 25, 26, 27, 28, 29, 30, 31}};

  int Constants::face_edge_order[nface][nfaceedge] = // [face][face_edge]
      {{0, 1, 2, 3}, {9, 5, 10, 1}, {4, 7, 6, 5}, {3, 11, 7, 8}, {8, 4, 9, 0}, {2, 10, 6, 11}};

  // face 0 returns number of nodes for all faces if homogeneous
  //        returns -1 if faces have differing topology
  int Constants::nodes_per_face[nface + 1] = {12, 12, 12, 12, 12, 12, 12};

  // face 0 returns number of edges for all faces if homogeneous
  //        returns -1 if faces have differing topology
  int Constants::edges_per_face[nface + 1] = {4, 4, 4, 4, 4, 4, 4};
} // namespace

void Ioss::Hex32::factory()
{
  static Ioss::Hex32 registerThis;
  Ioss::St_Hex32::factory();
}

Ioss::Hex32::Hex32() : Ioss::ElementTopology(Ioss::Hex32::name, "Hexahedron_32")
{
  Ioss::ElementTopology::alias(Ioss::Hex32::name, "Solid_Hex_32_3D");
}

int Ioss::Hex32::parametric_dimension() const { return 3; }
int Ioss::Hex32::spatial_dimension() const { return 3; }
int Ioss::Hex32::order() const { return 3; }

int Ioss::Hex32::number_corner_nodes() const { return 8; }
int Ioss::Hex32::number_nodes() const { return Constants::nnode; }
int Ioss::Hex32::number_edges() const { return Constants::nedge; }
int Ioss::Hex32::number_faces() const { return Constants::nface; }

int Ioss::Hex32::number_nodes_edge(int /* edge */) const { return Constants::nedgenode; }

int Ioss::Hex32::number_nodes_face(int face) const
{
  // face is 1-based.  0 passed in for all faces.
  assert(face >= 0 && face <= number_faces());
  return Constants::nodes_per_face[face];
}

int Ioss::Hex32::number_edges_face(int face) const
{
  // face is 1-based.  0 passed in for all faces.
  assert(face >= 0 && face <= number_faces());
  return Constants::edges_per_face[face];
}

Ioss::IntVector Ioss::Hex32::edge_connectivity(int edge_number) const
{
  assert(edge_number > 0 && edge_number <= Constants::nedge);
  Ioss::IntVector connectivity(Constants::nedgenode);

  for (int i = 0; i < Constants::nedgenode; i++) {
    connectivity[i] = Constants::edge_node_order[edge_number - 1][i];
  }

  return connectivity;
}

Ioss::IntVector Ioss::Hex32::face_connectivity(int face_number) const
{
  assert(face_number > 0 && face_number <= number_faces());
  Ioss::IntVector connectivity(Constants::nodes_per_face[face_number]);

  for (int i = 0; i < Constants::nodes_per_face[face_number]; i++) {
    connectivity[i] = Constants::face_node_order[face_number - 1][i];
  }

  return connectivity;
}

Ioss::IntVector Ioss::Hex32::element_connectivity() const
{
  Ioss::IntVector connectivity(number_nodes());
  for (int i = 0; i < number_nodes(); i++) {
    connectivity[i] = i;
  }
  return connectivity;
}

Ioss::ElementTopology *Ioss::Hex32::face_type(int face_number) const
{
  // face_number == 0 returns topology for all faces if
  // all faces are the same topology; otherwise, returns nullptr
  // face_number is 1-based.

  assert(face_number >= 0 && face_number <= number_faces());
  IOSS_ASSERT_USED(face_number);
  //  return Ioss::ElementTopology::factory("quadface12");
  return Ioss::ElementTopology::factory("quad12");
}

Ioss::ElementTopology *Ioss::Hex32::edge_type(int edge_number) const
{
  // edge_number == 0 returns topology for all edges if
  // all edges are the same topology; otherwise, returns nullptr
  // edge_number is 1-based.

  assert(edge_number >= 0 && edge_number <= number_edges());
  IOSS_ASSERT_USED(edge_number);
  return Ioss::ElementTopology::factory("edge4");
}

Ioss::IntVector Ioss::Hex32::face_edge_connectivity(int face_number) const
{
  assert(face_number > 0 && face_number <= Constants::nface);

  int             nface_edge = number_edges_face(face_number);
  Ioss::IntVector fcon(nface_edge);

  for (int i = 0; i < nface_edge; i++) {
    fcon[i] = Constants::face_edge_order[face_number - 1][i];
  }

  return fcon;
}
