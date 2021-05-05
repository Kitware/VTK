// Copyright(C) 1999-2020 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include "Ioss_CodeTypes.h"           // for IntVector
#include "Ioss_ElementTopology.h"     // for ElementTopology
#include <Ioss_ElementVariableType.h> // for ElementVariableType
#include <Ioss_Spring2.h>
#include <cassert> // for assert
#include <cstddef> // for nullptr

//------------------------------------------------------------------------
// Define a variable type for storage of this elements connectivity
namespace Ioss {
  const char *Spring2::name = "spring2";
  class St_Spring2 : public ElementVariableType
  {
  public:
    static void factory() { static St_Spring2 registerThis; }

  protected:
    St_Spring2() : ElementVariableType(Ioss::Spring2::name, 2) {}
  };
} // namespace Ioss
// ========================================================================
namespace {
  struct Constants
  {
    static const int nnode     = 2;
    static const int nedge     = 0;
    static const int nedgenode = 0;
    static const int nface     = 0;
    static const int nfacenode = 0;
    static const int nfaceedge = 0;
  };
} // namespace

void Ioss::Spring2::factory()
{
  static Ioss::Spring2 registerThis;
  Ioss::St_Spring2::factory();
}

Ioss::Spring2::Spring2() : Ioss::ElementTopology(Ioss::Spring2::name, "Spring_2") {}

Ioss::Spring2::~Spring2() = default;

int Ioss::Spring2::parametric_dimension() const { return 1; }
int Ioss::Spring2::spatial_dimension() const { return 3; }
int Ioss::Spring2::order() const { return 1; }

int Ioss::Spring2::number_corner_nodes() const { return 2; }
int Ioss::Spring2::number_nodes() const { return Constants::nnode; }
int Ioss::Spring2::number_edges() const { return Constants::nedge; }
int Ioss::Spring2::number_faces() const { return Constants::nface; }

int Ioss::Spring2::number_nodes_edge(int /* edge */) const { return Constants::nedgenode; }

int Ioss::Spring2::number_nodes_face(int face) const
{
  // face is 1-based.  0 passed in for all faces.
  assert(face >= 0 && face <= number_faces());
  return Constants::nfacenode;
}

int Ioss::Spring2::number_edges_face(int face) const
{
  // face is 1-based.  0 passed in for all faces.
  assert(face >= 0 && face <= number_faces());
  return Constants::nfaceedge;
}

Ioss::IntVector Ioss::Spring2::edge_connectivity(int /* edge_number */) const
{
  Ioss::IntVector connectivity;
  return connectivity;
}

Ioss::IntVector Ioss::Spring2::face_connectivity(int /* face_number */) const
{
  Ioss::IntVector connectivity;
  return connectivity;
}

Ioss::IntVector Ioss::Spring2::element_connectivity() const
{
  Ioss::IntVector connectivity(number_nodes());
  for (int i = 0; i < number_nodes(); i++) {
    connectivity[i] = i;
  }
  return connectivity;
}

Ioss::ElementTopology *Ioss::Spring2::face_type(int /* face_number */) const
{
  return (Ioss::ElementTopology *)nullptr;
}

Ioss::ElementTopology *Ioss::Spring2::edge_type(int /* edge_number */) const
{
  return (Ioss::ElementTopology *)nullptr;
}
