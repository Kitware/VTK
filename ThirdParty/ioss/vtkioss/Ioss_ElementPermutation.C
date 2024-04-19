// Copyright(C) 1999-2022 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include <Ioss_CodeTypes.h> // for IntVector
#include <Ioss_ElementPermutation.h>
#include <Ioss_ElementTopology.h>
#include <Ioss_Utils.h>

#include <cassert> // for assert
#include <cstddef> // for size_t
#include <ostream> // for basic_ostream, etc
#include <string>  // for string, char_traits, etc
#include <utility> // for pair
#include <vector>  // for vector

#include "vtk_fmt.h"
#include VTK_FMT(fmt/ostream.h)

namespace Ioss {
  void EPRegistry::insert(const Ioss::EPM_VP &value, bool delete_me)
  {
    m_registry.insert(value);
    if (delete_me) {
      m_deleteThese.push_back(value.second);
    }
  }

  EPRegistry::~EPRegistry()
  {
    for (auto &entry : m_deleteThese) {
      delete entry;
    }
  }

  //=========================================================================================
  ElementPermutation::ElementPermutation(std::string type, bool delete_me) : m_type(std::move(type))
  {
    registry().insert(EPM_VP(Ioss::Utils::lowercase(m_type), this), delete_me);
  }

  EPRegistry &ElementPermutation::registry()
  {
    static EPRegistry registry_;
    return registry_;
  }

  Ioss::ElementPermutation::~ElementPermutation() = default;

  ElementPermutation *Ioss::ElementPermutation::factory(const std::string &type)
  {
    std::string ltype = Ioss::Utils::lowercase(type);

    Ioss::ElementPermutation *inst = nullptr;
    auto                      iter = registry().find(ltype);

    if (iter == registry().end()) {
      std::string base1 = Ioss::SuperPermutation::basename;
      if (ltype.compare(0, base1.length(), base1) == 0) {
        // A ring permutation can have a varying number of nodes.  Create
        // a permutation type for this ring permutation. The node count
        // should be encoded in the 'type' as '[base1]42' for a 42-node
        // ring permutation.

        Ioss::SuperPermutation::make_super(ltype);
        iter = registry().find(ltype);
      }
    }

    if (iter == registry().end()) {
      std::ostringstream errmsg;
      fmt::print(errmsg, "ERROR: The permutation type '{}' is not supported.", type);
      IOSS_ERROR(errmsg);
    }
    else {
      inst = (*iter).second;
    }
    return inst;
  }

  NameList ElementPermutation::describe()
  {
    Ioss::NameList names;
    describe(&names);
    return names;
  }

  int ElementPermutation::describe(NameList *names)
  {
    int count = 0;
    for (auto &entry : registry()) {
      names->push_back(entry.first);
      count++;
    }
    return count;
  }

  const std::string &ElementPermutation::type() const { return m_type; }

  unsigned ElementPermutation::num_permutations() const { return m_numPermutations; }

  unsigned ElementPermutation::num_positive_permutations() const
  {
    return m_numPositivePermutations;
  }

  bool ElementPermutation::is_positive_polarity(Permutation permutation) const
  {
    return permutation < m_numPositivePermutations;
  }

  bool ElementPermutation::valid_permutation(Permutation permutation) const
  {
    return permutation < m_numPermutations;
  }

  bool ElementPermutation::fill_permutation_indices(Permutation           permutation,
                                                    std::vector<Ordinal> &nodeOrdinalVector) const
  {
    if (!valid_permutation(permutation))
      return false;

    nodeOrdinalVector.resize(num_permutation_nodes());
    const auto &ordinals = m_permutationNodeOrdinals[permutation];
    for (unsigned i = 0; i < num_permutation_nodes(); i++) {
      nodeOrdinalVector[i] = ordinals[i];
    }

    return true;
  }

  std::vector<Ordinal> ElementPermutation::permutation_indices(Permutation permutation) const
  {
    std::vector<Ordinal> nodeOrdinalVector;
    fill_permutation_indices(permutation, nodeOrdinalVector);
    return nodeOrdinalVector;
  }

  Permutation ElementPermutation::num_permutation_nodes() const { return m_numPermutationNodes; }

  void ElementPermutation::set_permutation(
      Permutation numPermutationNodes_, Permutation numPermutations_,
      Permutation                                  numPositivePermutations_,
      const std::vector<std::vector<Permutation>> &permutationNodeOrdinals_)
  {
    assert(permutationNodeOrdinals_.size() == numPermutations_);
    assert(numPositivePermutations_ <= numPermutations_);

    m_numPermutations         = numPermutations_;
    m_numPositivePermutations = numPositivePermutations_;
    m_numPermutationNodes     = numPermutationNodes_;

    for (const auto &ordinals : permutationNodeOrdinals_) {
      if (ordinals.size() != numPermutationNodes_) {
        std::ostringstream errmsg;
        fmt::print(errmsg,
                   "ERROR: Number of low order permutation ordinals: {} for permutation: {} does "
                   "not match permutation value: {}",
                   ordinals.size(), type(), numPermutationNodes_);
        IOSS_ERROR(errmsg);
      }

      for (const auto ordinal : ordinals) {
        if (ordinal >= numPermutationNodes_) {
          std::ostringstream errmsg;
          fmt::print(errmsg, "ERROR: Invalid value of ordinal: {} for permutation: {}", ordinal,
                     numPermutationNodes_);
          IOSS_ERROR(errmsg);
        }
      }
    }

    m_permutationNodeOrdinals = permutationNodeOrdinals_;
  }

  bool ElementPermutation::equal_(const ElementPermutation &rhs, bool quiet) const
  {
    if (this->m_type.compare(rhs.m_type) != 0) {
      if (!quiet) {
        fmt::print(Ioss::OUTPUT(), "Element Permutation: NAME mismatch ({} vs. {})\n",
                   this->m_type.c_str(), rhs.m_type.c_str());
      }
      return false;
    }

    if (this->m_numPermutations != rhs.m_numPermutations) {
      if (!quiet) {
        fmt::print(Ioss::OUTPUT(), "Element Permutation: NUM PERMUTATION mismatch ({} vs. {})\n",
                   this->m_numPermutations, rhs.m_numPermutations);
      }
      return false;
    }

    if (this->m_numPositivePermutations != rhs.m_numPositivePermutations) {
      if (!quiet) {
        fmt::print(Ioss::OUTPUT(),
                   "Element Permutation: NUM POSITIVE PERMUTATION mismatch ({} vs. {})\n",
                   this->m_numPositivePermutations, rhs.m_numPositivePermutations);
      }
      return false;
    }

    if (this->m_numPermutationNodes != rhs.m_numPermutationNodes) {
      if (!quiet) {
        fmt::print(Ioss::OUTPUT(),
                   "Element Permutation: NUM PERMUTATION NODES mismatch ({} vs. {})\n",
                   this->m_numPermutationNodes, rhs.m_numPermutationNodes);
      }
      return false;
    }

    if (this->m_permutationNodeOrdinals != rhs.m_permutationNodeOrdinals) {
      if (!quiet) {
        fmt::print(Ioss::OUTPUT(), "Element Permutation: PERMUTATION NODE ORDINALS mismatch\n");
      }
      return false;
    }

    return true;
  }

  bool ElementPermutation::operator==(const ElementPermutation &rhs) const
  {
    return equal_(rhs, true);
  }

  bool ElementPermutation::operator!=(const ElementPermutation &rhs) const
  {
    return !(*this == rhs);
  }

  bool ElementPermutation::equal(const ElementPermutation &rhs) const { return equal_(rhs, false); }

  //====================================================================================================
  const char *NullPermutation::name = "none";

  void NullPermutation::factory() { static NullPermutation registerThis; }

  NullPermutation::NullPermutation() : ElementPermutation(NullPermutation::name)
  {
    set_permutation(0, 0, 0, {});
  }

  //====================================================================================================
  const char *SpherePermutation::name = "sphere";

  void SpherePermutation::factory() { static SpherePermutation registerThis; }

  SpherePermutation::SpherePermutation() : ElementPermutation(SpherePermutation::name)
  {
    set_permutation(1, 1, 1, {{0}});
  }

  //====================================================================================================
  const char *LinePermutation::name = "line";

  void LinePermutation::factory() { static LinePermutation registerThis; }

  LinePermutation::LinePermutation() : ElementPermutation(LinePermutation::name)
  {
    set_permutation(2, 2, 1, {{0, 1}, {1, 0}});
  }

  //====================================================================================================
  const char *SpringPermutation::name = "spring";

  void SpringPermutation::factory() { static SpringPermutation registerThis; }

  SpringPermutation::SpringPermutation() : ElementPermutation(SpringPermutation::name)
  {
    set_permutation(2, 2, 2, {{0, 1}, {1, 0}});
  }

  //====================================================================================================
  const char *TriPermutation::name = "tri";

  void TriPermutation::factory() { static TriPermutation registerThis; }

  TriPermutation::TriPermutation() : ElementPermutation(TriPermutation::name)
  {
    set_permutation(3, 6, 3, {{0, 1, 2}, {2, 0, 1}, {1, 2, 0}, {0, 2, 1}, {2, 1, 0}, {1, 0, 2}});
  }

  //====================================================================================================
  const char *QuadPermutation::name = "quad";

  void QuadPermutation::factory() { static QuadPermutation registerThis; }

  QuadPermutation::QuadPermutation() : ElementPermutation(QuadPermutation::name)
  {
    set_permutation(4, 8, 4,
                    {{0, 1, 2, 3},
                     {3, 0, 1, 2},
                     {2, 3, 0, 1},
                     {1, 2, 3, 0},
                     {0, 3, 2, 1},
                     {3, 2, 1, 0},
                     {2, 1, 0, 3},
                     {1, 0, 3, 2}});
  }

  //====================================================================================================
  const char *TetPermutation::name = "tet";

  void TetPermutation::factory() { static TetPermutation registerThis; }

  TetPermutation::TetPermutation() : ElementPermutation(TetPermutation::name)
  {
    set_permutation(4, 12, 12,
                    {{0, 1, 2, 3},
                     {1, 2, 0, 3},
                     {2, 0, 1, 3},
                     {0, 3, 1, 2},
                     {3, 1, 0, 2},
                     {1, 0, 3, 2},
                     {0, 2, 3, 1},
                     {2, 3, 0, 1},
                     {3, 0, 2, 1},
                     {1, 3, 2, 0},
                     {3, 2, 1, 0},
                     {2, 1, 3, 0}});
  }

  //====================================================================================================
  const char *PyramidPermutation::name = "pyramid";

  void PyramidPermutation::factory() { static PyramidPermutation registerThis; }

  PyramidPermutation::PyramidPermutation() : ElementPermutation(PyramidPermutation::name)
  {
    set_permutation(5, 4, 4, {{0, 1, 2, 3, 4}, {1, 2, 3, 0, 4}, {2, 3, 0, 1, 4}, {3, 0, 1, 2, 4}});
  }

  //====================================================================================================
  const char *WedgePermutation::name = "wedge";

  void WedgePermutation::factory() { static WedgePermutation registerThis; }

  WedgePermutation::WedgePermutation() : ElementPermutation(WedgePermutation::name)
  {
    set_permutation(6, 6, 6,
                    {{0, 1, 2, 3, 4, 5},
                     {1, 2, 0, 4, 5, 3},
                     {2, 0, 1, 5, 3, 4},
                     {3, 5, 4, 0, 2, 1},
                     {5, 4, 3, 2, 1, 0},
                     {4, 3, 5, 1, 0, 2}});
  }

  //====================================================================================================
  const char *HexPermutation::name = "hex";

  void HexPermutation::factory() { static HexPermutation registerThis; }

  HexPermutation::HexPermutation() : ElementPermutation(HexPermutation::name)
  {
    set_permutation(8, 24, 24,
                    {{0, 1, 2, 3, 4, 5, 6, 7}, {0, 1, 5, 4, 3, 2, 6, 7}, {0, 4, 7, 3, 1, 5, 6, 2},
                     {1, 2, 3, 0, 5, 6, 7, 4}, {1, 2, 6, 5, 0, 3, 7, 4}, {1, 5, 4, 0, 2, 6, 7, 3},
                     {2, 3, 0, 1, 6, 7, 4, 5}, {2, 3, 7, 6, 1, 0, 4, 5}, {2, 6, 5, 1, 3, 7, 4, 0},
                     {3, 0, 1, 2, 7, 4, 5, 6}, {3, 0, 4, 7, 2, 1, 5, 6}, {3, 7, 6, 2, 0, 4, 5, 1},
                     {4, 0, 1, 5, 7, 3, 2, 6}, {4, 7, 3, 0, 5, 6, 2, 1}, {4, 7, 6, 5, 0, 3, 2, 1},
                     {5, 1, 2, 6, 4, 0, 3, 7}, {5, 4, 0, 1, 6, 7, 3, 2}, {5, 4, 7, 6, 1, 0, 3, 2},
                     {6, 2, 3, 7, 5, 1, 0, 4}, {6, 5, 1, 2, 7, 4, 0, 3}, {6, 5, 4, 7, 2, 1, 0, 3},
                     {7, 3, 0, 4, 6, 2, 1, 5}, {7, 6, 2, 3, 4, 5, 1, 0}, {7, 6, 5, 4, 3, 2, 1, 0}});
  }

  //====================================================================================================
  // Permutation based on round-robin labeling i,e "ring" permutation
  // Super permutation with 4 nodes will have the following positive permutations
  // {0, 1, 2, 3}, {1, 2, 3, 0}, {2, 3, 0, 1}, {3, 0, 1, 2}
  // and the following negative permutations
  // {0, 3, 2, 1}, {3, 2, 1, 0}, {2, 1, 0, 3}, {1, 0, 3, 2}
  const char *SuperPermutation::basename = "super";

  std::string SuperPermutation::get_name(unsigned n) { return basename + std::to_string(n); }

  void SuperPermutation::make_super(const std::string &type)
  {
    // Decode name to determine number of nodes...
    // Assume that digits at end specify number of nodes.
    std::string node_count_str = Ioss::Utils::get_trailing_digits(type);
    if (!node_count_str.empty()) {
      int node_count = std::stoi(node_count_str);
      SuperPermutation::factory(node_count);
    }
  }

  void SuperPermutation::factory() {}

  void SuperPermutation::factory(unsigned n)
  {
    auto iter = registry().find(get_name(n));
    if (iter == registry().end()) {
      new SuperPermutation(n);
    }
  }

  // Make sure the permutation is deleted ... boolean arg is true
  SuperPermutation::SuperPermutation() : ElementPermutation(get_name(0), true)
  {
    set_permutation(0, 0, 0, {});
  }

  // Make sure the permutation is deleted... boolean arg is true
  SuperPermutation::SuperPermutation(unsigned n) : ElementPermutation(get_name(n), true)
  {
    set_permutation(n, 2 * n, n, get_super_permutations(n));
  }

  std::vector<std::vector<Permutation>> SuperPermutation::get_super_permutations(unsigned n)
  {
    std::vector<std::vector<Permutation>> superPerms;

    // Positive permutations
    for (unsigned i = 0; i < n; i++) {
      std::vector<Permutation> perm;

      for (unsigned j = 0; j < n; j++) {
        perm.push_back((i + j) % n);
      }

      superPerms.push_back(perm);
    }

    // Negative permutations
    for (unsigned i = 0; i < n; i++) {
      std::vector<Permutation> perm;

      for (unsigned j = 0; j < n; j++) {
        perm.push_back(((i + n) - j) % n);
      }

      superPerms.push_back(perm);
    }

    return superPerms;
  }

} // namespace Ioss
