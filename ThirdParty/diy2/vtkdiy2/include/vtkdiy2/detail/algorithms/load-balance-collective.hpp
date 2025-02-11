#pragma once

#include <queue>
#include "load-balance.hpp"

namespace diy
{

namespace detail
{

// exchange work information among all processes using synchronous collective method
inline void exchange_work_info(diy::Master&            master,
                        const WorkInfo&         my_work_info,           // my process' work info
                        std::vector<WorkInfo>&  all_work_info)          // (output) global work info
{
    auto nprocs = master.communicator().size();     // global number of procs
    all_work_info.resize(nprocs);
    diy::mpi::detail::all_gather(master.communicator(), &my_work_info, sizeof(WorkInfo), MPI_BYTE, &all_work_info[0]);
}

// determine move info from work info
inline void decide_move_info(std::vector<WorkInfo>&        all_work_info,          // global work info
                      std::vector<MoveInfo>&        all_move_info)          // (output) move info for all moves
{
    all_move_info.clear();

    // move all blocks with an approximation to the longest processing time first (LPTF) scheduling algorithm
    // https://en.wikipedia.org/wiki/Longest-processing-time-first_scheduling
    // we iteratively move the heaviest block to lightest proc
    // constrained by only recording the heaviest block for each proc, not all blocks

    // minimum proc_work priority queue, top is min proc_work
    auto cmp = [&](WorkInfo& a, WorkInfo& b) { return a.proc_work > b.proc_work; };
    std::priority_queue<WorkInfo, std::vector<WorkInfo>, decltype(cmp)> min_proc_work_q(cmp);
    for (auto i = 0; i < all_work_info.size(); i++)
        min_proc_work_q.push(all_work_info[i]);

    // sort all_work_info by descending top_work
    std::sort(all_work_info.begin(), all_work_info.end(),
            [&](WorkInfo& a, WorkInfo& b) { return a.top_work > b.top_work; });

    // walk the all_work_info vector in descending top_work order
    // move the next heaviest block to the lightest proc
    for (auto i = 0; i < all_work_info.size(); i++)
    {
        auto src_work_info = all_work_info[i];                      // heaviest block
        auto dst_work_info = min_proc_work_q.top();                 // lightest proc

        // sanity check that the move makes sense
        if (src_work_info.proc_work - dst_work_info.proc_work > src_work_info.top_work &&   // improve load balance
                src_work_info.proc_rank != dst_work_info.proc_rank &&                       // not self
                src_work_info.nlids > 1)                                                    // don't leave a proc with no blocks
        {
            MoveInfo move_info;
            move_info.src_proc  = src_work_info.proc_rank;
            move_info.dst_proc  = dst_work_info.proc_rank;
            move_info.move_gid  = src_work_info.top_gid;
            all_move_info.push_back(move_info);

            // update the min_proc_work priority queue
            auto work_info = min_proc_work_q.top();                 // lightest proc
            work_info.proc_work += src_work_info.top_work;
            if (work_info.top_work < src_work_info.top_work)
            {
                work_info.top_work = src_work_info.top_work;
                work_info.top_gid  = src_work_info.top_gid;
            }
            min_proc_work_q.pop();
            min_proc_work_q.push(work_info);
        }
    }
}

// move one block from src to dst proc
inline void move_block(diy::Master&            master,
                const MoveInfo&         move_info)
{
    // sanity check that source and destination are different
    if (move_info.src_proc == move_info.dst_proc)
    {
        fmt::print(stderr, "Error: move_block(): source and destination are same. This should not happen.\n");
        abort();
    }

    if (master.communicator().rank() == move_info.src_proc)
    {
        diy::MemoryBuffer bb;

        // move the block from src to dst proc
        void* send_b = master.block(master.lid(move_info.move_gid));
        master.saver()(send_b, bb);
        master.communicator().send(move_info.dst_proc, 0, bb.buffer);

        // move the link for the moving block
        diy::Link* send_link = master.link(master.lid(move_info.move_gid));
        diy::LinkFactory::save(bb, send_link);
        master.communicator().send(move_info.dst_proc, 0, bb.buffer);

        // remove the block from the master
        int move_lid = master.lid(move_info.move_gid);
        master.destroyer()(master.release(move_lid));
    }
    else if (master.communicator().rank() == move_info.dst_proc)
    {
        diy::MemoryBuffer bb;

        // move the block from src to dst proc
        void* recv_b = master.creator()();
        master.communicator().recv(move_info.src_proc, 0, bb.buffer);
        master.loader()(recv_b, bb);

        // move the link for the moving block
        diy::Link* recv_link;
        master.communicator().recv(move_info.src_proc, 0, bb.buffer);
        recv_link = diy::LinkFactory::load(bb);

        // add the block to the master
        master.add(move_info.move_gid, recv_b, recv_link);
    }
}

}   // namespace detail

}   // namespace diy
