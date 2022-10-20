// Copyright(C) 1999-2022 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#pragma once

#include "ioss_export.h"

#include <Ioss_CodeTypes.h>
#include <assert.h>
#include <limits>
#include <map>    // for map, map<>::value_compare
#include <string> // for string, operator<
#include <vector> // for vector

namespace Ioss {
  class ElementTopology;
  class ElementPermutation;
} // namespace Ioss

namespace Ioss {

  using Ordinal     = uint16_t;
  using Permutation = uint32_t;

  static constexpr Ordinal     InvalidOrdinal     = std::numeric_limits<Ordinal>::max();
  static constexpr Permutation InvalidPermutation = std::numeric_limits<Permutation>::max();

  using ElementPermutationMap = std::map<std::string, ElementPermutation *, std::less<std::string>>;
  using EPM_VP                = ElementPermutationMap::value_type;

  class IOSS_EXPORT EPRegistry
  {
  public:
    void                            insert(const Ioss::EPM_VP &value, bool delete_me);
    ElementPermutationMap::iterator begin() { return m_registry.begin(); }
    ElementPermutationMap::iterator end() { return m_registry.end(); }
    ElementPermutationMap::iterator find(const std::string &type) { return m_registry.find(type); }

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
    ElementPermutation(const ElementPermutation &)            = delete;
    ElementPermutation &operator=(const ElementPermutation &) = delete;

    virtual ~ElementPermutation();

    unsigned num_permutations() const;

    // The number of positive permutations must be less than or equal to the total number of
    // permutations
    unsigned num_positive_permutations() const;

    bool is_positive_polarity(Permutation permutation) const;

    // Permutation type is unsigned so only need to check upper bound
    bool valid_permutation(Permutation permutation) const;

    // For a validated permutation, return the node ordinals
    bool fill_permutation_indices(Permutation           permutation,
                                  std::vector<Ordinal> &nodeOrdinalVector) const;

    // For a given permutation, return the node ordinals
    std::vector<Ordinal> permutation_indices(Permutation permutation) const;

    Permutation num_permutation_nodes() const;

    const std::string &type() const;

    static ElementPermutation *factory(const std::string &type);

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
    static NameList describe();

    bool operator==(const Ioss::ElementPermutation &rhs) const;
    bool operator!=(const Ioss::ElementPermutation &rhs) const;
    bool equal(const Ioss::ElementPermutation &rhs) const;

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
    bool equal_(const Ioss::ElementPermutation &rhs, bool quiet) const;

    std::string                           m_type{};
    Permutation                           m_numPermutations{0};
    Permutation                           m_numPositivePermutations{0};
    Permutation                           m_numPermutationNodes{0};
    std::vector<std::vector<Permutation>> m_permutationNodeOrdinals{};
  };

  class IOSS_EXPORT NullPermutation : public ElementPermutation
  {
  public:
    static const char *name;

    static void factory();
    ~NullPermutation() override              = default;
    NullPermutation(const NullPermutation &) = delete;

  protected:
    NullPermutation();
  };

  class IOSS_EXPORT SpherePermutation : public ElementPermutation
  {
  public:
    static const char *name;

    static void factory();
    ~SpherePermutation() override                = default;
    SpherePermutation(const SpherePermutation &) = delete;

  protected:
    SpherePermutation();
  };

  class IOSS_EXPORT LinePermutation : public ElementPermutation
  {
  public:
    static const char *name;

    static void factory();
    ~LinePermutation() override              = default;
    LinePermutation(const LinePermutation &) = delete;

  protected:
    LinePermutation();
  };

  class IOSS_EXPORT SpringPermutation : public ElementPermutation
  {
  public:
    static const char *name;

    static void factory();
    ~SpringPermutation() override                = default;
    SpringPermutation(const SpringPermutation &) = delete;

  protected:
    SpringPermutation();
  };

  class IOSS_EXPORT TriPermutation : public ElementPermutation
  {
  public:
    static const char *name;

    static void factory();
    ~TriPermutation() override             = default;
    TriPermutation(const TriPermutation &) = delete;

  protected:
    TriPermutation();
  };

  class IOSS_EXPORT QuadPermutation : public ElementPermutation
  {
  public:
    static const char *name;

    static void factory();
    ~QuadPermutation() override              = default;
    QuadPermutation(const QuadPermutation &) = delete;

  protected:
    QuadPermutation();
  };

  class IOSS_EXPORT TetPermutation : public ElementPermutation
  {
  public:
    static const char *name;

    static void factory();
    ~TetPermutation() override             = default;
    TetPermutation(const TetPermutation &) = delete;

  protected:
    TetPermutation();
  };

  class IOSS_EXPORT PyramidPermutation : public ElementPermutation
  {
  public:
    static const char *name;

    static void factory();
    ~PyramidPermutation() override                 = default;
    PyramidPermutation(const PyramidPermutation &) = delete;

  protected:
    PyramidPermutation();
  };

  class IOSS_EXPORT WedgePermutation : public ElementPermutation
  {
  public:
    static const char *name;

    static void factory();
    ~WedgePermutation() override               = default;
    WedgePermutation(const WedgePermutation &) = delete;

  protected:
    WedgePermutation();
  };

  class IOSS_EXPORT HexPermutation : public ElementPermutation
  {
  public:
    static const char *name;

    static void factory();
    ~HexPermutation() override             = default;
    HexPermutation(const HexPermutation &) = delete;

  protected:
    HexPermutation();
  };

  class IOSS_EXPORT SuperPermutation : public ElementPermutation
  {
  public:
    static const char *basename;

    static void make_super(const std::string &type);
    static void factory();
    static void factory(unsigned n);
    ~SuperPermutation() override               = default;
    SuperPermutation(const SuperPermutation &) = delete;

    static std::string get_name(unsigned n);

  protected:
    SuperPermutation();
    explicit SuperPermutation(unsigned n);

    static std::vector<std::vector<Permutation>> get_super_permutations(unsigned n);
  };
} // namespace Ioss
