//
// Abstract class for specifying behavior of data sources
//
#ifndef __vlPolySource_h
#define __vlPolySource_h

#include "Source.h"
#include "PolyData.h"

class vlPolySource : public vlSource, public vlPolyData 
{
public:
  void Update();
};

#endif


