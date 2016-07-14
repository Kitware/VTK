#ifndef DIY_TYPES_HPP
#define DIY_TYPES_HPP

// Provides sane names for C types + helper types

#include "types.h"

namespace diy
{
  typedef   gb_t        BlockID;

  typedef   bb_d_t      DiscreteBounds;
  typedef   bb_c_t      ContinuousBounds;

  //! Helper to create a 1-dimensional discrete domain with the specified extents
  diy::DiscreteBounds
  interval(int from, int to)            { DiscreteBounds domain; domain.min[0] = from; domain.max[0] = to; return domain; }

  struct Direction: public dir_t
  {
            Direction()                 { for (int i = 0; i < DIY_MAX_DIM; ++i) x[i] = 0; }

    int     operator[](int i) const     { return x[i]; }
    int&    operator[](int i)           { return x[i]; }

    // lexicographic comparison
    bool
    operator<(const diy::Direction& y) const
    {
      for (int i = 0; i < DIY_MAX_DIM; ++i)
      {
          if ((*this)[i] < y[i]) return true;
          if ((*this)[i] > y[i]) return false;
      }
      return false;
    }
  };

  // Selector of bounds value type
  template<class Bounds_>
  struct BoundsValue;

  template<>
  struct BoundsValue<DiscreteBounds>
  {
    typedef     int     type;
  };

  template<>
  struct BoundsValue<ContinuousBounds>
  {
    typedef     float   type;
  };
}

inline
bool
operator<(const diy::BlockID& x, const diy::BlockID& y)
{ return x.gid < y.gid; }

inline
bool
operator==(const diy::BlockID& x, const diy::BlockID& y)
{ return x.gid == y.gid; }

#endif
