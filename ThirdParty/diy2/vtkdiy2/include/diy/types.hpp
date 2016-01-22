#ifndef DIY_TYPES_HPP
#define DIY_TYPES_HPP

// Provides sane names for C types + helper types

#include "types.h"

namespace diy
{
  typedef   gb_t        BlockID;

  typedef   bb_d_t      DiscreteBounds;
  typedef   bb_c_t      ContinuousBounds;

  typedef   dir_t       Direction;

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
