// Copyright(C) 1999-2021 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include "Ioss_ElementVariableType.h" // for ElementVariableType
#include "Ioss_Wedge15.h"
#include <cassert> // for assert
#include <string>

#include "Ioss_CodeTypes.h"       // for IntVector
#include "Ioss_ElementTopology.h" // for ElementTopology
#include "Ioss_Utils.h"

//------------------------------------------------------------------------
// Define a variable type for storage of this elements connectivity
namespace Ioss {
  const char *Wedge15::name = "wedge15";
  class St_Wedge15 : public ElementVariableType
  {
  public:
    static void factory();

  protected:
    St_Wedge15() : ElementVariableType(Ioss::Wedge15::name, 15) {}
  };
} // namespace Ioss
void Ioss::St_Wedge15::factory() { static Ioss::St_Wedge15 registerThis; }

// ========================================================================
namespace {
  struct Constants
  {
    static const int nnode     = 15;
    static const int nedge     = 9;
    static const int nedgenode = 3;
    static const int nface     = 5;
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
      {{0, 1, 6},  {1, 2, 7}, {2, 0, 8},  {3, 4, 12}, {4, 5, 13},
       {5, 3, 14}, {0, 3, 9}, {1, 4, 10}, {2, 5, 11}};

  // Face numbers are zero-based [0..number_faces)
  int Constants::face_node_order[nface][nfacenode] = // [face][face_node]
      {{0, 1, 4, 3, 6, 10, 12, 9},
       {1, 2, 5, 4, 7, 11, 13, 10},
       {0, 3, 5, 2, 9, 14, 11, 8},
       {0, 2, 1, 8, 7, 6, -1, -1},
       {3, 4, 5, 12, 13, 14, -1, -1}};

  int Constants::face_edge_order[nface][nfaceedge] = // [face][face_edge]
      {{0, 7, 3, 6}, {1, 8, 4, 7}, {6, 5, 8, 2}, {2, 1, 0, -1}, {3, 4, 5, -1}};

  int Constants::nodes_per_face[nface + 1] = {-1, 8, 8, 8, 6, 6};

  int Constants::edges_per_face[nface + 1] = {-1, 4, 4, 4, 3, 3};
} // namespace

void Ioss::Wedge15::factory()
{
  static Ioss::Wedge15 registerThis;
  Ioss::St_Wedge15::factory();
}

Ioss::Wedge15::Wedge15() : Ioss::ElementTopology(Ioss::Wedge15::name, "Wedge_15")
{
  Ioss::ElementTopology::alias(Ioss::Wedge15::name, "Solid_Wedge_15_3D");
}

int Ioss::Wedge15::parametric_dimension() const { return 3; }
int Ioss::Wedge15::spatial_dimension() const { return 3; }
int Ioss::Wedge15::order() const { return 2; }

int Ioss::Wedge15::number_corner_nodes() const { return 6; }
int Ioss::Wedge15::number_nodes() const { return Constants::nnode; }
int Ioss::Wedge15::number_edges() const { return Constants::nedge; }
int Ioss::Wedge15::number_faces() const { return Constants::nface; }

bool Ioss::Wedge15::faces_similar() const { return false; }

int Ioss::Wedge15::number_nodes_edge(int /* edge */) const { return Constants::nedgenode; }

int Ioss::Wedge15::number_nodes_face(int face) const
{
  // face is 1-based.  0 passed in for all faces.
  assert(face >= 0 && face <= number_faces());
  return Constants::nodes_per_face[face];
}

int Ioss::Wedge15::number_edges_face(int face) const
{
  // face is 1-based.  0 passed in for all faces.
  assert(face >= 0 && face <= number_faces());
  return Constants::edges_per_face[face];
}

Ioss::IntVector Ioss::Wedge15::edge_connectivity(int edge_number) const
{
  Ioss::IntVector connectivity(number_nodes_edge(edge_number));

  for (int i = 0; i < number_nodes_edge(edge_number); i++) {
    connectivity[i] = Constants::edge_node_order[edge_number - 1][i];
  }

  return connectivity;
}

Ioss::IntVector Ioss::Wedge15::face_connectivity(int face_number) const
{
  assert(face_number > 0 && face_number <= number_faces());
  Ioss::IntVector connectivity(number_nodes_face(face_number));

  for (int i = 0; i < number_nodes_face(face_number); i++) {
    connectivity[i] = Constants::face_node_order[face_number - 1][i];
  }

  return connectivity;
}

Ioss::IntVector Ioss::Wedge15::element_connectivity() const
{
  Ioss::IntVector connectivity(number_nodes());
  for (int i = 0; i < number_nodes(); i++) {
    connectivity[i] = i;
  }
  return connectivity;
}

Ioss::ElementTopology *Ioss::Wedge15::face_type(int face_number) const
{
  assert(face_number >= 0 && face_number <= number_faces());
  if (face_number == 0) {
    return (Ioss::ElementTopology *)nullptr;
  }
  if (face_number <= 3) {
    //    return Ioss::ElementTopology::factory("quadface8");
    return Ioss::ElementTopology::factory("quad8");
  }

  //    return Ioss::ElementTopology::factory("triface6");
  return Ioss::ElementTopology::factory("tri6");
}

Ioss::ElementTopology *Ioss::Wedge15::edge_type(int edge_number) const
{
  assert(edge_number >= 0 && edge_number <= number_edges());
  IOSS_ASSERT_USED(edge_number);
  return Ioss::ElementTopology::factory("edge3");
}

Ioss::IntVector Ioss::Wedge15::face_edge_connectivity(int face_number) const
{
  assert(face_number > 0 && face_number <= Constants::nface);

  int             nface_edge = number_edges_face(face_number);
  Ioss::IntVector fcon(nface_edge);

  for (int i = 0; i < nface_edge; i++) {
    fcon[i] = Constants::face_edge_order[face_number - 1][i];
  }

  return fcon;
}
