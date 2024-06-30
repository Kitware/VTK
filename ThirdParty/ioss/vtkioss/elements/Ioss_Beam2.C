// Copyright(C) 1999-2021 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include "Ioss_Beam2.h"
#include "Ioss_ElementVariableType.h" // for ElementVariableType
#include <cassert>                    // for assert
#include <string>

#include "Ioss_CodeTypes.h"       // for IntVector
#include "Ioss_ElementTopology.h" // for ElementTopology
#include "Ioss_Utils.h"

//------------------------------------------------------------------------
// Define a variable type for storage of this elements connectivity
namespace Ioss {
  const char *Beam2::name = "bar2";
  class St_Beam2 : public ElementVariableType
  {
  public:
    static void factory() { static St_Beam2 registerThis; }

  protected:
    St_Beam2() : ElementVariableType(Ioss::Beam2::name, 2) {}
  };
} // namespace Ioss
// ========================================================================
namespace {
  struct Constants
  {
    static const int nnode     = 2;
    static const int nedge     = 2;
    static const int nedgenode = 2;
    static const int nface     = 0;
    static const int nfacenode = 0;
    static const int nfaceedge = 0;
  };
} // namespace

void Ioss::Beam2::factory()
{
  static Ioss::Beam2 registerThis;
  Ioss::St_Beam2::factory();
}

Ioss::Beam2::Beam2() : Ioss::ElementTopology(Ioss::Beam2::name, "Beam_2")
{
  Ioss::ElementTopology::alias(Ioss::Beam2::name, "Rod_2_3D");
  Ioss::ElementTopology::alias(Ioss::Beam2::name, "rod2");
  Ioss::ElementTopology::alias(Ioss::Beam2::name, "rod");
  Ioss::ElementTopology::alias(Ioss::Beam2::name, "beam2");
  Ioss::ElementTopology::alias(Ioss::Beam2::name, "bar");
  Ioss::ElementTopology::alias(Ioss::Beam2::name, "bar2");
  Ioss::ElementTopology::alias(Ioss::Beam2::name, "truss");
  Ioss::ElementTopology::alias(Ioss::Beam2::name, "truss2");
  Ioss::ElementTopology::alias(Ioss::Beam2::name, "beam");
  Ioss::ElementTopology::alias(Ioss::Beam2::name, "rod3d2");
  Ioss::ElementTopology::alias(Ioss::Beam2::name, "Rod_2_2D");
  Ioss::ElementTopology::alias(Ioss::Beam2::name, "rod2d2");
  Ioss::ElementTopology::alias(Ioss::Beam2::name, "beam-r");
  Ioss::ElementTopology::alias(Ioss::Beam2::name, "beam-r2");
  Ioss::ElementTopology::alias(Ioss::Beam2::name, "line");
  Ioss::ElementTopology::alias(Ioss::Beam2::name, "line2");
  Ioss::ElementTopology::alias(Ioss::Beam2::name, "BEAM_2");
}

int Ioss::Beam2::parametric_dimension() const { return 1; }
int Ioss::Beam2::spatial_dimension() const { return 3; }
int Ioss::Beam2::order() const { return 1; }

int Ioss::Beam2::number_corner_nodes() const { return 2; }
int Ioss::Beam2::number_nodes() const { return Constants::nnode; }
int Ioss::Beam2::number_edges() const { return Constants::nedge; }
int Ioss::Beam2::number_faces() const { return Constants::nface; }

int Ioss::Beam2::number_nodes_edge(int /* edge */) const { return Constants::nedgenode; }

int Ioss::Beam2::number_nodes_face(int face) const
{
  // face is 1-based.  0 passed in for all faces.
  assert(face >= 0 && face <= number_faces());
  IOSS_ASSERT_USED(face);
  return Constants::nfacenode;
}

int Ioss::Beam2::number_edges_face(int face) const
{
  // face is 1-based.  0 passed in for all faces.
  assert(face >= 0 && face <= number_faces());
  IOSS_ASSERT_USED(face);
  return Constants::nfaceedge;
}

Ioss::IntVector Ioss::Beam2::edge_connectivity(int edge_number) const
{
  Ioss::IntVector connectivity(Constants::nedgenode);
  if (edge_number == 1) {
    connectivity[0] = 0;
    connectivity[1] = 1;
  }
  else {
    connectivity[0] = 1;
    connectivity[1] = 0;
  }
  return connectivity;
}

Ioss::IntVector Ioss::Beam2::face_connectivity(int /* face_number */) const
{
  Ioss::IntVector connectivity;
  return connectivity;
}

Ioss::IntVector Ioss::Beam2::element_connectivity() const
{
  Ioss::IntVector connectivity(number_nodes());
  for (int i = 0; i < number_nodes(); i++) {
    connectivity[i] = i;
  }
  return connectivity;
}

Ioss::ElementTopology *Ioss::Beam2::face_type(int /* face_number */) const
{
  return (Ioss::ElementTopology *)nullptr;
}

Ioss::ElementTopology *Ioss::Beam2::edge_type(int /* edge_number */) const
{
  return Ioss::ElementTopology::factory("edge2");
}
