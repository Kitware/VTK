#ifndef DIY_CONSTANTS_H
#define DIY_CONSTANTS_H

// Default DIY_MAX_DIM to 4, unless provided by the user
// (used for static array size in various Bounds (bb_t in types.h))
#ifndef DIY_MAX_DIM
#define DIY_MAX_DIM 4
#endif

struct dir_t
{
    int x[DIY_MAX_DIM];
};

#endif
