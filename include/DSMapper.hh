//
// DataSetMapper takes DataSet as input
//
#ifndef __vlDataSetMapper_h
#define __vlDataSetMapper_h

#include "Mapper.hh"
#include "DataSet.hh"
#include "Renderer.hh"

class vlDataSetMapper : public vlMapper 
{
public:
  vlDataSetMapper();
  ~vlDataSetMapper();
  char *GetClassName() {return "vlDataSetMapper";};
  void Render(vlRenderer *ren);
  virtual void SetInput(vlDataSet *in);
  virtual vlDataSet* GetInput();

protected:
  vlDataSet *Input;
  vlMapper *Mapper;
};

#endif


