#ifndef DIY_TYPES_H
#define DIY_TYPES_H

#include "constants.h"

// Types that need to be exposed to the C API must be declared here

// Global block representation: (gid, proc)
struct gb_t
{
  int   gid;
  int   proc;
};

// Discrete box
struct bb_d_t
{
  int   min[DIY_MAX_DIM];
  int   max[DIY_MAX_DIM];
};

// Continuous box
struct bb_c_t
{
  float min[DIY_MAX_DIM];
  float max[DIY_MAX_DIM];
};

#endif
