//
// Abstract class for specifying behavior of data sources
//
#ifndef __vlPolySource_h
#define __vlPolySource_h

#include "Source.hh"
#include "PolyData.hh"

class vlPolySource : public vlSource, public vlPolyData 
{
public:
  void Update();
};

#endif


