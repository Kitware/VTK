// Copyright(C) 1999-2020 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include "Ioss_CodeTypes.h"           // for IntVector
#include "Ioss_ElementTopology.h"     // for ElementTopology
#include <Ioss_ElementVariableType.h> // for ElementVariableType
#include <Ioss_Hex8.h>
#include <cassert> // for assert

//------------------------------------------------------------------------
// Define a variable type for storage of this elements connectivity
namespace Ioss {
  const char *Hex8::name = "hex8";

  class St_Hex8 : public ElementVariableType
  {
  public:
    static void factory() { static St_Hex8 registerThis; }

  protected:
    St_Hex8() : ElementVariableType(Ioss::Hex8::name, 8) {}
  };
} // namespace Ioss

// ========================================================================
namespace {
  struct Constants
  {
    static const int nnode     = 8;
    static const int nedge     = 12;
    static const int nedgenode = 2;
    static const int nface     = 6;
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
      {{0, 1}, {1, 2}, {2, 3}, {3, 0}, {4, 5}, {5, 6},
       {6, 7}, {7, 4}, {0, 4}, {1, 5}, {2, 6}, {3, 7}};

  // Face numbers are zero-based [0..number_faces)
  int Constants::face_node_order[nface][nfacenode] = // [face][face_node]
      {{0, 1, 5, 4}, {1, 2, 6, 5}, {2, 3, 7, 6}, {0, 4, 7, 3}, {0, 3, 2, 1}, {4, 5, 6, 7}};

  int Constants::face_edge_order[nface][nfaceedge] = // [face][face_edge]
      {{0, 9, 4, 8}, {1, 10, 5, 9}, {2, 11, 6, 10}, {8, 7, 11, 3}, {3, 2, 1, 0}, {4, 5, 6, 7}};

  // face 0 returns number of nodes for all faces if homogeneous
  //        returns -1 if faces have differing topology
  int Constants::nodes_per_face[nface + 1] = {4, 4, 4, 4, 4, 4, 4};

  // face 0 returns number of edges for all faces if homogeneous
  //        returns -1 if faces have differing topology
  int Constants::edges_per_face[nface + 1] = {4, 4, 4, 4, 4, 4, 4};
} // namespace

void Ioss::Hex8::factory()
{
  static Ioss::Hex8 registerThis;
  Ioss::St_Hex8::factory();
}

Ioss::Hex8::Hex8() : Ioss::ElementTopology(Ioss::Hex8::name, "Hexahedron_8")
{
  Ioss::ElementTopology::alias(Ioss::Hex8::name, "hex");
  Ioss::ElementTopology::alias(Ioss::Hex8::name, "Solid_Hex_8_3D");
}

Ioss::Hex8::~Hex8() = default;

int Ioss::Hex8::parametric_dimension() const { return 3; }
int Ioss::Hex8::spatial_dimension() const { return 3; }
int Ioss::Hex8::order() const { return 1; }

int Ioss::Hex8::number_corner_nodes() const { return Constants::nnode; }
int Ioss::Hex8::number_nodes() const { return Constants::nnode; }
int Ioss::Hex8::number_edges() const { return Constants::nedge; }
int Ioss::Hex8::number_faces() const { return Constants::nface; }

int Ioss::Hex8::number_nodes_edge(int /* edge */) const { return Constants::nedgenode; }

int Ioss::Hex8::number_nodes_face(int face) const
{
  // face is 1-based.  0 passed in for all faces.
  assert(face >= 0 && face <= number_faces());
  return Constants::nodes_per_face[face];
}

int Ioss::Hex8::number_edges_face(int face) const
{
  // face is 1-based.  0 passed in for all faces.
  assert(face >= 0 && face <= number_faces());
  return Constants::edges_per_face[face];
}

Ioss::IntVector Ioss::Hex8::edge_connectivity(int edge_number) const
{
  assert(edge_number > 0 && edge_number <= Constants::nedge);
  Ioss::IntVector connectivity(Constants::nedgenode);

  for (int i = 0; i < Constants::nedgenode; i++) {
    connectivity[i] = Constants::edge_node_order[edge_number - 1][i];
  }

  return connectivity;
}

Ioss::IntVector Ioss::Hex8::face_connectivity(int face_number) const
{
  assert(face_number > 0 && face_number <= number_faces());
  Ioss::IntVector connectivity(Constants::nodes_per_face[face_number]);

  for (int i = 0; i < Constants::nodes_per_face[face_number]; i++) {
    connectivity[i] = Constants::face_node_order[face_number - 1][i];
  }

  return connectivity;
}

Ioss::IntVector Ioss::Hex8::element_connectivity() const
{
  Ioss::IntVector connectivity(number_nodes());
  for (int i = 0; i < number_nodes(); i++) {
    connectivity[i] = i;
  }
  return connectivity;
}

Ioss::ElementTopology *Ioss::Hex8::face_type(int face_number) const
{
  // face_number == 0 returns topology for all faces if
  // all faces are the same topology; otherwise, returns nullptr
  // face_number is 1-based.

  assert(face_number >= 0 && face_number <= number_faces());
  //  return Ioss::ElementTopology::factory("quadface4");
  return Ioss::ElementTopology::factory("quad4");
}

Ioss::ElementTopology *Ioss::Hex8::edge_type(int edge_number) const
{
  // edge_number == 0 returns topology for all edges if
  // all edges are the same topology; otherwise, returns nullptr
  // edge_number is 1-based.

  assert(edge_number >= 0 && edge_number <= number_edges());
  return Ioss::ElementTopology::factory("edge2");
}

Ioss::IntVector Ioss::Hex8::face_edge_connectivity(int face_number) const
{
  assert(face_number > 0 && face_number <= Constants::nface);

  int             nface_edge = number_edges_face(face_number);
  Ioss::IntVector fcon(nface_edge);

  for (int i = 0; i < nface_edge; i++) {
    fcon[i] = Constants::face_edge_order[face_number - 1][i];
  }

  return fcon;
}
