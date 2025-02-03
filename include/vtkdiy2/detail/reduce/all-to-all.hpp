#ifndef DIY_DETAIL_ALL_TO_ALL_HPP
#define DIY_DETAIL_ALL_TO_ALL_HPP

#include "../block_traits.hpp"

namespace diy
{

namespace detail
{
  template<class Op>
  struct AllToAllReduce
  {
    using Block = typename block_traits<Op>::type;

         AllToAllReduce(const Op& op_, const Assigner& assigner):
             op(op_)
    {
      for (int gid = 0; gid < assigner.nblocks(); ++gid)
      {
        BlockID nbr = { gid, assigner.rank(gid) };
        all_neighbors_link.add_neighbor(nbr);
      }
    }

    void operator()(Block* b, const ReduceProxy& srp, const RegularSwapPartners&) const
    {
      int k_in  = srp.in_link().size();
      int k_out = srp.out_link().size();

      if (k_in == 0 && k_out == 0)  // special case of a single block
      {
          ReduceProxy all_srp(std::move(const_cast<ReduceProxy&>(srp)), srp.block(), 0, srp.assigner(), empty_link, all_neighbors_link);

          op(b, all_srp);

          MemoryBuffer& in_queue = all_srp.incoming(all_srp.out_link().target(0).gid);
          in_queue.swap(all_srp.outgoing(all_srp.out_link().target(0)));
          in_queue.reset();
          all_srp.outgoing()->clear();

          // change to incoming proxy
          all_srp.set_round(1);
          auto& in_link  = const_cast<Link&>(all_srp.in_link());
          auto& out_link = const_cast<Link&>(all_srp.out_link());
          in_link.swap(out_link);

          op(b, all_srp);
          return;
      }

      if (k_in == 0)                // initial round
      {
        ReduceProxy all_srp(std::move(const_cast<ReduceProxy&>(srp)), srp.block(), 0, srp.assigner(), empty_link, all_neighbors_link);
        op(b, all_srp);

        Master::Proxy::OutgoingQueues all_queues;
        all_queues.swap(*all_srp.outgoing());       // clears out the queues and stores them locally

        // enqueue outgoing
        int group = all_srp.out_link().size() / k_out;
        for (int i = 0; i < k_out; ++i)
        {
          std::pair<int,int> range(i*group, (i+1)*group);
          srp.enqueue(srp.out_link().target(i), range);
          for (int j = i*group; j < (i+1)*group; ++j)
          {
            int from = srp.gid();
            int to   = all_srp.out_link().target(j).gid;
            srp.enqueue(srp.out_link().target(i), std::make_pair(from, to));
            srp.enqueue(srp.out_link().target(i), all_queues[all_srp.out_link().target(j)]);
          }
        }
      } else if (k_out == 0)        // final round
      {
        // dequeue incoming + reorder into the correct order
        ReduceProxy all_srp(std::move(const_cast<ReduceProxy&>(srp)), srp.block(), 1, srp.assigner(), all_neighbors_link, empty_link);

        Master::Proxy::IncomingQueues all_incoming;
        all_incoming.swap(*all_srp.incoming());

        std::pair<int, int> range;      // all the ranges should be the same
        for (int i = 0; i < k_in; ++i)
        {
          int gid_in = srp.in_link().target(i).gid;
          MemoryBuffer& in = all_incoming[gid_in];
          load(in, range);
          while(in)
          {
            std::pair<int, int> from_to;
            load(in, from_to);
            load(in, all_srp.incoming(from_to.first));
            all_srp.incoming(from_to.first).reset();
          }
        }

        op(b, all_srp);
      } else                                        // intermediate round: reshuffle queues
      {
        // add up buffer sizes
        std::vector<size_t> sizes_out(k_out, sizeof(std::pair<int,int>));
        std::pair<int, int> range;      // all the ranges should be the same
        for (int i = 0; i < k_in; ++i)
        {
          MemoryBuffer& in = srp.incoming(srp.in_link().target(i).gid);

          load(in, range);
          int group = (range.second - range.first)/k_out;

          std::pair<int, int> from_to;
          size_t s;
          while(in)
          {
            diy::load(in, from_to);
            diy::load(in, s);

            int j = (from_to.second - range.first) / group;
            sizes_out[j] += s + sizeof(size_t) + sizeof(std::pair<int,int>);
            in.skip(s);
          }
          in.reset();
        }

        // reserve outgoing buffers of correct size
        int group = (range.second - range.first)/k_out;
        for (int i = 0; i < k_out; ++i)
        {
          MemoryBuffer& out = srp.outgoing(srp.out_link().target(i));
          out.reserve(sizes_out[i]);

          std::pair<int, int> out_range;
          out_range.first  = range.first + group*i;
          out_range.second = range.first + group*(i+1);
          save(out, out_range);
        }

        // re-direct the queues
        for (int i = 0; i < k_in; ++i)
        {
          MemoryBuffer& in = srp.incoming(srp.in_link().target(i).gid);

          load(in, range);

          std::pair<int, int> from_to;
          while(in)
          {
            load(in, from_to);
            int j = (from_to.second - range.first) / group;

            MemoryBuffer& out = srp.outgoing(srp.out_link().target(j));
            save(out, from_to);
            MemoryBuffer::copy(in, out);
          }
        }
      }
    }

    const Op&           op;
    Link                all_neighbors_link, empty_link;
  };

  struct SkipIntermediate
  {
         SkipIntermediate(size_t rounds_):
            rounds(rounds_)                                     {}

    bool operator()(int round, int, const Master&) const        { if (round == 0 || round == (int) rounds) return false; return true; }

    size_t  rounds;
  };
}

}

#endif
