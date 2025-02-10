#pragma once

#include "load-balance.hpp"

namespace diy
{

namespace detail
{

// send requests for work info
inline void send_req(AuxBlock*,                                // local block (unused)
              const diy::Master::ProxyWithLink& cp,     // communication proxy for neighbor blocks
              std::set<int>& procs)                     // processes to query
{
    // send requests for work info to sample_procs
    int v = 1;                                          // any message will do
    for (auto proc_iter = procs.begin(); proc_iter != procs.end(); proc_iter++)
    {
        int gid    = *proc_iter;
        int proc   = *proc_iter;
        diy::BlockID dest_block = {gid, proc};
        cp.enqueue(dest_block, v);
    }
}

// receive requests for work info
inline void recv_req(AuxBlock*,                                // local block (unused)
              const diy::Master::ProxyWithLink& cp,     // communication proxy for neighbor blocks
              std::vector<int>& req_procs)              // processes requesting work info
{
    std::vector<int> incoming_gids;
    cp.incoming(incoming_gids);

    // for anything incoming, dequeue data received in the last exchange
    for (int i = 0; i < incoming_gids.size(); i++)
    {
        int gid = incoming_gids[i];
        if (cp.incoming(gid).size())
        {
            int v;
            cp.dequeue(gid, v);
            req_procs.push_back(gid);                   // aux_master has 1 gid per proc, so gid = proc
        }
    }
}

// get work information from a random sample of processes
inline void exchange_sample_work_info(diy::Master&             master,                 // the real master with multiple blocks per process
                               diy::Master&             aux_master,             // auxiliary master with 1 block per process for communicating between procs
                               float                    sample_frac,            // fraction of procs to sample 0.0 < sample_size <= 1.0
                               const WorkInfo&          my_work_info,           // my process' work info
                               std::vector<WorkInfo>&   sample_work_info)       // (output) vector of sorted sample work info, sorted by increasing total work per process
{
    auto nprocs = master.communicator().size();     // global number of procs
    auto my_proc = master.communicator().rank();    // rank of my proc

    // pick a random sample of processes, w/o duplicates, and excluding myself
    int nsamples = static_cast<int>(sample_frac * (nprocs - 1));
    std::set<int> sample_procs;
    for (auto i = 0; i < nsamples; i++)
    {
        int rand_proc;
        do
        {
            std::uniform_int_distribution<> distrib(0, nprocs - 1);     // inclusive
            rand_proc = distrib(master.mt_gen);
        } while (sample_procs.find(rand_proc) != sample_procs.end() || rand_proc == my_proc);
        sample_procs.insert(rand_proc);
    }

    // rexchange requests for work info
    std::vector<int> req_procs;     // requests for work info received from these processes
    aux_master.foreach([&](AuxBlock* b, const diy::Master::ProxyWithLink& cp)
            { send_req(b, cp, sample_procs); });
    aux_master.exchange(true);      // true = remote
    aux_master.foreach([&](AuxBlock* b, const diy::Master::ProxyWithLink& cp)
            { recv_req(b, cp, req_procs); });

    // send work info
    int work_info_tag = 0;
    std::vector<diy::mpi::request> reqs(req_procs.size());
    for (auto i = 0; i < req_procs.size(); i++)
        reqs[i] = mpi::detail::isend(MPI_Comm(master.communicator()), req_procs[i], work_info_tag, &my_work_info, sizeof(WorkInfo), MPI_BYTE);

    // receive work info
    sample_work_info.resize(nsamples);
    for (auto i = 0; i < nsamples; i++)
        mpi::detail::recv(MPI_Comm(master.communicator()), diy::mpi::any_source, work_info_tag, &sample_work_info[i], sizeof(WorkInfo), MPI_BYTE);

    // ensure all the send requests cleared
    for (auto i = 0; i < req_procs.size(); i++)
        reqs[i].wait();

    // sort sample_work_info by proc_work
    std::sort(sample_work_info.begin(), sample_work_info.end(),
            [&](WorkInfo& a, WorkInfo& b) { return a.proc_work < b.proc_work; });
}

// send block
inline void send_block(AuxBlock*,                                              // local block (unused)
                const diy::Master::ProxyWithLink&   cp,                 // communication proxy for neighbor blocks
                diy::Master&                        master,             // real master with multiple blocks per process
                const std::vector<WorkInfo>&        sample_work_info,   // sampled work info
                const WorkInfo&                     my_work_info,       // my work info
                float                               quantile)           // quantile cutoff above which to move blocks (0.0 - 1.0)
{
    MoveInfo move_info = {-1, -1, -1};

    // my rank's position in the sampled work info, sorted by proc_work
    int my_work_idx = sample_work_info.size();                                          // index where my work would be in the sample_work
    for (auto i = 0; i < sample_work_info.size(); i++)
    {
        if (my_work_info.proc_work < sample_work_info[i].proc_work)
        {
            my_work_idx = i;
            break;
        }
    }

    // send my heaviest block if it passes the quantile cutoff
    if (my_work_idx >= quantile * sample_work_info.size())
    {
        // pick the destination process to be the mirror image of my work location in the samples
        // ie, the heavier my process, the lighter the destination process
        int target = sample_work_info.size() - my_work_idx;

        auto src_work_info = my_work_info;
        auto dst_work_info = sample_work_info[target];

        // sanity check that the move makes sense
        if (src_work_info.proc_work - dst_work_info.proc_work > src_work_info.top_work &&   // improve load balance
                src_work_info.proc_rank != dst_work_info.proc_rank &&                       // not self
                src_work_info.nlids > 1)                                                    // don't leave a proc with no blocks
        {

            move_info.move_gid = my_work_info.top_gid;
            move_info.src_proc = my_work_info.proc_rank;
            move_info.dst_proc = sample_work_info[target].proc_rank;

            // destination in aux_master, where gid = proc
            diy::BlockID dest_block = {move_info.dst_proc, move_info.dst_proc};

            // enqueue the gid of the moving block
            cp.enqueue(dest_block, move_info.move_gid);

            // enqueue the block
            void* send_b = master.block(master.lid(move_info.move_gid));
            diy::MemoryBuffer bb;
            master.saver()(send_b, bb);
            cp.enqueue(dest_block, bb.buffer);

            // enqueue the link for the block
            diy::Link* send_link = master.link(master.lid(move_info.move_gid));
            diy::LinkFactory::save(bb, send_link);
            cp.enqueue(dest_block, bb.buffer);

            // remove the block from the master
            int move_lid = master.lid(move_info.move_gid);
            master.destroyer()(master.release(move_lid));
        }
    }
}

// receive block
inline void recv_block(AuxBlock*,                                      // local block (unused)
                const diy::Master::ProxyWithLink&   cp,         // communication proxy for neighbor blocks
                diy::Master&                        master)     // real master with multiple blocks per process
{
    std::vector<int> incoming_gids;
    cp.incoming(incoming_gids);

    // for anything incoming, dequeue data received in the last exchange
    for (int i = 0; i < incoming_gids.size(); i++)
    {
        int gid = incoming_gids[i];
        if (cp.incoming(gid).size())
        {
            // dequeue the gid of the moving block
            int move_gid;
            cp.dequeue(gid, move_gid);

            // dequeue the block
            void* recv_b = master.creator()();
            diy::MemoryBuffer bb;
            cp.dequeue(gid, bb.buffer);
            master.loader()(recv_b, bb);

            // dequeue the link
            diy::Link* recv_link;
            cp.dequeue(gid, bb.buffer);
            recv_link = diy::LinkFactory::load(bb);

            // add block to the master
            master.add(move_gid, recv_b, recv_link);
        }
    }
}

// move blocks based on sampled work info
inline void move_sample_blocks(diy::Master&                    master,                 // real master with multiple blocks per process
                        diy::Master&                    aux_master,             // auxiliary master with 1 block per process for communcating between procs
                        const std::vector<WorkInfo>&    sample_work_info,       // sampled work info
                        const WorkInfo&                 my_work_info,           // my work info
                        float                           quantile)               // quantile cutoff above which to move blocks (0.0 - 1.0)
{
    // rexchange moving blocks
    aux_master.foreach([&](AuxBlock* b, const diy::Master::ProxyWithLink& cp)
            { send_block(b, cp, master, sample_work_info, my_work_info, quantile); });
    aux_master.exchange(true);      // true = remote
    aux_master.foreach([&](AuxBlock* b, const diy::Master::ProxyWithLink& cp)
            { recv_block(b, cp, master); });
}

}   // namespace detail

}   // namespace diy
