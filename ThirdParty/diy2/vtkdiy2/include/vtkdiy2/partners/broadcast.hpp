#ifndef DIY_PARTNERS_BROADCAST_HPP
#define DIY_PARTNERS_BROADCAST_HPP

#include "merge.hpp"

namespace diy
{

class Master;

/**
 * \ingroup Communication
 * \brief Partners for broadcast
 *
 */
struct RegularBroadcastPartners: public RegularMergePartners
{
  typedef       RegularMergePartners                            Parent; //!< base class merge reduction

                //! contiguous parameter indicates whether to match partners contiguously or in a round-robin fashion;
                //! contiguous is useful when data needs to be united;
                //! round-robin is useful for vector-"halving"
  template<class Decomposer>
                RegularBroadcastPartners(const Decomposer& decomposer,  //!< domain decomposition
                                         int k,                         //!< target k value
                                         bool contiguous = true         //!< distance doubling (true) or halving (false)
                    ):
                  Parent(decomposer, k, contiguous)         {}
                RegularBroadcastPartners(const DivisionVector&   divs,//!< explicit division vector
                                         const KVSVector&        kvs, //!< explicit k vector
                                         bool  contiguous = true      //!< distance doubling (true) or halving (false)
                    ):
                  Parent(divs, kvs, contiguous)               {}

  //! returns total number of rounds
  size_t        rounds() const                                  { return Parent::rounds(); }
  //! returns size of a group of partners in a given round
  int           size(int round) const                           { return Parent::size(parent_round(round)); }
  //! returns dimension (direction of partners in a regular grid) in a given round
  int           dim(int round) const                            { return Parent::dim(parent_round(round)); }
  //! returns whether a given block in a given round has dropped out of the merge yet or not
  inline bool   active(int round, int gid, const Master& m) const { return Parent::active(parent_round(round), gid, m); }
  //! returns what the current round would be in the first or second parent merge reduction
  int           parent_round(int round) const                   { return rounds() - round; }

  // incoming is only valid for an active gid; it will only be called with an active gid
  inline void   incoming(int round, int gid, std::vector<int>& partners, const Master& m) const
  {
      Parent::outgoing(parent_round(round), gid, partners, m);
  }

  inline void   outgoing(int round, int gid, std::vector<int>& partners, const Master& m) const
  {
      Parent::incoming(parent_round(round), gid, partners, m);
  }
};

} // diy

#endif


