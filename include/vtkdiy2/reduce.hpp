#ifndef DIY_REDUCE_HPP
#define DIY_REDUCE_HPP

#include <vector>
#include "master.hpp"
#include "assigner.hpp"
#include "detail/block_traits.hpp"
#include "log.hpp"

namespace diy
{
//! Enables communication within a group during a reduction.
//! DIY creates the ReduceProxy for you in diy::reduce()
//! and provides a reference to ReduceProxy each time the user's reduction function is called
struct ReduceProxy: public Master::Proxy
{
    typedef     std::vector<int>                            GIDVector;

    ReduceProxy(Master::Proxy&&         proxy, //!< parent proxy
                void*                   block, //!< diy block
                unsigned                round, //!< current round
                const Assigner&         assigner, //!< assigner
                const GIDVector&        incoming_gids, //!< incoming gids in this group
                const GIDVector&        outgoing_gids): //!< outgoing gids in this group
      Master::Proxy(std::move(proxy)),
      block_(block),
      round_(round),
      assigner_(assigner)
    {
      // setup in_link
      for (unsigned i = 0; i < incoming_gids.size(); ++i)
      {
        BlockID nbr;
        nbr.gid  = incoming_gids[i];
        nbr.proc = assigner.rank(nbr.gid);
        in_link_.add_neighbor(nbr);
      }

      // setup out_link
      for (unsigned i = 0; i < outgoing_gids.size(); ++i)
      {
        BlockID nbr;
        nbr.gid  = outgoing_gids[i];
        nbr.proc = assigner.rank(nbr.gid);
        out_link_.add_neighbor(nbr);
      }
    }

    ReduceProxy(Master::Proxy&&         proxy, //!< parent proxy
                void*                   block, //!< diy block
                unsigned                round, //!< current round
                const Assigner&         assigner,
                const Link&             in_link,
                const Link&             out_link):
      Master::Proxy(std::move(proxy)),
      block_(block),
      round_(round),
      assigner_(assigner),
      in_link_(in_link),
      out_link_(out_link)
    {}

    //! returns pointer to block
    void*         block() const                           { return block_; }
    //! returns current round number
    unsigned      round() const                           { return round_; }
    //! returns incoming link
    const Link&   in_link() const                         { return in_link_; }
    //! returns outgoing link
    const Link&   out_link() const                        { return out_link_; }
    //! returns total number of blocks
    int           nblocks() const                         { return assigner_.nblocks(); }
    //! returns the assigner
    const Assigner& assigner() const                      { return assigner_; }

    //! advanced: change current round number
    void          set_round(unsigned r)                   { round_ = r; }

  private:
    void*         block_;
    unsigned      round_;
    const Assigner& assigner_;

    Link          in_link_;
    Link          out_link_;
};

namespace detail
{
  template<class Block, class Partners>
  struct ReductionFunctor;

  template<class Partners, class Skip>
  struct SkipInactiveOr;

  struct ReduceNeverSkip
  {
    bool operator()(int, int, const Master&) const  { return false; }
  };
}

/**
 * \ingroup Communication
 * \brief Implementation of the reduce communication pattern (includes
 *        swap-reduce, merge-reduce, and any other global communication).
 *
 */
template<class Reduce, class Partners, class Skip>
void reduce(Master&                    master,        //!< master object
            const Assigner&            assigner,      //!< assigner object
            const Partners&            partners,      //!< partners object
            const Reduce&              reduce,        //!< reduction callback function
            const Skip&                skip)          //!< object determining whether a block should be skipped
{
  auto log = get_logger();

  int original_expected = master.expected();

  using Block = typename detail::block_traits<Reduce>::type;

  unsigned round;
  for (round = 0; round < partners.rounds(); ++round)
  {
    log->debug("Round {}", round);
    master.foreach(detail::ReductionFunctor<Block,Partners>(round, reduce, partners, assigner),
                   detail::SkipInactiveOr<Partners,Skip>(round, partners, skip));
    master.execute();

    int expected = 0;
    for (unsigned i = 0; i < master.size(); ++i)
    {
      if (partners.active(round + 1, master.gid(i), master))
      {
        std::vector<int> incoming_gids;
        partners.incoming(round + 1, master.gid(i), incoming_gids, master);
        expected += static_cast<int>(incoming_gids.size());
        master.incoming(master.gid(i)).clear();
      }
    }
    master.set_expected(expected);
    master.flush(false);
  }
  // final round
  log->debug("Round {}", round);
  master.foreach(detail::ReductionFunctor<Block,Partners>(round, reduce, partners, assigner),
                 detail::SkipInactiveOr<Partners,Skip>(round, partners, skip));

  master.set_expected(original_expected);
}

/**
 * \ingroup Communication
 * \brief Implementation of the reduce communication pattern (includes
 *        swap-reduce, merge-reduce, and any other global communication).
 *
 */
template<class Reduce, class Partners>
void reduce(Master&                    master,        //!< master object
            const Assigner&            assigner,      //!< assigner object
            const Partners&            partners,      //!< partners object
            const Reduce&              reducer)       //!< reduction callback function
{
  reduce(master, assigner, partners, reducer, detail::ReduceNeverSkip());
}

namespace detail
{
  template<class Block, class Partners>
  struct ReductionFunctor
  {
    using Callback = std::function<void(Block*, const ReduceProxy&, const Partners&)>;

                ReductionFunctor(int round_, const Callback& reduce_, const Partners& partners_, const Assigner& assigner_):
                    round(round_), reduce(reduce_), partners(partners_), assigner(assigner_)        {}

    void        operator()(Block* b, const Master::ProxyWithLink& cp) const
    {
      if (!partners.active(round, cp.gid(), *cp.master())) return;

      std::vector<int> incoming_gids, outgoing_gids;
      if (round > 0)
          partners.incoming(round, cp.gid(), incoming_gids, *cp.master());        // receive from the previous round
      if (round < static_cast<int>(partners.rounds()))
          partners.outgoing(round, cp.gid(), outgoing_gids, *cp.master());        // send to the next round

      ReduceProxy   rp(std::move(const_cast<Master::ProxyWithLink&>(cp)), b, round, assigner, incoming_gids, outgoing_gids);
      reduce(b, rp, partners);

      // touch the outgoing queues to make sure they exist
      Master::Proxy::OutgoingQueues& outgoing = *rp.outgoing();
      if (outgoing.size() < static_cast<size_t>(rp.out_link().size()))
        for (BlockID target : rp.out_link().neighbors())
          outgoing[target];       // touch the outgoing queue, creating it if necessary
    }

    int             round;
    Callback        reduce;
    Partners        partners;
    const Assigner& assigner;
  };

  template<class Partners, class Skip>
  struct SkipInactiveOr
  {
                    SkipInactiveOr(int round_, const Partners& partners_, const Skip& skip_):
                        round(round_), partners(partners_), skip(skip_)         {}
    bool            operator()(int i, const Master& master) const               { return !partners.active(round, master.gid(i), master) || skip(round, i, master); }
    int             round;
    const Partners& partners;
    Skip            skip;
  };
}

} // diy

#endif // DIY_REDUCE_HPP
