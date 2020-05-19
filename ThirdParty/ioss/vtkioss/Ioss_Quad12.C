// Copyright(C) 2019 National Technology & Engineering Solutions
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

//------------------------------------------------------------------------
// Define a variable type for storage of this elements connectivity
#include "Ioss_CodeTypes.h"           // for IntVector
#include "Ioss_ElementTopology.h"     // for ElementTopology
#include <Ioss_ElementVariableType.h> // for ElementVariableType
#include <Ioss_Quad12.h>
#include <cassert> // for assert
#include <cstddef> // for nullptr

namespace Ioss {
  const char *Quad12::name = "quad12";
  class St_Quad12 : public ElementVariableType
  {
  public:
    static void factory() { static St_Quad12 registerThis; }

  protected:
    St_Quad12() : ElementVariableType(Ioss::Quad12::name, 12) {}
  };
} // namespace Ioss
// ========================================================================
Ioss::Quad12 Ioss::Quad12::instance_;

namespace {
  struct Constants
  {
    static const int nnode     = 12;
    static const int nedge     = 4;
    static const int nedgenode = 4;
    static const int nface     = 0;
    static int       edge_node_order[nedge][nedgenode];
  };

  // Edge numbers are zero-based [0..number_edges)
  int Constants::edge_node_order[nedge][nedgenode] = // [edge][edge_node]
      {{0, 1, 4, 5}, {1, 2, 6, 7}, {2, 3, 8, 9}, {3, 0, 10, 11}};
} // namespace

void Ioss::Quad12::factory()
{
  static Ioss::Quad12 registerThis;
  Ioss::St_Quad12::factory();
}

Ioss::Quad12::Quad12() : Ioss::ElementTopology(Ioss::Quad12::name, "Quadrilateral_12")
{
  Ioss::ElementTopology::alias(Ioss::Quad12::name, "Solid_Quad_12_2D");
  Ioss::ElementTopology::alias(Ioss::Quad12::name, "QUADRILATERAL_12_2D");
  Ioss::ElementTopology::alias(Ioss::Quad12::name, "Face_Quad_12_3D");
  Ioss::ElementTopology::alias(Ioss::Quad12::name, "quadface12");
}

Ioss::Quad12::~Quad12() = default;

int Ioss::Quad12::parametric_dimension() const { return 2; }
int Ioss::Quad12::spatial_dimension() const { return 2; }
int Ioss::Quad12::order() const { return 2; }

int Ioss::Quad12::number_corner_nodes() const { return 4; }
int Ioss::Quad12::number_nodes() const { return Constants::nnode; }
int Ioss::Quad12::number_edges() const { return Constants::nedge; }
int Ioss::Quad12::number_faces() const { return Constants::nface; }

int Ioss::Quad12::number_nodes_edge(int /* edge */) const { return Constants::nedgenode; }

int Ioss::Quad12::number_nodes_face(int /* face */) const { return 0; }
int Ioss::Quad12::number_edges_face(int /* face */) const { return 0; }

Ioss::IntVector Ioss::Quad12::edge_connectivity(int edge_number) const
{
  assert(edge_number > 0 && edge_number <= number_edges());
  Ioss::IntVector connectivity(Constants::nedgenode);
  assert(edge_number > 0 && edge_number <= Constants::nedge);

  for (int i = 0; i < Constants::nedgenode; i++) {
    connectivity[i] = Constants::edge_node_order[edge_number - 1][i];
  }

  return connectivity;
}

Ioss::IntVector Ioss::Quad12::face_connectivity(int /* face_number */) const
{
  Ioss::IntVector connectivity;
  return connectivity;
}

Ioss::IntVector Ioss::Quad12::element_connectivity() const
{
  Ioss::IntVector connectivity(number_nodes());
  for (int i = 0; i < number_nodes(); i++) {
    connectivity[i] = i;
  }
  return connectivity;
}

Ioss::ElementTopology *Ioss::Quad12::face_type(int /* face_number */) const
{
  return (Ioss::ElementTopology *)nullptr;
}

Ioss::ElementTopology *Ioss::Quad12::edge_type(int edge_number) const
{
  assert(edge_number >= 0 && edge_number <= number_edges());
  return Ioss::ElementTopology::factory("edge4");
}
