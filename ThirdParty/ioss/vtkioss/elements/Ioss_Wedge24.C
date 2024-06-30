// Copyright(C) 1999-2021 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include "Ioss_ElementVariableType.h" // for ElementVariableType
#include "Ioss_Wedge24.h"
#include <cassert> // for assert
#include <string>

#include "Ioss_CodeTypes.h"       // for IntVector
#include "Ioss_ElementTopology.h" // for ElementTopology
#include "Ioss_Utils.h"

//------------------------------------------------------------------------
// Define a variable type for storage of this elements connectivity
namespace Ioss {
  const char *Wedge24::name = "wedge24";
  class St_Wedge24 : public ElementVariableType
  {
  public:
    static void factory();

  protected:
    St_Wedge24() : ElementVariableType(Ioss::Wedge24::name, 24) {}
  };
} // namespace Ioss
void Ioss::St_Wedge24::factory() { static Ioss::St_Wedge24 registerThis; }

// ========================================================================
namespace {
  struct Constants
  {
    static const int nnode     = 24;
    static const int nedge     = 9;
    static const int nedgenode = 4;
    static const int nface     = 5;
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
      {{0, 1, 6, 7},   {1, 2, 8, 9},   {2, 0, 10, 11}, {3, 4, 18, 19}, {4, 5, 20, 21},
       {5, 3, 22, 23}, {0, 3, 12, 15}, {1, 4, 13, 16}, {2, 5, 14, 17}};

  // Face numbers are zero-based [0..number_faces)
  int Constants::face_node_order[nface][nfacenode] = // [face][face_node]
      {{0, 1, 4, 3, 6, 7, 13, 16, 19, 18, 15, 12},
       {1, 2, 5, 4, 8, 9, 14, 17, 21, 20, 16, 13},
       {0, 3, 5, 2, 12, 15, 23, 22, 17, 14, 10, 11},
       {0, 2, 1, 11, 10, 9, 8, 7, 6, -1, -1, -1},
       {3, 4, 5, 18, 19, 20, 21, 22, 23, -1, -1, -1}};

  int Constants::face_edge_order[nface][nfaceedge] = // [face][face_edge]
      {{0, 7, 3, 6}, {1, 8, 4, 7}, {6, 5, 8, 2}, {2, 1, 0, -1}, {3, 4, 5, -1}};

  int Constants::nodes_per_face[nface + 1] = {-1, 12, 12, 12, 9, 9};

  int Constants::edges_per_face[nface + 1] = {-1, 4, 4, 4, 3, 3};
} // namespace

void Ioss::Wedge24::factory()
{
  static Ioss::Wedge24 registerThis;
  Ioss::St_Wedge24::factory();
}

Ioss::Wedge24::Wedge24() : Ioss::ElementTopology(Ioss::Wedge24::name, "Wedge_24")
{
  Ioss::ElementTopology::alias(Ioss::Wedge24::name, "Solid_Wedge_24_3D");
}

int Ioss::Wedge24::parametric_dimension() const { return 3; }
int Ioss::Wedge24::spatial_dimension() const { return 3; }
int Ioss::Wedge24::order() const { return 3; }

int Ioss::Wedge24::number_corner_nodes() const { return 6; }
int Ioss::Wedge24::number_nodes() const { return Constants::nnode; }
int Ioss::Wedge24::number_edges() const { return Constants::nedge; }
int Ioss::Wedge24::number_faces() const { return Constants::nface; }

bool Ioss::Wedge24::faces_similar() const { return false; }

int Ioss::Wedge24::number_nodes_edge(int /* edge */) const { return Constants::nedgenode; }

int Ioss::Wedge24::number_nodes_face(int face) const
{
  // face is 1-based.  0 passed in for all faces.
  assert(face >= 0 && face <= number_faces());
  return Constants::nodes_per_face[face];
}

int Ioss::Wedge24::number_edges_face(int face) const
{
  // face is 1-based.  0 passed in for all faces.
  assert(face >= 0 && face <= number_faces());
  return Constants::edges_per_face[face];
}

Ioss::IntVector Ioss::Wedge24::edge_connectivity(int edge_number) const
{
  Ioss::IntVector connectivity(number_nodes_edge(edge_number));

  for (int i = 0; i < number_nodes_edge(edge_number); i++) {
    connectivity[i] = Constants::edge_node_order[edge_number - 1][i];
  }

  return connectivity;
}

Ioss::IntVector Ioss::Wedge24::face_connectivity(int face_number) const
{
  assert(face_number > 0 && face_number <= number_faces());
  Ioss::IntVector connectivity(number_nodes_face(face_number));

  for (int i = 0; i < number_nodes_face(face_number); i++) {
    connectivity[i] = Constants::face_node_order[face_number - 1][i];
  }

  return connectivity;
}

Ioss::IntVector Ioss::Wedge24::element_connectivity() const
{
  Ioss::IntVector connectivity(number_nodes());
  for (int i = 0; i < number_nodes(); i++) {
    connectivity[i] = i;
  }
  return connectivity;
}

Ioss::ElementTopology *Ioss::Wedge24::face_type(int face_number) const
{
  assert(face_number >= 0 && face_number <= number_faces());
  if (face_number == 0) {
    return (Ioss::ElementTopology *)nullptr;
  }
  if (face_number <= 3) {
    return Ioss::ElementTopology::factory("quad12");
  }

  return Ioss::ElementTopology::factory("tri9");
}

Ioss::ElementTopology *Ioss::Wedge24::edge_type(int edge_number) const
{
  assert(edge_number >= 0 && edge_number <= number_edges());
  IOSS_ASSERT_USED(edge_number);
  return Ioss::ElementTopology::factory("edge4");
}

Ioss::IntVector Ioss::Wedge24::face_edge_connectivity(int face_number) const
{
  assert(face_number > 0 && face_number <= Constants::nface);

  int             nface_edge = number_edges_face(face_number);
  Ioss::IntVector fcon(nface_edge);

  for (int i = 0; i < nface_edge; i++) {
    fcon[i] = Constants::face_edge_order[face_number - 1][i];
  }

  return fcon;
}
