//
// Class for performing common math operations (e.g., dot, cross products)
//
#ifndef __vlMath_hh
#define __vlMath_hh

#include <math.h>

class vlMath
{
public:
  vlMath();
  float Pi() {return 3.14159265358979;};
  float Dot(float x[3], float y[3]) 
    {return x[0]*y[0] + x[1]*y[1] + x[2]*y[2];};
  float Norm(float x[3])
    {return sqrt(x[0]*x[0] + x[1]*x[1] + x[2]*x[2]);};
  void RandomSeed(long s);  
  float Random();  
private:
  long Seed;
};

#endif
