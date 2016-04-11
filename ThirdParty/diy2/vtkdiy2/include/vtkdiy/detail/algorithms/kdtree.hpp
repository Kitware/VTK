#ifndef DIY_DETAIL_ALGORITHMS_KDTREE_HPP
#define DIY_DETAIL_ALGORITHMS_KDTREE_HPP

#include <vector>
#include <cassert>
#include "../../partners/all-reduce.hpp"

namespace diy
{
namespace detail
{

struct KDTreePartners;

template<class Block, class Point>
struct KDTreePartition
{
    typedef     diy::RegularContinuousLink      RCLink;
    typedef     diy::ContinuousBounds           Bounds;

    typedef     std::vector<size_t>             Histogram;

                KDTreePartition(int                             dim,
                                std::vector<Point>  Block::*    points,
                                size_t                          bins):
                    dim_(dim), points_(points), bins_(bins)            {}

    void        operator()(void* b_, const diy::ReduceProxy& srp, const KDTreePartners& partners) const;

    int         divide_gid(int gid, bool lower, int round, int rounds) const;
    void        update_links(Block* b, const diy::ReduceProxy& srp, int dim, int round, int rounds, bool wrap, const Bounds& domain) const;
    void        split_to_neighbors(Block* b, const diy::ReduceProxy& srp, int dim) const;
    diy::Direction
                find_wrap(const Bounds& bounds, const Bounds& nbr_bounds, const Bounds& domain) const;

    void        compute_local_histogram(Block* b, const diy::ReduceProxy& srp, int dim) const;
    void        add_histogram(Block* b, const diy::ReduceProxy& srp, Histogram& histogram) const;
    void        receive_histogram(Block* b, const diy::ReduceProxy& srp,       Histogram& histogram) const;
    void        forward_histogram(Block* b, const diy::ReduceProxy& srp, const Histogram& histogram) const;

    void        enqueue_exchange(Block* b, const diy::ReduceProxy& srp, int dim, const Histogram& histogram) const;
    void        dequeue_exchange(Block* b, const diy::ReduceProxy& srp, int dim) const;

    void        update_neighbor_bounds(Bounds& bounds, float split, int dim, bool lower) const;
    bool        intersects(const Bounds& x, const Bounds& y, int dim, bool wrap, const Bounds& domain) const;
    float       find_split(const Bounds& changed, const Bounds& original) const;

    int                             dim_;
    std::vector<Point>  Block::*    points_;
    size_t                          bins_;
};

}
}

struct diy::detail::KDTreePartners
{
  // bool = are we in a swap (vs histogram) round
  // int  = round within that partner
  typedef           std::pair<bool, int>                    RoundType;
  typedef           diy::ContinuousBounds                   Bounds;

                    KDTreePartners(int dim, int nblocks, bool wrap_, const Bounds& domain_):
                        decomposer(1, interval(0,nblocks-1), nblocks),
                        histogram(decomposer, 2),
                        swap(decomposer, 2, false),
                        wrap(wrap_),
                        domain(domain_)
  {
    for (unsigned i = 0; i < swap.rounds(); ++i)
    {
      // fill histogram rounds
      for (unsigned j = 0; j < histogram.rounds(); ++j)
      {
        rounds_.push_back(std::make_pair(false, j));
        dim_.push_back(i % dim);
        if (j == histogram.rounds() / 2 - 1 - i)
            j += 2*i;
      }

      // fill swap round
      rounds_.push_back(std::make_pair(true, i));
      dim_.push_back(i % dim);

      // fill link round
      rounds_.push_back(std::make_pair(true, -1));          // (true, -1) signals link round
      dim_.push_back(i % dim);
    }
  }

  size_t        rounds() const                              { return rounds_.size(); }
  size_t        swap_rounds() const                         { return swap.rounds(); }

  int           dim(int round) const                        { return dim_[round]; }
  bool          swap_round(int round) const                 { return rounds_[round].first; }
  int           sub_round(int round) const                  { return rounds_[round].second; }

  inline bool   active(int round, int gid, const diy::Master& m) const
  {
    if (round == (int) rounds())
        return true;
    else if (swap_round(round) && sub_round(round) < 0)     // link round
        return true;
    else if (swap_round(round))
        return swap.active(sub_round(round), gid, m);
    else
        return histogram.active(sub_round(round), gid, m);
  }

  inline void   incoming(int round, int gid, std::vector<int>& partners, const diy::Master& m) const
  {
    if (round == (int) rounds())
        link_neighbors(-1, gid, partners, m);
    else if (swap_round(round) && sub_round(round) < 0)       // link round
        swap.incoming(sub_round(round - 1) + 1, gid, partners, m);
    else if (swap_round(round))
        histogram.incoming(histogram.rounds(), gid, partners, m);
    else
    {
        if (round > 0 && sub_round(round) == 0)
            link_neighbors(-1, gid, partners, m);
        else if (round > 0 && sub_round(round - 1) != sub_round(round) - 1)        // jump through the histogram rounds
            histogram.incoming(sub_round(round - 1) + 1, gid, partners, m);
        else
            histogram.incoming(sub_round(round), gid, partners, m);
    }
  }

  inline void   outgoing(int round, int gid, std::vector<int>& partners, const diy::Master& m) const
  {
    if (round == (int) rounds())
        swap.outgoing(sub_round(round-1) + 1, gid, partners, m);
    else if (swap_round(round) && sub_round(round) < 0)       // link round
        link_neighbors(-1, gid, partners, m);
    else if (swap_round(round))
        swap.outgoing(sub_round(round), gid, partners, m);
    else
        histogram.outgoing(sub_round(round), gid, partners, m);
  }

  inline void   link_neighbors(int, int gid, std::vector<int>& partners, const diy::Master& m) const
  {
    int         lid  = m.lid(gid);
    diy::Link*  link = m.link(lid);

    std::set<int> result;       // partners must be unique
    for (int i = 0; i < link->size(); ++i)
        result.insert(link->target(i).gid);

    for (std::set<int>::const_iterator it = result.begin(); it != result.end(); ++it)
        partners.push_back(*it);
  }

  // 1-D domain to feed into histogram and swap
  diy::RegularDecomposer<diy::DiscreteBounds>   decomposer;

  diy::RegularAllReducePartners     histogram;
  diy::RegularSwapPartners          swap;

  std::vector<RoundType>            rounds_;
  std::vector<int>                  dim_;

  bool                              wrap;
  Bounds                            domain;
};

template<class Block, class Point>
void
diy::detail::KDTreePartition<Block,Point>::
operator()(void* b_, const diy::ReduceProxy& srp, const KDTreePartners& partners) const
{
    Block* b = static_cast<Block*>(b_);

    int dim;
    if (srp.round() < partners.rounds())
        dim = partners.dim(srp.round());
    else
        dim = partners.dim(srp.round() - 1);

    if (srp.round() == partners.rounds())
        update_links(b, srp, dim, partners.sub_round(srp.round() - 2), partners.swap_rounds(), partners.wrap, partners.domain); // -1 would be the "uninformative" link round
    else if (partners.swap_round(srp.round()) && partners.sub_round(srp.round()) < 0)       // link round
    {
        dequeue_exchange(b, srp, dim);         // from the swap round
        split_to_neighbors(b, srp, dim);
    }
    else if (partners.swap_round(srp.round()))
    {
        Histogram   histogram;
        receive_histogram(b, srp, histogram);
        enqueue_exchange(b, srp, dim, histogram);
    } else if (partners.sub_round(srp.round()) == 0)
    {
        if (srp.round() > 0)
        {
            int prev_dim = dim - 1;
            if (prev_dim < 0)
                prev_dim += dim_;
            update_links(b, srp, prev_dim, partners.sub_round(srp.round() - 2), partners.swap_rounds(), partners.wrap, partners.domain);    // -1 would be the "uninformative" link round
        }

        compute_local_histogram(b, srp, dim);
    } else if (partners.sub_round(srp.round()) < (int) partners.histogram.rounds()/2)
    {
        Histogram   histogram(bins_);
        add_histogram(b, srp, histogram);
        srp.enqueue(srp.out_link().target(0), histogram);
    }
    else
    {
        Histogram   histogram(bins_);
        add_histogram(b, srp, histogram);
        forward_histogram(b, srp, histogram);
    }
}

template<class Block, class Point>
int
diy::detail::KDTreePartition<Block,Point>::
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
diy::detail::KDTreePartition<Block,Point>::
update_links(Block* b, const diy::ReduceProxy& srp, int dim, int round, int rounds, bool wrap, const Bounds& domain) const
{
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
        float split; diy::Direction dir;

        int in_gid = link->target(i).gid;
        while(srp.incoming(in_gid))
        {
            srp.dequeue(in_gid, split);
            srp.dequeue(in_gid, dir);

            // reverse dir
            for (int j = 0; j < dim_; ++j)
                dir[j] = -dir[j];

            int k = link_map[std::make_pair(in_gid, dir)];
            splits[k] = split;
        }
    }

    RCLink      new_link(dim_, link->core(), link->core());

    bool lower = !(gid & (1 << (rounds - 1 - round)));

    // fill out the new link
    for (int i = 0; i < link->size(); ++i)
    {
        diy::Direction  dir      = link->direction(i);
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
                    new_link.add_wrap(diy::Direction());
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
                        new_link.add_wrap(diy::Direction());
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

    new_link.add_wrap(diy::Direction());    // dual block cannot be wrapped

    if (lower)
    {
        diy::Direction right;
        right[dim] = 1;
        new_link.add_direction(right);
    } else
    {
        diy::Direction left;
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
diy::detail::KDTreePartition<Block,Point>::
split_to_neighbors(Block* b, const diy::ReduceProxy& srp, int dim) const
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
diy::detail::KDTreePartition<Block,Point>::
compute_local_histogram(Block* b, const diy::ReduceProxy& srp, int dim) const
{
    int         lid  = srp.master()->lid(srp.gid());
    RCLink*     link = static_cast<RCLink*>(srp.master()->link(lid));

    // compute and enqueue local histogram
    Histogram histogram(bins_);

    float   width = (link->core().max[dim] - link->core().min[dim])/bins_;
    for (size_t i = 0; i < (b->*points_).size(); ++i)
    {
        float x = (b->*points_)[i][dim];
        int loc = (x - link->core().min[dim]) / width;
        if (loc < 0)
        {
            std::cerr << loc << " " << x << " " << link->core().min[dim] << std::endl;
            std::abort();
        }
        if (loc >= (int) bins_)
            loc = bins_ - 1;
        ++(histogram[loc]);
    }

    srp.enqueue(srp.out_link().target(0), histogram);
}

template<class Block, class Point>
void
diy::detail::KDTreePartition<Block,Point>::
add_histogram(Block* b, const diy::ReduceProxy& srp, Histogram& histogram) const
{
    // dequeue and add up the histograms
    for (int i = 0; i < srp.in_link().size(); ++i)
    {
        int nbr_gid = srp.in_link().target(i).gid;

        Histogram hist;
        srp.dequeue(nbr_gid, hist);
        for (size_t i = 0; i < hist.size(); ++i)
            histogram[i] += hist[i];
    }
}

template<class Block, class Point>
void
diy::detail::KDTreePartition<Block,Point>::
receive_histogram(Block* b, const diy::ReduceProxy& srp, Histogram& histogram) const
{
    srp.dequeue(srp.in_link().target(0).gid, histogram);
}

template<class Block, class Point>
void
diy::detail::KDTreePartition<Block,Point>::
forward_histogram(Block* b, const diy::ReduceProxy& srp, const Histogram& histogram) const
{
    for (int i = 0; i < srp.out_link().size(); ++i)
        srp.enqueue(srp.out_link().target(i), histogram);
}

template<class Block, class Point>
void
diy::detail::KDTreePartition<Block,Point>::
enqueue_exchange(Block* b, const diy::ReduceProxy& srp, int dim, const Histogram& histogram) const
{
    int         lid  = srp.master()->lid(srp.gid());
    RCLink*     link = static_cast<RCLink*>(srp.master()->link(lid));

    int k = srp.out_link().size();

    if (k == 0)        // final round; nothing needs to be sent; this is actually redundant
        return;

    // pick split points
    size_t total = 0;
    for (size_t i = 0; i < histogram.size(); ++i)
        total += histogram[i];
    //fprintf(stderr, "Histogram total: %lu\n", total);

    size_t cur   = 0;
    float  width = (link->core().max[dim] - link->core().min[dim])/bins_;
    float  split = 0;
    for (size_t i = 0; i < histogram.size(); ++i)
    {
        if (cur + histogram[i] > total/2)
        {
            split = link->core().min[dim] + width*i;
            break;
        }
        cur += histogram[i];
    }
    //std::cout << "Found split: " << split << " (dim=" << dim << ") in " << link->core().min[dim] << " - " << link->core().max[dim] << std::endl;

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
diy::detail::KDTreePartition<Block,Point>::
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
        {
            fprintf(stderr, "Warning: dequeued %f outside [%f,%f] (%d)\n",
                            in_points[j][dim], link->core().min[dim], link->core().max[dim], dim);
            std::abort();
        }
        (b->*points_).push_back(in_points[j]);
      }
    }
}

template<class Block, class Point>
void
diy::detail::KDTreePartition<Block,Point>::
update_neighbor_bounds(Bounds& bounds, float split, int dim, bool lower) const
{
    if (lower)
        bounds.max[dim] = split;
    else
        bounds.min[dim] = split;
}

template<class Block, class Point>
bool
diy::detail::KDTreePartition<Block,Point>::
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
diy::detail::KDTreePartition<Block,Point>::
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
diy::detail::KDTreePartition<Block,Point>::
find_wrap(const Bounds& bounds, const Bounds& nbr_bounds, const Bounds& domain) const
{
    diy::Direction wrap;
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
