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
#include <Ioss_Node.h>
#include <cassert> // for assert
#include <cstddef> // for nullptr

namespace Ioss {
  const char *Node::name = "node";
  class St_Node : public ElementVariableType
  {
  public:
    static void factory() { static St_Node registerThis; }

  protected:
    St_Node() : ElementVariableType(Ioss::Node::name, 1) {}
  };
} // namespace Ioss
// ========================================================================
namespace {
  struct Constants
  {
    static const int nnode     = 1;
    static const int nedge     = 0;
    static const int nedgenode = 0;
    static const int nface     = 0;
    static const int nfacenode = 0;
    static const int nfaceedge = 0;
  };
} // namespace

void Ioss::Node::factory()
{
  static Ioss::Node registerThis;
  Ioss::St_Node::factory();
}

Ioss::Node::Node() : Ioss::ElementTopology(Ioss::Node::name, "Node_0_3D")
{
  Ioss::ElementTopology::alias(Ioss::Node::name, "Node_0_2D");
  Ioss::ElementTopology::alias(Ioss::Node::name, "NODE");
}

Ioss::Node::~Node() = default;

int Ioss::Node::parametric_dimension() const { return 0; }
int Ioss::Node::spatial_dimension() const { return 3; }
int Ioss::Node::order() const { return 1; }

int Ioss::Node::number_corner_nodes() const { return Constants::nnode; }
int Ioss::Node::number_nodes() const { return Constants::nnode; }
int Ioss::Node::number_edges() const { return Constants::nedge; }
int Ioss::Node::number_faces() const { return Constants::nface; }

int Ioss::Node::number_nodes_edge(int /* edge */) const { return Constants::nedgenode; }

int Ioss::Node::number_nodes_face(int face) const
{
  // face is 1-based.  0 passed in for all faces.
  assert(face >= 0 && face <= number_faces());
  return Constants::nfacenode;
}

int Ioss::Node::number_edges_face(int face) const
{
  // face is 1-based.  0 passed in for all faces.
  assert(face >= 0 && face <= number_faces());
  return Constants::nfaceedge;
}

Ioss::IntVector Ioss::Node::edge_connectivity(int /* edge_number */) const
{
  Ioss::IntVector connectivity;
  return connectivity;
}

Ioss::IntVector Ioss::Node::face_connectivity(int face_number) const
{
  assert(face_number > 0 && face_number <= number_faces());
  Ioss::IntVector connectivity;
  return connectivity;
}

Ioss::IntVector Ioss::Node::element_connectivity() const
{
  Ioss::IntVector connectivity(number_nodes());
  for (int i = 0; i < number_nodes(); i++) {
    connectivity[i] = i;
  }
  return connectivity;
}

Ioss::ElementTopology *Ioss::Node::face_type(int face_number) const
{
  // face_number == 0 returns topology for all faces if
  // all faces are the same topology; otherwise, returns nullptr
  // face_number is 1-based.

  assert(face_number >= 0 && face_number <= number_faces());
  return (Ioss::ElementTopology *)nullptr;
}

Ioss::ElementTopology *Ioss::Node::edge_type(int edge_number) const
{
  // edge_number == 0 returns topology for all edges if
  // all edges are the same topology; otherwise, returns nullptr
  // edge_number is 1-based.

  assert(edge_number >= 0 && edge_number <= number_edges());
  return nullptr;
}
