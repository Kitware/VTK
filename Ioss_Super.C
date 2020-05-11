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

#include "Ioss_CodeTypes.h"           // for IntVector
#include "Ioss_ElementTopology.h"     // for ElementTopology
#include <Ioss_ElementVariableType.h> // for ElementVariableType
#include <Ioss_Super.h>
#include <cstddef> // for size_t
#include <cstdlib> // for atoi
#include <string>  // for string

//------------------------------------------------------------------------
// Define a variable type for storage of this elements connectivity
namespace Ioss {
  const char *Super::name = "super";
  class St_Super : public ElementVariableType
  {
  public:
    St_Super(const std::string &my_name, int node_count) : ElementVariableType(my_name, node_count)
    {
    }
  };
} // namespace Ioss

void Ioss::Super::factory() {}

// ========================================================================
// Note that since a superelement is created for each node_count, it isn't
// possible to precreate these element types statically, so they are created
// as needed and therefore, they must be deleted at end of run hence the 'true'
// argument to the ElementTopology constructor
Ioss::Super::Super(const std::string &my_name, int node_count)
    : Ioss::ElementTopology(my_name, "Unknown", true), nodeCount(node_count),
      storageType(new St_Super(my_name, node_count))
{
}

Ioss::Super::~Super() { delete storageType; }

void Ioss::Super::make_super(const std::string &type)
{
  // Decode name to determine number of nodes...
  // Assume that digits at end specify number of nodes.
  size_t digits = type.find_last_not_of("0123456789");
  if (digits != std::string::npos) {
    std::string node_count_str = type.substr(digits + 1);
    int         node_count     = std::stoi(node_count_str);
    new Ioss::Super(type, node_count);
  }
}

int Ioss::Super::parametric_dimension() const { return 3; }
int Ioss::Super::spatial_dimension() const { return 3; }
int Ioss::Super::order() const { return 1; }

int Ioss::Super::number_corner_nodes() const { return nodeCount; }
int Ioss::Super::number_nodes() const { return nodeCount; }
int Ioss::Super::number_edges() const { return 0; }
int Ioss::Super::number_faces() const { return 0; }

int Ioss::Super::number_nodes_edge(int /* edge */) const { return 0; }

int Ioss::Super::number_nodes_face(int /*face*/) const { return 0; }
int Ioss::Super::number_edges_face(int /*face*/) const { return 0; }

Ioss::IntVector Ioss::Super::edge_connectivity(int /*edge_number*/) const
{
  Ioss::IntVector connectivity(0);
  return connectivity;
}

Ioss::IntVector Ioss::Super::face_connectivity(int /*face_number*/) const
{
  Ioss::IntVector connectivity(0);
  return connectivity;
}

Ioss::IntVector Ioss::Super::element_connectivity() const
{
  Ioss::IntVector connectivity(number_nodes());
  for (int i = 0; i < number_nodes(); i++) {
    connectivity[i] = i;
  }
  return connectivity;
}

Ioss::ElementTopology *Ioss::Super::face_type(int /*face_number*/) const
{
  return Ioss::ElementTopology::factory("unknown");
}

Ioss::ElementTopology *Ioss::Super::edge_type(int /*edge_number*/) const
{
  return Ioss::ElementTopology::factory("unknown");
}

Ioss::IntVector Ioss::Super::face_edge_connectivity(int /*face_number*/) const
{
  Ioss::IntVector fcon(0);
  return fcon;
}
