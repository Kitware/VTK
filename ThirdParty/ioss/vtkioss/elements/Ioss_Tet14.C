// Copyright(C) 1999-2021 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include "Ioss_ElementVariableType.h" // for ElementVariableType
#include "Ioss_Tet14.h"
#include <cassert> // for assert
#include <string>

#include "Ioss_CodeTypes.h"       // for IntVector
#include "Ioss_ElementTopology.h" // for ElementTopology
#include "Ioss_Utils.h"

//------------------------------------------------------------------------
// Define a variable type for storage of this elements connectivity
namespace Ioss {
  const char *Tet14::name = "tetra14";
  class St_Tet14 : public ElementVariableType
  {
  public:
    static void factory() { static St_Tet14 registerThis; }

  protected:
    St_Tet14() : ElementVariableType(Ioss::Tet14::name, 14) {}
  };
} // namespace Ioss

// ========================================================================
namespace {
  struct Constants
  {
    static const int nnode     = 14;
    static const int nedge     = 6;
    static const int nedgenode = 3;
    static const int nface     = 4;
    static const int nfacenode = 7;
    static const int nfaceedge = 3;
    static int       edge_node_order[nedge][nedgenode];
    static int       face_node_order[nface][nfacenode];
    static int       face_edge_order[nface][nfaceedge];
    static int       nodes_per_face[nface + 1];
    static int       edges_per_face[nface + 1];
  };

  // Edge numbers are zero-based [0..number_edges)
  int Constants::edge_node_order[nedge][nedgenode] = // [edge][edge_node]
      {{0, 1, 4}, {1, 2, 5}, {2, 0, 6}, {0, 3, 7}, {1, 3, 8}, {2, 3, 9}};

  // Face numbers are zero-based [0..number_faces)
  int Constants::face_node_order[nface][nfacenode] = // [face][face_node]
      {{0, 1, 3, 4, 8, 7, 13},
       {1, 2, 3, 5, 9, 8, 11},
       {0, 3, 2, 7, 9, 6, 12},
       {0, 2, 1, 6, 5, 4, 10}};

  int Constants::face_edge_order[nface][nfaceedge] = // [face][face_edge]
      {{0, 4, 3}, {1, 5, 4}, {3, 5, 2}, {2, 1, 0}};

  // face 0 returns number of nodes for all faces if homogeneous
  //        returns -1 if faces have differing topology
  int Constants::nodes_per_face[nface + 1] = {nfacenode, nfacenode, nfacenode, nfacenode,
                                              nfacenode};

  // face 0 returns number of edges for all faces if homogeneous
  //        returns -1 if faces have differing topology
  int Constants::edges_per_face[nface + 1] = {nfaceedge, nfaceedge, nfaceedge, nfaceedge,
                                              nfaceedge};
} // namespace

void Ioss::Tet14::factory()
{
  static Ioss::Tet14 registerThis;
  Ioss::St_Tet14::factory();
}

Ioss::Tet14::Tet14() : Ioss::ElementTopology(Ioss::Tet14::name, "Tetrahedron_14")
{
  Ioss::ElementTopology::alias(Ioss::Tet14::name, "tet14");
  Ioss::ElementTopology::alias(Ioss::Tet14::name, "Solid_Tet_14_3D");
}

int Ioss::Tet14::parametric_dimension() const { return 3; }
int Ioss::Tet14::spatial_dimension() const { return 3; }
int Ioss::Tet14::order() const { return 2; }

int Ioss::Tet14::number_corner_nodes() const { return 4; }
int Ioss::Tet14::number_nodes() const { return Constants::nnode; }
int Ioss::Tet14::number_edges() const { return Constants::nedge; }
int Ioss::Tet14::number_faces() const { return Constants::nface; }

int Ioss::Tet14::number_nodes_edge(int /* edge */) const { return Constants::nedgenode; }

int Ioss::Tet14::number_nodes_face(int face) const
{
  // face is 1-based.  0 passed in for all faces.
  assert(face >= 0 && face <= number_faces());
  return Constants::nodes_per_face[face];
}

int Ioss::Tet14::number_edges_face(int face) const
{
  // face is 1-based.  0 passed in for all faces.
  assert(face >= 0 && face <= number_faces());
  return Constants::edges_per_face[face];
}

Ioss::IntVector Ioss::Tet14::edge_connectivity(int edge_number) const
{
  assert(edge_number > 0 && edge_number <= Constants::nedge);
  Ioss::IntVector connectivity(Constants::nedgenode);

  for (int i = 0; i < Constants::nedgenode; i++) {
    connectivity[i] = Constants::edge_node_order[edge_number - 1][i];
  }

  return connectivity;
}

Ioss::IntVector Ioss::Tet14::face_connectivity(int face_number) const
{
  assert(face_number > 0 && face_number <= number_faces());
  Ioss::IntVector connectivity(Constants::nodes_per_face[face_number]);

  for (int i = 0; i < Constants::nodes_per_face[face_number]; i++) {
    connectivity[i] = Constants::face_node_order[face_number - 1][i];
  }

  return connectivity;
}

Ioss::IntVector Ioss::Tet14::element_connectivity() const
{
  Ioss::IntVector connectivity(number_nodes());
  for (int i = 0; i < number_nodes(); i++) {
    connectivity[i] = i;
  }
  return connectivity;
}

Ioss::ElementTopology *Ioss::Tet14::face_type(int face_number) const
{
  // face_number == 0 returns topology for all faces if
  // all faces are the same topology; otherwise, returns nullptr
  // face_number is 1-based.

  assert(face_number >= 0 && face_number <= number_faces());
  IOSS_ASSERT_USED(face_number);
  //  return Ioss::ElementTopology::factory("triface6");
  return Ioss::ElementTopology::factory("tri7");
}

Ioss::ElementTopology *Ioss::Tet14::edge_type(int edge_number) const
{
  // edge_number == 0 returns topology for all edges if
  // all edges are the same topology; otherwise, returns nullptr
  // edge_number is 1-based.

  assert(edge_number >= 0 && edge_number <= number_edges());
  IOSS_ASSERT_USED(edge_number);
  return Ioss::ElementTopology::factory("edge3");
}

Ioss::IntVector Ioss::Tet14::face_edge_connectivity(int face_number) const
{
  assert(face_number > 0 && face_number <= Constants::nface);

  int             nface_edge = number_edges_face(face_number);
  Ioss::IntVector fcon(nface_edge);

  for (int i = 0; i < nface_edge; i++) {
    fcon[i] = Constants::face_edge_order[face_number - 1][i];
  }

  return fcon;
}
