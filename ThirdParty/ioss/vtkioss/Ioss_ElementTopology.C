// Copyright(C) 1999-2024 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include "Ioss_CodeTypes.h"          // for IntVector
#include "Ioss_ElementPermutation.h" // for ElementPermutation
#include "Ioss_ElementTopology.h"
#include "Ioss_Super.h" // for Super
#include "Ioss_Utils.h"

#include <cassert> // for assert
#include <cstddef> // for size_t
#include "vtk_fmt.h"
#include VTK_FMT(fmt/ostream.h)
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
  for (const auto &entry : m_deleteThese) {
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

  // See if we can recognize an element topology consisting of the first 3 or 4 letters
  // of the name concatenated with the digits at the end of the name (if any)...
  if (iter == registry().end()) {
    auto first_three    = ltype.substr(0, 3);
    auto first_four     = ltype.substr(0, 4);
    auto node_count_str = Ioss::Utils::get_trailing_digits(ltype);
    if (!node_count_str.empty()) {
      first_three += node_count_str;
      first_four += node_count_str;
    }

    iter = registry().find(first_four);
    if (iter == registry().end()) {
      iter = registry().find(first_three);
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
    fmt::print(Ioss::WarnOut(), "The topology type '{}' is not supported.\n", type);
  }
  else {
    Ioss::ElementTopology *inst = (*iter).second;
    hash_val                    = Ioss::Utils::hash(inst->name());
  }
  return hash_val;
}

/** \brief Get the names of element topologies known to Ioss.
 *
 *  \returns The list of known element topology names.
 */
Ioss::NameList Ioss::ElementTopology::describe()
{
  Ioss::NameList names;
  describe(&names);
  return names;
}

/** \brief Get the names of element topologies known to Ioss.
 *
 *  \param[out] names The list of known element topology names.
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
      // Spring-type element -- has node as boundary.
      return Ioss::IntVector{bnd_number - 1};
    }
  }
  else {
    if (parametric_dimension() == 2) {
      assert(spatial_dimension() == 3);
      return edge_connectivity(bnd_number);
    }
    if (parametric_dimension() == 1) {
      // Spring/line-type element -- has node as boundary.
      return Ioss::IntVector{bnd_number - 1};
    }
  }
  return {};
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
      // Spring-type element -- has node as boundary.
      return Ioss::ElementTopology::factory("node");
    }
  }
  else {
    if (parametric_dimension() == 2) {
      assert(spatial_dimension() == 3);
      return edge_type(bnd_number);
    }
    if (parametric_dimension() == 1) {
      assert(spatial_dimension() == 3 || spatial_dimension() == 2);
      return Ioss::ElementTopology::factory("node");
    }
  }
  return nullptr;
}

bool Ioss::ElementTopology::equal_(const Ioss::ElementTopology &rhs, bool quiet) const
{
  if (this->name_ != rhs.name_) {
    if (!quiet) {
      fmt::print(Ioss::OUTPUT(), "Element Topology: NAME mismatch ({} vs. {})\n",
                 this->name_.c_str(), rhs.name_.c_str());
    }
    return false;
  }

  if (this->masterElementName_ != rhs.masterElementName_) {
    if (!quiet) {
      fmt::print(Ioss::OUTPUT(), "Element Topology: MASTER ELEMENT NAME mismatch ({} vs. {})\n",
                 this->masterElementName_.c_str(), rhs.masterElementName_.c_str());
    }
    return false;
  }

  return true;
}

bool Ioss::ElementTopology::operator==(const Ioss::ElementTopology &rhs) const
{
  return equal_(rhs, true);
}

bool Ioss::ElementTopology::operator!=(const Ioss::ElementTopology &rhs) const
{
  return !(*this == rhs);
}

bool Ioss::ElementTopology::equal(const Ioss::ElementTopology &rhs) const
{
  return equal_(rhs, false);
}

Ioss::ElementPermutation *Ioss::ElementTopology::permutation() const
{
  auto *perm = Ioss::ElementPermutation::factory(base_topology_permutation_name());
  assert(perm != nullptr);
  if (validate_permutation_nodes()) {
    if (static_cast<int>(perm->num_permutation_nodes()) != number_corner_nodes()) {
      std::ostringstream errmsg;
      fmt::print(errmsg,
                 "ERROR: The permutation node count: {} for topology '{}' does not match expected "
                 "value: {}.",
                 perm->num_permutation_nodes(), name(), number_corner_nodes());
      IOSS_ERROR(errmsg);
    }
  }
  return perm;
}

const std::string &Ioss::ElementTopology::base_topology_permutation_name() const
{
  return topology_shape_to_permutation_name(shape());
}

const std::string &
Ioss::ElementTopology::topology_shape_to_permutation_name(Ioss::ElementShape topoShape)
{
  static ElementShapeMap shapeToPermutationNameMap_ = {
      {ElementShape::UNKNOWN, "none"},    {ElementShape::POINT, "none"},
      {ElementShape::SPHERE, "sphere"},   {ElementShape::LINE, "line"},
      {ElementShape::SPRING, "spring"},   {ElementShape::TRI, "tri"},
      {ElementShape::QUAD, "quad"},       {ElementShape::TET, "tet"},
      {ElementShape::PYRAMID, "pyramid"}, {ElementShape::WEDGE, "wedge"},
      {ElementShape::HEX, "hex"},         {ElementShape::SUPER, "super"}};

  auto iter = shapeToPermutationNameMap_.find(topoShape);
  if (iter == shapeToPermutationNameMap_.end()) {
    std::ostringstream errmsg;
    fmt::print(errmsg, "ERROR: The topology shape '{}' is not supported.",
               Ioss::Utils::shape_to_string(topoShape));
    IOSS_ERROR(errmsg);
  }

  return iter->second;
}
