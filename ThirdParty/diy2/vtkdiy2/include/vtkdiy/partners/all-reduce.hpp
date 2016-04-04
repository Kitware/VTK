#ifndef DIY_PARTNERS_ALL_REDUCE_HPP
#define DIY_PARTNERS_ALL_REDUCE_HPP

#include "merge.hpp"

namespace diy
{

class Master;

//! Allreduce (reduction with results broadcasted to all blocks) is
//! implemented as two merge reductions, with incoming and outgoing items swapped in second one.
//! Ie, follows merge reduction up and down the merge tree

/**
 * \ingroup Communication
 * \brief Partners for all-reduce
 *
 */
struct RegularAllReducePartners: public RegularMergePartners
{
  typedef       RegularMergePartners                            Parent; //!< base class merge reduction

                //! contiguous parameter indicates whether to match partners contiguously or in a round-robin fashion;
                //! contiguous is useful when data needs to be united;
                //! round-robin is useful for vector-"halving"
  template<class Decomposer>
                RegularAllReducePartners(const Decomposer& decomposer,  //!< domain decomposition
                                         int k,                         //!< target k value
                                         bool contiguous = true         //!< distance doubling (true) or halving (false)
                    ):
                  Parent(decomposer, k, contiguous)         {}
                RegularAllReducePartners(const DivisionVector&   divs,//!< explicit division vector
                                         const KVSVector&        kvs, //!< explicit k vector
                                         bool  contiguous = true      //!< distance doubling (true) or halving (false)
                    ):
                  Parent(divs, kvs, contiguous)               {}

  //! returns total number of rounds
  size_t        rounds() const                                  { return 2*Parent::rounds(); }
  //! returns size of a group of partners in a given round
  int           size(int round) const                           { return Parent::size(parent_round(round)); }
  //! returns dimension (direction of partners in a regular grid) in a given round
  int           dim(int round) const                            { return Parent::dim(parent_round(round)); }
  //! returns whether a given block in a given round has dropped out of the merge yet or not
  inline bool   active(int round, int gid, const Master& m) const { return Parent::active(parent_round(round), gid, m); }
  //! returns what the current round would be in the first or second parent merge reduction
  int           parent_round(int round) const                   { return round < (int) Parent::rounds() ? round : rounds() - round; }

  // incoming is only valid for an active gid; it will only be called with an active gid
  inline void   incoming(int round, int gid, std::vector<int>& partners, const Master& m) const
  {
      if (round <= (int) Parent::rounds())
          Parent::incoming(round, gid, partners, m);
      else
          Parent::outgoing(parent_round(round), gid, partners, m);
  }

  inline void   outgoing(int round, int gid, std::vector<int>& partners, const Master& m) const
  {
      if (round < (int) Parent::rounds())
          Parent::outgoing(round, gid, partners, m);
      else
          Parent::incoming(parent_round(round), gid, partners, m);
  }
};

} // diy

#endif


