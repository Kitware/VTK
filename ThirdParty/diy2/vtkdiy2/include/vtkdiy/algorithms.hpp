#ifndef DIY_ALGORITHMS_HPP
#define DIY_ALGORITHMS_HPP

#include <vector>

#include "master.hpp"
#include "assigner.hpp"
#include "reduce.hpp"
#include "reduce-operations.hpp"
#include "partners/swap.hpp"

#include "detail/algorithms/sort.hpp"
#include "detail/algorithms/kdtree.hpp"

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
        RegularSwapPartners   partners(1, assigner.nblocks(), k);
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
     * \brief build a kd-tree and sort a set of points into it
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
        {
            fprintf(stderr, "KD-tree requires a number of blocks that's a power of 2, got %d\n", assigner.nblocks());
            std::abort();
        }

        typedef     diy::RegularContinuousLink      RCLink;

        for (int i = 0; i < master.size(); ++i)
        {
            RCLink* link   = static_cast<RCLink*>(master.link(i));
            link->core()   = domain;
            link->bounds() = domain;
        }

        detail::KDTreePartition<Block,Point>    kdtree_partition(dim, points, bins);

        detail::KDTreePartners                  partners(dim, assigner.nblocks(), wrap, domain);
        reduce(master, assigner, partners, kdtree_partition);

        // update master.expected to match the links
        int expected = 0;
        for (int i = 0; i < master.size(); ++i)
            expected += master.link(i)->size_unique();
        master.set_expected(expected);
    }

}

#endif
