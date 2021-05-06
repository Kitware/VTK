// Copyright(C) 1999-2020 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include "Ioss_CodeTypes.h"           // for IntVector
#include "Ioss_ElementTopology.h"     // for ElementTopology
#include <Ioss_ElementVariableType.h> // for ElementVariableType
#include <Ioss_Wedge6.h>
#include <cassert> // for assert
#include <cstddef> // for nullptr

//------------------------------------------------------------------------
// Define a variable type for storage of this elements connectivity
namespace Ioss {
  const char *Wedge6::name = "wedge6";
  class St_Wedge6 : public ElementVariableType
  {
  public:
    static void factory();

  protected:
    St_Wedge6() : ElementVariableType(Ioss::Wedge6::name, 6) {}
  };
} // namespace Ioss
void Ioss::St_Wedge6::factory() { static Ioss::St_Wedge6 registerThis; }

// ========================================================================
namespace {
  struct Constants
  {
    static const int nnode     = 6;
    static const int nedge     = 9;
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
      /* (Reference: Fmwk_StdObjMeshTopologies.C) */
      {{0, 1}, {1, 2}, {2, 0}, {3, 4}, {4, 5}, {5, 3}, {0, 3}, {1, 4}, {2, 5}};

  // Face numbers are zero-based [0..number_faces)
  int Constants::face_node_order[nface][nfacenode] = // [face][face_node]
      {{0, 1, 4, 3}, {1, 2, 5, 4}, {0, 3, 5, 2}, {0, 2, 1, -1}, {3, 4, 5, -1}};

  int Constants::face_edge_order[nface][nfaceedge] = // [face][face_edge]
      {{0, 7, 3, 6}, {1, 8, 4, 7}, {6, 5, 8, 2}, {2, 1, 0, -1}, {3, 4, 5, -1}};

  int Constants::nodes_per_face[nface + 1] = {-1, 4, 4, 4, 3, 3};

  int Constants::nodes_per_edge[nedge + 1] = {2, 2, 2, 2, 2, 2, 2, 2, 2, 2};

  int Constants::edges_per_face[nface + 1] = {-1, 4, 4, 4, 3, 3};
} // namespace

void Ioss::Wedge6::factory()
{
  static Ioss::Wedge6 registerThis;
  Ioss::St_Wedge6::factory();
}

Ioss::Wedge6::Wedge6() : Ioss::ElementTopology(Ioss::Wedge6::name, "Wedge_6")
{
  Ioss::ElementTopology::alias(Ioss::Wedge6::name, "wedge");
  Ioss::ElementTopology::alias(Ioss::Wedge6::name, "Solid_Wedge_6_3D");
  Ioss::ElementTopology::alias(Ioss::Wedge6::name, "WEDGE_6");
}

Ioss::Wedge6::~Wedge6() = default;

int Ioss::Wedge6::parametric_dimension() const { return 3; }
int Ioss::Wedge6::spatial_dimension() const { return 3; }
int Ioss::Wedge6::order() const { return 1; }

int Ioss::Wedge6::number_corner_nodes() const { return number_nodes(); }
int Ioss::Wedge6::number_nodes() const { return Constants::nnode; }
int Ioss::Wedge6::number_edges() const { return Constants::nedge; }
int Ioss::Wedge6::number_faces() const { return Constants::nface; }

bool Ioss::Wedge6::faces_similar() const { return false; }

int Ioss::Wedge6::number_nodes_edge(int /* edge */) const { return Constants::nedgenode; }

int Ioss::Wedge6::number_nodes_face(int face) const
{
  // face is 1-based.  0 passed in for all faces.
  assert(face >= 0 && face <= number_faces());
  return Constants::nodes_per_face[face];
}

int Ioss::Wedge6::number_edges_face(int face) const
{
  // face is 1-based.  0 passed in for all faces.
  assert(face >= 0 && face <= number_faces());
  return Constants::edges_per_face[face];
}

Ioss::IntVector Ioss::Wedge6::edge_connectivity(int edge_number) const
{
  Ioss::IntVector connectivity(Constants::nodes_per_edge[edge_number]);

  for (int i = 0; i < Constants::nodes_per_edge[edge_number]; i++) {
    connectivity[i] = Constants::edge_node_order[edge_number - 1][i];
  }

  return connectivity;
}

Ioss::IntVector Ioss::Wedge6::face_connectivity(int face_number) const
{
  assert(face_number > 0 && face_number <= number_faces());
  Ioss::IntVector connectivity(Constants::nodes_per_face[face_number]);

  for (int i = 0; i < Constants::nodes_per_face[face_number]; i++) {
    connectivity[i] = Constants::face_node_order[face_number - 1][i];
  }

  return connectivity;
}

Ioss::IntVector Ioss::Wedge6::element_connectivity() const
{
  Ioss::IntVector connectivity(number_nodes());
  for (int i = 0; i < number_nodes(); i++) {
    connectivity[i] = i;
  }
  return connectivity;
}

Ioss::ElementTopology *Ioss::Wedge6::face_type(int face_number) const
{
  assert(face_number >= 0 && face_number <= number_faces());
  if (face_number == 0) {
    return (Ioss::ElementTopology *)nullptr;
  }
  if (face_number <= 3) {
    //    return Ioss::ElementTopology::factory("quadface4");
    return Ioss::ElementTopology::factory("quad4");
  }

  //    return Ioss::ElementTopology::factory("triface3");
  return Ioss::ElementTopology::factory("tri3");
}

Ioss::ElementTopology *Ioss::Wedge6::edge_type(int edge_number) const
{
  assert(edge_number >= 0 && edge_number <= number_edges());
  return Ioss::ElementTopology::factory("edge2");
}

Ioss::IntVector Ioss::Wedge6::face_edge_connectivity(int face_number) const
{
  assert(face_number > 0 && face_number <= Constants::nface);

  int             nface_edge = number_edges_face(face_number);
  Ioss::IntVector fcon(nface_edge);

  for (int i = 0; i < nface_edge; i++) {
    fcon[i] = Constants::face_edge_order[face_number - 1][i];
  }

  return fcon;
}
