//
// PolyToPolyFilter are filters that take PolyData in and generate PolyData
//
#ifndef __vlPolyToPolyFilter_h
#define __vlPolyToPolyFilter_h

#include "PolyF.hh"
#include "PolyData.hh"

class vlPolyToPolyFilter : public vlPolyFilter, public vlPolyData 
{
public:
  void Update();
  char *GetClassName() {return "vlPolyToPolyFilter";};
};

#endif


