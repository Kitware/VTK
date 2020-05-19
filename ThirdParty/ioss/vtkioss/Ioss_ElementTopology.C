// Copyright(C) 1999-2017, 2020 National Technology & Engineering Solutions
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

#include <Ioss_CodeTypes.h> // for IntVector
#include <Ioss_ElementTopology.h>
#include <Ioss_Super.h> // for Super
#include <Ioss_Utils.h>

#include <cassert> // for assert
#include <cstddef> // for size_t
#include <fmt/ostream.h>
#include <ostream> // for basic_ostream, etc
#include <string>  // for string, char_traits, etc
#include <utility> // for pair
#include <vector>  // for vector

void Ioss::ETRegistry::insert(const Ioss::ETM_VP &value, bool delete_me)
{
  m_registry.insert(value);
  if (delete_me) {
    m_deleteThese.push_back(value.second);
  }
}

Ioss::ETRegistry::~ETRegistry()
{
  for (auto &entry : m_deleteThese) {
    delete entry;
  }
}

// ========================================================================
Ioss::ElementTopology::ElementTopology(std::string type, std::string master_elem_name,
                                       bool delete_me)
    : name_(std::move(type)), masterElementName_(std::move(master_elem_name))
{
  registry().insert(Ioss::ETM_VP(name_, this), delete_me);
  std::string lname = Ioss::Utils::lowercase(name_);
  if (lname != name_) {
    alias(name_, lname);
  }
  alias(name_, masterElementName_);
}

void Ioss::ElementTopology::alias(const std::string &base, const std::string &syn)
{
  registry().insert(Ioss::ETM_VP(syn, factory(base)), false);
  std::string lsyn = Ioss::Utils::lowercase(syn);
  if (lsyn != syn) {
    alias(base, lsyn);
  }
}

Ioss::ETRegistry &Ioss::ElementTopology::registry()
{
  static ETRegistry registry_;
  return registry_;
}

Ioss::ElementTopology::~ElementTopology() = default;

bool Ioss::ElementTopology::edges_similar() const { return true; }
bool Ioss::ElementTopology::faces_similar() const { return true; }

Ioss::ElementTopology *Ioss::ElementTopology::factory(const std::string &type, bool ok_to_fail)
{
  std::string ltype = Ioss::Utils::lowercase(type);

  Ioss::ElementTopology *inst = nullptr;
  auto                   iter = registry().find(ltype);

  if (iter == registry().end()) {
    std::string base1 = "super";
    if (ltype.compare(0, 5, base1) == 0) {
      // A super element can have a varying number of nodes.  Create
      // an IO element type for this super element. The node count
      // should be encoded in the 'type' as 'super42' for a 42-node
      // superelement.

      Ioss::Super::make_super(ltype);
      iter = registry().find(ltype);
    }
    else {
      // See if 'type' contains a '-'.  Some codes create their
      // own topologies by adding a "-something" onto the end of a
      // standard topology.
      size_t dash = ltype.find('-');
      if (dash != std::string::npos) {
        std::string sub_type = ltype.substr(0, dash);
        iter                 = registry().find(sub_type);
      }
    }
  }

  if (iter == registry().end()) {
    if (!ok_to_fail) {
      std::ostringstream errmsg;
      fmt::print(errmsg, "ERROR: The topology type '{}' is not supported.", type);
      IOSS_ERROR(errmsg);
    }
  }
  else {
    inst = (*iter).second;
  }
  return inst;
}

Ioss::ElementTopology *Ioss::ElementTopology::factory(unsigned int unique_id)
{
  // Given a unique id obtained from 'get_unique_id', return the
  // topology type that it refers to...
  for (auto &entry : registry()) {
    if (Ioss::Utils::hash(entry.second->name()) == unique_id) {
      return entry.second;
    }
  }
  return nullptr;
}

unsigned int Ioss::ElementTopology::get_unique_id(const std::string &type)
{
  // Return a unique integer id corresponding to this topology type.
  // Basically used to simplify some parallel calculations so they can
  // deal with int instead of strings...
  if (type == "unknown") {
    return 0;
  }
  unsigned int hash_val = 0;
  std::string  ltype    = Ioss::Utils::lowercase(type);
  auto         iter     = registry().find(ltype);
  if (iter == registry().end()) {
    fmt::print(Ioss::WARNING(), "The topology type '{}' is not supported.\n", type);
  }
  else {
    Ioss::ElementTopology *inst = (*iter).second;
    hash_val                    = Ioss::Utils::hash(inst->name());
  }
  return hash_val;
}

/** \brief Get the names of element topologies known to Ioss.
 *
 *  \param[out] names The list of known element topoloty names.
 *  \returns The number of known element topologies.
 */
int Ioss::ElementTopology::describe(NameList *names)
{
  int count = 0;
  for (auto &entry : registry()) {
    names->push_back(entry.first);
    count++;
  }
  return count;
}

Ioss::IntVector Ioss::ElementTopology::face_edge_connectivity(int face_number) const
{
  assert(face_number > 0 && face_number <= number_faces());

  int             nface_edge = number_edges_face(face_number);
  Ioss::IntVector fcon(nface_edge);

  // This works for 2D elements, 3D elements override
  for (int i = 0; i < nface_edge; i++) {
    fcon[i] = i;
  }

  return fcon;
}

Ioss::IntVector Ioss::ElementTopology::element_edge_connectivity() const
{
  int             nedge = number_edges();
  Ioss::IntVector econ(nedge);
  for (int i = 0; i < nedge; i++) {
    econ[i] = i;
  }

  return econ;
}

bool Ioss::ElementTopology::is_alias(const std::string &my_alias) const
{
  std::string low_my_alias = Ioss::Utils::lowercase(my_alias);
  auto        iter         = registry().find(low_my_alias);
  if (iter == registry().end()) {
    return false;
  }
  return this == (*iter).second;
}

bool Ioss::ElementTopology::is_element() const
{
  // NOTE: This is overridden in some derived classes.
  // The definition here is the default if not overridden.
  return spatial_dimension() == parametric_dimension();
}

int Ioss::ElementTopology::number_boundaries() const
{
  if (parametric_dimension() == 3 && spatial_dimension() == 3) {
    return number_faces();
  }

  if (parametric_dimension() == 2 && spatial_dimension() == 2) {
    return number_edges();
  }

  if (parametric_dimension() == 1 && !is_element()) {
    return number_corner_nodes();
  }

  if (is_element()) {
    if (parametric_dimension() == 2) {
      assert(spatial_dimension() == 3);
      // A shell has faces and edges in its boundary...
      return number_faces() + number_edges();
    }
    if (parametric_dimension() == 1) {
      return 2; // For bar/beam/... boundary is nodes; for ShellLine it is edges
    }
  }
  else {
    if (parametric_dimension() == 2) {
      assert(spatial_dimension() == 3);
      return number_edges();
    }
  }
  return 0;
}

Ioss::IntVector Ioss::ElementTopology::boundary_connectivity(int bnd_number) const
{
  if (parametric_dimension() == 3 && spatial_dimension() == 3) {
    return face_connectivity(bnd_number);
  }

  if (parametric_dimension() == 2 && spatial_dimension() == 2) {
    return edge_connectivity(bnd_number);
  }

  if (is_element()) {
    if (parametric_dimension() == 2) {
      assert(spatial_dimension() == 3);
      // A shell has faces and edges in its boundary...
      if (bnd_number > number_faces()) {
        return edge_connectivity(bnd_number - number_faces());
      }
      return face_connectivity(bnd_number);
    }
    if (parametric_dimension() == 1) {
      if (number_edges() > 1) {
        return edge_connectivity(bnd_number);
      }
      else {
        // Spring-type element -- has node as boundary.
        return Ioss::IntVector{bnd_number - 1};
      }
    }
  }
  else {
    if (parametric_dimension() == 2) {
      assert(spatial_dimension() == 3);
      return edge_connectivity(bnd_number);
    }
  }
  return Ioss::IntVector();
}

Ioss::ElementTopology *Ioss::ElementTopology::boundary_type(int bnd_number) const
{
  if (parametric_dimension() == 3 && spatial_dimension() == 3) {
    return face_type(bnd_number);
  }

  if (parametric_dimension() == 2 && spatial_dimension() == 2) {
    return edge_type(bnd_number);
  }

  if (is_element()) {
    if (parametric_dimension() == 2) {
      // A shell has faces and edges in its boundary...
      if (bnd_number == 0) {
        return nullptr;
      }

      assert(spatial_dimension() == 3);
      if (bnd_number > number_faces()) {
        return edge_type(bnd_number - number_faces());
      }
      return face_type(bnd_number);
    }
    if (parametric_dimension() == 1) {
      if (number_edges() > 1) {
        return edge_type(bnd_number);
      }
      else {
        // Spring-type element -- has node as boundary.
        return Ioss::ElementTopology::factory("node");
      }
    }
  }
  else {
    if (parametric_dimension() == 2) {
      assert(spatial_dimension() == 3);
      return edge_type(bnd_number);
    }
  }
  return nullptr;
}
