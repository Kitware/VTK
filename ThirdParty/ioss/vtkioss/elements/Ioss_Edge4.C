// Copyright(C) 1999-2021 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include "Ioss_Edge4.h"
#include "Ioss_ElementVariableType.h" // for ElementVariableType
#include <cassert>                    // for assert
#include <string>

#include "Ioss_CodeTypes.h"       // for IntVector
#include "Ioss_ElementTopology.h" // for ElementTopology
#include "Ioss_Utils.h"

//------------------------------------------------------------------------
// Define a variable type for storage of this elements connectivity
namespace Ioss {
  const char *Edge4::name = "edge4";
  class St_Edge4 : public ElementVariableType
  {
  public:
    static void factory() { static St_Edge4 registerThis; }

  protected:
    St_Edge4() : ElementVariableType(Ioss::Edge4::name, 4) {}
  };
} // namespace Ioss
// ========================================================================
namespace {
  struct Constants
  {
    static const int nnode     = 4;
    static const int nedge     = 0;
    static const int nedgenode = 0;
    static const int nface     = 0;
    static const int nfacenode = 0;
    static const int nfaceedge = 0;
  };
} // namespace

void Ioss::Edge4::factory()
{
  static Ioss::Edge4 registerThis;
  Ioss::St_Edge4::factory();
}

Ioss::Edge4::Edge4() : Ioss::ElementTopology(Ioss::Edge4::name, "Line_4")
{
  Ioss::ElementTopology::alias(Ioss::Edge4::name, "edge3d4");
  Ioss::ElementTopology::alias(Ioss::Edge4::name, "LINE_4_1D");
}

int Ioss::Edge4::parametric_dimension() const { return 1; }
int Ioss::Edge4::spatial_dimension() const { return 3; }
int Ioss::Edge4::order() const { return 3; }

int Ioss::Edge4::number_corner_nodes() const { return 2; }
int Ioss::Edge4::number_nodes() const { return Constants::nnode; }
int Ioss::Edge4::number_edges() const { return Constants::nedge; }
int Ioss::Edge4::number_faces() const { return Constants::nface; }

int Ioss::Edge4::number_nodes_edge(int /* edge */) const { return Constants::nedgenode; }

int Ioss::Edge4::number_nodes_face(int face) const
{
  // face is 1-based.  0 passed in for all faces.
  assert(face >= 0 && face <= number_faces());
  IOSS_ASSERT_USED(face);
  return Constants::nfacenode;
}

int Ioss::Edge4::number_edges_face(int face) const
{
  // face is 1-based.  0 passed in for all faces.
  assert(face >= 0 && face <= number_faces());
  IOSS_ASSERT_USED(face);
  return Constants::nfaceedge;
}

Ioss::IntVector Ioss::Edge4::edge_connectivity(int /* edge_number */) const
{
  Ioss::IntVector connectivity;
  return connectivity;
}

Ioss::IntVector Ioss::Edge4::face_connectivity(int /* face_number */) const
{
  Ioss::IntVector connectivity;
  return connectivity;
}

Ioss::IntVector Ioss::Edge4::element_connectivity() const
{
  Ioss::IntVector connectivity(number_nodes());
  for (int i = 0; i < number_nodes(); i++) {
    connectivity[i] = i;
  }
  return connectivity;
}

Ioss::ElementTopology *Ioss::Edge4::face_type(int /* face_number */) const
{
  return (Ioss::ElementTopology *)nullptr;
}

Ioss::ElementTopology *Ioss::Edge4::edge_type(int /* edge_number */) const
{
  return Ioss::ElementTopology::factory("node1");
}
