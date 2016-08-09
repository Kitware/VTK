#ifndef DIY_PICK_HPP
#define DIY_PICK_HPP

#include "link.hpp"

namespace diy
{
    template<class Bounds, class Point, class OutIter>
    void near(const RegularLink<Bounds>& link, const Point& p, float r, OutIter out,
              const Bounds& domain);

    template<class Bounds, class Point, class OutIter>
    void in(const RegularLink<Bounds>& link, const Point& p, OutIter out, const Bounds& domain);

    template<class Point, class Bounds>
    float distance(int dim, const Bounds& bounds, const Point& p);

    template<class Bounds>
    inline
    float distance(int dim, const Bounds& bounds1, const Bounds& bounds2);

    template<class Bounds>
    void wrap_bounds(Bounds& bounds, Direction wrap_dir, const Bounds& domain, int dim);
}

//! Finds the neighbors within radius r of a target point.
template<class Bounds, class Point, class OutIter>
void
diy::
near(const RegularLink<Bounds>& link,  //!< neighbors
     const Point& p,                   //!< target point (must be in current block)
     float r,                          //!< target radius (>= 0.0)
     OutIter out,                      //!< insert iterator for output set of neighbors
     const Bounds& domain)             //!< global domain bounds
{
  Bounds neigh_bounds; // neighbor block bounds

  // for all neighbors of this block
  for (int n = 0; n < link.size(); n++)
  {
    // wrap neighbor bounds, if necessary, otherwise bounds will be unchanged
    neigh_bounds = link.bounds(n);
    wrap_bounds(neigh_bounds, link.wrap(n), domain, link.dimension());

    if (distance(link.dimension(), neigh_bounds, p) <= r)
        *out++ = n;
  } // for all neighbors
}

//! Find the distance between point `p` and box `bounds`.
template<class Point, class Bounds>
float
diy::
distance(int dim, const Bounds& bounds, const Point& p)
{
    float res = 0;
    for (int i = 0; i < dim; ++i)
    {
        // avoids all the annoying case logic by finding
        // diff = max(bounds.min[i] - p[i], 0, p[i] - bounds.max[i])
        float diff = 0, d;

        d = bounds.min[i] - p[i];
        if (d > diff) diff = d;
        d = p[i] - bounds.max[i];
        if (d > diff) diff = d;

        res += diff*diff;
    }
    return sqrt(res);
}

template<class Bounds>
float
diy::
distance(int dim, const Bounds& bounds1, const Bounds& bounds2)
{
    float res = 0;
    for (int i = 0; i < dim; ++i)
    {
        float diff = 0, d;

        float d1 = bounds1.max[i] - bounds2.min[i];
        float d2 = bounds2.max[i] - bounds1.min[i];

        if (d1 > 0 && d2 > 0)
            diff = 0;
        else if (d1 <= 0)
            diff = -d1;
        else if (d2 <= 0)
            diff = -d2;

        res += diff*diff;
    }
    return sqrt(res);
}

//! Finds the neighbor(s) containing the target point.
template<class Bounds, class Point, class OutIter>
void
diy::
in(const RegularLink<Bounds>& link,  //!< neighbors
   const Point& p,                   //!< target point
   OutIter out,                      //!< insert iterator for output set of neighbors
   const Bounds& domain)             //!< global domain bounds
{
  Bounds neigh_bounds; // neighbor block bounds

  // for all neighbors of this block
  for (int n = 0; n < link.size(); n++)
  {
    // wrap neighbor bounds, if necessary, otherwise bounds will be unchanged
    neigh_bounds = link.bounds(n);
    wrap_bounds(neigh_bounds, link.wrap(n), domain, link.dimension());

    if (distance(link.dimension(), neigh_bounds, p) == 0)
        *out++ = n;
  } // for all neighbors
}

// wraps block bounds
// wrap dir is the wrapping direction from original block to wrapped neighbor block
// overall domain bounds and dimensionality are also needed
template<class Bounds>
void
diy::
wrap_bounds(Bounds& bounds, Direction wrap_dir, const Bounds& domain, int dim)
{
  for (int i = 0; i < dim; ++i)
  {
    bounds.min[i] += wrap_dir[i] * (domain.max[i] - domain.min[i]);
    bounds.max[i] += wrap_dir[i] * (domain.max[i] - domain.min[i]);
  }
}


#endif
