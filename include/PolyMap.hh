//
// PolyMapper takes PolyData as input
//
#ifndef __vlPolyMapper_h
#define __vlPolyMapper_h

#include "Mapper.hh"
#include "PolyData.hh"
#include "Renderer.hh"

class vlPolyMapper : public vlMapper 
{
public:
  vlPolyMapper();
  ~vlPolyMapper();
  char *GetClassName() {return "vlPolyMapper";};
  void Render(vlRenderer *ren);
  virtual void SetInput(vlPolyData *in);
  virtual vlPolyData* GetInput();

  vlSetMacro(VertsVisibility,int);
  vlGetMacro(VertsVisibility,int);
  vlBooleanMacro(VertsVisibility,int);

  vlSetMacro(LinesVisibility,int);
  vlGetMacro(LinesVisibility,int);
  vlBooleanMacro(LinesVisibility,int);

  vlSetMacro(PolysVisibility,int);
  vlGetMacro(PolysVisibility,int);
  vlBooleanMacro(PolysVisibility,int);

  vlSetMacro(StripsVisibility,int);
  vlGetMacro(StripsVisibility,int);
  vlBooleanMacro(StripsVisibility,int);

protected:
  vlPolyData *Input;
  vlGeometryPrimitive *Verts;
  vlGeometryPrimitive *Lines;
  vlGeometryPrimitive *Polys;
  vlGeometryPrimitive *Strips;
  int VertsVisibility;
  int LinesVisibility;
  int PolysVisibility;
  int StripsVisibility;
};

#endif


