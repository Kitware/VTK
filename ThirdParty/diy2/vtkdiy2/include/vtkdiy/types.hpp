#ifndef DIY_TYPES_HPP
#define DIY_TYPES_HPP

#include <iostream>

// Provides sane names for C types + helper types

#include "types.h"

namespace diy
{
  typedef   gb_t        BlockID;

  typedef   bb_d_t      DiscreteBounds;
  typedef   bb_c_t      ContinuousBounds;

  //! Helper to create a 1-dimensional discrete domain with the specified extents
  inline
  diy::DiscreteBounds
  interval(int from, int to)            { DiscreteBounds domain; domain.min[0] = from; domain.max[0] = to; return domain; }

  struct Direction: public dir_t
  {
            Direction()                 { for (int i = 0; i < DIY_MAX_DIM; ++i) x[i] = 0; }
            Direction(int dir)
    {
        for (int i = 0; i < DIY_MAX_DIM; ++i) x[i] = 0;
        if (dir & DIY_X0) x[0] -= 1;
        if (dir & DIY_X1) x[0] += 1;
        if (dir & DIY_Y0) x[1] -= 1;
        if (dir & DIY_Y1) x[1] += 1;
        if (dir & DIY_Z0) x[2] -= 1;
        if (dir & DIY_Z1) x[2] += 1;
        if (dir & DIY_T0) x[3] -= 1;
        if (dir & DIY_T1) x[3] += 1;
    }

    int     operator[](int i) const     { return x[i]; }
    int&    operator[](int i)           { return x[i]; }

    bool
    operator==(const diy::Direction& y) const
    {
      for (int i = 0; i < DIY_MAX_DIM; ++i)
          if ((*this)[i] != y[i]) return false;
      return true;
    }

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

    friend std::ostream&
    operator<<(std::ostream& out, Direction dir)    { out << dir[0]; for (int i = 1; i < DIY_MAX_DIM; ++i) out << ' ' << dir[i]; return out; }
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
