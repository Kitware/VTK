//
// PolyMapper takes PolyData as input
//
#ifndef PolyMapper_h
#define PolyMapper_h

#include "Params.h"
#include "Mapper.h"
#include "PolyData.h"

class PolyMapper : virtual public Mapper {
public:
  PolyMapper() : input(0) {};
  ~PolyMapper();
  virtual void setInput(PolyData *in);
  virtual PolyData* getInput();
  virtual void execute();
  virtual void update();
  virtual void map();
protected:
  PolyData *input;

};

void PolyMapper::setInput(PolyData *in)
{
  if (in != input )
  {
    input = in;
    input->Register((void *)this);
    modified();
  }
}
PolyData* PolyMapper::getInput()
{
    return input;
}

PolyMapper::~PolyMapper()
{
  if ( input != 0 )
  {
    input->UnRegister((void *)this);
  }
}

void PolyMapper::update()
{
  // make sure input is available
  if ( !input )
  {
    cout << "No input available for PolyMapper\n";
    return;
  }

  // prevent chasing our tail
  if (updating) return;

  updating = 1;
  input->update();
  updating = 0;

  if (input->getMtime() > getMtime() )
  {
    (*startMethod)();
    execute();
    modified();
    (*endMethod)();
  }
 

}



#endif


