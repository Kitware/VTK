// Copyright(C) 1999-2020 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include "Ioss_CodeTypes.h"           // for IntVector
#include "Ioss_ElementTopology.h"     // for ElementTopology
#include <Ioss_ElementVariableType.h> // for ElementVariableType
#include <Ioss_Sphere.h>
#include <cassert> // for assert
#include <cstddef> // for nullptr

//------------------------------------------------------------------------
// Define a variable type for storage of this elements connectivity
namespace Ioss {
  const char *Sphere::name = "sphere";
  class St_Sphere : public ElementVariableType
  {
  public:
    static void factory() { static St_Sphere registerThis; }

  protected:
    St_Sphere() : ElementVariableType(Ioss::Sphere::name, 1) {}
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
void Ioss::Sphere::factory()
{
  static Ioss::Sphere registerThis;
  Ioss::St_Sphere::factory();
}

Ioss::Sphere::Sphere() : Ioss::ElementTopology(Ioss::Sphere::name, "Particle")
{
  Ioss::ElementTopology::alias(Ioss::Sphere::name, "sphere1");
  Ioss::ElementTopology::alias(Ioss::Sphere::name, "particle");
  Ioss::ElementTopology::alias(Ioss::Sphere::name, "particles");
  Ioss::ElementTopology::alias(Ioss::Sphere::name, "sphere-mass");
  Ioss::ElementTopology::alias(Ioss::Sphere::name, "Particle_1_3D");
  Ioss::ElementTopology::alias(Ioss::Sphere::name, "Particle_1_2D");
  Ioss::ElementTopology::alias(Ioss::Sphere::name, "circle");
  Ioss::ElementTopology::alias(Ioss::Sphere::name, "circle1");
  Ioss::ElementTopology::alias(Ioss::Sphere::name, "point");
  Ioss::ElementTopology::alias(Ioss::Sphere::name, "point1");
}

Ioss::Sphere::~Sphere() = default;

int Ioss::Sphere::parametric_dimension() const { return 0; }
int Ioss::Sphere::spatial_dimension() const { return 3; }
int Ioss::Sphere::order() const { return 1; }

int Ioss::Sphere::number_corner_nodes() const { return number_nodes(); }
int Ioss::Sphere::number_nodes() const { return Constants::nnode; }
int Ioss::Sphere::number_edges() const { return Constants::nedge; }
int Ioss::Sphere::number_faces() const { return Constants::nface; }

int Ioss::Sphere::number_nodes_edge(int /* edge */) const { return Constants::nedgenode; }

int Ioss::Sphere::number_nodes_face(int face) const
{
  // face is 1-based.  0 passed in for all faces.
  assert(face >= 0 && face <= number_faces());
  return Constants::nfacenode;
}

int Ioss::Sphere::number_edges_face(int face) const
{
  // face is 1-based.  0 passed in for all faces.
  assert(face >= 0 && face <= number_faces());
  return Constants::nfaceedge;
}

Ioss::IntVector Ioss::Sphere::edge_connectivity(int /* edge_number */) const
{
  Ioss::IntVector connectivity;
  return connectivity;
}

Ioss::IntVector Ioss::Sphere::face_connectivity(int face_number) const
{
  assert(face_number > 0 && face_number <= number_faces());
  Ioss::IntVector connectivity;
  return connectivity;
}

Ioss::IntVector Ioss::Sphere::element_connectivity() const
{
  Ioss::IntVector connectivity(number_nodes());
  for (int i = 0; i < number_nodes(); i++) {
    connectivity[i] = i;
  }
  return connectivity;
}

Ioss::ElementTopology *Ioss::Sphere::face_type(int face_number) const
{
  // face_number == 0 returns topology for all faces if
  // all faces are the same topology; otherwise, returns nullptr
  // face_number is 1-based.

  assert(face_number >= 0 && face_number <= number_faces());
  return (Ioss::ElementTopology *)nullptr;
}

Ioss::ElementTopology *Ioss::Sphere::edge_type(int edge_number) const
{
  // edge_number == 0 returns topology for all edges if
  // all edges are the same topology; otherwise, returns nullptr
  // edge_number is 1-based.

  assert(edge_number >= 0 && edge_number <= number_edges());
  return nullptr;
}
