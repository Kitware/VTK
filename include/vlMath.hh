//
// Macros for performing common math operations like dot product, cross product, 
// Euclidean norm, etc.
//
#ifndef __vlMath_hh
#define __vlMath_hh

inline float vlDOT(float x[3], float y[3]) {return x[0]*y[0] + x[1]*y[1] + x[2]*y[2];}

#endif
