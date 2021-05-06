// Copyright(C) 1999-2020 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include "Ioss_CodeTypes.h"           // for IntVector
#include "Ioss_ElementTopology.h"     // for ElementTopology
#include <Ioss_ElementVariableType.h> // for ElementVariableType
#include <Ioss_Tet4.h>
#include <cassert> // for assert

//------------------------------------------------------------------------
// Define a variable type for storage of this elements connectivity
namespace Ioss {
  const char *Tet4::name = "tetra4";
  class St_Tet4 : public ElementVariableType
  {
  public:
    static void factory() { static St_Tet4 registerThis; }

  protected:
    St_Tet4() : ElementVariableType(Ioss::Tet4::name, 4) {}
  };
} // namespace Ioss

// ========================================================================
namespace {
  struct Constants
  {
    static const int nnode     = 4;
    static const int nedge     = 6;
    static const int nedgenode = 2;
    static const int nface     = 4;
    static const int nfacenode = 3;
    static const int nfaceedge = 3;
    static int       edge_node_order[nedge][nedgenode];
    static int       face_node_order[nface][nfacenode];
    static int       face_edge_order[nface][nfaceedge];
    static int       nodes_per_face[nface + 1];
    static int       edges_per_face[nface + 1];
  };

  // Edge numbers are zero-based [0..number_edges)
  int Constants::edge_node_order[nedge][nedgenode] = // [edge][edge_node]
      {{0, 1}, {1, 2}, {2, 0}, {0, 3}, {1, 3}, {2, 3}};

  // Face numbers are zero-based [0..number_faces)
  int Constants::face_node_order[nface][nfacenode] = // [face][face_node]
      {{0, 1, 3}, {1, 2, 3}, {0, 3, 2}, {0, 2, 1}};

  int Constants::face_edge_order[nface][nfaceedge] = // [face][face_edge]
      {{0, 4, 3}, {1, 5, 4}, {3, 5, 2}, {2, 1, 0}};

  // face 0 returns number of nodes for all faces if homogeneous
  //        returns -1 if faces have differing topology
  int Constants::nodes_per_face[nface + 1] = {3, 3, 3, 3, 3};

  // face 0 returns number of edges for all faces if homogeneous
  //        returns -1 if faces have differing topology
  int Constants::edges_per_face[nface + 1] = {3, 3, 3, 3, 3};
} // namespace

void Ioss::Tet4::factory()
{
  static Ioss::Tet4 registerThis;
  Ioss::St_Tet4::factory();
}

Ioss::Tet4::Tet4() : Ioss::ElementTopology(Ioss::Tet4::name, "Tetrahedron_4")
{
  Ioss::ElementTopology::alias(Ioss::Tet4::name, "tetra");
  Ioss::ElementTopology::alias(Ioss::Tet4::name, "tet4");
  Ioss::ElementTopology::alias(Ioss::Tet4::name, "tet");
  Ioss::ElementTopology::alias(Ioss::Tet4::name, "Solid_Tet_4_3D");
}

Ioss::Tet4::~Tet4() = default;

int Ioss::Tet4::parametric_dimension() const { return 3; }
int Ioss::Tet4::spatial_dimension() const { return 3; }
int Ioss::Tet4::order() const { return 1; }

int Ioss::Tet4::number_corner_nodes() const { return number_nodes(); }
int Ioss::Tet4::number_nodes() const { return Constants::nnode; }
int Ioss::Tet4::number_edges() const { return Constants::nedge; }
int Ioss::Tet4::number_faces() const { return Constants::nface; }

int Ioss::Tet4::number_nodes_edge(int /* edge */) const { return Constants::nedgenode; }

int Ioss::Tet4::number_nodes_face(int face) const
{
  // face is 1-based.  0 passed in for all faces.
  assert(face >= 0 && face <= number_faces());
  return Constants::nodes_per_face[face];
}

int Ioss::Tet4::number_edges_face(int face) const
{
  // face is 1-based.  0 passed in for all faces.
  assert(face >= 0 && face <= number_faces());
  return Constants::edges_per_face[face];
}

Ioss::IntVector Ioss::Tet4::edge_connectivity(int edge_number) const
{
  assert(edge_number > 0 && edge_number <= Constants::nedge);
  Ioss::IntVector connectivity(Constants::nedgenode);

  for (int i = 0; i < Constants::nedgenode; i++) {
    connectivity[i] = Constants::edge_node_order[edge_number - 1][i];
  }

  return connectivity;
}

Ioss::IntVector Ioss::Tet4::face_connectivity(int face_number) const
{
  assert(face_number > 0 && face_number <= number_faces());
  Ioss::IntVector connectivity(Constants::nodes_per_face[face_number]);

  for (int i = 0; i < Constants::nodes_per_face[face_number]; i++) {
    connectivity[i] = Constants::face_node_order[face_number - 1][i];
  }

  return connectivity;
}

Ioss::IntVector Ioss::Tet4::element_connectivity() const
{
  Ioss::IntVector connectivity(number_nodes());
  for (int i = 0; i < number_nodes(); i++) {
    connectivity[i] = i;
  }
  return connectivity;
}

Ioss::ElementTopology *Ioss::Tet4::face_type(int face_number) const
{
  // face_number == 0 returns topology for all faces if
  // all faces are the same topology; otherwise, returns nullptr
  // face_number is 1-based.

  assert(face_number >= 0 && face_number <= number_faces());
  //  return Ioss::ElementTopology::factory("triface3");
  return Ioss::ElementTopology::factory("tri3");
}

Ioss::ElementTopology *Ioss::Tet4::edge_type(int edge_number) const
{
  // edge_number == 0 returns topology for all edges if
  // all edges are the same topology; otherwise, returns nullptr
  // edge_number is 1-based.

  assert(edge_number >= 0 && edge_number <= number_edges());
  return Ioss::ElementTopology::factory("edge2");
}

Ioss::IntVector Ioss::Tet4::face_edge_connectivity(int face_number) const
{
  assert(face_number > 0 && face_number <= Constants::nface);

  int             nface_edge = number_edges_face(face_number);
  Ioss::IntVector fcon(nface_edge);

  for (int i = 0; i < nface_edge; i++) {
    fcon[i] = Constants::face_edge_order[face_number - 1][i];
  }

  return fcon;
}
