// Copyright(C) 1999-2025 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#pragma once

#include "Ioss_CodeTypes.h"
#include <assert.h>
#include <limits>
#include <map> // for map, map<>::value_compare
#include <stdint.h>
#include <string> // for string, operator<
#include <vector> // for vector

#include "ioss_export.h"
#include "vtk_ioss_mangle.h"

namespace Ioss {
  class ElementTopology;
  class ElementPermutation;
} // namespace Ioss

namespace Ioss {

  using Ordinal     = uint16_t;
  using Permutation = uint32_t;

  static constexpr Ordinal     InvalidOrdinal     = std::numeric_limits<Ordinal>::max();
  static constexpr Permutation InvalidPermutation = std::numeric_limits<Permutation>::max();

  using ElementPermutationMap = std::map<std::string, ElementPermutation *, std::less<>>;
  using EPM_VP                = ElementPermutationMap::value_type;

  class IOSS_EXPORT EPRegistry
  {
  public:
    void           insert(const Ioss::EPM_VP &value, bool delete_me);
    IOSS_NODISCARD ElementPermutationMap::iterator begin() { return m_registry.begin(); }
    IOSS_NODISCARD ElementPermutationMap::iterator end() { return m_registry.end(); }
    IOSS_NODISCARD ElementPermutationMap::iterator find(const std::string &type)
    {
      return m_registry.find(type);
    }

    ~EPRegistry();

  private:
    Ioss::ElementPermutationMap             m_registry;
    std::vector<Ioss::ElementPermutation *> m_deleteThese;
  };

  // Permutation data is stored such that the positive permutations are listed first ... the order
  // of the positive permutations within that group is irrelevant. The remaining permutations listed
  // after the positive ones are the negative permutations hence, any permutation index outside of
  // the positive range is a negative permutation. By convention, the first permutation listed
  // matches the default listed in the Exodus manual

  class IOSS_EXPORT ElementPermutation
  {
  public:
    IOSS_NODISCARD unsigned num_permutations() const;

    // The number of positive permutations must be less than or equal to the total number of
    // permutations
    IOSS_NODISCARD unsigned num_positive_permutations() const;

    IOSS_NODISCARD bool is_positive_polarity(Permutation permutation) const;

    // Permutation type is unsigned so only need to check upper bound
    IOSS_NODISCARD bool valid_permutation(Permutation permutation) const;

    // For a validated permutation, return the node ordinals
    bool fill_permutation_indices(Permutation           permutation,
                                  std::vector<Ordinal> &nodeOrdinalVector) const;

    // For a given permutation, return the node ordinals
    IOSS_NODISCARD std::vector<Ordinal> permutation_indices(Permutation permutation) const;

    IOSS_NODISCARD Permutation num_permutation_nodes() const;

    IOSS_NODISCARD const std::string &type() const;

    IOSS_NODISCARD static ElementPermutation *factory(const std::string &type);

    /** \brief Get the names of element permutations known to Ioss.
     *
     *  \param[out] names The list of known element topology names.
     *  \returns The number of known element topologies.
     */
    static int describe(NameList *names);

    /** \brief Get the names of element permutations known to Ioss.
     *
     *  \returns The list of known element topology names.
     */
    IOSS_NODISCARD static NameList describe();

    IOSS_NODISCARD bool operator==(const Ioss::ElementPermutation &rhs) const;
    IOSS_NODISCARD bool operator!=(const Ioss::ElementPermutation &rhs) const;
    IOSS_NODISCARD bool equal(const Ioss::ElementPermutation &rhs) const;

  protected:
    explicit ElementPermutation(std::string type, bool delete_me = false);

    // Store low order permutation data regarding this topology .. the positive permutations are
    // listed first If this topology is a high order topology, the data is only for the nodes of the
    // associated low order topology. This implies that any usage of this assumes that the higher
    // order nodes are numbered correctly relative to the low order nodes.
    //
    //        {{0, 1, 2,  3, 4, 5},               {{0, 1, 2},
    //         {2, 0, 1,  5, 3, 4},                {2, 0, 1},
    //  Tri6   {1, 2, 0,  4, 5, 3},   -->    Tri3  {1, 2, 0},
    //         {0, 2, 1,  5, 4, 3},                {0, 2, 1},
    //         {2, 1, 0,  4, 3, 5},                {2, 1, 0},
    //         {1, 0, 2,  3, 5, 4}}                {1, 0, 2}}

    void set_permutation(Permutation numPermutationNodes_, Permutation numPermutations_,
                         Permutation                                  numPositivePermutations_,
                         const std::vector<std::vector<Permutation>> &permutationNodeOrdinals_);

    static EPRegistry &registry();

  private:
    IOSS_NODISCARD bool equal_(const Ioss::ElementPermutation &rhs, bool quiet) const;

    std::string                           m_type{};
    Permutation                           m_numPermutations{0};
    Permutation                           m_numPositivePermutations{0};
    Permutation                           m_numPermutationNodes{0};
    std::vector<std::vector<Permutation>> m_permutationNodeOrdinals{};
  };

  class IOSS_EXPORT NullPermutation : public ElementPermutation
  {
  public:
    static void factory();

  protected:
    NullPermutation();
  };

  class IOSS_EXPORT SpherePermutation : public ElementPermutation
  {
  public:
    static void factory();

  protected:
    SpherePermutation();
  };

  class IOSS_EXPORT LinePermutation : public ElementPermutation
  {
  public:
    static void factory();

  protected:
    LinePermutation();
  };

  class IOSS_EXPORT SpringPermutation : public ElementPermutation
  {
  public:
    static void factory();

  protected:
    SpringPermutation();
  };

  class IOSS_EXPORT TriPermutation : public ElementPermutation
  {
  public:
    static void factory();

  protected:
    TriPermutation();
  };

  class IOSS_EXPORT QuadPermutation : public ElementPermutation
  {
  public:
    static void factory();

  protected:
    QuadPermutation();
  };

  class IOSS_EXPORT TetPermutation : public ElementPermutation
  {
  public:
    static void factory();

  protected:
    TetPermutation();
  };

  class IOSS_EXPORT PyramidPermutation : public ElementPermutation
  {
  public:
    static void factory();

  protected:
    PyramidPermutation();
  };

  class IOSS_EXPORT WedgePermutation : public ElementPermutation
  {
  public:
    static void factory();

  protected:
    WedgePermutation();
  };

  class IOSS_EXPORT HexPermutation : public ElementPermutation
  {
  public:
    static void factory();

  protected:
    HexPermutation();
  };

  class IOSS_EXPORT SuperPermutation : public ElementPermutation
  {
  public:
    static void make_super(const std::string &type);
    static void factory();
    static void factory(unsigned n);

    static std::string get_name(unsigned n);

  protected:
    SuperPermutation();
    explicit SuperPermutation(unsigned n);

    static std::vector<std::vector<Permutation>> get_super_permutations(unsigned n);
  };
} // namespace Ioss
