//
// Class generates scalar data from position of points along some 
//
#ifndef __vlElevationFilter_h
#define __vlElevationFilter_h

#include "DS2DSF.hh"

class vlElevationFilter : public vlDataSetToDataSetFilter 
{
public:
  vlElevationFilter();
  char *GetClassName() {return "vlElevationFilter";};

  vlSetVector3Macro(LowPoint,float);
  vlGetVectorMacro(LowPoint,float);

  vlSetVector3Macro(HighPoint,float);
  vlGetVectorMacro(HighPoint,float);

  vlSetVector2Macro(ScalarRange,float);
  vlGetVectorMacro(ScalarRange,float);

protected:
  void Execute();
  float LowPoint[3];
  float HighPoint[3];
  float ScalarRange[2];
};

#endif


