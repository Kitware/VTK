/*
  Copyright (c) 2006-2016, The Regents of the University of California,
  through Lawrence Berkeley National Laboratory (subject to receipt of any
  required approvals from the U.S. Dept. of Energy) and the Paul Scherrer
  Institut (Switzerland).  All rights reserved.

  License: see file COPYING in top level of source distribution.
*/

#ifndef __H5T_OCTREE_H
#define __H5T_OCTREE_H

#include "h5core/h5_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// it is very unlikely that there are more than 2^32 octants needed. since there is no id
// we dont need that...
//#if defined(USE_LARGE_INDICES)
//typedef int64_t h5_oct_idx_t;                 // type for a octant
//typedef h5_uint64_t	h5_oct_userlev_t;	// type for user defined levels
//#define OCT_USERLEV_LENGTH 64
//#else
typedef h5_int32_t h5_oct_idx_t;                // type for a octant
typedef h5_uint32_t	h5_oct_userlev_t;	// type for user defined levels
#define OCT_USERLEV_LENGTH 32
//#endif

typedef h5_int32_t h5_oct_level_t;
typedef int16_t h5_oct_orient_t;		// orientation of an octant
typedef int16_t h5_oct_dir_t; 			// direction to look for neighboring octants


#define OCT_CHG_INTERNAL  11
#define OCT_CHG_USERDATA  12
#define OCT_X  13
#define OCT_Y  14
#define OCT_Z  15

struct h5_oct_point;
typedef struct h5_oct_point h5_oct_point_t;

struct h5_octant;
typedef struct h5_octant h5t_octant_t;

struct h5_octree;
typedef struct h5_octree h5t_octree_t;

struct h5t_oct_iterator;
typedef struct h5t_oct_iterator h5t_oct_iterator_t;

#ifdef __cplusplus
}
#endif

#endif
