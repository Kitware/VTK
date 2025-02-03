#ifndef DIY_ALGORITHMS_HPP
#define DIY_ALGORITHMS_HPP

#include <vector>

#include "master.hpp"
#include "assigner.hpp"
#include "reduce.hpp"
#include "reduce-operations.hpp"
#include "partners/swap.hpp"
#include "resolve.hpp"

#include "detail/algorithms/sort.hpp"
#include "detail/algorithms/kdtree.hpp"
#include "detail/algorithms/kdtree-sampling.hpp"
#include "detail/algorithms/load-balance-collective.hpp"
#include "detail/algorithms/load-balance-sampling.hpp"

#include "log.hpp"

namespace diy
{
    /**
     * \ingroup Algorithms
     * \brief sample sort `values` of each block, store the boundaries between blocks in `samples`
     */
    template<class Block, class T, class Cmp>
    void sort(Master&                   master,               //!< master object
              const Assigner&           assigner,             //!< assigner object
              std::vector<T> Block::*   values,               //!< all values to sort
              std::vector<T> Block::*   samples,              //!< (output) boundaries of blocks
              size_t                    num_samples,          //!< desired number of samples
              const Cmp&                cmp,                  //!< comparison function
              int                       k   = 2,              //!< k-ary reduction will be used
              bool                      samples_only = false) //!< false: results will be all_to_all exchanged; true: only sort but don't exchange results
    {
        bool immediate = master.immediate();
        master.set_immediate(false);

        // NB: although sorter will go out of scope, its member functions sample()
        //     and exchange() will return functors whose copies get saved inside reduce
        detail::SampleSort<Block,T,Cmp> sorter(values, samples, cmp, num_samples);

        // swap-reduce to all-gather samples
        RegularDecomposer<DiscreteBounds> decomposer(1, interval(0,assigner.nblocks()), assigner.nblocks());
        RegularSwapPartners   partners(decomposer, k);
        reduce(master, assigner, partners, sorter.sample(), detail::SkipIntermediate(partners.rounds()));

        // all_to_all to exchange the values
        if (!samples_only)
            all_to_all(master, assigner, sorter.exchange(), k);

        master.set_immediate(immediate);
    }


    /**
     * \ingroup Algorithms
     * \brief sample sort `values` of each block, store the boundaries between blocks in `samples`
     * shorter version of above sort algorithm with the default less-than comparator used for T
     * and all_to_all exchange included
     */
    template<class Block, class T>
    void sort(Master&                   master,      //!< master object
              const Assigner&           assigner,    //!< assigner object
              std::vector<T> Block::*   values,      //!< all values to sort
              std::vector<T> Block::*   samples,     //!< (output) boundaries of blocks
              size_t                    num_samples, //!< desired number of samples
              int                       k   = 2)     //!< k-ary reduction will be used
    {
        sort(master, assigner, values, samples, num_samples, std::less<T>(), k);
    }

    /**
     * \ingroup Algorithms
     * \brief build a kd-tree and sort a set of points into it (use histograms to determine split values)
     */
    template<class Block, class Point>
    void kdtree(Master&                         master,      //!< master object
                const Assigner&                 assigner,    //!< assigner object
                int                             dim,         //!< dimensionality
                const ContinuousBounds&         domain,      //!< global data extents
                std::vector<Point>  Block::*    points,      //!< input points to sort into kd-tree
                size_t                          bins,        //!< number of histogram bins for splitting a dimension
                bool                            wrap = false)//!< periodic boundaries in all dimensions
    {
        if (assigner.nblocks() & (assigner.nblocks() - 1))
            throw std::runtime_error(fmt::format("KD-tree requires a number of blocks that's a power of 2, got {}", assigner.nblocks()));

        typedef     diy::RegularContinuousLink      RCLink;

        for (int i = 0; i < static_cast<int>(master.size()); ++i)
        {
            RCLink* link   = static_cast<RCLink*>(master.link(i));
            *link = RCLink(dim, domain, domain);

            if (wrap)       // set up the links to self
            {
                diy::BlockID self = { master.gid(i), master.communicator().rank() };
                for (int j = 0; j < dim; ++j)
                {
                    diy::Direction dir(dim,0), wrap_dir(dim,0);

                    // left
                    dir[j] = -1; wrap_dir[j] = -1;
                    link->add_neighbor(self);
                    link->add_bounds(domain);
                    link->add_direction(dir);
                    link->add_wrap(wrap_dir);

                    // right
                    dir[j] = 1; wrap_dir[j] = 1;
                    link->add_neighbor(self);
                    link->add_bounds(domain);
                    link->add_direction(dir);
                    link->add_wrap(wrap_dir);
                }
            }
        }

        detail::KDTreePartition<Block,Point>    kdtree_partition(dim, points, bins);

        detail::KDTreePartners                  partners(dim, assigner.nblocks(), wrap, domain);
        reduce(master, assigner, partners, kdtree_partition);

        // update master.expected to match the links
        int expected = 0;
        for (int i = 0; i < static_cast<int>(master.size()); ++i)
            expected += master.link(i)->size_unique();
        master.set_expected(expected);
    }

    /**
     * \ingroup Algorithms
     * \brief build a kd-tree and sort a set of points into it (use sampling to determine split values)
     */
    template<class Block, class Point>
    void kdtree_sampling
               (Master&                         master,      //!< master object
                const Assigner&                 assigner,    //!< assigner object
                int                             dim,         //!< dimensionality
                const ContinuousBounds&         domain,      //!< global data extents
                std::vector<Point>  Block::*    points,      //!< input points to sort into kd-tree
                size_t                          samples,     //!< number of samples to take in each block
                bool                            wrap = false)//!< periodic boundaries in all dimensions
    {
        if (assigner.nblocks() & (assigner.nblocks() - 1))
            throw std::runtime_error(fmt::format("KD-tree requires a number of blocks that's a power of 2, got {}", assigner.nblocks()));

        typedef     diy::RegularContinuousLink      RCLink;

        for (int i = 0; i < static_cast<int>(master.size()); ++i)
        {
            RCLink* link   = static_cast<RCLink*>(master.link(i));
            *link = RCLink(dim, domain, domain);

            if (wrap)       // set up the links to self
            {
                diy::BlockID self = { master.gid(i), master.communicator().rank() };
                for (int j = 0; j < dim; ++j)
                {
                    diy::Direction dir(dim,0), wrap_dir(dim,0);

                    // left
                    dir[j] = -1; wrap_dir[j] = -1;
                    link->add_neighbor(self);
                    link->add_bounds(domain);
                    link->add_direction(dir);
                    link->add_wrap(wrap_dir);

                    // right
                    dir[j] = 1; wrap_dir[j] = 1;
                    link->add_neighbor(self);
                    link->add_bounds(domain);
                    link->add_direction(dir);
                    link->add_wrap(wrap_dir);
                }
            }
        }

        detail::KDTreeSamplingPartition<Block,Point>    kdtree_partition(dim, points, samples);

        detail::KDTreePartners                          partners(dim, assigner.nblocks(), wrap, domain);
        reduce(master, assigner, partners, kdtree_partition);

        // update master.expected to match the links
        int expected = 0;
        for (int i = 0; i < static_cast<int>(master.size()); ++i)
            expected += master.link(i)->size_unique();
        master.set_expected(expected);
    }

    template<class B>
    using LBCallback = std::function<Work(B*, int)>;

    // load balancing using collective method
    template<class Callback>
    void load_balance_collective(
            diy::Master&                master,             // diy master
            diy::DynamicAssigner&       dynamic_assigner,   // diy dynamic assigner
            const Callback&             f)                  // callback to get work for a block
    {
        // assert that master.destroyer() exists, will be needed for moving blocks
        if (!master.destroyer())
        {
            fmt::print(stderr, "DIY error: Master must have a block destroyer function in order to use load balancing. Please define one.\n");
            abort();
        }

        using Block = typename detail::block_traits<Callback>::type;
        const LBCallback<Block>& f_ = f;

        // compile my work info
        diy::detail::WorkInfo my_work_info = { master.communicator().rank(), -1, 0, 0, static_cast<int>(master.size()) };
        for (auto i = 0; i < master.size(); i++)
        {
            Block* block = static_cast<Block*>(master.block(i));
            Work w = f_(block, master.gid(i));
            my_work_info.proc_work += w;
            if (my_work_info.top_gid == -1 || my_work_info.top_work < w)
            {
                my_work_info.top_gid    = master.gid(i);
                my_work_info.top_work   = w;
            }
        }

        // exchange info about load balance
        std::vector<diy::detail::WorkInfo>   all_work_info; // work info collected from all mpi processes
        diy::detail::exchange_work_info(master, my_work_info, all_work_info);

        // decide what to move where
        std::vector<diy::detail::MoveInfo>   all_move_info; // move info for all moves
        diy::detail::decide_move_info(all_work_info, all_move_info);

        // move blocks from src to dst proc
        for (auto i = 0; i < all_move_info.size(); i++)
            diy::detail::move_block(master, all_move_info[i]);

        // fix links
        diy::fix_links(master, dynamic_assigner);
    }

    // load balancing using sampling method
    template<class Callback>
    void load_balance_sampling(
            diy::Master&                master,
            diy::DynamicAssigner&       dynamic_assigner,   // diy dynamic assigner
            const Callback&             f,                  // callback to get work for a block
            float                       sample_frac = 0.5,  // fraction of procs to sample 0.0 < sample_size <= 1.0
            float                       quantile    = 0.8)  // quantile cutoff above which to move blocks (0.0 - 1.0)
    {
        // assert that master.destroyer() exists, will be needed for moving blocks
        if (!master.destroyer())
        {
            fmt::print(stderr, "DIY error: Master must have a block destroyer function in order to use load balancing. Please define one.\n");
            abort();
        }

        using Block = typename detail::block_traits<Callback>::type;
        const LBCallback<Block>& f_ = f;

        // compile my work info
        diy::detail::WorkInfo my_work_info = { master.communicator().rank(), -1, 0, 0, static_cast<int>(master.size()) };
        for (auto i = 0; i < master.size(); i++)
        {
            Block* block = static_cast<Block*>(master.block(i));
            Work w = f_(block, master.gid(i));
            my_work_info.proc_work += w;
            if (my_work_info.top_gid == -1 || my_work_info.top_work < w)
            {
                my_work_info.top_gid    = master.gid(i);
                my_work_info.top_work   = w;
            }
        }

        // "auxiliary" master and decomposer for using rexchange for load balancing, 1 block per process
        diy::Master                     aux_master(master.communicator(), 1, -1, &diy::detail::AuxBlock::create, &diy::detail::AuxBlock::destroy);
        diy::ContiguousAssigner         aux_assigner(aux_master.communicator().size(), aux_master.communicator().size());
        diy::DiscreteBounds aux_domain(1);                               // any fake domain
        aux_domain.min[0] = 0;
        aux_domain.max[0] = aux_master.communicator().size() + 1;
        diy::RegularDecomposer<diy::DiscreteBounds>  aux_decomposer(1, aux_domain, aux_master.communicator().size());
        aux_decomposer.decompose(aux_master.communicator().rank(), aux_assigner, aux_master);

        // exchange info about load balance
        std::vector<diy::detail::WorkInfo>   sample_work_info;           // work info collecting from sampling other mpi processes
        diy::detail::exchange_sample_work_info(master, aux_master, sample_frac, my_work_info, sample_work_info);

        // move blocks
        diy::detail::move_sample_blocks(master, aux_master, sample_work_info, my_work_info, quantile);

        // fix links
        diy::fix_links(master, dynamic_assigner);
    }

}

#endif
