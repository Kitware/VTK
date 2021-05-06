// Copyright(C) 1999-2020 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include "Ioss_CodeTypes.h"       // for IntVector
#include "Ioss_ElementTopology.h" // for ElementTopology
#include <Ioss_Beam4.h>
#include <Ioss_ElementVariableType.h> // for ElementVariableType
#include <cassert>                    // for assert
#include <cstddef>                    // for nullptr

//------------------------------------------------------------------------
// Define a variable type for storage of this elements connectivity
namespace Ioss {
  const char *Beam4::name = "bar4";
  class St_Beam4 : public ElementVariableType
  {
  public:
    static void factory() { static St_Beam4 registerThis; }

  protected:
    St_Beam4() : ElementVariableType(Ioss::Beam4::name, 4) {}
  };
} // namespace Ioss
// ========================================================================
namespace {
  struct Constants
  {
    static const int nnode     = 4;
    static const int nedge     = 2;
    static const int nedgenode = 4;
    static const int nface     = 0;
    static const int nfacenode = 0;
    static const int nfaceedge = 0;
  };
} // namespace

void Ioss::Beam4::factory()
{
  static Ioss::Beam4 registerThis;
  Ioss::St_Beam4::factory();
}

Ioss::Beam4::Beam4() : Ioss::ElementTopology(Ioss::Beam4::name, "Beam_4")
{
  Ioss::ElementTopology::alias(Ioss::Beam4::name, "Rod_4_3D");
  Ioss::ElementTopology::alias(Ioss::Beam4::name, "rod4");
  Ioss::ElementTopology::alias(Ioss::Beam4::name, "rod3d4");
  Ioss::ElementTopology::alias(Ioss::Beam4::name, "truss4");
  Ioss::ElementTopology::alias(Ioss::Beam4::name, "beam4");
  Ioss::ElementTopology::alias(Ioss::Beam4::name, "Rod_4_2D");
  Ioss::ElementTopology::alias(Ioss::Beam4::name, "rod2d4");
}

Ioss::Beam4::~Beam4() = default;

int Ioss::Beam4::parametric_dimension() const { return 1; }
int Ioss::Beam4::spatial_dimension() const { return 3; }
int Ioss::Beam4::order() const { return 3; }

int Ioss::Beam4::number_corner_nodes() const { return 2; }
int Ioss::Beam4::number_nodes() const { return Constants::nnode; }
int Ioss::Beam4::number_edges() const { return Constants::nedge; }
int Ioss::Beam4::number_faces() const { return Constants::nface; }

int Ioss::Beam4::number_nodes_edge(int /* edge */) const { return Constants::nedgenode; }

int Ioss::Beam4::number_nodes_face(int face) const
{
  // face is 1-based.  0 passed in for all faces.
  assert(face >= 0 && face <= number_faces());
  return Constants::nfacenode;
}

int Ioss::Beam4::number_edges_face(int face) const
{
  // face is 1-based.  0 passed in for all faces.
  assert(face >= 0 && face <= number_faces());
  return Constants::nfaceedge;
}

Ioss::IntVector Ioss::Beam4::edge_connectivity(int edge_number) const
{
  Ioss::IntVector connectivity(Constants::nedgenode);
  if (edge_number == 1) {
    connectivity[0] = 0;
    connectivity[1] = 1;
    connectivity[2] = 2;
  }
  else {
    connectivity[0] = 1;
    connectivity[1] = 0;
    connectivity[2] = 2;
  }
  return connectivity;
}

Ioss::IntVector Ioss::Beam4::face_connectivity(int /* face_number */) const
{
  Ioss::IntVector connectivity;
  return connectivity;
}

Ioss::IntVector Ioss::Beam4::element_connectivity() const
{
  Ioss::IntVector connectivity(number_nodes());
  for (int i = 0; i < number_nodes(); i++) {
    connectivity[i] = i;
  }
  return connectivity;
}

Ioss::ElementTopology *Ioss::Beam4::face_type(int /* face_number */) const
{
  return (Ioss::ElementTopology *)nullptr;
}

Ioss::ElementTopology *Ioss::Beam4::edge_type(int /* edge_number */) const
{
  return Ioss::ElementTopology::factory("edge4");
}
