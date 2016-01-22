#ifndef DIY_DECOMPOSITION_HPP
#define DIY_DECOMPOSITION_HPP

#include <vector>
#include <algorithm>
#include <iostream>
#include <cmath>

#include "link.hpp"
#include "assigner.hpp"
#include "master.hpp"

namespace diy
{
namespace detail
{
  template<class Bounds_>
  struct BoundsHelper;

  template<>
  struct BoundsHelper<DiscreteBounds>
  {
    static int      from(int i, int n, int min, int max, bool)          { return min + (max - min + 1)/n * i; }
    static int      to(int i, int n, int min, int max, bool shared_face)
    {
      if (i == n - 1)
        return max;
      else
        return from(i+1, n, min, max, shared_face) - (shared_face ? 0 : 1);
    }

    static int      lower(int x, int n, int min, int max, bool shared)
    {
        int width = (max - min + 1)/n;
        int res = (x - min)/width;
        if (res >= n) res = n - 1;

        if (shared && x == from(res, n, min, max, shared))
            --res;
        return res;
    }
    static int      upper(int x, int n, int min, int max, bool shared)
    {
        int width = (max - min + 1)/n;
        int res = (x - min)/width + 1;
        if (shared && x == from(res, n, min, max, shared))
            ++res;
        return res;
    }
  };

  template<>
  struct BoundsHelper<ContinuousBounds>
  {
    static float    from(int i, int n, float min, float max, bool)      { return min + (max - min)/n * i; }
    static float    to(int i, int n, float min, float max, bool)        { return min + (max - min)/n * (i+1); }

    static int      lower(float x, int n, float min, float max, bool)   { float width = (max - min)/n; float res = std::floor((x - min)/width); if (min + res*width == x) return (res - 1); else return res; }
    static int      upper(float x, int n, float min, float max, bool)   { float width = (max - min)/n; float res = std::ceil ((x - min)/width); if (min + res*width == x) return (res + 1); else return res; }
  };
}

  //! \ingroup Decomposition
  //! Decomposes a regular (discrete or continuous) domain into even blocks;
  //! creates Links with Bounds along the way.
  template<class Bounds_>
  struct RegularDecomposer
  {
    typedef         Bounds_                                         Bounds;
    typedef         typename BoundsValue<Bounds>::type              Coordinate;
    typedef         typename RegularLinkSelector<Bounds>::type      Link;

    typedef         std::vector<bool>                               BoolVector;
    typedef         std::vector<Coordinate>                         CoordinateVector;
    typedef         std::vector<int>                                DivisionsVector;

    /// @param assigner:  decides how processors are assigned to blocks (maps a gid to a rank)
    ///                   also communicates the total number of blocks
    /// @param wrap:      indicates dimensions on which to wrap the boundary
    /// @param ghosts:    indicates how many ghosts to use in each dimension
    /// @param divisions: indicates how many cuts to make along each dimension
    ///                   (0 means "no constraint," i.e., leave it up to the algorithm)
                    RegularDecomposer(int               dim_,
                                      const Bounds&     domain_,
                                      const Assigner&   assigner_,
                                      BoolVector        share_face_ = BoolVector(),
                                      BoolVector        wrap_       = BoolVector(),
                                      CoordinateVector  ghosts_     = CoordinateVector(),
                                      DivisionsVector   divisions_  = DivisionsVector()):
                      dim(dim_), domain(domain_), assigner(assigner_),
                      share_face(share_face_),
                      wrap(wrap_), ghosts(ghosts_), divisions(divisions_)
    {
      if (share_face.size() < dim)  share_face.resize(dim);
      if (wrap.size() < dim)        wrap.resize(dim);
      if (ghosts.size() < dim)      ghosts.resize(dim);
      if (divisions.size() < dim)   divisions.resize(dim);

      int nblocks = assigner.nblocks();
      fill_divisions(nblocks);
    }

    // Calls create(int gid, const Bounds& bounds, const Link& link)
    template<class Creator>
    void            decompose(int rank, const Creator& create);

    // find lowest gid that owns a particular point
    template<class Point>
    int             lowest_gid(const Point& p) const;

    void            gid_to_coords(int gid, DivisionsVector& coords) const       { gid_to_coords(gid, coords, divisions); }
    int             coords_to_gid(const DivisionsVector& coords) const          { return coords_to_gid(coords, divisions); }
    void            fill_divisions(int nblocks)                                 { fill_divisions(dim, nblocks, divisions); }

    void            fill_bounds(Bounds& bounds, const DivisionsVector& coords, bool add_ghosts = false) const;
    void            fill_bounds(Bounds& bounds, int gid, bool add_ghosts = false) const;

    static bool     all(const std::vector<int>& v, int x);
    static void     gid_to_coords(int gid, DivisionsVector& coords, const DivisionsVector& divisions);
    static int      coords_to_gid(const DivisionsVector& coords, const DivisionsVector& divisions);
    static void     fill_divisions(int dim, int nblocks, std::vector<int>& divisions);
    static void     factor(std::vector<unsigned>& factors, int n);

    // Point to GIDs functions
    template<class Point>
    void            point_to_gids(std::vector<int>& gids, const Point& p) const;

    //! returns gid of a block that contains the point; ignores ghosts
    template<class Point>
    int             point_to_gid(const Point& p) const;

    template<class Point>
    int             num_gids(const Point& p) const;

    template<class Point>
    void            top_bottom(int& top, int& bottom, const Point& p, int axis) const;


    int               dim;
    const Bounds&     domain;
    const Assigner&   assigner;
    BoolVector        share_face;
    BoolVector        wrap;
    CoordinateVector  ghosts;
    DivisionsVector   divisions;

  };

  /**
   * \ingroup Decomposition
   * \brief Decomposes the domain into a prescribed pattern of blocks.
   *
   * @param dim        dimension of the domain
   * @param rank       local rank
   * @param assigner   decides how processors are assigned to blocks (maps a gid to a rank)
   *                   also communicates the total number of blocks
   * @param create     the callback functor
   * @param wrap       indicates dimensions on which to wrap the boundary
   * @param ghosts     indicates how many ghosts to use in each dimension
   * @param divs       indicates how many cuts to make along each dimension
   *                   (0 means "no constraint," i.e., leave it up to the algorithm)
   *
   * `create(...)` is called with each block assigned to the local domain. See [decomposition example](#decomposition-example).
   */
  template<class Bounds, class Assigner, class Creator>
  void decompose(int                dim,
                 int                rank,
                 const Bounds&      domain,
                 const Assigner&    assigner,
                 const Creator&     create,
                 typename RegularDecomposer<Bounds>::BoolVector       share_face = typename RegularDecomposer<Bounds>::BoolVector(),
                 typename RegularDecomposer<Bounds>::BoolVector       wrap       = typename RegularDecomposer<Bounds>::BoolVector(),
                 typename RegularDecomposer<Bounds>::CoordinateVector ghosts     = typename RegularDecomposer<Bounds>::CoordinateVector(),
                 typename RegularDecomposer<Bounds>::DivisionsVector  divs       = typename RegularDecomposer<Bounds>::DivisionsVector())
  {
    RegularDecomposer<Bounds>(dim, domain, assigner, share_face, wrap, ghosts, divs).decompose(rank, create);
  }

namespace detail
{
  template<class Bounds>
  struct AddBlock
  {
    typedef typename RegularDecomposer<Bounds>::Link        Link;

            AddBlock(diy::Master* master):
              master_(master)               {}

    void    operator()(int gid, const Bounds& core, const Bounds& bounds, const Bounds& domain, const Link& link) const
    {
      void*     b = master_->create();
      Link*     l = new Link(link);
      master_->add(gid, b, l);
    }

    diy::Master* master_;
  };
}

  /**
   * \ingroup Decomposition
   * \brief Decomposes the domain into a prescribed pattern of blocks.
   *
   * @param dim        dimension of the domain
   * @param rank       local rank
   * @param assigner   decides how processors are assigned to blocks (maps a gid to a rank)
   *                   also communicates the total number of blocks
   * @param master     gets the blocks once this function returns
   * @param wrap       indicates dimensions on which to wrap the boundary
   * @param ghosts     indicates how many ghosts to use in each dimension
   * @param divs       indicates how many cuts to make along each dimension
   *                   (0 means "no constraint," i.e., leave it up to the algorithm)
   *
   * `master` must have been supplied a create function in order for this function to work.
   */
  template<class Bounds, class Assigner>
  void decompose(int                dim,
                 int                rank,
                 const Bounds&      domain,
                 const Assigner&    assigner,
                 Master&            master,
                 typename RegularDecomposer<Bounds>::BoolVector       share_face = typename RegularDecomposer<Bounds>::BoolVector(),
                 typename RegularDecomposer<Bounds>::BoolVector       wrap       = typename RegularDecomposer<Bounds>::BoolVector(),
                 typename RegularDecomposer<Bounds>::CoordinateVector ghosts     = typename RegularDecomposer<Bounds>::CoordinateVector(),
                 typename RegularDecomposer<Bounds>::DivisionsVector  divs       = typename RegularDecomposer<Bounds>::DivisionsVector())
  {
    RegularDecomposer<Bounds>(dim, domain, assigner, share_face, wrap, ghosts, divs).decompose(rank, detail::AddBlock<Bounds>(&master));
  }

  /**
   * \ingroup Decomposition
   * \brief A "null" decompositon that simply creates the blocks and adds them to the master
   *
   * @param rank       local rank
   * @param assigner   decides how processors are assigned to blocks (maps a gid to a rank)
   *                   also communicates the total number of blocks
   * @param master     gets the blocks once this function returns
   */
  inline
  void decompose(int                rank,
                 const Assigner&    assigner,
                 Master&            master)
  {
    std::vector<int>  local_gids;
    assigner.local_gids(rank, local_gids);

    for (size_t i = 0; i < local_gids.size(); ++i)
      master.add(local_gids[i], master.create(), new diy::Link);
  }

  //! Decomposition example: \example decomposition/test-decomposition.cpp
  //! Direct master insertion example: \example decomposition/test-direct-master.cpp
}

template<class Bounds>
template<class Creator>
void
diy::RegularDecomposer<Bounds>::
decompose(int rank, const Creator& create)
{
  std::vector<int> gids;
  assigner.local_gids(rank, gids);
  for (int i = 0; i < (int)gids.size(); ++i)
  {
    int gid = gids[i];

    DivisionsVector coords;
    gid_to_coords(gid, coords);

    Bounds core, bounds;
    fill_bounds(core,   coords);
    fill_bounds(bounds, coords, true);

    // Fill link with all the neighbors
    Link link(dim, core, bounds);
    std::vector<int>  offsets(dim, -1);
    offsets[0] = -2;
    while (!all(offsets, 1))
    {
      // next offset
      int i;
      for (i = 0; i < dim; ++i)
        if (offsets[i] == 1)
          offsets[i] = -1;
        else
          break;
      ++offsets[i];

      if (all(offsets, 0)) continue;      // skip ourselves

      DivisionsVector     nhbr_coords(dim);
      int                 dir      = 0;
      bool                inbounds = true;
      for (int i = 0; i < dim; ++i)
      {
        nhbr_coords[i] = coords[i] + offsets[i];

        // wrap
        if (nhbr_coords[i] < 0)
        {
          if (wrap[i])
          {
            nhbr_coords[i] = divisions[i] - 1;
            link.add_wrap(Direction(1 << 2*i));
          }
          else
            inbounds = false;
        }

        if (nhbr_coords[i] >= divisions[i])
        {
          if (wrap[i])
          {
            nhbr_coords[i] = 0;
            link.add_wrap(Direction(1 << (2*i + 1)));
          }
          else
            inbounds = false;
        }

        // NB: this needs to match the addressing scheme in dir_t (in constants.h)
        if (offsets[i] == -1)
          dir |= 1 << (2*i + 0);
        if (offsets[i] == 1)
          dir |= 1 << (2*i + 1);
      }
      if (!inbounds) continue;

      int nhbr_gid = coords_to_gid(nhbr_coords);
      BlockID bid; bid.gid = nhbr_gid; bid.proc = assigner.rank(nhbr_gid);
      link.add_neighbor(bid);

      Bounds nhbr_bounds;
      fill_bounds(nhbr_bounds, nhbr_coords);
      link.add_bounds(nhbr_bounds);

      link.add_direction(static_cast<Direction>(dir));
    }

    create(gid, core, bounds, domain, link);
  }
}

template<class Bounds>
bool
diy::RegularDecomposer<Bounds>::
all(const std::vector<int>& v, int x)
{
  for (unsigned i = 0; i < v.size(); ++i)
    if (v[i] != x)
      return false;
  return true;
}

template<class Bounds>
void
diy::RegularDecomposer<Bounds>::
gid_to_coords(int gid, DivisionsVector& coords, const DivisionsVector& divisions)
{
  int dim = divisions.size();
  for (int i = 0; i < dim; ++i)
  {
    coords.push_back(gid % divisions[i]);
    gid /= divisions[i];
  }
}

template<class Bounds>
int
diy::RegularDecomposer<Bounds>::
coords_to_gid(const DivisionsVector& coords, const DivisionsVector& divisions)
{
  int gid = 0;
  for (int i = coords.size() - 1; i >= 0; --i)
  {
    gid *= divisions[i];
    gid += coords[i];
  }
  return gid;
}

//! \ingroup Decomposition
//! Gets the bounds, with or without ghosts, for a block specified by its block coordinates
template<class Bounds>
void
diy::RegularDecomposer<Bounds>::
fill_bounds(Bounds& bounds,                  //!< (output) bounds
            const DivisionsVector& coords,   //!< coordinates of the block in the decomposition
            bool add_ghosts)                 //!< whether to include ghosts in the output bounds
    const
{
  for (int i = 0; i < dim; ++i)
  {
    bounds.min[i] = detail::BoundsHelper<Bounds>::from(coords[i], divisions[i], domain.min[i], domain.max[i], share_face[i]);
    bounds.max[i] = detail::BoundsHelper<Bounds>::to  (coords[i], divisions[i], domain.min[i], domain.max[i], share_face[i]);
  }

  for (int i = dim; i < DIY_MAX_DIM; ++i)   // set the unused dimension to 0
  {
    bounds.min[i] = 0;
    bounds.max[i] = 0;
  }

  if (!add_ghosts)
    return;

  for (int i = 0; i < dim; ++i)
  {
    if (wrap[i])
    {
      bounds.min[i] -= ghosts[i];
      bounds.max[i] += ghosts[i];
    } else
    {
      bounds.min[i] = std::max(domain.min[i], bounds.min[i] - ghosts[i]);
      bounds.max[i] = std::min(domain.max[i], bounds.max[i] + ghosts[i]);
    }
  }
}

//! \ingroup Decomposition
//! Gets the bounds, with or without ghosts, for a block specified by its gid
template<class Bounds>
void
diy::RegularDecomposer<Bounds>::
fill_bounds(Bounds& bounds,                  //!< (output) bounds
            int gid,                         //!< global id of the block
            bool add_ghosts)                 //!< whether to include ghosts in the output bounds
    const
{
    DivisionsVector coords;
    gid_to_coords(gid, coords);
    if (add_ghosts)
        fill_bounds(bounds, coords, true);
    else
        fill_bounds(bounds, coords);
}

template<class Bounds>
void
diy::RegularDecomposer<Bounds>::
fill_divisions(int dim, int nblocks, std::vector<int>& divisions)
{
  int prod = 1; int c = 0;
  for (unsigned i = 0; i < dim; ++i)
    if (divisions[i] != 0)
    {
      prod *= divisions[i];
      ++c;
    }

  if (nblocks % prod != 0)
  {
    std::cerr << "Incompatible requirements" << std::endl;
    return;
  }

  if (c == divisions.size())
    return;

  std::vector<unsigned> factors;
  factor(factors, nblocks/prod);

  // Fill the missing divs using LPT algorithm
  std::vector<unsigned> missing_divs(divisions.size() - c, 1);
  for (int i = factors.size() - 1; i >= 0; --i)
    *std::min_element(missing_divs.begin(), missing_divs.end()) *= factors[i];

  c = 0;
  for (unsigned i = 0; i < dim; ++i)
    if (divisions[i] == 0)
      divisions[i] = missing_divs[c++];
}

template<class Bounds>
void
diy::RegularDecomposer<Bounds>::
factor(std::vector<unsigned>& factors, int n)
{
  while (n != 1)
    for (unsigned i = 2; i <= n; ++i)
    {
      if (n % i == 0)
      {
        factors.push_back(i);
        n /= i;
        break;
      }
    }
}

// Point to GIDs
// TODO: deal with wrap correctly
// TODO: add an optional ghosts argument to ignore ghosts (if we want to find the true owners, or something like that)
template<class Bounds>
template<class Point>
void
diy::RegularDecomposer<Bounds>::
point_to_gids(std::vector<int>& gids, const Point& p) const
{
    std::vector< std::pair<int, int> > ranges(dim);
    for (int i = 0; i < dim; ++i)
        top_bottom(ranges[i].second, ranges[i].first, p, i);

    // look up gids for all combinations
    DivisionsVector coords(dim), location(dim);
    while(location.back() < ranges.back().second - ranges.back().first)
    {
        for (int i = 0; i < dim; ++i)
            coords[i] = ranges[i].first + location[i];
        gids.push_back(coords_to_gid(coords, divisions));

        location[0]++;
        unsigned i = 0;
        while (i < dim-1 && location[i] == ranges[i].second - ranges[i].first)
        {
            location[i] = 0;
            ++i;
            location[i]++;
        }
    }
}

template<class Bounds>
template<class Point>
int
diy::RegularDecomposer<Bounds>::
point_to_gid(const Point& p) const
{
    int gid = 0;
    for (int axis = dim - 1; axis >= 0; --axis)
    {
      int bottom  = detail::BoundsHelper<Bounds>::lower(p[axis], divisions[axis], domain.min[axis], domain.max[axis], share_face[axis]);
          bottom  = std::max(0, bottom);

      // coupled with coords_to_gid
      gid *= divisions[axis];
      gid += bottom;
    }

    return gid;
}

template<class Bounds>
template<class Point>
int
diy::RegularDecomposer<Bounds>::
num_gids(const Point& p) const
{
    int res = 1;
    for (int i = 0; i < dim; ++i)
    {
        int top, bottom;
        top_bottom(top, bottom, p, i);
        res *= top - bottom;
    }
    return res;
}

template<class Bounds>
template<class Point>
void
diy::RegularDecomposer<Bounds>::
top_bottom(int& top, int& bottom, const Point& p, int axis) const
{
    Coordinate l = p[axis] - ghosts[axis];
    Coordinate r = p[axis] + ghosts[axis];

    top     = detail::BoundsHelper<Bounds>::upper(r, divisions[axis], domain.min[axis], domain.max[axis], share_face[axis]);
    bottom  = detail::BoundsHelper<Bounds>::lower(l, divisions[axis], domain.min[axis], domain.max[axis], share_face[axis]);

    if (!wrap[axis])
    {
        bottom  = std::max(0, bottom);
        top     = std::min(divisions[axis], top);
    }
}

// find lowest gid that owns a particular point
template<class Bounds>
template<class Point>
int
diy::RegularDecomposer<Bounds>::
lowest_gid(const Point& p) const
{
    // TODO: optimize - no need to compute all gids
    std::vector<int> gids;
    point_to_gids(gids, p);
    std::sort(gids.begin(), gids.end());
    return gids[0];
}

#endif
