//
// Abstract class to map pipeline data to graphics primitives
//
#ifndef __vlMapper_hh
#define __vlMapper_hh

#include "Object.hh"
#include "GeomPrim.hh"
#include "Lut.hh"

class vlRenderer;

class vlMapper : public vlObject 
{
public:
  vlMapper();
  ~vlMapper();
  virtual void Render(vlRenderer *) = 0;
  void SetStartRender(void (*f)());
  void SetEndRender(void (*f)());

  vlSetObjectMacro(Lut,vlLookupTable);
  vlGetObjectMacro(Lut,vlLookupTable);

  vlSetMacro(ScalarsVisible,int);
  vlGetMacro(ScalarsVisible,int);
  vlBooleanMacro(ScalarsVisible,int);

  vlSetVector2Macro(ScalarRange,float)
  vlGetVectorMacro(ScalarRange,float)

protected:
  void (*StartRender)();
  void (*EndRender)();
  vlLookupTable *Lut;
  int ScalarsVisible;
  vlTimeStamp BuildTime;
  float ScalarRange[2];

};

#endif


