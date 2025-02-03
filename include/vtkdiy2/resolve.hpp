#ifndef DIY_RESOLVE_HPP
#define DIY_RESOLVE_HPP

#include "master.hpp"
#include "assigner.hpp"

namespace diy
{
    // record master gids in assigner and then lookup the procs for all gids in the links
    inline void    fix_links(diy::Master& master, diy::DynamicAssigner& assigner);

    // auxiliary functions; could stick them into detail namespace, but they might be useful on their own
    inline void    record_local_gids(const diy::Master& master, diy::DynamicAssigner& assigner);
    inline void    update_links(diy::Master& master, const diy::DynamicAssigner& assigner);
}

void
diy::
record_local_gids(const diy::Master& master, diy::DynamicAssigner& assigner)
{
    // figure out local ranks
    std::vector<std::tuple<int,int>> local_gids;
    for (int i = 0; i < static_cast<int>(master.size()); ++i)
        local_gids.emplace_back(std::make_tuple(master.communicator().rank(), master.gid(i)));

    assigner.set_ranks(local_gids);
}

void
diy::
update_links(diy::Master& master, const diy::DynamicAssigner& assigner)
{
    // figure out all the gids we need
    std::vector<int> nbr_gids;
    for (int i = 0; i < static_cast<int>(master.size()); ++i)
    {
        auto* link = master.link(i);
        for (auto blockid : link->neighbors())
            nbr_gids.emplace_back(blockid.gid);
    }

    // keep only unique gids to avoid unnecessary lookups
    std::sort(nbr_gids.begin(), nbr_gids.end());
    nbr_gids.resize(std::unique(nbr_gids.begin(), nbr_gids.end()) - nbr_gids.begin());

    // get the neighbor ranks
    std::vector<int> nbr_procs = assigner.ranks(nbr_gids);

    // convert to a map
    std::unordered_map<int, int>    gid_to_proc;
    for (size_t i = 0; i < nbr_gids.size(); ++i)
        gid_to_proc[nbr_gids[i]] = nbr_procs[i];

    // fix the procs in links
    for (int i = 0; i < static_cast<int>(master.size()); ++i)
    {
        auto* link = master.link(i);
        for (auto& blockid : link->neighbors())
            blockid.proc = gid_to_proc[blockid.gid];
    }
}

void
diy::
fix_links(diy::Master& master, diy::DynamicAssigner& assigner)
{
    record_local_gids(master, assigner);
    master.communicator().barrier();        // make sure everyone has set ranks
    update_links(master, assigner);
}

#endif
