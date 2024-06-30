// Copyright(C) 1999-2021 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include "Ioss_ElementVariableType.h" // for ElementVariableType
#include "Ioss_Unknown.h"
#include <cassert> // for assert
#include <string>

#include "Ioss_CodeTypes.h"       // for IntVector
#include "Ioss_ElementTopology.h" // for ElementTopology
#include "Ioss_Utils.h"

// ========================================================================
namespace Ioss {
  const char *Unknown::name = "unknown";
  class St_Unknown : public ElementVariableType
  {
  public:
    static void factory();

  protected:
    St_Unknown() : ElementVariableType(Ioss::Unknown::name, 0) {}
  };
} // namespace Ioss
void Ioss::St_Unknown::factory() { static Ioss::St_Unknown registerThis; }

// ========================================================================
namespace {
  struct Constants
  {
    static const int nnode     = 0;
    static const int nedge     = 0;
    static const int nedgenode = 0;
    static const int nface     = 0;
    static const int nfacenode = 0;
  };
} // namespace

void Ioss::Unknown::factory()
{
  static Ioss::Unknown registerThis;
  Ioss::St_Unknown::factory();
}

Ioss::Unknown::Unknown() : Ioss::ElementTopology(Ioss::Unknown::name, Ioss::Unknown::name)
{
  Ioss::ElementTopology::alias(Ioss::Unknown::name, "invalid_topology");
}

int Ioss::Unknown::parametric_dimension() const { return 0; }
int Ioss::Unknown::spatial_dimension() const { return 3; }
int Ioss::Unknown::order() const { return 0; }

int Ioss::Unknown::number_corner_nodes() const { return number_nodes(); }
int Ioss::Unknown::number_nodes() const { return Constants::nnode; }
int Ioss::Unknown::number_edges() const { return Constants::nedge; }
int Ioss::Unknown::number_faces() const { return Constants::nface; }

int Ioss::Unknown::number_nodes_edge(int /* edge */) const { return Constants::nedgenode; }

int Ioss::Unknown::number_nodes_face(int face) const
{
  // face is 1-based.  0 passed in for all faces.
  assert(face >= 0 && face <= number_faces());
  IOSS_ASSERT_USED(face);
  return Constants::nfacenode;
}

int Ioss::Unknown::number_edges_face(int face) const
{
  // face is 1-based.  0 passed in for all faces.
  assert(face >= 0 && face <= number_faces());
  IOSS_ASSERT_USED(face);
  return Constants::nfacenode;
}

Ioss::IntVector Ioss::Unknown::edge_connectivity(int edge_number) const
{
  Ioss::IntVector connectivity;
  assert(edge_number >= 0 && edge_number <= Constants::nedge);
  IOSS_ASSERT_USED(edge_number);
  return connectivity;
}

Ioss::IntVector Ioss::Unknown::face_connectivity(int face_number) const
{
  assert(face_number >= 0 && face_number <= number_faces());
  IOSS_ASSERT_USED(face_number);
  Ioss::IntVector connectivity;
  return connectivity;
}

Ioss::IntVector Ioss::Unknown::element_connectivity() const
{
  Ioss::IntVector connectivity;
  return connectivity;
}

Ioss::ElementTopology *Ioss::Unknown::face_type(int face_number) const
{
  // face_number == 0 returns topology for all faces if
  // all faces are the same topology; otherwise, returns nullptr
  // face_number is 1-based.

  assert(face_number >= 0 && face_number <= number_faces());
  IOSS_ASSERT_USED(face_number);
  return Ioss::ElementTopology::factory(Ioss::Unknown::name);
}

Ioss::ElementTopology *Ioss::Unknown::edge_type(int edge_number) const
{
  // edge_number == 0 returns topology for all edges if
  // all edges are the same topology; otherwise, returns nullptr
  // edge_number is 1-based.

  assert(edge_number >= 0 && edge_number <= number_edges());
  IOSS_ASSERT_USED(edge_number);
  return Ioss::ElementTopology::factory(Ioss::Unknown::name);
}
