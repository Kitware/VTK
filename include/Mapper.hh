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
  vlMapper() : StartRender(0), EndRender(0), Lut(0), ScalarsVisible(1) {};
  ~vlMapper();
  virtual void Render(vlRenderer *) = 0;
  void SetStartRender(void (*f)());
  void SetEndRender(void (*f)());

  vlSetObjectMacro(Lut,vlLookupTable);
  vlGetObjectMacro(Lut,vlLookupTable);

  vlSetMacro(ScalarsVisible,int);
  vlGetMacro(ScalarsVisible,int);
  vlBooleanMacro(ScalarsVisible,int);

protected:
  void (*StartRender)();
  void (*EndRender)();
  vlLookupTable *Lut;
  int ScalarsVisible;
  vlTimeStamp BuildTime;

};

#endif


