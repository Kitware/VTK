#ifndef DIY_DETAIL_ALL_TO_ALL_HPP
#define DIY_DETAIL_ALL_TO_ALL_HPP

namespace diy
{

namespace detail
{
  template<class Op>
  struct AllToAllReduce
  {
         AllToAllReduce(const Op& op_, const Assigner& assigner):
             op(op_)
    {
      for (int gid = 0; gid < assigner.nblocks(); ++gid)
      {
        BlockID nbr = { gid, assigner.rank(gid) };
        all_neighbors_link.add_neighbor(nbr);
      }
    }

    void operator()(void* b, const ReduceProxy& srp, const RegularSwapPartners& partners) const
    {
      int k_in  = srp.in_link().size();
      int k_out = srp.out_link().size();

      if (k_in == 0 && k_out == 0)  // special case of a single block
      {
          ReduceProxy all_srp_out(srp, srp.block(), 0, srp.assigner(), empty_link,         all_neighbors_link);
          ReduceProxy all_srp_in (srp, srp.block(), 1, srp.assigner(), all_neighbors_link, empty_link);

          op(b, all_srp_out);
          MemoryBuffer& in_queue = all_srp_in.incoming(all_srp_in.in_link().target(0).gid);
          in_queue.swap(all_srp_out.outgoing(all_srp_out.out_link().target(0)));
          in_queue.reset();

          op(b, all_srp_in);
          return;
      }

      if (k_in == 0)                // initial round
      {
        ReduceProxy all_srp(srp, srp.block(), 0, srp.assigner(), empty_link, all_neighbors_link);
        op(b, all_srp);

        Master::OutgoingQueues all_queues;
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
        ReduceProxy all_srp(srp, srp.block(), 1, srp.assigner(), all_neighbors_link, empty_link);

        Master::IncomingQueues all_incoming;
        all_incoming.swap(*srp.incoming());

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

          std::pair<int, int> range;
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
