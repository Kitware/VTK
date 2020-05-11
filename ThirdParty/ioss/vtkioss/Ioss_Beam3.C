// Copyright(C) 1999-2017 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials provided
//       with the distribution.
//
//     * Neither the name of NTESS nor the names of its
//       contributors may be used to endorse or promote products derived
//       from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "Ioss_CodeTypes.h"       // for IntVector
#include "Ioss_ElementTopology.h" // for ElementTopology
#include <Ioss_Beam3.h>
#include <Ioss_ElementVariableType.h> // for ElementVariableType
#include <cassert>                    // for assert
#include <cstddef>                    // for nullptr

//------------------------------------------------------------------------
// Define a variable type for storage of this elements connectivity
namespace Ioss {
  const char *Beam3::name = "bar3";
  class St_Beam3 : public ElementVariableType
  {
  public:
    static void factory() { static St_Beam3 registerThis; }

  protected:
    St_Beam3() : ElementVariableType(Ioss::Beam3::name, 3) {}
  };
} // namespace Ioss
// ========================================================================
namespace {
  struct Constants
  {
    static const int nnode     = 3;
    static const int nedge     = 2;
    static const int nedgenode = 3;
    static const int nface     = 0;
    static const int nfacenode = 0;
    static const int nfaceedge = 0;
  };
} // namespace

void Ioss::Beam3::factory()
{
  static Ioss::Beam3 registerThis;
  Ioss::St_Beam3::factory();
}

Ioss::Beam3::Beam3() : Ioss::ElementTopology(Ioss::Beam3::name, "Beam_3")
{
  Ioss::ElementTopology::alias(Ioss::Beam3::name, "Rod_3_3D");
  Ioss::ElementTopology::alias(Ioss::Beam3::name, "rod3");
  Ioss::ElementTopology::alias(Ioss::Beam3::name, "rod3d3");
  Ioss::ElementTopology::alias(Ioss::Beam3::name, "truss3");
  Ioss::ElementTopology::alias(Ioss::Beam3::name, "beam3");
  Ioss::ElementTopology::alias(Ioss::Beam3::name, "Rod_3_2D");
  Ioss::ElementTopology::alias(Ioss::Beam3::name, "rod2d3");
}

Ioss::Beam3::~Beam3() = default;

int Ioss::Beam3::parametric_dimension() const { return 1; }
int Ioss::Beam3::spatial_dimension() const { return 3; }
int Ioss::Beam3::order() const { return 2; }

int Ioss::Beam3::number_corner_nodes() const { return 2; }
int Ioss::Beam3::number_nodes() const { return Constants::nnode; }
int Ioss::Beam3::number_edges() const { return Constants::nedge; }
int Ioss::Beam3::number_faces() const { return Constants::nface; }

int Ioss::Beam3::number_nodes_edge(int /* edge */) const { return Constants::nedgenode; }

int Ioss::Beam3::number_nodes_face(int face) const
{
  // face is 1-based.  0 passed in for all faces.
  assert(face >= 0 && face <= number_faces());
  return Constants::nfacenode;
}

int Ioss::Beam3::number_edges_face(int face) const
{
  // face is 1-based.  0 passed in for all faces.
  assert(face >= 0 && face <= number_faces());
  return Constants::nfaceedge;
}

Ioss::IntVector Ioss::Beam3::edge_connectivity(int edge_number) const
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

Ioss::IntVector Ioss::Beam3::face_connectivity(int /* face_number */) const
{
  Ioss::IntVector connectivity;
  return connectivity;
}

Ioss::IntVector Ioss::Beam3::element_connectivity() const
{
  Ioss::IntVector connectivity(number_nodes());
  for (int i = 0; i < number_nodes(); i++) {
    connectivity[i] = i;
  }
  return connectivity;
}

Ioss::ElementTopology *Ioss::Beam3::face_type(int /* face_number */) const
{
  return (Ioss::ElementTopology *)nullptr;
}

Ioss::ElementTopology *Ioss::Beam3::edge_type(int /* edge_number */) const
{
  return Ioss::ElementTopology::factory("edge3");
}
