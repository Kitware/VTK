//
// DataSetToPolyFilter are filters that take DataSets in and generate PolyData
//
#ifndef __vlDataSetToPolyFilter_h
#define __vlDataSetToPolyFilter_h

#include "DataSetF.hh"
#include "PolyData.hh"

class vlDataSetToPolyFilter : public vlDataSetFilter, public vlPolyData 
{
public:
  void Update();
  char *GetClassName() {return "vlDataSetToPolyFilter";};
};

#endif


