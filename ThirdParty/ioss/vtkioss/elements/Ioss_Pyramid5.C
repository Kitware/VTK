// Copyright(C) 1999-2021 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include "Ioss_ElementVariableType.h" // for ElementVariableType
#include "Ioss_Pyramid5.h"
#include <cassert> // for assert
#include <string>

//------------------------------------------------------------------------
// Define a variable type for storage of this elements connectivity
#include "Ioss_CodeTypes.h"       // for IntVector
#include "Ioss_ElementTopology.h" // for ElementTopology
#include "Ioss_Utils.h"

namespace Ioss {
  const char *Pyramid5::name = "pyramid5";
  class St_Pyramid5 : public ElementVariableType
  {
  public:
    static void factory();

  protected:
    St_Pyramid5() : ElementVariableType(Ioss::Pyramid5::name, 5) {}
  };
} // namespace Ioss

void Ioss::St_Pyramid5::factory() { static Ioss::St_Pyramid5 registerThis; }

// ========================================================================
namespace {
  struct Constants
  {
    static const int nnode     = 5;
    static const int nedge     = 8;
    static const int nedgenode = 2;
    static const int nface     = 5;
    static const int nfacenode = 4;
    static const int nfaceedge = 4;
    static int       edge_node_order[nedge][nedgenode];
    static int       face_node_order[nface][nfacenode];
    static int       face_edge_order[nface][nfaceedge];
    static int       nodes_per_face[nface + 1];
    static int       nodes_per_edge[nedge + 1];
    static int       edges_per_face[nface + 1];
  };

  // Edge numbers are zero-based [0..number_edges)
  int Constants::edge_node_order[nedge][nedgenode] = // [edge][edge_node]
      {
          {0, 1}, {1, 2}, {2, 3}, {3, 0}, {0, 4}, {1, 4}, {2, 4}, {3, 4},
  };

  // Face numbers are zero-based [0..number_faces)
  int Constants::face_node_order[nface][nfacenode] = // [face][face_node]
      {{0, 1, 4, -1}, {1, 2, 4, -1}, {2, 3, 4, -1}, {3, 0, 4, -1}, {0, 3, 2, 1}};

  int Constants::face_edge_order[nface][nfaceedge] = // [face][face_edge]
      {{0, 5, 4, -1}, {1, 6, 5, -1}, {2, 7, 6, -1}, {3, 4, 7, -1}, {3, 2, 1, 0}};

  int Constants::nodes_per_face[nface + 1] = {-1, 3, 3, 3, 3, 4};

  int Constants::nodes_per_edge[nedge + 1] = {2, 2, 2, 2, 2, 2, 2, 2, 2};

  int Constants::edges_per_face[nface + 1] = {-1, 3, 3, 3, 3, 4};
} // namespace

void Ioss::Pyramid5::factory()
{
  static Ioss::Pyramid5 registerThis;
  Ioss::St_Pyramid5::factory();
}

Ioss::Pyramid5::Pyramid5() : Ioss::ElementTopology(Ioss::Pyramid5::name, "Pyramid_5")
{
  Ioss::ElementTopology::alias(Ioss::Pyramid5::name, "pyramid");
  Ioss::ElementTopology::alias(Ioss::Pyramid5::name, "Solid_Pyramid_5_3D");
  Ioss::ElementTopology::alias(Ioss::Pyramid5::name, "pyra5");
}

int Ioss::Pyramid5::parametric_dimension() const { return 3; }
int Ioss::Pyramid5::spatial_dimension() const { return 3; }
int Ioss::Pyramid5::order() const { return 1; }

int Ioss::Pyramid5::number_corner_nodes() const { return Constants::nnode; }
int Ioss::Pyramid5::number_nodes() const { return Constants::nnode; }
int Ioss::Pyramid5::number_edges() const { return Constants::nedge; }
int Ioss::Pyramid5::number_faces() const { return Constants::nface; }

bool Ioss::Pyramid5::faces_similar() const { return false; }

int Ioss::Pyramid5::number_nodes_edge(int /* edge */) const { return Constants::nedgenode; }

int Ioss::Pyramid5::number_nodes_face(int face) const
{
  // face is 1-based.  0 passed in for all faces.
  assert(face >= 0 && face <= number_faces());
  return Constants::nodes_per_face[face];
}

int Ioss::Pyramid5::number_edges_face(int face) const
{
  // face is 1-based.  0 passed in for all faces.
  assert(face >= 0 && face <= number_faces());
  return Constants::edges_per_face[face];
}

Ioss::IntVector Ioss::Pyramid5::edge_connectivity(int edge_number) const
{
  Ioss::IntVector connectivity(Constants::nodes_per_edge[edge_number]);

  for (int i = 0; i < Constants::nodes_per_edge[edge_number]; i++) {
    connectivity[i] = Constants::edge_node_order[edge_number - 1][i];
  }

  return connectivity;
}

Ioss::IntVector Ioss::Pyramid5::face_connectivity(int face_number) const
{
  assert(face_number > 0 && face_number <= number_faces());
  Ioss::IntVector connectivity(Constants::nodes_per_face[face_number]);

  for (int i = 0; i < Constants::nodes_per_face[face_number]; i++) {
    connectivity[i] = Constants::face_node_order[face_number - 1][i];
  }

  return connectivity;
}

Ioss::IntVector Ioss::Pyramid5::element_connectivity() const
{
  Ioss::IntVector connectivity(number_nodes());
  for (int i = 0; i < number_nodes(); i++) {
    connectivity[i] = i;
  }
  return connectivity;
}

Ioss::ElementTopology *Ioss::Pyramid5::face_type(int face_number) const
{
  assert(face_number >= 0 && face_number <= number_faces());
  if (face_number == 0) {
    return (Ioss::ElementTopology *)nullptr;
  }
  if (face_number <= 4) {
    //    return Ioss::ElementTopology::factory("triface3");
    return Ioss::ElementTopology::factory("tri3");
  }

  //    return Ioss::ElementTopology::factory("quadface4");
  return Ioss::ElementTopology::factory("quad4");
}

Ioss::ElementTopology *Ioss::Pyramid5::edge_type(int edge_number) const
{
  assert(edge_number >= 0 && edge_number <= number_edges());
  IOSS_ASSERT_USED(edge_number);
  return Ioss::ElementTopology::factory("edge2");
}

Ioss::IntVector Ioss::Pyramid5::face_edge_connectivity(int face_number) const
{
  assert(face_number > 0 && face_number <= Constants::nface);

  int             nface_edge = number_edges_face(face_number);
  Ioss::IntVector fcon(nface_edge);

  for (int i = 0; i < nface_edge; i++) {
    fcon[i] = Constants::face_edge_order[face_number - 1][i];
  }

  return fcon;
}
