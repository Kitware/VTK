#ifndef DIY_DETAIL_ALGORITHMS_KDTREE_SAMPLING_HPP
#define DIY_DETAIL_ALGORITHMS_KDTREE_SAMPLING_HPP

#include <vector>
#include <cassert>
#include "../../partners/all-reduce.hpp"
#include "../../log.hpp"

// TODO: technically, what's done now is not a perfect subsample:
//       we take the same number of samples from every block, in reality this number should be selected at random,
//       so that the total number of samples adds up to samples*nblocks
//
// NB: random samples are chosen using rand(), which is assumed to be seeded
//     externally. Once we switch to C++11, we should use its more advanced
//     random number generators (and take a generator as an external parameter)
//     (TODO)

namespace diy
{
namespace detail
{

template<class Block, class Point>
struct KDTreeSamplingPartition
{
    typedef     diy::RegularContinuousLink      RCLink;
    typedef     diy::ContinuousBounds           Bounds;

    typedef     std::vector<float>              Samples;

                KDTreeSamplingPartition(int                             dim,
                                        std::vector<Point>  Block::*    points,
                                        size_t                          samples):
                    dim_(dim), points_(points), samples_(samples)           {}

    void        operator()(Block* b, const diy::ReduceProxy& srp, const KDTreePartners& partners) const;

    int         divide_gid(int gid, bool lower, int round, int rounds) const;
    void        update_links(Block* b, const diy::ReduceProxy& srp, int dim, int round, int rounds, bool wrap, const Bounds& domain) const;
    void        split_to_neighbors(Block* b, const diy::ReduceProxy& srp, int dim) const;
    diy::Direction
                find_wrap(const Bounds& bounds, const Bounds& nbr_bounds, const Bounds& domain) const;

    void        compute_local_samples(Block* b, const diy::ReduceProxy& srp, int dim) const;
    void        add_samples(Block* b, const diy::ReduceProxy& srp, Samples& samples) const;
    void        receive_samples(Block* b, const diy::ReduceProxy& srp,       Samples& samples) const;
    void        forward_samples(Block* b, const diy::ReduceProxy& srp, const Samples& samples) const;

    void        enqueue_exchange(Block* b, const diy::ReduceProxy& srp, int dim, const Samples& samples) const;
    void        dequeue_exchange(Block* b, const diy::ReduceProxy& srp, int dim) const;

    void        update_neighbor_bounds(Bounds& bounds, float split, int dim, bool lower) const;
    bool        intersects(const Bounds& x, const Bounds& y, int dim, bool wrap, const Bounds& domain) const;
    float       find_split(const Bounds& changed, const Bounds& original) const;

    int                             dim_;
    std::vector<Point>  Block::*    points_;
    size_t                          samples_;
};

}
}


template<class Block, class Point>
void
diy::detail::KDTreeSamplingPartition<Block,Point>::
operator()(Block* b, const diy::ReduceProxy& srp, const KDTreePartners& partners) const
{
    int dim;
    if (srp.round() < partners.rounds())
        dim = partners.dim(srp.round());
    else
        dim = partners.dim(srp.round() - 1);

    if (srp.round() == partners.rounds())
        update_links(b, srp, dim, partners.sub_round((int)srp.round() - 2), (int)partners.swap_rounds(), partners.wrap, partners.domain); // -1 would be the "uninformative" link round
    else if (partners.swap_round(srp.round()) && partners.sub_round(srp.round()) < 0)       // link round
    {
        dequeue_exchange(b, srp, dim);         // from the swap round
        split_to_neighbors(b, srp, dim);
    }
    else if (partners.swap_round(srp.round()))
    {
        Samples samples;
        receive_samples(b, srp, samples);
        enqueue_exchange(b, srp, dim, samples);
    } else if (partners.sub_round(srp.round()) == 0)
    {
        if (srp.round() > 0)
        {
            int prev_dim = dim - 1;
            if (prev_dim < 0)
                prev_dim += dim_;
            update_links(b, srp, prev_dim, partners.sub_round((int)srp.round() - 2), (int)partners.swap_rounds(), partners.wrap, partners.domain);    // -1 would be the "uninformative" link round
        }

        compute_local_samples(b, srp, dim);
    } else if (partners.sub_round(srp.round()) < (int) partners.histogram.rounds()/2)     // we are reusing partners class, so really we are talking about the samples rounds here
    {
        Samples samples;
        add_samples(b, srp, samples);
        srp.enqueue(srp.out_link().target(0), samples);
    } else
    {
        Samples samples;
        add_samples(b, srp, samples);
        if (samples.size() != 1)
        {
            // pick the median
            std::nth_element(samples.begin(), samples.begin() + samples.size()/2, samples.end());
            std::swap(samples[0], samples[samples.size()/2]);
            //std::sort(samples.begin(), samples.end());
            //samples[0] = (samples[samples.size()/2] + samples[samples.size()/2 + 1])/2;
            samples.resize(1);
        }
        forward_samples(b, srp, samples);
    }
}

template<class Block, class Point>
int
diy::detail::KDTreeSamplingPartition<Block,Point>::
divide_gid(int gid, bool lower, int round, int rounds) const
{
    if (lower)
        gid &= ~(1 << (rounds - 1 - round));
    else
        gid |=  (1 << (rounds - 1 - round));
    return gid;
}

// round here is the outer iteration of the algorithm
template<class Block, class Point>
void
diy::detail::KDTreeSamplingPartition<Block,Point>::
update_links(Block*, const diy::ReduceProxy& srp, int dim, int round, int rounds, bool wrap, const Bounds& domain) const
{
    auto        log  = get_logger();
    int         gid  = srp.gid();
    int         lid  = srp.master()->lid(gid);
    RCLink*     link = static_cast<RCLink*>(srp.master()->link(lid));

    // (gid, dir) -> i
    std::map<std::pair<int,diy::Direction>, int> link_map;
    for (int i = 0; i < link->size(); ++i)
        link_map[std::make_pair(link->target(i).gid, link->direction(i))] = i;

    // NB: srp.enqueue(..., ...) should match the link
    std::vector<float>  splits(link->size());
    for (int i = 0; i < link->size(); ++i)
    {
        float split; diy::Direction dir(dim_,0);

        int in_gid = link->target(i).gid;
        while(srp.incoming(in_gid))
        {
            srp.dequeue(in_gid, split);
            srp.dequeue(in_gid, dir);

            // reverse dir
            for (int j = 0; j < dim_; ++j)
                dir[j] = -dir[j];

            int k = link_map[std::make_pair(in_gid, dir)];
            log->trace("{} {} {} -> {}", in_gid, dir, split, k);
            splits[k] = split;
        }
    }

    RCLink      new_link(dim_, link->core(), link->core());

    bool lower = !(gid & (1 << (rounds - 1 - round)));

    // fill out the new link
    for (int i = 0; i < link->size(); ++i)
    {
        diy::Direction  dir = link->direction(i);
        //diy::Direction  wrap_dir = link->wrap(i);     // we don't use existing wrap, but restore it from scratch
        if (dir[dim] != 0)
        {
            if ((dir[dim] < 0 && lower) || (dir[dim] > 0 && !lower))
            {
                int nbr_gid = divide_gid(link->target(i).gid, !lower, round, rounds);
                diy::BlockID nbr = { nbr_gid, srp.assigner().rank(nbr_gid) };
                new_link.add_neighbor(nbr);

                new_link.add_direction(dir);

                Bounds bounds = link->bounds(i);
                update_neighbor_bounds(bounds, splits[i], dim, !lower);
                new_link.add_bounds(bounds);

                if (wrap)
                    new_link.add_wrap(find_wrap(new_link.bounds(), bounds, domain));
                else
                    new_link.add_wrap(diy::Direction(dim_,0));
            }
        } else // non-aligned side
        {
            for (int j = 0; j < 2; ++j)
            {
                int nbr_gid = divide_gid(link->target(i).gid, j == 0, round, rounds);

                Bounds  bounds  = link->bounds(i);
                update_neighbor_bounds(bounds, splits[i], dim, j == 0);

                if (intersects(bounds, new_link.bounds(), dim, wrap, domain))
                {
                    diy::BlockID nbr = { nbr_gid, srp.assigner().rank(nbr_gid) };
                    new_link.add_neighbor(nbr);
                    new_link.add_direction(dir);
                    new_link.add_bounds(bounds);

                    if (wrap)
                        new_link.add_wrap(find_wrap(new_link.bounds(), bounds, domain));
                    else
                        new_link.add_wrap(diy::Direction(dim_,0));
                }
            }
        }
    }

    // add link to the dual block
    int dual_gid = divide_gid(gid, !lower, round, rounds);
    diy::BlockID dual = { dual_gid, srp.assigner().rank(dual_gid) };
    new_link.add_neighbor(dual);

    Bounds nbr_bounds = link->bounds();     // old block bounds
    update_neighbor_bounds(nbr_bounds, find_split(new_link.bounds(), nbr_bounds), dim, !lower);
    new_link.add_bounds(nbr_bounds);

    new_link.add_wrap(diy::Direction(dim_,0));    // dual block cannot be wrapped

    if (lower)
    {
        diy::Direction right(dim_,0);
        right[dim] = 1;
        new_link.add_direction(right);
    } else
    {
        diy::Direction left(dim_,0);
        left[dim] = -1;
        new_link.add_direction(left);
    }

    // update the link; notice that this won't conflict with anything since
    // reduce is using its own notion of the link constructed through the
    // partners
    link->swap(new_link);
}

template<class Block, class Point>
void
diy::detail::KDTreeSamplingPartition<Block,Point>::
split_to_neighbors(Block*, const diy::ReduceProxy& srp, int) const
{
    int         lid  = srp.master()->lid(srp.gid());
    RCLink*     link = static_cast<RCLink*>(srp.master()->link(lid));

    // determine split
    float split = find_split(link->core(), link->bounds());

    for (int i = 0; i < link->size(); ++i)
    {
        srp.enqueue(link->target(i), split);
        srp.enqueue(link->target(i), link->direction(i));
    }
}

template<class Block, class Point>
void
diy::detail::KDTreeSamplingPartition<Block,Point>::
compute_local_samples(Block* b, const diy::ReduceProxy& srp, int dim) const
{
    // compute and enqueue local samples
    Samples samples;
    size_t points_size = (b->*points_).size();
    size_t n = (std::min)(points_size, samples_);
    samples.reserve(n);
    for (size_t i = 0; i < n; ++i)
    {
        float x = (b->*points_)[rand() % points_size][dim];
        samples.push_back(x);
    }

    srp.enqueue(srp.out_link().target(0), samples);
}

template<class Block, class Point>
void
diy::detail::KDTreeSamplingPartition<Block,Point>::
add_samples(Block*, const diy::ReduceProxy& srp, Samples& samples) const
{
    // dequeue and combine the samples
    for (int i = 0; i < srp.in_link().size(); ++i)
    {
        int nbr_gid = srp.in_link().target(i).gid;

        Samples smpls;
        srp.dequeue(nbr_gid, smpls);
        for (size_t j = 0; j < smpls.size(); ++j)
            samples.push_back(smpls[j]);
    }
}

template<class Block, class Point>
void
diy::detail::KDTreeSamplingPartition<Block,Point>::
receive_samples(Block*, const diy::ReduceProxy& srp, Samples& samples) const
{
    srp.dequeue(srp.in_link().target(0).gid, samples);
}

template<class Block, class Point>
void
diy::detail::KDTreeSamplingPartition<Block,Point>::
forward_samples(Block*, const diy::ReduceProxy& srp, const Samples& samples) const
{
    for (int i = 0; i < srp.out_link().size(); ++i)
        srp.enqueue(srp.out_link().target(i), samples);
}

template<class Block, class Point>
void
diy::detail::KDTreeSamplingPartition<Block,Point>::
enqueue_exchange(Block* b, const diy::ReduceProxy& srp, int dim, const Samples& samples) const
{
    int         lid  = srp.master()->lid(srp.gid());
    RCLink*     link = static_cast<RCLink*>(srp.master()->link(lid));

    int k = srp.out_link().size();

    if (k == 0)        // final round; nothing needs to be sent; this is actually redundant
        return;

    // pick split points
    float split = samples[0];

    // subset and enqueue
    std::vector< std::vector<Point> > out_points(srp.out_link().size());
    for (size_t i = 0; i < (b->*points_).size(); ++i)
    {
      float x = (b->*points_)[i][dim];
      int loc = x < split ? 0 : 1;
      out_points[loc].push_back((b->*points_)[i]);
    }
    int pos = -1;
    for (int i = 0; i < k; ++i)
    {
      if (srp.out_link().target(i).gid == srp.gid())
      {
        (b->*points_).swap(out_points[i]);
        pos = i;
      }
      else
        srp.enqueue(srp.out_link().target(i), out_points[i]);
    }
    if (pos == 0)
        link->core().max[dim] = split;
    else
        link->core().min[dim] = split;
}

template<class Block, class Point>
void
diy::detail::KDTreeSamplingPartition<Block,Point>::
dequeue_exchange(Block* b, const diy::ReduceProxy& srp, int dim) const
{
    int         lid  = srp.master()->lid(srp.gid());
    RCLink*     link = static_cast<RCLink*>(srp.master()->link(lid));

    for (int i = 0; i < srp.in_link().size(); ++i)
    {
      int nbr_gid = srp.in_link().target(i).gid;
      if (nbr_gid == srp.gid())
          continue;

      std::vector<Point>   in_points;
      srp.dequeue(nbr_gid, in_points);
      for (size_t j = 0; j < in_points.size(); ++j)
      {
        if (in_points[j][dim] < link->core().min[dim] || in_points[j][dim] > link->core().max[dim])
            throw std::runtime_error(fmt::format("Dequeued {} outside [{},{}] ({})",
                                                 in_points[j][dim], link->core().min[dim], link->core().max[dim], dim));
        (b->*points_).push_back(in_points[j]);
      }
    }
}

template<class Block, class Point>
void
diy::detail::KDTreeSamplingPartition<Block,Point>::
update_neighbor_bounds(Bounds& bounds, float split, int dim, bool lower) const
{
    if (lower)
        bounds.max[dim] = split;
    else
        bounds.min[dim] = split;
}

template<class Block, class Point>
bool
diy::detail::KDTreeSamplingPartition<Block,Point>::
intersects(const Bounds& x, const Bounds& y, int dim, bool wrap, const Bounds& domain) const
{
    if (wrap)
    {
        if (x.min[dim] == domain.min[dim] && y.max[dim] == domain.max[dim])
            return true;
        if (y.min[dim] == domain.min[dim] && x.max[dim] == domain.max[dim])
            return true;
    }
    return x.min[dim] <= y.max[dim] && y.min[dim] <= x.max[dim];
}

template<class Block, class Point>
float
diy::detail::KDTreeSamplingPartition<Block,Point>::
find_split(const Bounds& changed, const Bounds& original) const
{
    for (int i = 0; i < dim_; ++i)
    {
        if (changed.min[i] != original.min[i])
            return changed.min[i];
        if (changed.max[i] != original.max[i])
            return changed.max[i];
    }
    assert(0);
    return -1;
}

template<class Block, class Point>
diy::Direction
diy::detail::KDTreeSamplingPartition<Block,Point>::
find_wrap(const Bounds& bounds, const Bounds& nbr_bounds, const Bounds& domain) const
{
    diy::Direction wrap(dim_,0);
    for (int i = 0; i < dim_; ++i)
    {
        if (bounds.min[i] == domain.min[i] && nbr_bounds.max[i] == domain.max[i])
            wrap[i] = -1;
        if (bounds.max[i] == domain.max[i] && nbr_bounds.min[i] == domain.min[i])
            wrap[i] =  1;
    }
    return wrap;
}


#endif
