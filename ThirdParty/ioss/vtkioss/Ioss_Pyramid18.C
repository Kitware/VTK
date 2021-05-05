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
#include <Ioss_Pyramid18.h>
#include <cassert> // for assert
#include <cstddef> // for nullptr

namespace Ioss {
  const char *Pyramid18::name = "pyramid18";
  class St_Pyramid18 : public ElementVariableType
  {
  public:
    static void factory();

  protected:
    St_Pyramid18() : ElementVariableType(Ioss::Pyramid18::name, 18) {}
  };
} // namespace Ioss

void Ioss::St_Pyramid18::factory() { static Ioss::St_Pyramid18 registerThis; }

// ========================================================================
namespace {
  struct Constants
  {
    static const int nnode     = 18;
    static const int nedge     = 8;
    static const int nedgenode = 3;
    static const int nface     = 5;
    static const int nfacenode = 9;
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
          {0, 1, 5}, {1, 2, 6}, {2, 3, 7}, {3, 0, 8}, {0, 4, 9}, {1, 4, 10}, {2, 4, 11}, {3, 4, 12},
  };

  // Face numbers are zero-based [0..number_faces)
  int Constants::face_node_order[nface][nfacenode] = // [face][face_node]
      {{0, 1, 4, 5, 10, 9, 14, -1, -1},
       {1, 2, 4, 6, 11, 10, 15, -1, -1},
       {2, 3, 4, 7, 12, 11, 16, -1, -1},
       {3, 0, 4, 8, 9, 12, 17, -1, -1},
       {0, 3, 2, 1, 8, 7, 6, 5, 13}};

  int Constants::face_edge_order[nface][nfaceedge] = // [face][face_edge]
      {{0, 5, 4, -1}, {1, 6, 5, -1}, {2, 7, 6, -1}, {3, 4, 7, -1}, {3, 2, 1, 0}};

  int Constants::nodes_per_face[nface + 1] = {-1, 7, 7, 7, 7, 9};

  int Constants::nodes_per_edge[nedge + 1] = {3, 3, 3, 3, 3, 3, 3, 3, 3};

  int Constants::edges_per_face[nface + 1] = {-1, 3, 3, 3, 3, 4};
} // namespace

void Ioss::Pyramid18::factory()
{
  static Ioss::Pyramid18 registerThis;
  Ioss::St_Pyramid18::factory();
}

Ioss::Pyramid18::Pyramid18() : Ioss::ElementTopology(Ioss::Pyramid18::name, "Pyramid_18")
{
  Ioss::ElementTopology::alias(Ioss::Pyramid18::name, "Solid_Pyramid_18_3D");
  Ioss::ElementTopology::alias(Ioss::Pyramid18::name, "pyra18");
}

Ioss::Pyramid18::~Pyramid18() = default;

int Ioss::Pyramid18::parametric_dimension() const { return 3; }
int Ioss::Pyramid18::spatial_dimension() const { return 3; }
int Ioss::Pyramid18::order() const { return 2; }

int Ioss::Pyramid18::number_corner_nodes() const { return 5; }
int Ioss::Pyramid18::number_nodes() const { return Constants::nnode; }
int Ioss::Pyramid18::number_edges() const { return Constants::nedge; }
int Ioss::Pyramid18::number_faces() const { return Constants::nface; }

bool Ioss::Pyramid18::faces_similar() const { return false; }

int Ioss::Pyramid18::number_nodes_edge(int /* edge */) const { return Constants::nedgenode; }

int Ioss::Pyramid18::number_nodes_face(int face) const
{
  // face is 1-based.  0 passed in for all faces.
  assert(face >= 0 && face <= number_faces());
  return Constants::nodes_per_face[face];
}

int Ioss::Pyramid18::number_edges_face(int face) const
{
  // face is 1-based.  0 passed in for all faces.
  assert(face >= 0 && face <= number_faces());
  return Constants::edges_per_face[face];
}

Ioss::IntVector Ioss::Pyramid18::edge_connectivity(int edge_number) const
{
  Ioss::IntVector connectivity(Constants::nodes_per_edge[edge_number]);

  for (int i = 0; i < Constants::nodes_per_edge[edge_number]; i++) {
    connectivity[i] = Constants::edge_node_order[edge_number - 1][i];
  }

  return connectivity;
}

Ioss::IntVector Ioss::Pyramid18::face_connectivity(int face_number) const
{
  assert(face_number > 0 && face_number <= number_faces());
  Ioss::IntVector connectivity(Constants::nodes_per_face[face_number]);

  for (int i = 0; i < Constants::nodes_per_face[face_number]; i++) {
    connectivity[i] = Constants::face_node_order[face_number - 1][i];
  }

  return connectivity;
}

Ioss::IntVector Ioss::Pyramid18::element_connectivity() const
{
  Ioss::IntVector connectivity(number_nodes());
  for (int i = 0; i < number_nodes(); i++) {
    connectivity[i] = i;
  }
  return connectivity;
}

Ioss::ElementTopology *Ioss::Pyramid18::face_type(int face_number) const
{
  assert(face_number >= 0 && face_number <= number_faces());
  if (face_number == 0) {
    return (Ioss::ElementTopology *)nullptr;
  }
  if (face_number <= 4) {
    //    return Ioss::ElementTopology::factory("triface6");
    return Ioss::ElementTopology::factory("tri7");
  }

  //    return Ioss::ElementTopology::factory("quadface9");
  return Ioss::ElementTopology::factory("quad9");
}

Ioss::ElementTopology *Ioss::Pyramid18::edge_type(int edge_number) const
{
  assert(edge_number >= 0 && edge_number <= number_edges());
  return Ioss::ElementTopology::factory("edge3");
}

Ioss::IntVector Ioss::Pyramid18::face_edge_connectivity(int face_number) const
{
  assert(face_number > 0 && face_number <= Constants::nface);

  int             nface_edge = number_edges_face(face_number);
  Ioss::IntVector fcon(nface_edge);

  for (int i = 0; i < nface_edge; i++) {
    fcon[i] = Constants::face_edge_order[face_number - 1][i];
  }

  return fcon;
}
