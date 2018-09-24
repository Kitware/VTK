#ifndef DIY_PARTNERS_SWAP_HPP
#define DIY_PARTNERS_SWAP_HPP

#include "common.hpp"

namespace diy
{

class Master;

/**
 * \ingroup Communication
 * \brief Partners for swap-reduce
 *
 */
struct RegularSwapPartners: public RegularPartners
{
  typedef       RegularPartners                                 Parent;

                // contiguous parameter indicates whether to match partners contiguously or in a round-robin fashion;
                // contiguous is useful when data needs to be united;
                // round-robin is useful for vector-"halving"
  template<class Decomposer>
                RegularSwapPartners(const Decomposer& decomposer,   //!< domain decomposition
                                    int k,                          //!< target k value
                                    bool contiguous = true          //!< distance halving (true) or doubling (false)
                    ):
                    Parent(decomposer, k, contiguous)         {}
                RegularSwapPartners(const DivisionVector&   divs, //!< explicit division vector
                                    const KVSVector&        kvs,  //!< explicit k vector
                                    bool  contiguous = true       //!< distance halving (true) or doubling (false)
                    ):
                    Parent(divs, kvs, contiguous)               {}

  bool          active(int, int, const Master&) const                                           { return true; }    // in swap-reduce every block is always active

  void          incoming(int round, int gid, std::vector<int>& partners, const Master&) const   { Parent::fill(round - 1, gid, partners); }
  void          outgoing(int round, int gid, std::vector<int>& partners, const Master&) const   { Parent::fill(round, gid, partners); }
};

} // diy

#endif
